/*
	TOOLBOX.C - Aangepaste en STATISCHE versie van
				de tools van C.A. de Groot
				Speciaal voor KILLUSER.EXE

	$Header: /home/cvs/repository/mine/toolbox/toolbox.c,v 1.3 1993/02/05 13:49:29 cg Rel $

	(c)1989,90,91 C.A. de Groot

 */
 /*
	$Id: toolbox.c,v 1.3 1993/02/05 13:49:29 cg Rel $
*/

/*
	wait.c - wacht via int15, funct 86

	wait in mu's (1,000,000 / sec, da's nog eens precisie)
*/
#include <dos.h>

void wait(unsigned long ms)
{
	union REGS r;

	r.x.cx = (unsigned) (ms >> 16);
	r.x.dx = (unsigned) (ms & 0x0000ffff);
	r.h.ah = 0x86;
	int86(0x15, &r, &r);
}

#include <conio.h>
#include <time.h>
#include <alloc.h>
#include <stdio.h>

static void save_crt(void);

/*
	Terminal functions
 */

/* Initialize the terminal */
void _fill_scrn(void);
static int initd = FALSE;
void init_crt(void)
{
    if (initd)
        return;
	initd = TRUE;
	clr_scrn();
	_fill_scrn();	/* In tb_wins */
    _under = _high = svp = 0;
    sgr();
}


void clr_scrn()
{
	clrscr();
}


static int ct = CURS_SMALL;
void cursor(int type)
{
    char top,bot;

    switch(type)
    {
        case CURS_SMALL: top = 11;
                         bot = 12;
                         break;
        case CURS_BIG:   top = 0;
                         bot = 12;
                         break;
		case CURS_OFF:   top = 0xff;
						 bot = 0xff;
                         break;
		default:
						 return;
    }
	ct = type;
    _CH = top;
    _CL = bot;
    _AH = 1;
    geninterrupt(0x10);
}

int getcursor(void)
{
	return ct;
}

void cursorxy(int x, int y)
{
    gotoxy(x+1, y+1);
}

/********************************************************
*   Module met exploding windows						*
*   (C)1989, C.A. de Groot              				*
********************************************************/
#include <string.h>

/* definities voor windowkader */
/* FST_GSING-style */
#define GSING_VBAR (unsigned) 179   /*'³*/
#define GSING_HBAR (unsigned) 196   /*'Ä*/
#define GSING_LTOP (unsigned) 218   /*'Ú*/
#define GSING_RTOP (unsigned) 191   /*'¿*/
#define GSING_LBOT (unsigned) 192   /*'À*/
#define GSING_RBOT (unsigned) 217   /*'Ù*/

/* FST_GDOUB-style */
#define GDOUB_VBAR (unsigned) 186   /*'º*/
#define GDOUB_HBAR (unsigned) 205   /*'Í*/
#define GDOUB_LTOP (unsigned) 201   /*'É*/
#define GDOUB_RTOP (unsigned) 187   /*'»*/
#define GDOUB_LBOT (unsigned) 200   /*'È*/
#define GDOUB_RBOT (unsigned) 188   /*'¼*/

/* FST_NOGR1-style */
#define NOGR1_VBAR (unsigned) '|'
#define NOGR1_HBAR (unsigned) '-'
#define NOGR1_LTOP (unsigned) ' '
#define NOGR1_RTOP (unsigned) ' '
#define NOGR1_LBOT (unsigned) ' '
#define NOGR1_RBOT (unsigned) ' '

/* FST_NOGR2-style */
#define NOGR2_VBAR (unsigned) '*'
#define NOGR2_HBAR (unsigned) '*'
#define NOGR2_LTOP (unsigned) '*'
#define NOGR2_RTOP (unsigned) '*'
#define NOGR2_LBOT (unsigned) '*'
#define NOGR2_RBOT (unsigned) '*'

