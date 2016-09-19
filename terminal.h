/* ------------- terminal.h --------------- */
/* Include file for Terminal functions      */
/* ---------------------------------------- */
/*
	$Id: terminal.h,v 1.3 1993/02/05 13:03:00 cg Rel $
*/

#if __STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif

void  _Cdecl init_crt(void);
int   _Cdecl get_byte(void);
void  _Cdecl high_intensity(int h);
void  _Cdecl underline(int u);
void  _Cdecl inverse(int i);
void  _Cdecl put_byte(int c);
void  _Cdecl clr_scrn(void);
void  _Cdecl save_gr(void);
void  _Cdecl rstr_gr(void);
void  _Cdecl cursorxy(int x, int y);
