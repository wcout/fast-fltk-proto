// Syntax highlighting stuff...
#define TS 14 // default editor textsize

static Fl_Text_Buffer *stylebuf = 0;
static bool HighLightFltk = true;

static Fl_Text_Display::Style_Table_Entry
	styletable[] = {                                // Style table
	{ FL_BLACK,      FL_COURIER,           TS },    // A - Plain
	{ FL_DARK_GREEN, FL_COURIER_ITALIC,    TS },    // B - Line comments
	{ FL_DARK_GREEN, FL_COURIER_ITALIC,    TS },    // C - Block comments
	{ FL_BLUE,       FL_COURIER,           TS },    // D - Strings
	{ FL_DARK_RED,   FL_COURIER,           TS },    // E - Directives
	{ FL_DARK_RED,   FL_COURIER_BOLD,      TS },    // F - Types
	{ FL_BLUE,       FL_COURIER_BOLD,      TS },    // G - Keywords
	{ FL_BLACK,      FL_COURIER_BOLD,      TS },    // H - FLTK Keywords
};

static const char *code_keywords[] = { // List of known C/C++ keywords...
	"and",
	"and_eq",
	"asm",
	"bitand",
	"bitor",
	"break",
	"case",
	"catch",
	"compl",
	"continue",
	"default",
	"delete",
	"do",
	"else",
	"false",
	"for",
	"goto",
	"if",
	"new",
	"not",
	"not_eq",
	"operator",
	"or",
	"or_eq",
	"return",
	"switch",
	"template",
	"this",
	"throw",
	"true",
	"try",
	"while",
	"xor",
	"xor_eq"
};

static const char *code_types[] = {    // List of known C/C++ types...
	"auto",
	"bool",
	"char",
	"class",
	"const",
	"const_cast",
	"double",
	"dynamic_cast",
	"enum",
	"explicit",
	"extern",
	"float",
	"friend",
	"inline",
	"int",
	"long",
	"mutable",
	"namespace",
	"private",
	"protected",
	"public",
	"register",
	"short",
	"signed",
	"sizeof",
	"static",
	"static_cast",
	"struct",
	"template",
	"typedef",
	"typename",
	"union",
	"unsigned",
	"using",
	"virtual",
	"void",
	"volatile"
};