unsigned char token[4][6] =
{
    {
        GSING_VBAR, GSING_HBAR, GSING_LTOP,
        GSING_RTOP, GSING_LBOT, GSING_RBOT
    },
    {
        GDOUB_VBAR, GDOUB_HBAR, GDOUB_LTOP,
        GDOUB_RTOP, GDOUB_LBOT, GDOUB_RBOT
    },
    {
        NOGR1_VBAR, NOGR1_HBAR, NOGR1_LTOP,
        NOGR1_RTOP, NOGR1_LBOT, NOGR1_RBOT
    },
    {
        NOGR2_VBAR, NOGR2_HBAR, NOGR2_LTOP,
        NOGR2_RTOP, NOGR2_LBOT, NOGR2_RBOT
    }
};

char *win_errmsg[] =
{
	"No error",
	"Out of memory",
	"Bad horizontal positioning",
	"Bad vertical positioning",
	"Title too long",
	"Unkown drawing style",
	"Gotowin: Invalid windowpointer",
};

static int draw(WIN *wn, int expfl);
static void gettextv(int lx, int ly, int rx, int ry, void *d);

/***************************************************
    MAKEWINC:
        controleer arguments,
        roep makewin aan.
***************************************************/

WIN *makewinc(int w, int h, char *title, int fr_attr, int wn_attr,
		int tit_attr, int style, int expflag)
{
    return(makewin((41-(w/2)),(13-(h/2)),w,h,title,fr_attr,
		wn_attr, tit_attr, style, expflag));
}

/***************************************************
    MAKEWIN:
        controleer arguments,
        save scherm,
        zet window neer.
***************************************************/

/* Lijst-pointers */
static WIN *front, *back;
/* Aantal windows */
static int num_windows;
/* Virtueel scherm */
static int vscr[25][80];
/* Info originele scherm */
static struct text_info orig_info;
/* Globale error variabele */
int win_error;

WIN *makewin(int x, int y, int w, int h,
        char *title, int fr_attr, int wn_attr,
		int tit_attr, int style, int expfl)
{
    int result;
    WIN *wn;

    /* Check arguments */
    if ((x + w - 1) > 80 || w < 3)
    {
        win_error = WN_BADWX;
        return(NULL);
    }
    if ((y + h - 1) > 24 || h < 3)
    {
        win_error = WN_BADHY;
        return(NULL);
    }
    if (strlen(title) > (w - 2))
    {
        win_error = WN_BADTIT;
        return(NULL);
    }
    if (style < FST_GSING || style > FST_NOGR2)
    {
        win_error = WN_BADSTL;
        return(NULL);
    }

    /* Arguments OK, fill in a window-structure */
    wn = malloc(sizeof (*wn));
    if (wn == NULL)
    {
        win_error = WN_NOMEM;
        return(NULL);
    }
    wn->x = x;
    wn->y = y;
    wn->w = w;
    wn->h = h;
    wn->title = title;
    wn->fr_attr = fr_attr;
    wn->wn_attr = wn_attr;
    wn->tit_attr = tit_attr;
    wn->style = style;

    /* Get memoryblocks */
    wn->save = malloc(w * h * 2);
    if (wn->save == NULL)
    {
        win_error = WN_NOMEM;
        free(wn);
        return(NULL);
    }
    wn->cont = malloc(w * h * 2);

    if (wn->cont == NULL)
    {
        win_error = WN_NOMEM;
        free(wn->save);
        free(wn);
        return(NULL);
    }

    if (front != NULL)
    {
        /* Bewaar gegevens oude window */
        gettext(front->x, front->y, front->x + front->w - 1,
            front->y + front->h - 1, front->cont);
        front->xsav = wherex();
        front->ysav = wherey();
    }
    else
    {
		/* Dit is de eerste window: vul virtuele scherm. */
        gettext(1,1,80,25,vscr);
        /* en sla text-info op */
        gettextinfo(&orig_info);
    }

	/* Sla op wat het window bedekt */
	/* Dit was gettextv !!! */
	gettext(x, y, x + w - 1, y + h - 1, wn->save);

    /* Teken window */
	result = draw(wn, expfl);

    if (result != WN_OK) {
        free(wn->save);
        free(wn->cont);
        free(wn);
        win_error = result;
        return(NULL);
    }

    /* Lees window contents */
    gettext(wn->x, wn->y, wn->x + wn->w - 1, wn->y + wn->h - 1, wn->cont);

    wn->xsav = 1;
    wn->ysav = 1;
    textattr(wn_attr);
    /* Bovenaan de lijst met windows invoegen */
    /* er is nog geen window */
    if (front == NULL)
    {
        front = wn;
        back = wn;
    }
    /* er zijn al windows */
    else
    {
        /* nieuwe window wordt bovenste window */
        front->prev = wn;
        wn->next = front;
        wn->prev = NULL;
        front = wn;
    }
    /* Alles OK */
    num_windows++;
    return(wn);
}

