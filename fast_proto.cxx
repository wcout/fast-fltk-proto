/*
 FLTK Fast Prototyping demo.

 (c) 2017-2018 wcout wcout<wcout@gmx.net>

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
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>

#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <unistd.h>
#ifndef WIN32
#include <sys/wait.h>
#endif
using namespace std;

#define xstr(a) str(a)
#define str(a) #a

class TextEditor : public Fl_Text_Editor
{
public:
	TextEditor( int x_, int y_, int w_, int h_, const char *l_ = 0 );
	int handle( int e_ );
	void draw();
	TextEditor& scrollTo( int line_, int col_ = 0 );
};

static const char APPLICATION[] = "fltk_fast_proto";
static pid_t child_pid = -1;
static Fl_Window *win = 0;
static Fl_Text_Buffer *textbuff = 0;
static TextEditor *editor = 0;
static Fl_Button *errorbox = 0;
static Fl_Input *searchbox = 0;
static string temp( "./temp_xxxx" );
static string temp_cxx( temp + ".cxx" );
static string errfile( "error.txt" );
static bool position_to_first_error = true;
#if 0
// use simple compile method (without warnings)
static string compile_cmd(
#ifndef FLTK_CONFIG
	"fltk-config"
#else
	xstr(FLTK_CONFIG)
#endif
	" --use-images --compile"
);
#else
// use a command like this to specify compiler flags (like -Wall to get warnings)
// $(TARGET) will be replaced with the executable name
// $(SRC) will be replaced with the source name
static string compile_cmd( "g++ -Wall -o $(TARGET) `"
#ifndef FLTK_CONFIG
	"fltk-config"
#else
	xstr(FLTK_CONFIG)
#endif
	" --use-images --cxxflags` $(SRC) `"
#ifndef FLTK_CONFIG
	"fltk-config"
#else
	xstr(FLTK_CONFIG)
#endif
	" --use-images --ldflags`" );
#endif
static string changed_cmd( "shasum" );
static string changed;
static string style_check_cmd( "cppcheck --enable=all");
static string cxx_template;
static string backup_file;
static map<string, bool> warning_ignores;

static bool ShowWarnings = true;
static bool CheckStyle = true;
static int CxxSyntax = -1;
static bool FirstMessage = true;
static bool DoActions = true;
static double CompileDelay = 0.5;

static bool regain_focus = true;

static const Fl_Color ErrorColor = FL_RED;
static const Fl_Color WarningColor = FL_GREEN;
static const Fl_Color StyleWarningColor = FL_YELLOW;
static const Fl_Color OkColor = FL_GRAY;

#include "cxx_style.cxx"
#include "clipboard.cxx"

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

int parse_first_error( int &line_, int &col_, string& err_, const string& errfile_,
                       bool warning_ = false )
{
	line_ = 0;
	col_ = -1;
	err_.erase();
	stringstream ifs;
	ifs << errfile_;
	string buf;
	string temp_cxx_filename( fl_filename_name( temp_cxx.c_str() ) );
	while ( getline( ifs, buf ) )
	{
		// parse for line with error
		string s( temp_cxx );
		size_t errpos = buf.find( s );
		if ( errpos == string::npos )
		{
			s = temp_cxx_filename;
			errpos = buf.find( s );
		}
		if ( errpos <= 2 )
		{
			errpos += s.size() + 1;
			// found potential error line - line number is afterwards
			int line = atoi( buf.substr( errpos ).c_str() );
			err_ = buf;
			if ( line ) // if error line found, we are finished
			{
				if ( warning_ && warning_ignores[err_] )
				{
					err_.erase();
					line = 0;
					continue;
				}
				// special case: ignore a warning that comes before an error
				if ( !warning_ && buf.find( "warning", errpos ) != string::npos )
					continue;
				line_ = line;
				// try to read a column position
				size_t col_pos = buf.find( ':', errpos );
				if ( col_pos != string::npos && col_pos - errpos < 6 )
					col_ = atoi( buf.substr( col_pos + 1 ).c_str() );
				break;
			}
		}
	}
	if ( !line_ && !warning_ )
	{
		// look for "external" errors i.e. errors in included source
		ifs.clear();
		ifs.seekg( 0, ios::beg );
		while ( getline( ifs, buf ) )
		{
			// parse for line with error
			size_t errpos = buf.find( " error: " );
			if ( errpos != string::npos )
			{
				err_ = buf; // report them, but without setting 'line_'
				break;
			}
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
		Fl::focus( 0 ); // ensure re-gaining try is always done
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

string compileCmd( const string& cmd_, const string& src_ )
{
	static const string TARGET = "$(TARGET)";
	static const string SRC = "$(SRC)";
	string cmd( cmd_ );
	size_t target_pos = cmd.find( TARGET );
	if ( target_pos != string::npos )
	{
		cmd.erase( target_pos, TARGET.size() );
		cmd.insert( target_pos, temp );
	}
	size_t src_pos = cmd.find( SRC );
	if ( src_pos != string::npos )
	{
		cmd.erase( src_pos, SRC.size() );
		cmd.insert( src_pos, src_ );
	}
	else
	{
		cmd.push_back( ' ');
		cmd.append( src_ );
	}
	cmd += " 2>&1";
//	printf( "compile_cmd: '%s'\n", cmd.c_str() );
	return cmd;
}

void no_errors()
{
	static const char help[] = "^y delete line, ^l duplicate line, ^r restart, ^t save template, F9 d/a compiling, ESC exit";
	textbuff->unhighlight();
	if ( !FirstMessage )
		errorbox->copy_label( "No errors" );
	else
		errorbox->label( help );
	FirstMessage = false;
	errorbox->color( OkColor );
	errorbox->selection_color( errorbox->color() );
	errorbox->tooltip( 0 );
}

int compile_and_run( const string& code_ )
{
	//  write code to temp file
	ofstream outf( temp_cxx.c_str() );
	outf << code_;
	outf.close();

	// remove exe file
	remove( temp.c_str() );
	string cmd( compileCmd( compile_cmd, temp_cxx ) );

	// compile..
	string result = run_cmd( cmd );

	// exe created?
	bool exe = access( temp.c_str(), R_OK ) == 0;
	bool warnings( false );

//	printf( "Compile '%s' => '%s'\n", cmd.c_str(), result.c_str() );
	if ( result.size() )
	{
		// there may have been compile errors..
		int line;
		int col;
		string err;
		warnings = exe;	// if exe was created this can only be a warning!
		parse_first_error( line, col, err, result, exe );
		if ( line || err.size() )
		{
			if ( warnings && !ShowWarnings )
				return 0;
			// display error/warning line in error panel
//			printf( "error in line %d: %s\n", line, err.c_str() );
			if ( line )
			{
				int start = textbuff->skip_lines( 0, line - 1 );
				int end = textbuff->skip_lines( start, 1 );
				if ( col > 0 )
					start += ( col - 1 );
				textbuff->highlight( start, end );
				if ( position_to_first_error )
					editor->scrollTo( line, col );
			}
			errorbox->copy_label( err.c_str() );
			errorbox->color( exe ? WarningColor : ErrorColor ); // warning green / error red
			errorbox->selection_color( errorbox->color() );
			errorbox->copy_tooltip( result.substr( 0, 1024 ).c_str() );
			if ( !exe )
				return 0;
		}
		else
		{
			result.erase();
		}
	}
	if ( result.empty() )
	{
		no_errors();
	}
	// re-run only if exe changed
	if ( exe )
	{
		string cmd = changed_cmd + " " + temp;
		string result = run_cmd( cmd );
		if ( changed == result )
		{
//			printf( "no change\n" );
			return warnings ? 0 : 1;
		}
		changed = result;
	}

	if ( child_pid > 0 )
		terminate( child_pid );

	child_pid = execute( temp.c_str() );
	return warnings ? 0 : 2;
}

void style_check( const string& name_ )
{
	if ( style_check_cmd.empty() )
		return;
	string cmd( style_check_cmd + " " + name_ + " 2>&1" );
	string result = run_cmd( cmd );
//	printf( "style check: [%s]\n", result.c_str() );
	if ( result.size() )
	{
		// there may have been style errors..
		int line;
		int col;
		string err;
		parse_first_error( line, col, err, result, true );
		if ( line )
		{
			// display error line in error panel
//			printf( "error in line %d: %s\n", line, err.c_str() );
			int start = textbuff->skip_lines( 0, line - 1 );
			int end = textbuff->skip_lines( start, 1 );
			if ( col > 0 )
				start += ( col - 1 );
			textbuff->highlight( start, end );
			if ( position_to_first_error )
				editor->scrollTo( line, col );
			errorbox->copy_label( err.c_str() );
			errorbox->color( StyleWarningColor );
			errorbox->selection_color( errorbox->color() );
			errorbox->copy_tooltip( result.substr( 0, 1024 ).c_str() );
		}
	}
}

void cb_compile( void *v_ )
{
	// compile the source an re-run the target
	Fl_Text_Buffer *buf = (Fl_Text_Buffer *)v_;
	char *t = buf->text();
	int ret = compile_and_run( t );
	free( t );
	if ( ret > 0 && CheckStyle )
	{
		// compiled without errors (file is saved in temp_cxx)
		style_check( temp_cxx );
	}
	position_to_first_error = false;
}

void changed_cb( int, int nInserted_, int nDeleted_, int, const char*, void* v_ )
{
	if ( nInserted_ || nDeleted_ )
	{
		// text in editor has been changed
		// => prepare to compile source if no other changes are
		//    made within the next 0.3 seconds
		Fl::remove_timeout( cb_compile, v_ );
		if ( DoActions )
			Fl::add_timeout( CompileDelay, cb_compile, v_ );
	}
}

static int kf_delete_line( int c_, Fl_Text_Editor *e_ )
{
	int pos = e_->insert_position();
	int beg = e_->buffer()->line_start( pos );
	int end = e_->buffer()->line_end( pos );
	e_->buffer()->remove( beg, end + 1);
	e_->redraw();
	return 1;
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
	return 1;
}

static int kf_save_template( int c_, Fl_Text_Editor *e_ )
{
	// save current edit as template into config file
	char *text = e_->buffer()->text();
	cxx_template = text;
	free( text );
	if ( errorbox->color() == OkColor )	// save only when no errors/warnings
		errorbox->copy_label( "Saved as template!" );
	return 1;
}

static int kf_restart( int c_, Fl_Text_Editor *e_ )
{
	// restore from backup
	if ( backup_file.size() &&
	     fl_choice( "Restart loosing all changes?", "Yes", "No", 0 ) == 0 )
		textbuff->loadfile( backup_file.c_str() );
	return 1;
}

static int kf_ignore_warning( int c_, Fl_Text_Editor *e_ )
{
	// put currently displayed warning into ignore list
	if ( errorbox->color() == OkColor || errorbox->color() == ErrorColor )
		return 1;
	string warning = errorbox->label();
	warning_ignores[warning] = true;
	printf( "Warning: '%s' ignored\n", warning.c_str() );
	cb_compile( e_->buffer() ); // re-run style check to remove warning display
	return 1;
}

static int kf_toggle_compile( int c_, Fl_Text_Editor *e_ )
{
	DoActions = !DoActions;
	if ( !DoActions )
		errorbox->copy_label( "Compiling disabled - F9 to enable" );
	else
	{
		changed.erase(); // ensure re-execution
		cb_compile( e_->buffer() );
	}
	return 1;
}

static int kf_toggle_search( int c_, Fl_Text_Editor *e_ )
{
	static bool shown = true;
	if ( shown )
	{
		searchbox->hide();
		searchbox->value( 0 );
		searchbox->do_callback();
		errorbox->resize( 0, errorbox->y(), e_->w(), errorbox->h() );
		editor->take_focus();
	}
	else
	{
		searchbox->show();
		errorbox->resize( searchbox->w(), errorbox->y(), e_->w() - searchbox->w(), errorbox->h() );
		errorbox->parent()->redraw();
		searchbox->take_focus();
	}
	shown = !shown;
	return 1;
}

static int global_shortcut( int event_ )
{
	if ( event_ != FL_SHORTCUT )
		return 0;

	bool toggle_search( false );
	if ( Fl::test_shortcut( FL_Escape ) )
	{
		// ESC in search input closes search
		if ( Fl::focus() != searchbox )
			return 0; // let FLTK close program
		toggle_search = true;
	}

	if ( toggle_search || Fl::test_shortcut( FL_CTRL + 's' ) )
	{
		kf_toggle_search( 0, editor );
		return 1;
	}
	return 0;
}

static void set_editor_textsize( Fl_Text_Editor *e_, int ts_ )
{
	e_->textsize( ts_ );
	e_->linenumber_size( ts_ );
	fl_font( e_->linenumber_font(), ts_ );
	e_->linenumber_width( fl_width( "000000" ) ); // show line numbers
	style_init( ts_, CxxSyntax != 1 );
	e_->resize( e_->x(), e_->y(), e_->w(), e_->h() );
	e_->parent()->redraw();
}

static int kf_bigger( int c_, Fl_Text_Editor *e_ )
{
	int ts = e_->textsize();
	if ( ts < 99 )
	{
		ts++;
		set_editor_textsize( e_, ts );
	}
	return 1;
}

static int kf_smaller( int c_, Fl_Text_Editor *e_ )
{
	int ts = e_->textsize();
	if ( ts > 6 )
	{
		ts--;
		set_editor_textsize( e_, ts );
	}
	return 1;
}

void delete_cb( Fl_Widget *, void * )
{
	textbuff->remove_selection();
}

static void errorbox_cb( Fl_Widget *w_, void *d_ )
{
	// user clicked on errorbox, do something useful
	Fl_Button *errorbox = static_cast<Fl_Button *>( w_ );
	if ( Fl::event_button() == FL_LEFT_MOUSE )
	{
		if ( errorbox->color() == OkColor )
		{
			// show help line, if no error..
			FirstMessage = true;
			no_errors();
		}
		else
		{
			if ( Fl::event_clicks() == 0 )
			{
				// position to first error with single click
				position_to_first_error = true;
				Fl_Text_Editor *editor= static_cast<Fl_Text_Editor *>( d_ );
				cb_compile( editor->buffer() ); // re-run compile
			}
			else if ( Fl::event_clicks() == 1 )
			{
				// run 'ignore warning' with double click
				position_to_first_error = true;
				Fl_Text_Editor *editor= static_cast<Fl_Text_Editor *>( d_ );
				kf_ignore_warning( 0, editor );
			}
		}
	}
}

static void searchbox_cb( Fl_Widget *w_, void *d_ )
{
	string s( searchbox->value() );
	if ( s.empty() )
	{
		textbuff->unhighlight();
		return;
	}
	int found_pos = 0;
	int found = Fl::event_ctrl() ?
		textbuff->search_backward( editor->insert_position() - 1, s.c_str(), &found_pos ) :
		textbuff->search_forward( editor->insert_position() + 1, s.c_str(), &found_pos );
	if ( found )
	{
		textbuff->unhighlight();
		textbuff->highlight( found_pos, found_pos + s.size() );
		editor->insert_position( found_pos );
		editor->show_insert_position();
	}
	else
	{
		fl_beep();
	}
}

static void show_options_and_exit()
{
	char buf[1024];
	snprintf( buf, sizeof( buf ), "Usage:\n"
	        "\tfast_proto [OPTION ..] [CXXFILE]\n\n"
	        "FLTK fast prototyping v0.2\n\n"
	        "OPTIONS\n"
	        "\t-w\tdon't show warnings\n"
	        "\t-s\tdon't show style check messages\n"
	        "\t-p\tuse global preferences file (otherwise use '%s.prefs' file in current folder)\n"
	        "\t-x\tdon't syntax highlight source\n"
	        "\t-xf\tdon't syntax highlight FLTK keywords\n\n"
	        "CXXFILE\tuse this existing source file (otherwise use '%s' in current folder)",
		APPLICATION, temp_cxx.c_str() );
	printf( "%s\n", buf );
	fl_alert( "%s", buf );
	exit( 0 );
}

TextEditor::TextEditor( int x_, int y_, int w_, int h_, const char *l_ ) :
	Fl_Text_Editor( x_, y_, w_, h_, l_ )
{
	// add some editing key shortcuts (line deletion/duplication)
	add_key_binding( 'y', FL_CTRL, kf_delete_line );
	add_key_binding( 'd', FL_CTRL, kf_delete_line );
	add_key_binding( 'l', FL_CTRL, kf_duplicate_line );
	add_key_binding( 'd', FL_CTRL + FL_SHIFT, kf_duplicate_line );
	add_key_binding( 's', FL_CTRL, kf_toggle_search );
}

int TextEditor::handle( int e_ )
{
	// intercept Ctrl-Mousewheel to resize editor font
	if ( e_ == FL_MOUSEWHEEL && Fl::event_ctrl() && Fl::event_dy() )
	{
		if ( Fl::event_dy() > 0 )
			return kf_bigger( 0, this );
		return kf_smaller( 0, this );
	}
	// display clipboard functions popup with right mouse button
	else if ( e_ == FL_PUSH && Fl::event_button() == FL_RIGHT_MOUSE )
	{
		Clipboard clip;
		Fl_Menu_Item menu[10];
		menu[0].text = 0;
		string cliptext = clip.get();
		if ( buffer() && buffer()->selected() )
		{
			menu->add( "Cu&t", FL_COMMAND + 'x', (Fl_Callback *)Fl_Text_Editor::kf_cut );
			menu->add( "&Copy", FL_COMMAND + 'c', (Fl_Callback *)Fl_Text_Editor::kf_copy );
			menu->add( "&Delete", 0, (Fl_Callback *)delete_cb );
		}
		if ( cliptext.size() )
		{
			menu->add( "&Paste", FL_COMMAND + 'v', (Fl_Callback *)Fl_Text_Editor::kf_paste );
		}
		if ( menu[0].text )
		{
			fl_cursor( FL_CURSOR_DEFAULT );
			const Fl_Menu_Item *m = menu->popup( Fl::event_x(), Fl::event_y() );
			if ( m )
			{
				m->do_callback( 0, this );
			}
		}
		return 1;
	}
	else if ( e_ == FL_RELEASE && Fl::event_button() == FL_RIGHT_MOUSE )
	{
		return 1;
	}

	return Fl_Text_Editor::handle( e_ );
}

void TextEditor::draw()
{
	Fl_Text_Editor::draw();
	// use area at bottom/left to display information (cursor column, insert mode)
	int s = Fl::scrollbar_size();
	int line, col;
	if ( !position_to_linecol( insert_position(), &line, &col ) )
		return;
	char buf[30];
	fl_font( FL_HELVETICA, s - 2 );
	static int minw = 0;
	if ( !minw )
		minw = fl_width( "XXX | 000" );
	if ( linenumber_width() < minw )
		snprintf( buf, sizeof( buf ), "%s %d", insert_mode() ? "I" : "O", col + 1 );
	else
		snprintf( buf, sizeof( buf ), "%s | %d", insert_mode() ? "INS" : "OVR", col + 1 );
	fl_push_clip( 2, h() - s, linenumber_width(), s - 1 );
	fl_rectf( 2, h() - s, linenumber_width(), s - 1, FL_GRAY );
	fl_color( FL_WHITE );
	fl_draw( buf, 4, h() - 4 );
	fl_pop_clip();
}

TextEditor& TextEditor::scrollTo( int line_, int col_/* = 0*/ )
{
	// set cursor to line/col and set view
	if ( col_ )
		col_--;
	insert_position( skip_lines( 0, line_ - 1, true ) + col_ );
	show_insert_position();
	return *this;
}

