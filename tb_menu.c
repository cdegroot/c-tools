/*
	menu.c

	pop-up (pull-down) menu's via structuren gestuurd;
	top-line menu voor event-programmeren (bestuurt
	tevens alle submenu's
*/
/*
	$Id: tb_menu.c,v 1.2 1993/02/05 13:02:46 cg Rel $
*/

#include <menu.h>
#include <keys.h>
#include <terminal.h>

#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <bios.h>
#include <stdio.h>
#define ln fprintf(stdprn,"%s:%d\n",__FILE__,__LINE__)
/* Default stijl-variabelen, door de applicatie in te stellen */
int		framestyle = FST_GSING,
		titleattr  = ATT_NORM,
		frameattr  = ATT_INVERSE;

static int getmaxlen(struct menu_item *mi, int n);
static struct text_info ti;

int menu_exec(struct linem *lm)
{
	int totlen;		/* totale lengte items */
	int i;
	int spaces;		/* te verdelen spaties */
	int spi;		/* spaces per item */
	int x;
	int len[MXSELS];	/* Lengte van items */
	int pos[MXSELS];	/* Positie van items */
	int oldk, newk, ready, key;
	int popm_flag = 0;	/* Display popmenu ja/nee */
	int popm_return;	/* Return-key van popmenu */

	totlen = 0;
	/* Bepaal totale lengte items */
	for (i = 0; i < lm->ns; i++)
	{
		len[i] = strlen(lm->mi[i].description);
		totlen += len[i];
	}

	/* Genoeg ruimte ?? */
    if ((totlen + lm->ns + 1) > 80)
		return ERROR;

	/* Verdeel en heers */
	spaces = 80 - totlen;	/* Totale hoeveelheid spaties */
	spi = spaces / (lm->ns + 1);
	/* Stel posities in */
	x = spi + 1;
	for (i = 0; i < lm->ns; i++)
	{
		pos[i] = x;
		x += (spi + len[i]);
	}
	/* Bewaar tekst-instellingen */
	gettextinfo(&ti);

	/* Stel het goede venster in: gehele scherm */
	window(1,1,80,25);

	/* Maak de regel */
	gotoxy(1,1);
	for (i = 1; i <= 80; i++)
		putch(' ');
	fieldattr(1,1,80,ATT_INVERSE);
	for (i = 0; i < lm->ns; i++)
	{
		gotoxy(pos[i], 1);
		cprintf(lm->mi[i].description);
		fieldattr(pos[i], 1, len[i], ATT_INVERSE);
	}

    /* Maak eerste keuze zichtbaar */
	if (!lm->def_keuze)
		lm->def_keuze = 1;
    fieldattr(pos[lm->def_keuze - 1], 1,
			  len[lm->def_keuze - 1], ATT_NORM);

    /* Cosmetica: zet cursor uit */
    cursor(CURS_OFF);
    /* Lus voor maken van de keuze */
    oldk = newk = lm->def_keuze;
	ready = popm_return = 0;
    while (!ready)
    {
		if (!popm_return)
		{
	        /* Lees toets */
	        key = get_byte();
		}
		else
		{
			/* Verwerk toetsaanslag van popmenu */
			key = popm_return;
			/* Zet variabele voor volgende keer weer op nul */
			popm_return = 0;
		}

		if (key < 32) /* Functietoetsen */
		{
	        switch (key)
	        {
	            case UP:	/* Geen effect */
					break;
	            case '\n':	/* Ga naar pop-menu, indien van toepassing */
					if (lm->mi[oldk-1].t_type == TR_MENU)
						popm_flag = 1;
					break;
				case '\b':
					newk = oldk - 1;
					if (newk == 0)
						newk = lm->ns;
					break;
				case FWD:
					newk = oldk + 1;
					if (newk > lm->ns)
						newk = 1;
					break;
				case '\r':
					ready = 1;
					break;
				case ESC:
                    ready = 1;
					newk = 0;
					break;
				default:
					continue;
            }
        }
        else
		{
			/* Kijk of een high-video karakter is ingetoetst */
			int i;

			for (i = 0; i < lm->ns; i++)
				if (toupper(lm->mi[i].description[lm->mi[i].hv_pos]) ==
					toupper(key))
				{
					ready = 1;
					newk = i + 1;
					break;
				}
		}

        /* Verversen van het menu */
		if (oldk != newk)
		{
		    fieldattr(pos[oldk-1],1,
			  len[oldk - 1], ATT_INVERSE);
		    fieldattr(pos[newk-1],1,
			  len[newk - 1], ATT_NORM);
#ifdef DISP_LETTER
			fieldattr(pos[oldk-1] + lm->mi[oldk-1].hv_pos,
					1, 1, ATT_NORM);
#endif
    	    oldk = newk;
		}

		/* Uitvoeren van trigger-actie indien klaar met keuze, en geen Esc */
		if ((ready == 1 && newk != 0) ||
			(popm_flag && lm->mi[newk-1].t_type == TR_MENU))
		{
			if (lm->mi[newk-1].t_type == TR_MENU)
			{
				struct popm *pm = lm->mi[newk-1].trigmenu;

				/* Stel juiste (x,y) in */
				pm->y = 2;
				pm->x = pos[newk-1] - 1;

				/* Zet menu op het scherm */
				popm_return = popmenu_struct(pm);

				/* ESC zet popm_flag uit */
				if ((char) popm_return == (char) 27)
					popm_flag = 0;
				else
					popm_flag = 1;
				/* Geef toets links/rechts door */
				if (popm_return != '\b' && popm_return != FWD)
					popm_return = 0;
			}
			else
			{
				cursor(CURS_SMALL);
			    fieldattr(pos[newk-1],1,
				  len[newk - 1], ATT_INVERSE);
#ifdef  DISP_LETTER
				fieldattr(pos[newk-1] + lm->mi[newk-1].hv_pos,
					1, 1, ATT_NORM);
#endif
				window(ti.winleft, ti.wintop, ti.winright, ti.winbottom);
				(*(lm->mi[newk-1].trigfunc))();
				gettextinfo(&ti);
				window(1,1,80,25);
			    fieldattr(pos[newk-1],1,
				  len[newk - 1], ATT_NORM);
				cursor(CURS_OFF);
			}
        	ready = 0;
		}
    }
    cursor(CURS_SMALL);
	window(ti.winleft, ti.wintop, ti.winright, ti.winbottom);
	return OK;
}