static const char *fltk_keywords[] = { // List of some FLTK keywords...
	"FL_ACCUM",
	"FL_ACTIVATE",
	"FL_ALIGN_BOTTOM",
	"FL_ALIGN_BOTTOM_LEFT",
	"FL_ALIGN_BOTTOM_RIGHT",
	"FL_ALIGN_CENTER",
	"FL_ALIGN_CLIP",
	"FL_ALIGN_IMAGE_BACKDROP",
	"FL_ALIGN_IMAGE_MASK",
	"FL_ALIGN_IMAGE_NEXT_TO_TEXT",
	"FL_ALIGN_IMAGE_OVER_TEXT",
	"FL_ALIGN_INSIDE",
	"FL_ALIGN_LEFT",
	"FL_ALIGN_LEFT_BOTTOM",
	"FL_ALIGN_LEFT_TOP",
	"FL_ALIGN_NOWRAP",
	"FL_ALIGN_POSITION_MASK",
	"FL_ALIGN_RIGHT",
	"FL_ALIGN_RIGHT_BOTTOM",
	"FL_ALIGN_RIGHT_TOP",
	"FL_ALIGN_TEXT_NEXT_TO_IMAGE",
	"FL_ALIGN_TEXT_OVER_IMAGE",
	"FL_ALIGN_TOP",
	"FL_ALIGN_TOP_LEFT",
	"FL_ALIGN_TOP_RIGHT",
	"FL_ALIGN_WRAP",
	"FL_ALPHA",
	"FL_ALT",
	"FL_Alt_L",
	"FL_Alt_R",
	"FL_BACKGROUND2_COLOR",
	"FL_BACKGROUND_COLOR",
	"FL_BLACK",
	"FL_BLUE",
	"FL_BOLD",
	"FL_BOLD_ITALIC",
	"FL_BUTTON1",
	"FL_BUTTON2",
	"FL_BUTTON3",
	"FL_BUTTONS",
	"FL_Back",
	"FL_BackSpace",
	"FL_Button",
	"FL_CAPS_LOCK",
	"FL_Caps_Lock",
	"FL_CAP_FLAT",
	"FL_CAP_ROUND",
	"FL_CAP_SQUARE",
	"FL_CIRCLE_BOX",
	"FL_CLOSE",
	"FL_COLOR_CUBE",
	"FL_COMMAND",
	"FL_CONTROL",
	"FL_COURIER",
	"FL_COURIER_BOLD",
	"FL_COURIER_BOLD_ITALIC",
	"FL_COURIER_ITALIC",
	"FL_CTRL",
	"FL_CURSOR_ARROW",
	"FL_CURSOR_CROSS",
	"FL_CURSOR_DEFAULT",
	"FL_CURSOR_E",
	"FL_CURSOR_HAND",
	"FL_CURSOR_HELP",
	"FL_CURSOR_INSERT",
	"FL_CURSOR_MOVE",
	"FL_CURSOR_N",
	"FL_CURSOR_NE",
	"FL_CURSOR_NESW",
	"FL_CURSOR_NONE",
	"FL_CURSOR_NS",
	"FL_CURSOR_NW",
	"FL_CURSOR_NWSE",
	"FL_CURSOR_S",
	"FL_CURSOR_SE",
	"FL_CURSOR_SW",
	"FL_CURSOR_W",
	"FL_CURSOR_WAIT",
	"FL_CURSOR_WE",
	"FL_CYAN",
	"FL_Control_L",
	"FL_Control_R",
	"FL_DAMAGE_ALL",
	"FL_DAMAGE_CHILD",
	"FL_DAMAGE_EXPOSE",
	"FL_DAMAGE_OVERLAY",
	"FL_DAMAGE_SCROLL",
	"FL_DAMAGE_USER1",
	"FL_DAMAGE_USER2",
	"FL_DARK1",
	"FL_DARK2",
	"FL_DARK3",
	"FL_DARK_BLUE",
	"FL_DARK_CYAN",
	"FL_DARK_GREEN",
	"FL_DARK_MAGENTA",
	"FL_DARK_RED",
	"FL_DARK_YELLOW",
	"FL_DASH",
	"FL_DASHDOT",
	"FL_DASHDOTDOT",
	"FL_DEACTIVATE",
	"FL_DEPTH",
	"FL_DIAMOND_BOX",
	"FL_DIAMOND_DOWN_BOX",
	"FL_DIAMOND_UP_BOX",
	"FL_DND_DRAG",
	"FL_DND_ENTER",
	"FL_DND_LEAVE",
	"FL_DND_RELEASE",
	"FL_DOUBLE",
	"FL_DRAG",
	"FL_Delete",
	"FL_Down",
	"FL_EMBOSSED_LABEL",
	"FL_ENGRAVED_BOX",
	"FL_ENGRAVED_FRAME",
	"FL_ENGRAVED_LABEL",
	"FL_ENTER",
	"FL_EXCEPT",
	"FL_Eisu",
	"FL_End",
	"FL_Enter",
	"FL_Escape",
	"FL_F",
	"FL_FAKE_SINGLE",
	"FL_FOCUS",
	"FL_FOREGROUND_COLOR",
	"FL_FRAME",
	"FL_FRAME_BOX",
	"FL_FREE_COLOR",
	"FL_FREE_FONT",
	"FL_FULLSCREEN",
	"FL_F_Last",
	"FL_Favorites",
	"FL_Forward",
	"FL_GLEAM_DOWN_BOX",
	"FL_GLEAM_DOWN_FRAME",
	"FL_GLEAM_ROUND_DOWN_BOX",
	"FL_GLEAM_ROUND_UP_BOX",
	"FL_GLEAM_THIN_DOWN_BOX",
	"FL_GLEAM_THIN_UP_BOX",
	"FL_GLEAM_UP_BOX",
	"FL_GLEAM_UP_FRAME",
	"FL_GRAY",
	"FL_GRAY0",
	"FL_GRAY_RAMP",
	"FL_GREEN",
	"FL_GTK_DOWN_BOX",
	"FL_GTK_DOWN_FRAME",
	"FL_GTK_ROUND_DOWN_BOX",
	"FL_GTK_ROUND_UP_BOX",
	"FL_GTK_THIN_DOWN_BOX",
	"FL_GTK_THIN_DOWN_FRAME",
	"FL_GTK_THIN_UP_BOX",
	"FL_GTK_THIN_UP_FRAME",
	"FL_GTK_UP_BOX",
	"FL_GTK_UP_FRAME",
	"FL_HELVETICA",
	"FL_HELVETICA_BOLD",
	"FL_HELVETICA_BOLD_ITALIC",
	"FL_HELVETICA_ITALIC",
	"FL_HIDE",
	"FL_Help",
	"FL_Home",
	"FL_Home_Page",
	"FL_IMAGE_WITH_ALPHA",
	"FL_INACTIVE_COLOR",
	"FL_INDEX",
	"FL_Insert",
	"FL_Iso_Key",
	"FL_ITALIC",
	"FL_JOIN_BEVEL",
	"FL_JOIN_MITER",
	"FL_JOIN_ROUND",
	"FL_JIS_Underscore",
	"FL_KEYBOARD",
	"FL_KEYDOWN",
	"FL_KEY_MASK",
	"FL_KEYUP",
	"FL_KP",
	"FL_KP_Enter",
	"FL_KP_Last",
	"FL_Kana",
	"FL_LEAVE",
	"FL_LEFT_MOUSE",
	"FL_Left",
	"FL_LIGHT1",
	"FL_LIGHT2",
	"FL_LIGHT3",
	"FL_MAGENTA",
	"FL_META",
	"FL_MIDDLE_MOUSE",
	"FL_MOUSEWHEEL",
	"FL_MOVE",
	"FL_MULTISAMPLE",
	"FL_Mail",
	"FL_Media_Next",
	"FL_Media_Play",
	"FL_Media_Prev",
	"FL_Media_Stop",
	"FL_Menu",
	"FL_Meta_L",
	"FL_Meta_R",
	"FL_NO_BOX",
	"FL_NO_EVENT",
	"FL_NORMAL_LABEL",
	"FL_NUM_BLUE",
	"FL_NUM_FREE_COLOR16",
	"FL_NUM_GRAY",
	"FL_NUM_GREEN",
	"FL_NUM_LOCK",
	"FL_NUM_RED",
	"FL_Num_Lock",
	"FL_OFLAT_BOX",
	"FL_OPENGL3",
	"FL_OSHADOW_BOX",
	"FL_OVAL_BOX",
	"FL_OVAL_FRAME",
	"FL_PATH_MAX",
	"FL_PASTE",
	"FL_PLASTIC_DOWN_BOX",
	"FL_PLASTIC_DOWN_FRAME",
	"FL_PLASTIC_ROUND_DOWN_BOX",
	"FL_PLASTIC_ROUND_UP_BOX",
	"FL_PLASTIC_THIN_DOWN_BOX",
	"FL_PLASTIC_THIN_UP_BOX",
	"FL_PLASTIC_UP_BOX",
	"FL_PLASTIC_UP_FRAME",
	"FL_PUSH",
	"FL_Page_Down",
	"FL_Page_Up",
	"FL_Pause",
	"FL_Print",
	"FL_READ",
	"FL_RED",
	"FL_RELEASE",
	"FL_RFLAT_BOX",
	"FL_RGB",
	"FL_RGB8",
	"FL_RIGHT_MOUSE",
	"FL_ROUND_DOWN_BOX",
	"FL_ROUNDED_BOX",
	"FL_ROUNDED_FRAME",
	"FL_ROUND_UP_BOX",
	"FL_RSHADOW_BOX",
	"FL_Refresh",
	"FL_Right",
	"FL_SCREEN",
	"FL_SCREEN_BOLD",
	"FL_SCREEN_CONFIGURATION_CHANGED",
	"FL_SCROLL_LOCK",
	"FL_SELECTIONCLEAR",
	"FL_SELECTION_COLOR",
	"FL_SHADOW_BOX",
	"FL_SHADOW_FRAME",
	"FL_SHADOW_LABEL",
	"FL_SHIFT",
	"FL_SHOW",
	"FL_SINGLE",
	"FL_SHORTCUT",
	"FL_SOLID",
	"FL_STENCIL",
	"FL_STEREO",
	"FL_SUBMENU",
	"FL_SYMBOL",
	"FL_SYMBOL_LABEL",
	"FL_Scroll_Lock",
	"FL_Search",
	"FL_Shift_L",
	"FL_Shift_R",
	"FL_Sleep",
	"FL_Stop",
	"FL_Tab",
	"FL_TIMES",
	"FL_TIMES_BOLD",
	"FL_TIMES_BOLD_ITALIC",
	"FL_TIMES_ITALIC",
	"FL_UNFOCUS",
	"FL_Up",
	"FL_Volume_Down",
	"FL_Volume_Mute",
	"FL_Volume_Up",
	"FL_WHEN_CHANGED",
	"FL_WHEN_ENTER_KEY",
	"FL_WHEN_ENTER_KEY_ALWAYS",
	"FL_WHEN_ENTER_KEY_CHANGED",
	"FL_WHEN_NEVER",
	"FL_WHEN_NOT_CHANGED",
	"FL_WHEN_RELEASE",
	"FL_WHEN_RELEASE_ALWAYS",
	"FL_WHITE",
	"FL_WRITE",
	"FL_YELLOW",
	"FL_Yen",
	"FL_ZAPF_DINGBATS",
	"FL_ZOOM_GESTURE",
	"Fl",
	"Fl_Adjuster",
	"Fl_BMP_Image",
	"Fl_Bitmap",
	"Fl_Box",
	"Fl_Browser",
	"Fl_Button",
	"Fl_Cairo_Window",
	"Fl_Callback",
	"Fl_Chart",
	"Fl_Check_Browser",
	"Fl_Check_Button",
	"Fl_Choice",
	"Fl_Clock",
	"Fl_Color_Chooser",
	"Fl_Copy_Surface",
	"Fl_Counter",
	"Fl_Device",
	"Fl_Dial",
	"Fl_Double_Window",
	"Fl_File_Browser",
	"Fl_File_Chooser",
	"Fl_File_Icon",
	"Fl_File_Input",
	"Fl_Fill_Dial",
	"Fl_Fill_Slider",
	"Fl_Float_Input",
	"Fl_FormsBitmap",
	"Fl_FormsPixmap",
	"Fl_Free",
	"Fl_GIF_Image",
	"Fl_Gl_Window",
	"Fl_Group",
	"Fl_Help_Dialog",
	"Fl_Help_View",
	"Fl_Hold_Browser",
	"Fl_Hor_Fill_Slider",
	"Fl_Hor_Nice_Slider",
	"Fl_Hor_Slider",
	"Fl_Hor_Value_Slider",
	"Fl_Image",
	"Fl_Image_Surface",
	"Fl_Input",
	"Fl_Input_Choice",
	"Fl_Int_Input",
	"Fl_JPEG_Image",
	"Fl_Light_Button",
	"Fl_Line_Dial",
	"Fl_Menu_Bar",
	"Fl_Menu_Button",
	"Fl_Menu",
	"Fl_Menu_Item",
	"Fl_Menu_Window",
	"Fl_Multi_Browser",
	"Fl_Multi_Label",
	"Fl_Multiline_Input",
	"Fl_Multiline_Output",
	"Fl_Native_File_Chooser",
	"Fl_Nice_Slider",
	"Fl_Object",
	"Fl_Output",
	"Fl_Overlay_Window",
	"Fl_PNG_Image",
	"Fl_PNM_Image",
	"Fl_Pack",
	"Fl_Paged_Device",
	"Fl_Pixmap",
	"Fl_Plugin",
	"Fl_Positioner",
	"Fl_PostScript",
	"Fl_Preferences",
	"Fl_Printer",
	"Fl_Progress",
	"Fl_RGB_Image",
	"Fl_Radio_Button",
	"Fl_Radio_Light_Button",
	"Fl_Radio_Round_Button",
	"Fl_Repeat_Button",
	"Fl_Return_Button",
	"Fl_Roller",
	"Fl_Round_Button",
	"Fl_Round_Clock",
	"Fl_Scrollbar",
	"Fl_Scroll",
	"Fl_Secret_Input",
	"Fl_Select_Browser",
	"Fl_Shared_Image",
	"Fl_Simple_Counter",
	"Fl_Single_Window",
	"Fl_Slider",
	"Fl_Spinner",
	"Fl_Sys_Menu_Bar",
	"Fl_Table",
	"Fl_Table_Row",
	"Fl_Tabs",
	"Fl_Text_Buffer",
	"Fl_Text_Display",
	"Fl_Text_Editor",
	"Fl_Tile",
	"Fl_Tiled_Image",
	"Fl_Timer",
	"Fl_Toggle_Button",
	"Fl_Toggle_Light_Button",
	"Fl_Toggle_Round_Button",
	"Fl_Tooltip",
	"Fl_Tree",
	"Fl_Tree_Item_Array",
	"Fl_Tree_Item",
	"Fl_Tree_Prefs",
	"Fl_Valuator",
	"Fl_Value_Input",
	"Fl_Value_Output",
	"Fl_Value_Slider",
	"Fl_Widget",
	"Fl_Window",
	"Fl_Wizard",
	"Fl_XBM_Image",
	"Fl_XPM_Image",
	"fl_access",
	"fl_add_symbol",
	"fl_alert",
	"fl_alphasort",
	"fl_arc",
	"fl_ask",
	"fl_beep",
	"fl_begin_complex_polygon",
	"fl_begin_line",
	"fl_begin_loop",
	"fl_begin_points",
	"fl_begin_polygon",
	"fl_chmod",
	"fl_choice",
	"fl_chord",
	"fl_circle",
	"fl_clip_box",
	"fl_clip_region",
	"fl_color",
	"fl_color_average",
	"fl_color_chooser",
	"fl_contrast",
	"fl_cursor",
	"fl_curve",
	"fl_darker",
	"fl_decode_uri",
	"fl_descent",
	"fl_draw",
	"fl_draw_box",
	"fl_draw_image",
	"fl_draw_image_mono",
	"fl_draw_pixmap",
	"fl_draw_symbol",
	"fl_end_complex_polygon",
	"fl_end_line",
	"fl_end_loop",
	"fl_end_polygon",
	"fl_execvp",
	"fl_expand_text",
	"fl_file_chooser",
	"fl_filename_absolute",
	"fl_filename_expand",
	"fl_filename_ext",
	"fl_filename_isdir",
	"fl_filename_list",
	"fl_filename_match",
	"fl_filename_name",
	"fl_filename_relative",
	"fl_font",
	"fl_frame",
	"fl_frame2",
	"fl_gap",
	"fl_getcwd",
	"fl_getenv",
	"fl_height",
	"fl_lighter",
	"fl_line",
	"fl_line_style",
	"fl_loop",
	"fl_make_path",
	"fl_make_path_for_file",
	"fl_measure",
	"fl_measure_pixmap",
	"fl_message",
	"fl_message_font",
	"fl_message_hotspot",
	"fl_message_title",
	"fl_message_title_default",
	"fl_mkdir",
	"fl_mult_matrix",
	"fl_nonspacing",
	"fl_not_clipped",
	"fl_old_shortcut",
	"fl_open",
	"fl_open_callback",
	"fl_open_uri",
	"fl_overlay_clear",
	"fl_overlay_rect",
	"fl_parse_color",
	"fl_pie",
	"fl_point",
	"fl_polygon",
	"fl_pop_clip",
	"fl_pop_matrix",
	"fl_push_clip",
	"fl_push_matrix",
	"fl_push_no_clip",
	"fl_read_image",
	"fl_rect",
	"fl_rectf",
	"fl_register_images",
	"fl_rename",
	"fl_reset_spot",
	"fl_restore_clip",
	"fl_rmdir",
	"fl_rgb_color",
	"fl_rotate",
	"fl_rtl_draw",
	"fl_scale",
	"fl_scroll",
	"fl_set_spot",
	"fl_set_status",
	"fl_shortcut_label",
	"fl_show_colormap",
	"fl_size",
	"fl_stat",
	"fl_system",
	"fl_text_extents",
	"fl_tolower",
	"fl_toupper",
	"fl_transform_dx",
	"fl_transform_dy",
	"fl_transformed_vertex",
	"fl_transform_x",
	"fl_transform_y",
	"fl_translate",
	"fl_ucs_to_Utf16",
	"fl_unlink",
	"fl_utf2mbcs",
	"fl_utf8back",
	"fl_utf8bytes",
	"fl_utf8decode",
	"fl_utf8encode",
	"fl_utf8froma",
	"fl_utf8from_mb",
	"fl_utf8fromwc",
	"fl_utf8fwd",
	"fl_utf8len",
	"fl_utf8locale",
	"fl_utf8test",
	"fl_utf8toa",
	"fl_utf8to_mb",
	"fl_utf8toUtf16",
	"fl_utf8towc",
	"fl_utf_nb_char",
	"fl_utf_strcasecmp",
	"fl_utf_strncasecmp",
	"fl_utf_tolower",
	"fl_utf_toupper",
	"fl_vertex",
	"fl_wcwidth",
	"fl_width",
	"fl_xyline",
	"fl_yxline"
};


