// clipboard handling helper method/class
static char *getEventText()
{
	char *ret = 0;
	int len = Fl::event_length();
	if ( len )
	{
		ret = (char *)malloc( len + 1 );
		if ( ret )
		{
			const char *text = Fl::event_text();
			char *p = ret;
			*p = 0;
			int l = 0;
			for ( int i = 0; i < len; i++ )
			{
				int c = text[i];
				if ( c )
				{
					*p++ = c;
					l++;
				}
				if ( l >= len )
					break;
			}
			// event_text is terminated by 0x0d normally - remove it
			if ( l && ( *(p - 1) == '\n' || *(p - 1) == '\r') )
				p--;
			*p = 0;
		}
	}
	return ret;
} // getEventText

class Clipboard : public Fl_Box
{
typedef Fl_Box Inherited;
public:
	Clipboard() : Inherited( 0, 0, 10, 10 ),
		_waitPaste( false ) {}
	string get()
	{
		_clip.erase();
		if ( Fl::clipboard_contains( Fl::clipboard_plain_text ) )
		{
			_waitPaste = true;
			Fl::paste( *this, 1, Fl::clipboard_plain_text );
			while ( _waitPaste )
			{
				Fl::wait();
			}
		}
		return _clip;
	}
	virtual int handle( int e_ )
	{
		switch ( e_ )
		{
			case FL_PASTE:
				char *text = getEventText();
				_clip = text ? text : "";
				free( text );
				_waitPaste = false;
				return 1;
		}
		return Inherited::handle( e_ );
	}
private:
	string _clip;
	bool _waitPaste;
};