/***************************************************
    GOTOWIN:
        verander lijstpositie
        sla inhoud oude window op
        zet nieuwe window neer
***************************************************/
static int sect_rect(WIN *a, WIN *b);
int gotowin(WIN *wn)
{
    struct text_info ti;
	WIN *old;

    /* als het al de eerste is, hoeft er niets te gebeuren */
    if (wn == front)
        return(WN_OK);

	/* Check voor ongeldige pointer, anders loopt het programma */
	/* vrij desastreus vast */
	if (wn != wn->prev->next)
		return WN_BADPTR;

	/* Bewaar pointer naar oude window */
	old = front;

    /* Bovenaan opnieuw invoegen */
    if (wn == back)
    {
        /* window is onderste window */
        wn->prev->next = NULL;
        back = wn->prev;
    }
    else /* niet bovenste of onderste */
    {
        wn->prev->next = wn->next;
        wn->next->prev = wn->prev;
    }
    front->prev = wn;
    wn->next = front;
    wn->prev = NULL;
    front = wn;

	/* Put- en Gettext alleen nodig bij overlap */
	if (sect_rect(old, wn))
	{
	    /* Sla inhoud oude window op */
	    gettext(old->x, old->y, old->x + old->w - 1,
	            old->y + old->h - 1, old->cont);

	    /* zet nieuwe window op het scherm */
	    puttext(wn->x, wn->y, wn->x + wn->w - 1,
	            wn->y + wn->h - 1, wn->cont);
    }

	/* Sla oude attributen op */
    gettextinfo(&ti);
    old->wn_attr = ti.attribute;
    old->xsav = wherex();
    old->ysav = wherey();

    /* open nieuwe textwindow */
    window(wn->x + 1, wn->y + 1, wn->x + wn->w - 2, wn->y + wn->h - 2);

    /* ga naar oude positie */
    gotoxy(wn->xsav, wn->ysav);
    /* stel oude attribuut in */
    textattr(wn->wn_attr);

    return(WN_OK);
}


/***************************************************
    CLOSEWIN:
        sluit actieve window
        maak andere window actief
***************************************************/

int closewin(void)
{
	WIN *tmp;

	/* Restore schermgegevens */
    puttext(front->x, front->y, front->x + front->w - 1,
			front->y + front->h - 1, front->save);

    /* Geef geheugen vrij */
    free(front->save);
    free(front->cont);
	tmp = front;

    /* Haal window uit lijst */
    if (front == back) {
        front = NULL;
        back = NULL;
    }
    else
    {
        front->next->prev = NULL;
        front = front->next;
    }
    num_windows--;
    free(tmp);

    /* Ga naar eerste de beste andere window */
    if (front != NULL)
    {
		/* Alle windows opnieuw tekenen */
#if 0   /* Is dit wel noodzakelijk ???? */
        wn = back;
        while (wn != NULL)
        {
            puttext(wn->x, wn->y, wn->x + wn->w - 1,
                    wn->y + wn->h - 1, wn->cont);
            wn = wn->prev;
		}
		/* zet nieuwe window op het scherm */
		if (sect_rect(old, front))
			puttext(front->x, front->y, front->x + front->w - 1,
					front->y + front->h - 1, front->cont);
#endif
        /* open textwindow */
        window(front->x + 1, front->y + 1, front->x + front->w - 2,
                front->y + front->h - 2);

        /* ga naar oude positie */
        gotoxy(front->xsav, front->ysav);
    }
    else
    {
        /* Laatste window gesloten: zet originele scherm terug */
        puttext(1,1,80,25,vscr);
        /* en zet originele waarden terug */
        window(orig_info.winleft, orig_info.wintop, orig_info.winright,
                orig_info.winbottom);
        gotoxy(orig_info.curx, orig_info.cury);
        textattr(orig_info.attribute);
    }

    return(WN_OK);
}

