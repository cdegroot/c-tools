/* ---------- terminal.c -------------- */
/*
	$Id: tb_term.c,v 1.3 1993/02/05 13:02:58 cg Rel $
*/

/* Terminal dependent functions, coded for Turbo-C */
#include <conio.h>
#include <dos.h>
#include <time.h>
#include <alloc.h>
#include <bios.h>
#include <stdlib.h>

#include <toolset.h>
#include <keys.h>
#include <terminal.h>
#include <window.h>
#include <stdio.h>

int _curr_crt = 0;
int _under = 0;
int _high = 0;
int _inverse = 0;

/*
   Attributes for colors
*/
int ATT_NORM, ATT_HIGH, ATT_INVERSE;

/*
	Saved screen information
*/
static struct text_info orig_screen;

/* Pseudo stack for nested levels of gr. rend. */
#define SVDEPTH 10
static int sv_ints[SVDEPTH], sv_und[SVDEPTH], sv_inv[SVDEPTH], svp = 0;
static void sgr(void);
static void save_crt(void);
static void restore_crt(void);

/* Initialize the terminal */
void _fill_scrn(void);
static int initd = FALSE;

void init_crt(void)
{
	unsigned int equipment;

	if (initd)
		return;
	initd = TRUE;

	/*
		Save textinfo information, and make sure it
		gets restored
	*/
	gettextinfo(&orig_screen);
	atexit(restore_crt);

	clr_scrn();

	/*
		Setup colors, depending on current mode
	*/
	equipment = biosequip();
	if ((equipment & 0x30) == 0x30)
	{
		ATT_NORM	= LIGHTGRAY | (BLACK << 4);
		ATT_HIGH	= WHITE     | (BLACK << 4);
		ATT_INVERSE = BLACK     | (LIGHTGRAY << 4);
	}
	else
	{
		ATT_NORM	= YELLOW | (BLUE << 4);
		ATT_HIGH	= WHITE  | (BLUE << 4);
		ATT_INVERSE = BLUE   | (LIGHTGRAY << 4);
	}

	_fill_scrn();	/* In tb_wins */
	_under = _high = svp = 0;
	sgr();
}


/* Stopwatch */
static time_t start, stop;
/* Get a character, and keep track of elapsed time */
static char get_it(void)
{
	while (!kbhit())
	{
		time(&stop);
		if (stop - start > SAVE_SECS)
		{
			save_crt();
			time(&start); /* Screen is unsaved, reset starttime */
		}
	}
	return getch();
}

/* Keyboard input routine */
int get_byte(void)
{
    register int c;
	register int i;
    static char kys[] = ";<=>?@ABCDGHIKMOPQRS";
    static int cc[] = {F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, HOME, UP, 0,
                        '\b', FWD, END, '\n', 0, INSERT, DELETE};

	time(&start);
    if (!(c = get_it()))       /* first byte = NULL means func. key */
    {
		c = get_it();
        for (i = 0; i < sizeof kys; i++)
			if (c == kys[i])
				return(cc[i]);
    }
    if (c == 8)
        c = RUBOUT;
	return c;
}

void high_intensity(int i)
{
    _high = i;
	sgr();
}

void underline(int u)
{
    _under = u;
    sgr();
}

void inverse(int i)
{
    _inverse = i;
    sgr();
}

static void sgr(void)
{
	int attr;

	/*
		Use color scheme:
			Normal:      Yellow on Blue
			Highlight:   White on Blue
			UnderLine:   White on Blue
			High&Under:  White on Blue
			Inverse:     Blue on Gray
	*/
#if 1
	attr = ATT_NORM;
	if (_high)
		attr = ATT_HIGH;
	if (_inverse)
		attr = ATT_INVERSE;
#else
	if (_high)
		attr = 8;  /* Darkgray */
	else
		attr = 7; /* was 16 */
	if (_under)
		attr += 1;
	else if (_inverse)
		attr += (7 << 4);
#endif
	textattr(attr);
}

void save_gr(void)
{
	if (svp < SVDEPTH)
    {
        sv_ints[svp] = _high;
        sv_inv[svp] = _inverse;
		sv_und[svp++] = _under;
    }
}

void rstr_gr(void)
{
    if (svp)
    {
        _high = sv_ints[--svp];
		_inverse = sv_inv[svp];
        _under = sv_und[svp];
        sgr();
    }
}

void put_byte(int c)
{
    if (c == FWD)
        gotoxy(wherex()+1,wherey());
    else if (c == UP)
        gotoxy(wherex(),wherey()-1);
    else if (c == BELL)
	{
        sound(110);  /* Low 'A' */
        delay(100);  /* 500 ms */
		nosound();
	}
    else
		putch(c);
}

void clr_scrn()
{
	clrscr();
}

void cursorxy(int x, int y)
{
    gotoxy(x+1, y+1);
}

/* Save the screen, until a key is hit */
static void save_crt(void)
{
	void *ptr;
	int ctype;
	struct text_info ti;

	/* Get the text */
	ptr = malloc(25*80*2);
	if (ptr == NULL)
		return;
	gettext(1,1,80,25,ptr);

	/* Save everything else */
	ctype = getcursor();
	gettextinfo(&ti);

	/* Erase screen */
	window(1,1,80,25);
	cursor(CURS_OFF);
	textattr(0);
	clr_scrn();

	/* Wait for a key */
	getch();
	/* Empty buffer */
	while (kbhit())
		getch();

	/* Restore image */
	puttext(1,1,80,25,ptr);
	free(ptr);

	/* Restore settings */
	cursor(ctype);
	window(ti.winleft, ti.wintop, ti.winright /*- ti.winleft + 1 */,
			ti.winbottom /*- ti.wintop + 1*/);
	gotoxy(ti.curx, ti.cury);
	textattr(ti.attribute);
}

/*
	atexit() function to restore crt mode
*/
void restore_crt(void)
{
	textattr(orig_screen.attribute);
	window(orig_screen.winleft, orig_screen.wintop,
		   orig_screen.winright, orig_screen.winbottom);
	clrscr();
}