//
// 'compare_keywords()' - Compare two keywords...
//

extern "C" {
static int compare_keywords(const void *a,
	const void *b)
{
	return strcmp(*((const char **)a), *((const char **)b));
}
}

static inline bool iskey(char c)
{
	return isalpha(c);
}

//
// 'style_parse()' - Parse text and produce style data.
//

static void style_parse(const char *text,
	char       *style,
	int length)
{
	char current;
	int col;
	int last;
	char buf[255],
	*bufptr;
	const char *temp;

	// Style letters:
	//
	// A - Plain
	// B - Line comments
	// C - Block comments
	// D - Strings
	// E - Directives
	// F - Types
	// G - Keywords
	// H - FLTK Keywords

	for (current = *style, col = 0, last = 0; length > 0; length--, text++)
	{
		if (current == 'B' || current == 'F' || current == 'G' || current == 'H')
			current = 'A';
		if (current == 'A')
		{
			// Check for directives, comments, strings, and keywords...
			if (col == 0 && *text == '#')
			{
				// Set style to directive
				current = 'E';
			}
			else if (strncmp(text, "//", 2) == 0)
			{
				current = 'B';
				for (; length > 0 && *text != '\n'; length--, text++)
					*style++ = 'B';

				if (length == 0)
					break;
			}
			else if (strncmp(text, "/*", 2) == 0)
			{
				current = 'C';
			}
			else if (strncmp(text, "\\\"", 2) == 0)
			{
				// Quoted quote...
				*style++ = current;
				*style++ = current;
				text++;
				length--;
				col += 2;
				continue;
			}
			else if (*text == '\"')
			{
				current = 'D';
			}
			else if (!last && (iskey(*text) || *text == '_'))
			{
				// Might be a keyword...
				for (temp = text, bufptr = buf;
				     (iskey(*temp) || *temp == '_') && bufptr < (buf + sizeof(buf) - 1);
				     *bufptr++ = *temp++)
				{
					// nothing
				}

				if (!iskey(*temp) && *temp != '_')
				{
					*bufptr = '\0';

					bufptr = buf;

					if (bsearch(&bufptr, code_types,
						    sizeof(code_types) / sizeof(code_types[0]),
						    sizeof(code_types[0]), compare_keywords))
					{
						while (text < temp)
						{
							*style++ = 'F';
							text++;
							length--;
							col++;
						}

						text--;
						length++;
						last = 1;
						continue;
					}
					else if (bsearch(&bufptr, code_keywords,
							 sizeof(code_keywords) / sizeof(code_keywords[0]),
							 sizeof(code_keywords[0]), compare_keywords))
					{
						while (text < temp)
						{
							*style++ = 'G';
							text++;
							length--;
							col++;
						}

						text--;
						length++;
						last = 1;
						continue;
					}

					else if (HighLightFltk && bsearch(&bufptr, fltk_keywords,
							 sizeof(fltk_keywords) / sizeof(fltk_keywords[0]),
							 sizeof(fltk_keywords[0]), compare_keywords))
					{
						while (text < temp)
						{
							*style++ = 'H';
							text++;
							length--;
							col++;
						}

						text--;
						length++;
						last = 1;
						continue;
					}


				}
			}
		}
		else if (current == 'C' && strncmp(text, "*/", 2) == 0)
		{
			// Close a C comment...
			*style++ = current;
			*style++ = current;
			text++;
			length--;
			current = 'A';
			col += 2;
			continue;
		}
		else if (current == 'D')
		{
			// Continuing in string...
			if (strncmp(text, "\\\"", 2) == 0)
			{
				// Quoted end quote...
				*style++ = current;
				*style++ = current;
				text++;
				length--;
				col += 2;
				continue;
			}
			else if (*text == '\"')
			{
				// End quote...
				*style++ = current;
				col++;
				current = 'A';
				continue;
			}
		}

		// Copy style info...
		if (current == 'A' && (*text == '{' || *text == '}'))
			*style++ = 'G';
		else
			*style++ = current;
		col++;

		last = isalnum((*text) & 255) || *text == '_' || *text == '.';

		if (*text == '\n')
		{
			// Reset column and possibly reset the style
			col = 0;
			if (current == 'B' || current == 'E')
				current = 'A';
		}
	}
} // style_parse


