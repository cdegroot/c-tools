/********************************************************
*   WINDOWS.H, include-file voor window-mgmt            *
*   (C)1989, C.A. de Groot                              *
********************************************************/
/*
	$Id: window.h,v 1.4 1993/02/05 13:49:30 cg Rel $
*/

#if __STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif

#if !defined(__WINDOW_DEF)
#define __WINDOW_DEF

#include <conio.h>

typedef struct window_data
{
    int     x;      /* Abs. kolomlocatie linkerbovenhoek */
    int     y;      /* Abs. rijlocatie linkerbovenhoek */
    int     w;      /* Breedte, inclusief frame */
    int     h;      /* Hoogte, inclusief frame */
	char     *title;     /* Title van window, "" is toegestaan */
	int     fr_attr;    /* Attribute van frame */
	int     wn_attr;    /* Attribute van text in window */
	int     tit_attr;   /* Attribute van title */
	int     style;      /* Frame-style, zie onder */
	/* Gereserveerd voor systeem: */
	char     *save;      /* Gebruikt door funcs voor saven scherm */
	char     *cont;      /* Window-inhoud */
    int     xsav;
    int     ysav;
	struct  window_data  *prev;
	struct  window_data  *next;
} WIN;

WIN  *  _Cdecl makewin(int x, int y, int w, int h,
							  char  *title, int fr_attr, int wn_attr,
								int tit_attr, int style, int expfl);
WIN  *  _Cdecl makewinc(int w, int h,
							  char  *title, int fr_attr, int wn_attr,
								int tit_attr, int style, int expfl);
int        _Cdecl gotowin(WIN  *window);
int        _Cdecl closewin(void);
WIN  *  _Cdecl popmenu(int x, int y, char  *title,
					char  *entry[], int n_entry, int fr_attr,
					int tit_attr, int style, int leave,
					int  *keuze, int def_keuze, WIN  *def_win);
WIN  *  _Cdecl popmenuc(char  *title, char  *entry[],
					int n_entry, int fr_attr, int tit_attr,
					int style, int leave, int  *keuze,
					int def_keuze, WIN  *def_win);
void       _Cdecl fieldattr(int x, int y, int len, char attr);
void       _Cdecl makestat(const char  *status);
void       _Cdecl nostat(void);
void       _Cdecl cursor(int type);
int		   _Cdecl getcursor(void);

extern  int win_error;
extern unsigned char token[4][6];
extern char  *win_errmsg[];

#define VBAR 0
#define HBAR 1
#define LTOP 2
#define RTOP 3
#define LBOT 4
#define RBOT 5
#define NOLEAVE 0
#define LEAVE   1

#define WN_OK       0   /* Functie ging goed */
#define WN_NOMEM    1   /* Geheugenfout (te weinig geheugen) */
#define WN_BADWX    2   /* Rechterkolom valt buiten scherm */
                /* of breedte kleiner dan drie */
#define WN_BADHY    3   /* Onderste regel valt buiten scherm */
                /* of hoogte kleiner dan drie */
#define WN_BADTIT   4   /* Title is langer dan window breed is */
#define WN_BADSTL   5   /* Niet-bestaande stijl */
#define WN_BADPTR	6	/* Slecht argument bij gotowin */

#if 1
extern int ATT_NORM, ATT_HIGH, ATT_INVERSE;
#else
#define ATT_NORM    0x07    /* Normal video */
#define ATT_HIGH    0x0F    /* High intensity */
#define ATT_INVERSE 0x70    /* Inverse video */
#endif
#define ATT_BLINK   0x80    /* Blink, kan gecombineerd worden */

#define FST_GSING   0       /* IBM Graph chars, enkel */
#define FST_GDOUB   1       /* IBM Graph chars, dubbel */
#define FST_NOGR1   2       /* ASCII 7-bits, streepjes */
#define FST_NOGR2   3       /* ASCII 7-bits, sterretjes */

#define CURS_SMALL  0       /* Normale cursor */
#define CURS_BIG    1       /* Cursor die hele positie vult */
#define CURS_OFF    2       /* Geen cursor */

#endif
