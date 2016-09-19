/***************************************************
*   ED.EXE, programma voor het enig document       *
*   (C)1989, C.A. de Groot                         *
*                                                  *
*   MODULE: RECORDIO.C                             *
*       bevat I/O geindexeerde records             *
***************************************************/
/*
	$Id: tb_rcdio.c,v 1.2 1993/02/05 13:02:50 cg Rel $
*/

/**************************************************/
/* Include files library                          */
/**************************************************/
#include <mem.h>

#include <window.h>
#include <toolset.h>
#include <file.h>
#include <btree.h>
#include <screen.h>
#include <terminal.h>
#include <keys.h>

#include <stdio.h>
/**************************************************/
/* Include files programma                        */
/**************************************************/
#include <recordio.h>
/**************************************************/
/* Macro's en constanten                          */
/**************************************************/

/**************************************************/
/* Locale datatypen                               */
/**************************************************/

/**************************************************/
/* Locale data                                    */
/**************************************************/

/**************************************************/
/* Locale functiedeclaraties                      */
/**************************************************/

/**************************************************/
/* Globale functiedefinities                      */
/**************************************************/

/* Schrijf een veranderd record terug of maak een nieuw record */
void rtn_rcd(rcd_ctl *r)
{
    register char *s = r->data;
    register char *d = r->hold;
    int i = r->len;

    if (r->exfl)
    {
        /* Kijk of record veranderd is */
        while (i)
            if (*s++ != *d++)
                break;
            else
                i--;
        if (!i)
            return;
        put_record(r->df, r->adr, r->data);
    }
    else
    {
        /* Kijk of record ingevuld is */
        while (i)
            if (*s == ' ' || *s == '\0')
            {
                s++;
                i--;
            }
            else
                break;
        if (!i)
            return;
        r->adr = new_record(r->df, r->data);
        for (i = 0; i < MAXIKEYS; i++)
            if (r->ndx[i].used)
                insert_key(r->ndx[i].bt, (char *) r->data + r->ndx[i].ky_off,
                            r->adr, r->ndx[i].unq);
    }
    /* Bewaar nieuwe record in hold-area */
    movmem(r->data, r->hold, r->len);
}


/* Leeg record areas */
void clr_rcd(rcd_ctl *r)
{
    int i;

    r->exfl = FALSE;

    setmem(r->data, r->len, ' ');
    setmem(r->hold, r->len, ' ');

    for (i = 0; i < MAXIKEYS; i++)
        if (r->ndx[i].prot)
            *(r->ndx[i].prot) = FALSE;
    /* Roep functie aan */
    if (r->clr_fptr != NULL)
        (*r->clr_fptr)();
}


/* Haal een record op */
void get_rcd(rcd_ctl *r)
{
    int i;

    r->exfl = TRUE;
    get_record(r->df, r->adr, r->data);
    movmem(r->data, r->hold, r->len);
    for (i = 0; i < MAXIKEYS; i++)
        if (r->ndx[i].prot)
            *(r->ndx[i].prot) = TRUE;
    /* Roep functie aan */
    if (r->get_fptr != NULL)
        (*r->get_fptr)();
}

/* Haal volgende record op */
void get_nxt(rcd_ctl *r, int key_no)
{
    ADDR ad;
    clrmsg();

    if ((ad = get_next(r->ndx[key_no].bt)) == (ADDR) 0)
        notice ("End of file");
    else
    {
        r->adr = ad;
        get_rcd(r);
    }
}

/* Haal vorige record op */
void get_prv(rcd_ctl *r, int key_no)
{
    ADDR ad;

    clrmsg();

    if ((ad = get_prev(r->ndx[key_no].bt)) == (ADDR) 0)
        notice ("Beginning of file");
    else
    {
        r->adr = ad;
        get_rcd(r);
    }
}

static char *oldstat;


void process_records(rcd_ctl *r, struct scrn_buf *s, int mk, int sn)
{
    char term = '\0';

	/* Statusregel, bewaar de oude */
	oldstat = default_status;
    default_status = "F1 = Bewaar | F9/F10 = Vorig/Volgend | Esc = Einde";
	notice(default_status);

    /* Processlus */
    while (term != ESC)
    {
        term = scrn_proc(sn, s, FALSE);
        switch(term)
        {
            case F10:       /* NEXT RECORD */
                rtn_rcd(r);
                get_nxt(r, mk);
                break;
            case F9:        /* PREVIOUS RECORD */
                rtn_rcd(r);
                get_prv(r, mk);
                break;
            case F7:        /* DELETE RECORD */
                if (r->exfl)
                {
                    err_msg("Bevestig wissen met F7");
                    if (get_byte() == F7)
                    {
                        int i;

                        delete_record(r->df, r->adr);
                        for (i = 0; i < MAXIKEYS; i++)
                            if (r->ndx[i].used)
                                delete_key(r->ndx[i].bt,
										(char *) r->data + r->ndx[i].ky_off,
                                        r->adr);
                        clr_rcd(r);
                        r->adr = 0;
                        clrmsg();
                    }
                    else
                        notice("Niet gewist");
				}
				else
					clr_rcd(r);
                break;
            case GO:        /* WRITE RECORD */
                rtn_rcd(r);
                clr_rcd(r);
                break;
        }
    }
	default_status = oldstat;
	clrmsg();
}
