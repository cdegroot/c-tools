/********************************************************
*   POPMENU                                             *
*   object-module die pop-menu's implementeert          *
*                                                       *
*   declaratie:                                         *
*                                                       *
*   WIN *popmenu(int x, int y, char *title,             *
*       char *entry[], int n_entry,                     *
*       int fr_attr, int tit_attr,                      *
*       int style, int leave, int *keuze,               *
*       int def_keuze, WIN *def_win);                   *
*                                                       *
*   Geeft een pointer naar een window-struct            *
*   terug, of NULL bij een fout. Als leave              *
*   gelijk is aan NOLEAVE, is deze pointer              *
*   zinloos, en dient slechts ter onderscheid           *
*   van NULL. De keuze staat in keuze als index         *
*   in entry[] + 1. Als de keuze 0 is, is er op         *
*   ESC gedrukt.                                        *
********************************************************/
/*
	$Id: tb_popm.c,v 1.2 1993/02/05 13:02:47 cg Rel $
*/

#include <window.h>
#include <terminal.h>
#include <keys.h>
#include <string.h>
#include <dos.h>
#include <alloc.h>
#include <conio.h>

#include <stdio.h>
#define ln fprintf(stdprn,"%s:%d\n",__FILE__,__LINE__)


static int getmaxlen(char *data[], int n_data);

WIN *popmenu(int x, int y, char *title, char *entry[], int n_entry,
    int fr_attr, int tit_attr, int style, int leave,
    int *keuze, int def_keuze, WIN *def_win)
{
    WIN *wn, *wnreturn;
        int maxlen, titlen, i, key, ready = 0, newk, oldk;

    /* Bepaal breedte window */
    maxlen = getmaxlen(entry,n_entry);
    titlen = strlen(title);
    if (titlen > maxlen)
        maxlen = titlen;
    /* Maak window als def_window NULL is */
    if (def_win == NULL) {
        /* Controleer breedte window */
        if ((x + maxlen - 1) > 80)
        {
            win_error = WN_BADWX;
            return(NULL);
        }
        /* Contoleer stijl */
        if (style < 0 || style > 3)
        {
            win_error = WN_BADSTL;
            return(NULL);
        }
        /* Controller lengte window */
        if (n_entry > 23)
        {
            win_error = WN_BADHY;
            return(NULL);
        }

        /* Open window */
        wn = makewin(x, y, maxlen + 4, n_entry + 2, title,
                fr_attr, ATT_NORM, tit_attr, style,0);
        if (wn == NULL)
        {
            return(NULL);
        }
        wnreturn = wn;
    }
    else
    {
        /* Maak window actief */
        gotowin(def_win);
        wnreturn = def_win;
    }
    /* Zet keuzes in window */
    for (i = 0; i < n_entry; i++)
    {
        gotoxy(2, 1+i);
        cprintf(entry[i]);
    }
    /* Maak eerste keuze zichtbaar */
    fieldattr(1, def_keuze, maxlen+2, ATT_INVERSE);


    /* Cosmetica: zet cursor uit */
    cursor(CURS_OFF);
    /* Lus voor maken van de keuze */
    oldk = newk = def_keuze;
    while (!ready)
    {
        key = get_byte();

        /* Check voor extended codes */
        if (key < 32) /* Speciale toetsen */
        {
            switch (key)
            {
                case UP: newk = oldk - 1;
                     if (newk == 0)
                        newk = n_entry;
                     break;
                case '\n': newk = oldk + 1;
                     if (newk == (n_entry+1))
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
        /* Geen extended code: Ga door */
        else
			continue;
        /* Verversen van het menu */
        fieldattr(1, oldk, maxlen+2, ATT_NORM);
        fieldattr(1, newk, maxlen+2, ATT_INVERSE);
        oldk = newk;
    }
    /* Keuze is gemaakt */
    *keuze = newk;
    cursor(CURS_SMALL);

    /* Kijk of het window moet blijven staan */
    if (leave == NOLEAVE)
        closewin();


    /* Klaar, return windowpointer */
    return(wnreturn);
}

WIN *popmenuc(char *title, char *entry[], int n_entry,
    int fr_attr, int tit_attr, int style, int leave,
    int *keuze, int def_keuze, WIN *def_win)
{
    int maxlen, titlen;

    maxlen = getmaxlen(entry,n_entry);
    titlen = strlen(title);
    if (titlen > maxlen)
        maxlen = titlen;
    return(popmenu((41-((maxlen+4)/2)),(13-((n_entry+2)/2)), title,
            entry, n_entry, fr_attr, tit_attr, style, leave,
            keuze, def_keuze, def_win));
}

static int getmaxlen(char *data[], int n_data)
{
    int i, max = 0, len;

    for (i = 0; i < n_data; i++)
    {
        len = strlen(data[i]);
        if (len > max)
            max = len;
    }
    return(max);
}



#if 0


/**********************
***********************
MAIN: TESTFUNCTIE
***********************
**********************/

char *entry[5] =
{
    "Keuze 1",
    "Tweede keus",
    "Keuze numero tres",
    "Vier",
    "Laatste mogelijkheid"
};

void main(void)
{
        WIN *wn, *wn2;
    int keuze1, keuze2;

    wn = popmenu(10, 10, "Testmenu", entry, 5,
        ATT_HIGH, ATT_INVERSE, FST_GSING, LEAVE,
        &keuze1, 1);
    wn2 = popmenu(15, 10+1+keuze1, "Testmenu", entry, 5,
        ATT_HIGH, ATT_INVERSE, FST_GSING, NOLEAVE,
        &keuze2, 3);
    closewin();
    cprintf("Keuze1 = %d\n",keuze1);
    cprintf("Keuze2 = %d\n",keuze2);
}

#endif