int popmenu_struct(struct popm *pm)
{
    int maxlen, titlen, i, key, ready = 0, newk, oldk;
	WIN *wn;

    /* Bepaal breedte window */
    maxlen = getmaxlen(pm->mi, pm->ns);
    titlen = strlen(pm->title);
    if (titlen > maxlen)
        maxlen = titlen;

    /* Controleer breedte window */
	if (maxlen > 76)
	{
		win_error = WN_BADWX;
		return(ERROR);
	}
    /* Controleer lengte window */
    if (pm->ns > 22)
    {
        win_error = WN_BADHY;
        return(ERROR);
    }
	/* Controleer of window binnen scherm valt */
    if ((pm->x + maxlen + 4) > 80)
		/* Pas x-waarde aan */
		pm->x = 80 - (maxlen + 4);
	if ((pm->y + pm->ns + 2) > 22)
		/* Pas y-waarde aan */
		pm->y = 22 - (pm->ns + 4);

    /* Open window */
    wn = makewin(pm->x, pm->y, maxlen + 4, pm->ns + 2, pm->title,
					frameattr, ATT_INVERSE, titleattr, framestyle,0);
	if (wn == NULL)
        return(ERROR);

    /* Zet keuzes in window */
    for (i = 0; i < pm->ns; i++)
    {
        gotoxy(2, 1+i);
        cprintf(pm->mi[i].description);
		if (pm->mi[i].t_type == TR_PROT)
			fieldattr(1, i + 1, maxlen+1, ATT_HIGH);
		/* Zet het juiste karakter in high-video */
		fieldattr(2+pm->mi[i].hv_pos, 1+i, 1, ATT_NORM);
    }
    /* Maak eerste keuze zichtbaar */
	if (!pm->def_keuze)
		pm->def_keuze = 1;
	while (pm->mi[pm->def_keuze-1].t_type == TR_PROT)
		pm->def_keuze++;
	fieldattr(1, pm->def_keuze, maxlen+2, ATT_NORM);


    /* Lus voor maken van de keuze */
    oldk = newk = pm->def_keuze;
    while (!ready)
    {
        /* Lees toets */
        key = get_byte();

        /* Check voor speciale toetsen */
        if (key < 32)
        {
            switch (key)
            {
                case UP: newk = oldk - 1;
                    if (newk <= 0)
                       newk = pm->ns;
                    while (pm->mi[newk-1].t_type == TR_PROT)
					{
						newk--;
	                    if (newk <= 0)
    	                   newk = pm->ns;
					}
                    break;
                case '\n': newk = oldk + 1;
					if (newk >= (pm->ns + 1))
                        newk = 1;
                    while (pm->mi[newk-1].t_type == TR_PROT)
					{
						newk++;
	                    if (newk >= (pm->ns + 1))
    	                    newk = 1;
					}
                    break;
				case '\b':
				case FWD:
					newk = 0;
					ready = 1;
					break;
				case '\r':
					ready = 1;
					break;
				case ESC:
					ready = 1;
					newk = 0;
                default: continue;
            }
        }
        /* Geen extended code */
        else
        {
			/* Kijk of een high-video karakter is ingetoetst */
			int i;

			for (i = 0; i < pm->ns; i++)
				if (toupper(pm->mi[i].description[pm->mi[i].hv_pos]) ==
					toupper(key) && pm->mi[i].t_type != TR_PROT)
				{
					ready = 1;
					newk = i + 1;
					break;
				}
		}

        /* Verversen van het menu */
		if (newk && oldk != newk)
		{
			fieldattr(1, oldk, maxlen+2, ATT_INVERSE);
			fieldattr(2+pm->mi[oldk-1].hv_pos, oldk, 1, ATT_NORM);
			fieldattr(1, newk, maxlen+2, ATT_NORM);
	        oldk = newk;
		}
		/* Uitvoeren van trigger-actie indien klaar met keuze, en geen Esc */
		if (ready == 1 && newk != 0)
		{
			if (pm->mi[newk-1].t_type == TR_MENU)
			{
				struct popm *sm = pm->mi[newk-1].trigmenu;

				sm->x = wn->x + 5;
				sm->y = wn->y + newk + 1;
				popmenu_struct(sm);
			}
			else
			{
				fieldattr(1, newk, maxlen+2, ATT_INVERSE);
				cursor(CURS_SMALL);
				window(ti.winleft, ti.wintop, ti.winright, ti.winbottom);
				(*(pm->mi[newk-1].trigfunc))();
				gettextinfo(&ti);
				window(wn->x + 1, wn->y + 1, wn->x + wn->w - 2,
						wn->y + wn->h - 2);
				cursor(CURS_OFF);
				/* Kijk of er speciale acties vereist zijn */
				if (pm->mi[newk-1].t_type == TR_FCHC)
				{
					/* Re-display huidige entry */
			        gotoxy(2, newk);
					cprintf(pm->mi[newk-1].description);
					fieldattr(1, newk, maxlen+2, ATT_NORM);
				}
				if (pm->mi[newk-1].t_type == TR_FCHA)
				{
					int i;
					/* Re-display alle entries */
					for (i = 0; i < pm->ns; i++)
					{
				        gotoxy(2, 1+i);
				        cprintf(pm->mi[i].description);
						if (pm->mi[i].t_type == TR_PROT)
							fieldattr(1, i + 1, maxlen+2, ATT_HIGH);
						/* Zet het juiste karakter in high-video */
						fieldattr(2+pm->mi[i].hv_pos, 1+i, 1, ATT_NORM);
					}
				}
				/* Zet de huidige entry in inverse */
				fieldattr(1, newk, maxlen+1, ATT_NORM);
			}
			/* Uitgevoerd: niet klaar, terug naar window */
			gotowin(wn);
        	ready = 0;
		}
    }
	/* Stel nieuwe default-keuze in */
	pm->def_keuze = (newk == 0 ? oldk : newk);
	/* Sluit venster */
	closewin();
	return key;
}