int main( int argc_, char *argv_[] )
{
	// check if a source file or options are given
	string source;
	bool local_prefs( true );
	for ( int i = 1; i < argc_; i++ )
	{
		string arg( argv_[ i ] );
		if ( arg == "--help" || arg == "-h" )
			show_options_and_exit();
		else if ( arg == "-w" )
			ShowWarnings = false;
		else if ( arg == "-s" )
			CheckStyle = false;
		else if ( arg == "-p" )
			local_prefs = false;
		else if ( arg == "-x" )
			CxxSyntax = 0;
		else if ( arg == "-xf" )
			CxxSyntax = 1;
		else if ( arg[0] != '-' )
			source = arg;
		else
		{
			fprintf( stderr, "Invalid option: '%s'\n", arg.c_str() );
			show_options_and_exit();
		}
	}

	// read in configurable values
	Fl_Preferences &cfg = *( local_prefs  ?
		new Fl_Preferences( ".", NULL, APPLICATION ) :
		new Fl_Preferences( Fl_Preferences::USER, "wcout", APPLICATION ) );
	int x, y, w, h;
	cfg.get( "x", x, 100 );
	cfg.get( "y", y, 100 );
	cfg.get( "w", w, 800 );
	cfg.get( "h", h, 600 );
	char *text;
	cfg.get( "compile_cmd", text, compile_cmd.c_str() );
	compile_cmd = text;
	cfg.get( "changed_cmd", text, "shasum" );
	changed_cmd = text;
	cfg.get( "style_check_cmd", text, "cppcheck --enable=all" );
	style_check_cmd = text;
	cfg.get( "cxx_template", text, "// type FLTK program here..\nint main() {}" );
	cxx_template = text;

	if ( source.size() )
	{
		if ( access( source.c_str(), R_OK ) == 0 )
		{
			size_t ext_pos = source.find( ".cxx" );
			if ( ext_pos == string::npos )
				ext_pos = source.find( ".cpp" );
			if ( ext_pos != string::npos )
			{
				// use source
				printf( "Use source file '%s'\n", source.c_str() );
				temp_cxx = source;
				temp = "./" + (string)fl_filename_name( source.substr( 0, ext_pos ).c_str() );
			}
		}
	}

	// create the editor window using last size
	win = new Fl_Double_Window( w, h, "Fast FLTK prototyping" );
	textbuff = new Fl_Text_Buffer();
	int ts;
	cfg.get( "ts", ts, 14 );
	if ( CxxSyntax )
	{
		style_init( ts, CxxSyntax != 1 );
	}
	editor = new TextEditor( 0, 0, win->w(), win->h() - 30 );
	#define SB_WIDTH 150
	errorbox = new Fl_Button( 0 + SB_WIDTH, 0 + editor->h(), win->w() - SB_WIDTH, 30 );
	errorbox->box( FL_FLAT_BOX );
	errorbox->visible_focus( 0 );
	errorbox->callback( errorbox_cb, editor );
	searchbox = new Fl_Input( 0, 0 + editor->h(), SB_WIDTH, 30 );
	searchbox->box( FL_PLASTIC_DOWN_BOX );
	searchbox->callback( searchbox_cb, editor );
	searchbox->when( FL_WHEN_ENTER_KEY | FL_WHEN_NOT_CHANGED );
	searchbox->tooltip( "Search text: Enter=forward, ^Enter=backward\n"
	                    "\t(^s toggles search box display)" );

	int bgcolor, selcolor;
	if ( CxxSyntax )
	{
		cfg.get( "bgcolor_syntax", bgcolor, (int)FL_WHITE );
		cfg.get( "selcolor_syntax", selcolor, (int)FL_YELLOW );
	}
	else
	{
		cfg.get( "bgcolor", bgcolor, (int)fl_lighter( FL_BLUE ) );
		cfg.get( "selcolor", selcolor, (int)FL_BLUE );
	}
	editor->color( Fl_Color( bgcolor ) );
	editor->selection_color( Fl_Color( selcolor ) );
	editor->textcolor( fl_contrast( FL_WHITE, editor->color() ) );
	editor->cursor_style( Fl_Text_Editor::SIMPLE_CURSOR );
#ifdef HAVE_LINE_HIGHLIGHT
	editor->line_highlight( 1 );
#endif
	int cursor_color;
	cfg.get( "cursor_color", cursor_color, (int)FL_GREEN );
	editor->cursor_color( (Fl_Color)cursor_color );

	editor->textfont( FL_COURIER );
	set_editor_textsize( editor, ts );

	// and some meta keys for actions
	editor->add_key_binding( 't', FL_CTRL, kf_save_template );
	editor->add_key_binding( 'r', FL_CTRL, kf_restart );
	editor->add_key_binding( 'w', FL_CTRL, kf_ignore_warning );
	editor->add_key_binding( FL_F + 9, 0 , kf_toggle_compile );
	editor->add_key_binding( '+', FL_ALT , kf_bigger );
	editor->add_key_binding( '-', FL_ALT , kf_smaller );

	editor->buffer( textbuff ); // attach text buffer to editor
	no_errors();
	FirstMessage = true;
	if ( CxxSyntax )
	{
		editor->highlight_data( stylebuf, styletable,
		                        sizeof(styletable) / sizeof(styletable[0]),
		                        'A', style_unfinished_cb, 0);
		textbuff->add_modify_callback( style_update, editor );
	}
	textbuff->add_modify_callback( changed_cb, textbuff );
	int tab_size;
	cfg.get( "tab_size", tab_size, 3 );
	textbuff->tab_distance( tab_size );
	if ( textbuff->loadfile( temp_cxx.c_str() ) )
	{
		// a new source file is started
		textbuff->text( cxx_template.c_str() );
		textbuff->select( 0, textbuff->length() );
	}
	else
	{
		// make a "backup" of the original source
		backup_file = temp_cxx + ".orig";
		if ( textbuff->outputfile( backup_file.c_str(), 0, textbuff->length() ) )
			backup_file.erase();
	}

	// position cursor at end of file
	editor->insert_position( textbuff->length() );
	editor->show_insert_position();

	win->end();
	win->resizable( editor );
	win->show();
	win->position( x, y );	// try to show window on last position

	// show source filename in title
	string title( temp_cxx + " - " + win->label() );
	win->copy_label( title.c_str() );

	// add a global key handler
	Fl::add_handler( global_shortcut );

	// enter the main loop
	Fl::run();

	// stop a running program
	if ( child_pid > 0 )
		terminate( child_pid );

	// save preferences
	cfg.set( "x", win->x() );
	cfg.set( "y", win->y() );
	cfg.set( "w", win->w() );
	cfg.set( "h", win->h() );
	cfg.set( "ts", editor->textsize() );
	cfg.set( "tab_size", textbuff->tab_distance() );
	cfg.set( CxxSyntax ? "bgcolor_syntax" : "bgcolor", (int)editor->color() );
	cfg.set( CxxSyntax ? "selcolor_syntax" : "selcolor", (int)editor->selection_color() );
	cfg.set( "cursor_color", (int)editor->cursor_color() );
	cfg.set( "compile_cmd", compile_cmd.c_str() );
	cfg.set( "changed_cmd", changed_cmd.c_str() );
	cfg.set( "style_check_cmd", style_check_cmd.c_str() );
	cfg.set( "cxx_template", cxx_template.c_str() );
	cfg.flush();
}
