/*
   FLTK Fast Prototyping demo.

   - Linux
   - fltk-config available
   - Compiler error output compatible with gcc

*/
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl.H>
#include <FL/filename.H>

#include <string>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

pid_t child_pid = -1;
Fl_Window *win = 0;
Fl_Text_Buffer *textbuff = 0;
Fl_Box *errorbox = 0;
string temp( "./temp_xxxx" );
string temp_cxx( temp + ".cxx" );
string errfile( "error.txt" );
string compile_cmd( "fltk-config --use-images --compile" );

void focus_cb( void *v_ )
{
	if ( win )
	{
//		printf( "take focus\n" );
		win->show();
		win->take_focus();
	}
}

int parse_first_error( int &line_, string& err_, string errfile_ )
{
//	printf( "parse_first_error\n" );
	line_ = 0;
	err_.erase();
	stringstream ifs;
	ifs << errfile_;
	string buf;
	while ( getline( ifs, buf ) )
	{
//		printf( "parsing: '%s'\n", buf.c_str() );
		size_t errpos;
		if ( ( errpos = buf.find( temp_cxx ) ) == 0 )
		{
			errpos += temp_cxx.size() + 1;
//			printf( "errpos %u\n", errpos );
			line_ = atoi( buf.substr( errpos ).c_str() );
//			printf( "error line: %d\n", line_ );
			err_ = buf;
			if ( line_ )
				break;
		}
	}
	return line_;
}

void compile_and_run( string code_ )
{
//	printf( "compile and run: '%s'\n", code_.c_str() );
	ofstream outf( temp_cxx.c_str() );
	outf << code_;
	outf.close();
	remove( temp.c_str() );
	string cmd( compile_cmd + " " + temp_cxx + " 2>&1" );
	FILE *f = popen( cmd.c_str(), "r" );
	string result;
	if ( f )
	{
		char buf[512];
		while ( fgets( buf, sizeof( buf ), f ) != NULL )
			result += buf;
		pclose( f );
	}
//	printf( "Compile '%s' FILE = %p\n", cmd.c_str(), f );
//	printf( "result: '%s'\n", result.c_str());
	if ( result.size() )
	{
		int line;
		string err;
		parse_first_error( line, err, result );
		if ( line )
		{
//			printf( "(line %d): %s\n", line, err.c_str());
			int start = textbuff->skip_lines( 0, line - 1 );
			int end = textbuff->skip_lines( start, 1 );
			textbuff->highlight( start, end );
			errorbox->copy_label( err.c_str() );
			errorbox->color( FL_RED );
			return;
		}
		else
		{
			textbuff->unhighlight();
			errorbox->copy_label( "" );
			errorbox->color( FL_GRAY );
		}
	}
	if ( child_pid > 0 )
		kill( child_pid, SIGTERM );

	child_pid = fork();
	if ( child_pid > 0 )
	{
		int status;
		waitpid( child_pid, &status, WNOHANG );
		Fl::add_timeout( 0.2, focus_cb );
	}
	else if ( child_pid == 0 )
	{
//		printf("execute '%s'\n", temp.c_str());
		execlp( temp.c_str(), temp.c_str(), NULL );
		_exit( EXIT_FAILURE );
	}
}

void cb_compile( void *v_ )
{
	Fl_Text_Buffer *buf = (Fl_Text_Buffer *)v_;
	char *t = buf->text();
	compile_and_run( t );
	free( t );
}

void changed_cb( int, int nInserted_, int nDeleted_, int, const char*, void* v_ )
{
	if ( nInserted_ || nDeleted_ )
	{
		Fl::remove_timeout( cb_compile, v_ );
		Fl::add_timeout( 0.3, cb_compile, v_ );
	}
}

int main( int argc_, char *argv_[] )
{
	Fl_Preferences cfg( ".", NULL, "fltk_fast_proto" );
	int x, y, w, h;
	cfg.get( "x", x, 100 );
	cfg.get( "y", y, 100 );
	cfg.get( "w", w, 800 );
	cfg.get( "h", h, 600 );
	char *text;
	cfg.get( "compile_cmd", text, "fltk-config --use-images --compile" );
	compile_cmd = text;
	if ( argc_ > 1 )
	{
		string f = argv_[ 1 ];
		printf( "File '%s'\n", f.c_str() );
		if ( access( f.c_str(), R_OK ) == 0 )
		{
			size_t ext_pos = f.find( ".cxx" );
			if ( ext_pos == string::npos )
				ext_pos = f.find( ".cpp" );
			if ( ext_pos != string::npos )
			{
				temp_cxx = f;
				temp = "./" + (string)fl_filename_name( f.substr( 0, ext_pos ).c_str() );
			}
		}
	}

	win = new Fl_Double_Window( w, h, "Fast FLTK prototyping" );
	textbuff = new Fl_Text_Buffer();
	Fl_Text_Editor disp( 10, 10, win->w() - 20, win->h() - 50 );
	errorbox = new Fl_Box( 10, 10 + disp.h(), win->w() - 20, 30 );
	errorbox->box( FL_FLAT_BOX );
	disp.color( fl_lighter( FL_BLUE ) );
	disp.textcolor( FL_WHITE );
	disp.cursor_style( Fl_Text_Editor::SIMPLE_CURSOR );
	disp.cursor_color( FL_GREEN );
	disp.linenumber_width( 50 );
	disp.textfont( FL_COURIER );
	int ts;
	cfg.get( "ts", ts, 14 );
	disp.textsize( ts );
	disp.linenumber_size( ts );
	disp.buffer( textbuff ); // attach text buffer to display widget
	textbuff->add_modify_callback( changed_cb, textbuff );
	textbuff->tab_distance( 3 );
	if ( textbuff->loadfile( temp_cxx.c_str() ) )
		textbuff->text( "// type FLTK program here..\n" );
	disp.insert_position( textbuff->length() );
	win->end();
	win->resizable( win );
	win->show();
	win->position( x, y );

	Fl::run();

	if ( child_pid > 0 )
		kill( child_pid, SIGTERM );

	cfg.set( "x", win->x() );
	cfg.set( "y", win->y() );
	cfg.set( "w", win->w() );
	cfg.set( "h", win->h() );
	cfg.set( "ts", disp.textsize() );
	cfg.set( "compile_cmd", compile_cmd.c_str() );
	cfg.flush();
}