/***************************************************
    DRAW:
        zet window neer
***************************************************/
static void explode(int x, int y, int w, int h, int style, int a, int f);

/* Locale variabelen voor snelheid */
static int winbld[80*24];
static int dw, dh, dwh;
static int dblank, dfa, dfv, dfh;

static int draw(WIN *wn, int expfl)
{
	register int i;
	int titlex;

	/* Kopieer naar locale opslag */
	dw = wn->w; dh = wn->h;
	dwh = dw * dh;
	dblank = 0x20 | (wn->wn_attr << 8);
	dfa = (wn->fr_attr << 8);
	dfh = token[wn->style][HBAR] + dfa;
	dfv = token[wn->style][VBAR] + dfa;


    /* neem het hele scherm */
	window(1,1,80,25);
	/* wis de window-inhoud */
	for (i = 0; i < dwh; i++)
		winbld[i] = dblank;


    /* Trek kader in 'gewone' geheugen */
    /* Horizontaal */
	for (i = 1; i < dw; i++)
    {
		winbld[i] = dfh;
		winbld[i+(dw*(dh-1))] = dfh;
    }
    /* Verticaal */
    for (i = 1; i < dh; i++)
    {
		winbld[dw*i] = dfv;
		winbld[(dw*(i+1))-1] = dfv;
    }
    /* Hoeken */
	winbld[0] = token[wn->style][LTOP] + dfa;
	winbld[(dw)-1] = token[wn->style][RTOP] + dfa;
	winbld[(dw*(dh-1))] = token[wn->style][LBOT] + dfa;
	winbld[(dw*dh)-1] = token[wn->style][RBOT] + dfa;

/* Explodeer */
	if (expfl)
		explode(wn->x, wn->y, dw, dh, wn->style,
				wn->wn_attr, wn->fr_attr);

    /* Dump kader op het scherm */
	puttext(wn->x, wn->y, wn->x + dw - 1, wn->y + dh - 1, winbld);

    /* Title */
    textattr(wn->tit_attr);
	titlex = wn->x + (dw / 2) - (strlen(wn->title) / 2);
	gotoxy(titlex - 1, wn->y);
	if (wn->title && *(wn->title))
		cprintf(" %s ", wn->title);

    /* open textwindow */
	window(wn->x + 1, wn->y + 1, wn->x + dw - 2, wn->y + dh - 2);

    /* stel goede attribuut in */
    textattr(wn->wn_attr);

    /* ga naar begin van window */
    gotoxy(1,1);

    return(WN_OK);
}



/* Exploding windows */
/* Locale variabelen voor snelheid */
static int ll, lt, lr, lb, lw, lh, lhh, lhw, lmwh, lwh, lwhm;
static int lblank, lflt, lflb, lfv, lfh, lfrt, lfrb;
static int expbld[80*24];
static int lhstep, lwstep;
#define DL_TIME 15000
void wait(unsigned long mus);