//
// 'style_init()' - Initialize the style buffer...
//

void
style_init(int ts = TS, bool highlight_fltk = true)
{
	HighLightFltk = highlight_fltk;
	for (size_t i = 0; i < sizeof(styletable) / sizeof(styletable[0]); i++)
		styletable[i].size = ts;
	char *style = new char[textbuff->length() + 1];
	char *text = textbuff->text();

	memset(style, 'A', textbuff->length());
	style[textbuff->length()] = '\0';

	if (!stylebuf)
		stylebuf = new Fl_Text_Buffer(textbuff->length());

	style_parse(text, style, textbuff->length());

	stylebuf->text(style);
	delete[] style;
	free(text);
}


//
// 'style_unfinished_cb()' - Update unfinished styles.
//

void
style_unfinished_cb(int, void *)
{
}


//
// 'style_update()' - Update the style buffer...
//

void
style_update(int pos,                   // I - Position of update
	int nInserted,                  // I - Number of inserted chars
	int nDeleted,                   // I - Number of deleted chars
	int /*nRestyled*/,              // I - Number of restyled chars
	const char * /*deletedText*/,   // I - Text that was deleted
	void       *cbArg)              // I - Callback data
{
	int start,                      // Start of text
		end;                    // End of text
	char last,                      // Last style on line
	*style,                         // Style data
	*text;                          // Text data


	// If this is just a selection change, just unselect the style buffer...
	if (nInserted == 0 && nDeleted == 0)
	{
		stylebuf->unselect();
		return;
	}

	// Track changes in the text buffer...
	if (nInserted > 0)
	{
		// Insert characters into the style buffer...
		style = new char[nInserted + 1];
		memset(style, 'A', nInserted);
		style[nInserted] = '\0';

		stylebuf->replace(pos, pos + nDeleted, style);
		delete[] style;
	}
	else
	{
		// Just delete characters in the style buffer...
		stylebuf->remove(pos, pos + nDeleted);
	}

	// Select the area that was just updated to avoid unnecessary
	// callbacks...
	stylebuf->select(pos, pos + nInserted - nDeleted);

	// Re-parse the changed region; we do this by parsing from the
	// beginning of the line of the changed region to the end of
	// the line of the changed region...  Then we check the last
	// style character and keep updating if we have a multi-line
	// comment character...
	start = textbuff->line_start(pos);
	// the following code checks the style of the last character of the previous
	// line. If it is a block comment, the previous line is interpreted as well.
	int altStart = textbuff->prev_char(start);
	if (altStart > 0)
	{
		altStart = textbuff->prev_char(altStart);
		if (altStart >= 0 && stylebuf->byte_at(start - 2) == 'C')
			start = textbuff->line_start(altStart);
	}

	end   = textbuff->line_end(pos + nInserted);
	text  = textbuff->text_range(start, end);
	style = stylebuf->text_range(start, end);
	if (start == end)
		last = 0;
	else
		last  = style[end - start - 1];

//  printf("start = %d, end = %d, text = \"%s\", style = \"%s\", last='%c'...\n",
//         start, end, text, style, last);

	style_parse(text, style, end - start);

//  printf("new style = \"%s\", new last='%c'...\n",
//         style, style[end - start - 1]);

	stylebuf->replace(start, end, style);
	((Fl_Text_Editor *)cbArg)->redisplay_range(start, end);

	if (start == end || last != style[end - start - 1])
	{
//    printf("Recalculate the rest of the buffer style\n");
		// Either the user deleted some text, or the last character
		// on the line changed styles, so reparse the
		// remainder of the buffer...
		free(text);
		free(style);

		end   = textbuff->length();
		text  = textbuff->text_range(start, end);
		style = stylebuf->text_range(start, end);

		style_parse(text, style, end - start);

		stylebuf->replace(start, end, style);
		((Fl_Text_Editor *)cbArg)->redisplay_range(start, end);
	}

	free(text);
	free(style);
} // style_update