static int getmaxlen(struct menu_item *mi, int n)
{
    int i, max = 0, len;

    for (i = 0; i < n; i++)
    {
        len = strlen(mi[i].description);
        if (len > max)
            max = len;
    }
    return(max);
}

/**********************
***********************
MAIN: TESTFUNCTIE
***********************
**********************/
#if 0


mfptr editor
{
	makewinc(15,3, "Editor", ATT_NORM, ATT_NORM, ATT_NORM, FST_GDOUB,1);
	cprintf("Hit a key...");
	getch();
	closewin();
}

mfptr load_func
{
	makewinc(15,3, "Loader", ATT_NORM, ATT_NORM, ATT_NORM, FST_GDOUB,1);
	cprintf("Hit a key...");
	getch();
	closewin();
}

mfptr save_func
{
	makewinc(15,3, "Saver", ATT_NORM, ATT_NORM, ATT_NORM, FST_GDOUB,1);
	cprintf("Hit a key...");
	getch();
	closewin();
}


struct popm file_menu =
{
	"",
	2,
	0,0,
	9,
	{
		{"Load      F3", 0, load_func, NULL, TR_FUNC},
		{"Pick  Alt-F3", 0, load_func, NULL, TR_FUNC},
		{"New", 0, load_func, NULL, TR_FUNC},
		{"Save      F2", 0, save_func, NULL, TR_FUNC},
		{"Write to", 0, load_func, NULL, TR_FUNC},
		{"Directory", 0, load_func, NULL, TR_FUNC},
		{"Change Dir", 0, load_func, NULL, TR_FUNC},
		{"OS Shell", 0, load_func, NULL, TR_FUNC},
		{"Quit   Alt-X", 0, load_func, NULL, TR_FUNC},
	},
};