static void explode(int x, int y, int w, int h, int s, int a, int f)
{
	register int c;
	/* Kopieer alles naar locale variabelen voor top-snelheid */
	lw = w; lh = h;
	ll = x; lt = y; lr = x + lw - 1; lb = y + lh - 1;
	/* Zet ook de scherm-zaken in locale variabelen */
	lblank = 0x20 | (a << 8);
	lfh = token[s][HBAR] + (f << 8);
	lfv = token[s][VBAR] + (f << 8);
	lflt = token[s][LTOP] + (f << 8);
	lflb = token[s][LBOT] + (f << 8);
	lfrt = token[s][RTOP] + (f << 8);
	lfrb = token[s][RBOT] + (f << 8);

	/* Bereken halve hoogte/breedte */
	lhh = lh / 2;
	lhw = lw / 2;
	/* Bewaar het minimum */
	lmwh = (lhh < lhw ? lhh : lhw) - 1;
	if (lmwh < 1)
		return;
if (lhh < lhw)
{
	lhstep = 1;
/*	lwstep = (lw - (2 * (lmwh + 1)))/(lmwh + 1);*/
	lwstep = ((lw - 2) / 2) / ((lh - 2) / 2);
}
else
{
	lwstep = 1;
/*	lhstep = (lh - (2 * (lmwh + 1)))/(lmwh + 1);*/
	lhstep = ((lh - 2) / 2) / ((lw - 2) / 2);
}
	/* Bereken eerste venster */
#if 0
	ll += (lmwh + 1);
	lt += (lmwh + 1);
	lr -= lmwh;
	lb -= lmwh;
	lh -= ((lmwh * 2) + 1);
	lw -= ((lmwh * 2) + 1);
#endif

	ll += ((lmwh ) * lwstep);
	lt += ((lmwh ) * lhstep);
	lr -= lmwh * lwstep;
	lb -= lmwh * lhstep;
	lh = lb - lt + 1;
	lw = lr - ll + 1;

	/* Doe het exploderen */
	for (c = 0; c < lmwh; c++)
	{
		register int i;

		/* Pauseer even */
		wait(DL_TIME);

		/* Wis venster */
		lwh = lh * lw;
		for (i = 0; i < lwh; i++)
			expbld[i] = lblank;

		/* Trek kader in 'gewone' geheugen */
		/* Horizontaal */
		lwhm = lw * (lh - 1) ;
		for (i = 1; i < lw; i++)
		{
			expbld[i] = lfh;
			expbld[i+lwhm] = lfh;
		}
		/* Verticaal */
		for (i = 1; i < lh; i++)
		{
			expbld[lw*i] = lfv;
			expbld[(lw*(i+1))-1] = lfv;
		}
		/* Hoeken */
		expbld[0] = lflt;
		expbld[(lw)-1] = lfrt;
		expbld[lwhm] = lflb;
		expbld[lwh-1] = lfrb;

		/* Dump kader op het scherm */
		puttext(ll, lt, lr, lb, expbld);


		/* Pas grootte aan */
#if 0
		ll --;
		lt --;
		lr ++;
		lb ++;
		lh ++;
		lh ++;
		lw ++;
		lw ++;
#endif
		ll -= lwstep;
		lt -= lhstep;
		lr += lwstep;
		lb += lhstep;
		lh = lb - lt + 1;
		lw = lr - ll + 1;

	}
	/* delay voordat het definitieve venster wordt neergezet */
	wait(DL_TIME);
}

#define SP_FILL  ' ' + (0x07 << 8)
#define BL_FILL  176 + (0x70 << 8)
/* Vul regel 2-24 met blokjes */
void _fill_scrn(void)
{
	int x, y;

	for (x = 0; x < 80; x++)
	{
		for (y = 0; y < 25; y++)
			expbld[x + (y * 80)] = BL_FILL;
	}
	puttext(1, 1, 80, 25, expbld);
}

/* Kijk of twee vierhoeken elkaar snijden */
static int sect_rect(WIN *a, WIN *b)
{
	if (a->y < b->y + b->h ||
		a->x + a->w < b->x ||
		a->y + a->h > b->y ||
		a->x > b->x + b->w)
		return 1;
	else
		return 0;
}



#if 0
/***************************
****************************
MAIN: TESTROUTINE
****************************
***************************/

WIN *w[2];
static int save[80*24];

void main()
{
	int i;
	_fill_scrn();
	makewin(1, 1, 80, 3, "EXPLODE", ATT_NORM, ATT_NORM, ATT_NORM,
		FST_GDOUB, 1);

	/*getch();
	closewin();
	for (i = 1; i < 15; i++)
	{
		if (i > 1)
			puttext(i-1, i-1, i+8, i+3, save);
		gettext(i, i, i+9, i+4, save);
		puttext(i, i, i+9, i+4, winbld);
		delay(1500);
	}
	getch();        */

}
#endif


            