/*
 FLTK Fast Prototyping demo.

 (c) 2017 wcout wcout<wcout@gmx.net>

 Requirements:

   - Linux
   - fltk-config available
   - Compiler error output compatible with gcc

 This code is free software: you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by
 the Free Software Foundation,  either version 3 of the License, or
 (at your option) any later version.

 This code is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY;  without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details:
 http://www.gnu.org/licenses/.

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
#ifndef WIN32
#include <sys/wait.h>
#endif
using namespace std;

pid_t child_pid = -1;
Fl_Window *win = 0;
Fl_Text_Buffer *textbuff = 0;
Fl_Text_Editor *editor = 0;
Fl_Box *errorbox = 0;
string temp( "./temp_xxxx" );
string temp_cxx( temp + ".cxx" );
string errfile( "error.txt" );
string compile_cmd( "fltk-config --use-images --compile" );
string changed_cmd( "shasum" );
string changed;
bool regain_focus = true;

void focus_cb( void *v_ )
{
	if ( win )
	{
		static int state = 0;
		state++;
		// do we need/have the cursor focus already?
		if ( win && regain_focus && Fl::focus() == editor )
		{
			regain_focus = false;
//			printf( "focus regained.\n ");
		}
		if ( regain_focus )
		{
			// try to set focus to editor
			win->show();
			win->take_focus();
		}
		// als handle cursor blinking here..
		if ( editor )
			editor->show_cursor( state % 2 );
		Fl::add_timeout( 0.2, focus_cb );
	}
}

int parse_first_error( int &line_, string& err_, string errfile_ )
{
	line_ = 0;
	err_.erase();
	stringstream ifs;
	ifs << errfile_;
	string buf;
	while ( getline( ifs, buf ) )
	{
		// parse for line with error
		size_t errpos;
		if ( ( errpos = buf.find( temp_cxx ) ) == 0 )
		{
			errpos += temp_cxx.size() + 1;
			// found potential error line - line number is afterwards
			line_ = atoi( buf.substr( errpos ).c_str() );
			err_ = buf;
			if ( line_ ) // if error line found, we are finished
				break;
		}
	}
	return line_;
}

string run_cmd( const string& cmd_ )
{
	// run a shell command and collect its output
	FILE *f = popen( cmd_.c_str(), "r" );
	string result;
	if ( f )
	{
		char buf[512];
		while ( fgets( buf, sizeof( buf ), f ) != NULL )
			result += buf;
		pclose( f );
	}
	return result;
}

pid_t execute( const char *exe_ )
{
//	printf( "execute '%s'\n", exe_ );
#ifdef WIN32
	pid_t child_pid = -1;
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };
	DWORD dwCreationFlags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS;
	si.cb = sizeof( STARTUPINFO );
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;
	si.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
	si.hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );
	si.hStdError = GetStdHandle( STD_ERROR_HANDLE );
	si.wShowWindow = SW_HIDE;
	if ( CreateProcess( NULL, (LPSTR)exe_, NULL, NULL, FALSE,
		                            dwCreationFlags, NULL, NULL, &si, &pi ) )
	{
		CloseHandle( pi.hThread );
		child_pid = (pid_t)pi.hProcess;
	}
#else
	pid_t child_pid = fork();
	if ( child_pid > 0 )
	{
		int status;
		waitpid( child_pid, &status, WNOHANG );
	}
	else if ( child_pid == 0 )
	{
		execlp( exe_, exe_, NULL );
		_exit( EXIT_FAILURE );
	}
#endif
	if ( child_pid >  0 )
	{
		// we possibly have lost cursor focus,
		// try to regain it..
		Fl::remove_timeout( focus_cb );
		Fl::add_timeout( 0.2, focus_cb );
		regain_focus = true;
	}
	return child_pid;
}

void terminate( pid_t pid_ )
{
#ifdef WIN32
	TerminateProcess( (HANDLE)pid_, 0 );
#else
	kill( pid_, SIGTERM );
#endif
}

void compile_and_run( string code_ )
{
	//  write code to temp file
	ofstream outf( temp_cxx.c_str() );
	outf << code_;
	outf.close();

	// remove exe file
	remove( temp.c_str() );
	string cmd( compile_cmd + " " + temp_cxx + " 2>&1" );

	// compile..
	string result = run_cmd( cmd );

//	printf( "Compile '%s' => '%s'\n", cmd.c_str(), result.c_str() );
	if ( result.size() )
	{
		// there may have been compile errors..
		int line;
		string err;
		parse_first_error( line, err, result );
		if ( line )
		{
			// display error line in error panel
//			printf( "error in line %d: %s\n", line, err.c_str() );
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
			errorbox->copy_label( "No errors" );
			errorbox->color( FL_GRAY );
		}
	}
	// re-run only if exe changed
	if ( access( temp.c_str(), R_OK ) == 0 )
	{
		string cmd = changed_cmd + " " + temp;
		string result = run_cmd( cmd );
		if ( changed == result )
		{
//			printf( "no change\n" );
			return;
		}
		changed = result;
	}

	if ( child_pid > 0 )
		terminate( child_pid );

	child_pid = execute( temp.c_str() );
}

void cb_compile( void *v_ )
{
	// compile the source an re-run the target
	Fl_Text_Buffer *buf = (Fl_Text_Buffer *)v_;
	char *t = buf->text();
	compile_and_run( t );
	free( t );
}

void changed_cb( int, int nInserted_, int nDeleted_, int, const char*, void* v_ )
{
	if ( nInserted_ || nDeleted_ )
	{
		// text in editor has been changed
		// => prepare to compile source if no other changes are
		//    made within the next 0.3 seconds
		Fl::remove_timeout( cb_compile, v_ );
		Fl::add_timeout( 0.3, cb_compile, v_ );
	}
}

static int kf_delete_line( int c_, Fl_Text_Editor *e_ )
{
	int pos = e_->insert_position();
	int beg = e_->buffer()->line_start( pos );
	int end = e_->buffer()->line_end( pos );
	e_->buffer()->remove( beg, end + 1);
	e_->redraw();
}

static int kf_duplicate_line( int c_, Fl_Text_Editor *e_ )
{
	int pos = e_->insert_position();
	char *line = e_->buffer()->line_text( pos );
	int end = e_->buffer()->line_end( pos );
	e_->buffer()->insert( end + 1, "\n");
	e_->buffer()->insert( end + 1, line );
	free( line );
	e_->redraw();
}

int main( int argc_, char *argv_[] )
{
	// read in configurable values
	Fl_Preferences cfg( ".", NULL, "fltk_fast_proto" );
	int x, y, w, h;
	cfg.get( "x", x, 100 );
	cfg.get( "y", y, 100 );
	cfg.get( "w", w, 800 );
	cfg.get( "h", h, 600 );
	char *text;
	cfg.get( "compile_cmd", text, "fltk-config --use-images --compile" );
	compile_cmd = text;
	cfg.get( "changed_cmd", text, "shasum" );
	changed_cmd = text;

	// check if a source file is given
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
				// use default filename
				temp_cxx = f;
				temp = "./" + (string)fl_filename_name( f.substr( 0, ext_pos ).c_str() );
			}
		}
	}

	// create the editor window using last size
	win = new Fl_Double_Window( w, h, "Fast FLTK prototyping" );
	textbuff = new Fl_Text_Buffer();
	editor = new Fl_Text_Editor( 0, 0, win->w(), win->h() - 30 );
	errorbox = new Fl_Box( 0, 0 + editor->h(), win->w(), 30 );
	errorbox->box( FL_FLAT_BOX );
	editor->color( fl_lighter( FL_BLUE ) );
	editor->textcolor( FL_WHITE );
	editor->cursor_style( Fl_Text_Editor::SIMPLE_CURSOR );
	editor->cursor_color( FL_GREEN );
	editor->linenumber_width( 50 ); // show line numbers
	editor->textfont( FL_COURIER );
	int ts;
	cfg.get( "ts", ts, 14 );
	editor->textsize( ts );
	editor->linenumber_size( ts );
	editor->buffer( textbuff ); // attach text buffer to editor

	// add some key shortcuts (line deletion/duplication)
	editor->add_key_binding( 'y', FL_CTRL, kf_delete_line );
	editor->add_key_binding( 'd', FL_CTRL, kf_delete_line );
	editor->add_key_binding( 'l', FL_CTRL, kf_duplicate_line );
	editor->add_key_binding( 'd', FL_CTRL + FL_SHIFT, kf_duplicate_line );

	textbuff->add_modify_callback( changed_cb, textbuff );
	textbuff->tab_distance( 3 );
	if ( textbuff->loadfile( temp_cxx.c_str() ) )
	{
		// a new source file is started
		textbuff->text( "// type FLTK program here..\n" );
		textbuff->select( 0, textbuff->length() );
	}
	else
	{
		// make a "backup" of the original source
		string backup_file( temp_cxx + ".orig" );
		textbuff->outputfile( backup_file.c_str(), 0, textbuff->length() );
	}

	// position cursor at end of file
	editor->insert_position( textbuff->length() );

	win->end();
	win->resizable( win );
	win->show();
	win->position( x, y );	// try to show window on last position

	// show source filename in title
	string title( temp_cxx + " - " + win->label() );
	win->copy_label( title.c_str() );

	// enter the main loop
	Fl::run();

	// stop a running program
	if ( child_pid > 0 )
		terminate( child_pid );

	// save prefences
	cfg.set( "x", win->x() );
	cfg.set( "y", win->y() );
	cfg.set( "w", win->w() );
	cfg.set( "h", win->h() );
	cfg.set( "ts", editor->textsize() );
	cfg.set( "compile_cmd", compile_cmd.c_str() );
	cfg.set( "changed_cmd", changed_cmd.c_str() );
	cfg.flush();
}