struct popm proj_menu =
{
	"",
	2,
	0, 0,
	3,
	{
		{"Project name   MENUTEST.PRJ ", 0, editor, NULL, TR_FUNC},
		{"Break make on  Fatal Errors", 0, editor, NULL, TR_FUNC},
		{"Clear project", 0, editor, NULL, TR_FUNC},
	},
};

struct popm comp_menu =
{
	"",
    4,
	0, 0,
	5,
	{
		{"Compile to OBJ   D:MENU.OBJ",0,editor,NULL,TR_FUNC},
		{"Make EXE file    D:MENUTEST.EXE",0,editor,NULL,TR_FUNC},
		{"Link EXE file",0,editor,NULL,TR_FUNC},
		{"Build all",0,editor,NULL,TR_FUNC},
		{"Primary C file:",0,editor,NULL,TR_FUNC},
	}
};

struct popm opt_menu =
{
	"",
    4,
	0, 0,
	7,
	{
		{"Compiler",0,editor,NULL,TR_FUNC},
		{"Linker",0,editor,NULL,TR_FUNC},
		{"Environment",0,editor,NULL,TR_FUNC},
		{"Directories",0,editor,NULL,TR_FUNC},
		{"Args",0,editor,NULL,TR_FUNC},
		{"Retreive options",0,editor,NULL,TR_FUNC},
		{"Store    options",0,editor,NULL,TR_FUNC},
	}
};

mfptr track_func;
struct popm debug_menu =
{
	"",
    1,
	0, 0,
	4,
	{
		{"Track messages    Current file ",0,track_func,NULL,TR_FCHC},
		{"Clear messages",0,editor,NULL,TR_FUNC},
		{"Keep  messages",0,editor,NULL,TR_FUNC},
		{"Available Memory  137K",0,NULL,NULL,TR_PROT},
	}
};

int tr = 0;
char *trackstrings[] =
{
	"Current file",
	"All files   ",
	"Off         ",
};

mfptr track_func
{
	tr++;
	if (tr > 2)
		tr = 0;
	strcpy((debug_menu.mi[0].description)+18,trackstrings[tr]);
}

struct linem main_menu =
{
	1,
	7,
	{
		{"File", 0, NULL, &file_menu, TR_MENU},
		{"Edit", 0, editor, NULL, TR_FUNC},
		{"Run",  0, editor, NULL, TR_FUNC},
		{"Compile",0,NULL, &comp_menu, TR_MENU},
		{"Project",0,NULL, &proj_menu, TR_MENU},
		{"Options",0,NULL, &opt_menu, TR_MENU},
		{"Debug",0,NULL, &debug_menu, TR_MENU},
	},
};


void main(void)
{
	init_crt();
	menu_exec(&main_menu);
}
#endif
