#pragma startmhdr
/******************************************************************

	C Standaarddocumentatie - (c)1990 C.A. de Groot

	Project      :	TOOLBOX

	Module       :	tb_screen.c

	Beschrijving :	In deze module bevinden zich alle functies
					die te maken hebben met scherm I/O op hoog
					niveau. Schermdefinities worden apart opgesla-
					gen, en er wordt gebruik gemaakt van controle-
					structuren die door de aanroepende functies
					geinitieerd worden.

*******************************************************************/
/*
	$Id: tb_scrn.c,v 1.3 1993/02/05 13:02:53 cg Rel $
*/
#pragma endhdr

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mem.h>
#include <dir.h>
#include <conio.h>
#include <ctype.h>

#include <window.h>
#include <toolset.h>
#include <screen.h>
#include <terminal.h>
#include <keys.h>



#define PADCHAR 26  /* padding character in an ASCII file */

static struct
{
    int no_flds;                    /* number of fields on the screen */
	char *s_mptr;                   /* ->screen image */
	int xt;                         /* window left */
	int yt;                         /* window top */
	int xs;                         /* window height */
	int ys;                         /* window width */
	char title[80];                 /* window title */
	WIN *wn;                        /* window pointer */
	struct
	{
		char *f_mptr;               /* ->field mask */
		int f_len;                  /* field length */
		int f_x;                    /* x coordinate */
		int f_y;                    /* y coordinate */
		int din;                    /* true = data has been entered */
	} f_[MAXFIELDS];
} s_[MAXSCRNS];

static int open_seq[MAXSCRNS];		/* window open sequence */
static int seq_no;					/* current screen sequence number */

static int nbrcrts = 0;             /* number of screens in file */
static int inserting = FALSE;       /* insert mode */
static int last_x = 0, last_y = 0;  /* current curs. location */
extern int _curr_crt;               /* current crt number */
static int msg_up = FALSE;          /* message line flag */
static int first_key = TRUE;		/* TRUE if first key after Enter */

/* Hook voor IDI, indien gewenst */
#ifdef IDI_ON
int	IDI_SWITCH;						/* TRUE: IDI staat aan */
int IDI_FLAGS[MAXFIELDS];			/* Flags voor de velden,
									    0 = GEEN IDI,
										Andere waarde: wel */
static int IDI_CF;					/* Huidige vlag */
static WIN *IDI_WN;
void (*IDI_FP)(int IDI_CF, const char *b);	/* Functie pointers */
void (*END_FP)(int IDI_CF, char *b, char k);
#endif

/* Local routines */
static void display_crt(int crt);
static char *find_field(int crt, int field);
static char get_data(char type, char *mask, char *buffer);
static int  isnumr(char c);
static void r_just(char *s);
static void z_just(char *s);
static void zl_just(char *s);
static int  edit_date(char *date);
static void disp_fld(char *buffer, char *mask);
static void insert_line(void);
static int  punct(char c);

#define ln fprintf(stdprn,"LINE %d\n",__LINE__);

#pragma startfhdr
/******************************************************************

	Functie			:	void read_screens(const char *screen_name);

	Gebruik         :	Inlezen scherm lay-out.

	Prototype in    :	screen.h

	Beschrijving    :	read_screens leest en parset een ASCII-file
						met naam screen_name, waarin de lay-out
						van een of meerdere schermen is vastgelegd.

						De lay-out moet aan de volgende voorwaarden
						voldoen:

						- Elk scherm moet beginnen met een asterisk
						  (*), waarna er vier getallen volgen met
						  de grootte en plaatsing van het venster in
						  de volgorde: kolom, rij van de linkerboven-
						  hoek; breedte en hoogte van het kader.
						  Hierna volgt een titel, die boven het
						  venster geplaatst wordt. De asterisk mag
						  nergens anders gebruikt worden.
						- Binnen een definitie worden underscores
						  (_) gebruikt om een veld aan te duiden.
						  Binnen een veld mogen als punctuatie-tekens
						  voorkomen: punt, komma, haakjes, koppelteken,
						  schuine streep voorwaarts, dubbele punt.
						- Er mag verder overal tekst voorkomen.
						- Tabs worden geexpandeerd naar vier spaties.

						In het algemeen ziet een lay-out er exact het-
						zelfde uit als het resultaat in een invoer-
						venster.

	Terugkeerwaarde :	Geen. Het programma genereert een fatale fout
						indien het bestand niet wordt gevonden.

*******************************************************************/
#pragma endhdr

void read_screens(const char *screen_name)
{
	FILE *sc_file;
    int x = 0, y = 0, titlen;
	int c;
	register char *scp = NULL;
	int sc = 0;
	int fl = 0;
	char *cct;
	char full_name[MAXPATH];

	/*
		Make full pathname
	*/
	strcpy(full_name, SCRN_FILES);
	strcat(full_name, screen_name);

	seq_no = 0;
	setmem(s_, sizeof s_, '\0');
	init_crt();
	_curr_crt = 0;
	if ((sc_file = fopen(full_name, "rt")) == NULL)
	{
		printf("\n%s",full_name);
		fatal(4);
	}

	/* Read in the file of screen templates */
	while ((c = getc(sc_file)) != EOF && c != PADCHAR)
	{
        switch (c)
        {
			case SCRNID:            /* screen id character */
                if (scp)            /* if this isn't the first */
                {
                    sc++;
					scp--; /* Erase previous newline */
					*scp = (char) c;
                }
                /* Read window info into s_ */
				if (fscanf(sc_file,"%2d%2d%2d%2d",
						&(s_[sc].xt), &(s_[sc].yt),
                        &(s_[sc].xs), &(s_[sc].ys)) != 4)
				{
					printf("\n%s",full_name);
                    fatal(2);
                }
                /* Read title */
                titlen = 0;
                while ((c = getc(sc_file)) != '\n' && c != EOF &&
                        c != PADCHAR)
                    if (titlen < 80)
						s_[sc].title[titlen++] = c;

                fl = 0;
                if ((cct = scp = malloc(SCBUFF)) == NULL)
                    fatal(5);
				setmem(scp, SCBUFF, '\0');
                s_[sc].s_mptr = scp;
                x = y = 0;
                break;
            case FLDFILL:
				s_[sc].no_flds++;           /* count fields */
				s_[sc].f_[fl].f_mptr = scp; /*point to mask */
				s_[sc].f_[fl].f_x = x;
				s_[sc].f_[fl].f_y = y;
                s_[sc].f_[fl].din = FALSE;
                while(c == FLDFILL || punct(c))
                {
                    if (c == FLDFILL)
                        s_[sc].f_[fl].f_len++;
                    *scp++ = (char) c;
					x++;
                    c = getc(sc_file);
                }
                *scp++ = '\0';              /* null term at end fld */
				ungetc(c, sc_file);
				fl++;
                break;
            case '\n':
                x = 0;
				y++;
				*scp++ = '\r';				/* Voeg CR in */
				*scp++ = (char) c;
                break;
            case '\t':
                x = ((x / HT) * HT) + HT;
                *scp++ = (char) c;
				break;
			default:
                if (c >= ' ')
                {
					x++;
                    *scp++ = (char) c;
                }
                break;
        }
		if (scp > cct + SCBUFF)
            fatal(7);
	}
	/*
		If errno is set, we didn't reach EOF normally
	*/
/*	if (errno)
		fatal(11);*/

	fclose(sc_file);
    nbrcrts = sc;
    *scp = SCRNID;
/*for (x = 0; x < 2; x++)
fprintf(stdprn,"field %1d(%02d,%02d): len = %03d, mask = %s\n",
x, s_[0].f_[x].f_x, s_[0].f_[x].f_y, s_[0].f_[x].f_len,
(s_[0].f_[x].f_mptr == NULL) ? "(empty)" : s_[0].f_[x].f_mptr);*/
}

#pragma startfhdr
/******************************************************************

	Functie			:	void reset_screens(void);

	Gebruik         :	Vrijgeven schermdefinitie.

	Prototype in    :	screen.h

	Beschrijving    :	reset_screens wordt aangeroepen als men
						klaar is met de I/O. De functie geeft al
						het geheugen, waarin de lay-out was opge-
						slagen weer vrij, zodat de module klaar is
						voor een nieuwe aanroep van read_screens.

	Terugkeerwaarde :	Geen.

*******************************************************************/
#pragma endhdr

void reset_screens(void)
{
	/*
		Free mem from nbrcrts to zero
	*/
	while (nbrcrts != (-1))
		free(s_[nbrcrts--].s_mptr);

	/*
		Close windows in opening sequence
	*/
	while (--seq_no != (-1))
	{
		if (s_[open_seq[seq_no]].wn)
		{
			if (gotowin(s_[open_seq[seq_no]].wn))
				fatal(10);
			if (closewin())
				fatal(10);
			s_[open_seq[seq_no]].wn = NULL;
		}
	}
	seq_no = 0;
	underline(FALSE);
	high_intensity(FALSE);
}

#pragma startfhdr
/******************************************************************

	Functie			:	char scrn_proc(int scrn_no,
							struct scrn_buf *scrn_data,
							int go_last);

	Gebruik         :	Uitvoeren van scherm I/O.

	Prototype in    :	screen.h

	Beschrijving    :	Dit is de hoofdroutine van de scherm-module.
						Deze routine doet de complete afhandeling
						van invoer van een scherm, totdat er op een
						speciale toets wordt gedrukt.

						scrn_no geeft het nummer van het scherm aan
						binnen het lay-out bestand. go_last is een
						flag (TRUE of FALSE) die aangeeft of pas
						teruggekeerd moet worden na een funcietoets.
						Indien deze flag FALSE is, keert de routine
						terug na het invullen van het laatste veld
						van het scherm.

						scrn_data verwijst naar een array van structures
						die als volgt gedefinieerd zijn (in screen.h):

						struct scrn_buf
						{
							char item_type;
							int protect;
							char *item_value;
							edit_func (*editfptr);
							help_func (*helpfptr);
							int fill_data;
							int ditto;
							char *dit_ptr;
						};

						Voor elk veld in de lay-out moet er een array-
						element zijn. De structure-velden hebben de
						volgende betekenissen:

						item_type:	Geeft het type data aan:
									'C' : Calculator. Invoer begint
									voor de decimale punt, waarbij het
									getal naar links loopt. Als er op
									de punt-toets of het pijltje naar
									rechts indrukt, kan men het gedeelte
									achter de decimale punt invullen.
									'A' : Alfa-numeriek.
									'D' : Datum type ddmmjj. Wordt gecon-
									troleerd op geldigheid.
									'Z' : Getal, wordt links uitgevuld
									met nullen.
									'N' : Getal, wordt links uitgevuld
									met spaties.
						protect:	TRUE of FALSE. Indien TRUE, kan het
									veld niet ingevoerd worden.
						item_value: Pointer naar de buffer bij het veld.
						editfptr:	Pointer naar een functie die aange-
									roepen wordt als het veld wordt ver-
									laten. De definitie is als volgt:

									typedef int edit_func(char *s, char k);

									s wijst naar de buffer, k wijst naar de
									toets waarmee het veld verlaten werd.
									De functie kan een controle op het veld
									uitvoeren, s veranderen en die met
									de functie backfill weer op het scherm
									zetten.
						helpfptr:	Pointer naar een functie die aangeroepen
									wordt als er op HELP gedrukt wordt. De
									definitie is als volgt:

									typedef char *help_func(void);

									De geretourneerde string wordt in de
									schermbuffer geplaatst. Indien van de-
									ze faciliteit geen gebruik wordt gemaakt,
									moet een null string ("") worden terug-
									gestuurd.

						fill_data:  TRUE of FALSE. Geeft aan of de routine
									het veld moet invullen als het scherm
									voor het eerst vertoond wordt.

						ditto:		TRUE of FALSE. Geeft aan of een auto-
									matische DITTO wordt gebruikt. Als
									dit zo is, dan wordt automatisch
									item_value vanuit dit_ptr gevuld.

						dit_ptr:	Houder voor de DITTO-waarde. De scherm-
									routine slaat hier de waarde van de
									vorige keer in op, zodat de gebruiker
									met behulp van de DITTO-toets een
									veld kan overnemen.

	Terugkeerwaarde :	De routine geeft de toetsaanslag waarmee het
						scherm verlaten werd terug. Voor definities
						van de toetsen, zie keys.h. '\0' wordt terug-
						gegeven als de routine terugkeerde door het
						overlopen van het laatste veld.

*******************************************************************/
#pragma endhdr

char scrn_proc(int scrn_no, struct scrn_buf *scrn_data, int go_last)
{
    int (*edit_funct)(char *data_ptr, char term_byte);
    char *(*help_funct)(void), *char1;
    struct scrn_buf *sb;
    int fld_ct, f, this_x, this_y;
    int fld_ptr, done, len, edit_result;
	char ex, prev = '\r';
	register char *edit_mask, *bfptr;

	if (scrn_no != _curr_crt)
	{
		display_crt(scrn_no);
        _curr_crt = scrn_no;
    }
    else
		gotowin(s_[scrn_no-1].wn); /* For security reasons */

	fld_ct = s_[scrn_no-1].no_flds;

    /* for each field, process ditto, fill, backfill */

    for (fld_ptr = 1; fld_ptr <= fld_ct; fld_ptr++)
    {
        find_field(scrn_no, fld_ptr);   /* set the cursor */
        sb = scrn_data + fld_ptr - 1;   /* ->description */
        bfptr = sb->item_value;         /* ->collection buff */
		len = s_[scrn_no-1].f_[fld_ptr-1].f_len;
        *(bfptr+len) = '\0';
		if (!sb->fill_data)
            while (len--)
                *bfptr++ = ' ';
        else if (sb->ditto && sb->dit_ptr)
            strcpy(bfptr, sb->dit_ptr);
        else while (len--)
        {
            if (!isprint(*bfptr))
				*bfptr = ' ';
            bfptr++;
        }
        backfill(scrn_no, fld_ptr, scrn_data);
    }
    fld_ptr = 1;

    /* Collect data from the keyboard into the screen fields */

	done = FALSE;
    while (!done)
    {
#ifdef IDI_ON
		/* Stel current flag in */
		if (IDI_SWITCH)
		{
			IDI_CF = IDI_FLAGS[fld_ptr - 1];
			IDI_WN = s_[scrn_no - 1].wn;
		}
#endif
		sb = scrn_data + fld_ptr - 1;
        bfptr = sb->item_value;
        edit_mask = find_field(scrn_no, fld_ptr);
        if (sb->protect)
            prev = ex = (fld_ptr == 1 || fld_ptr == fld_ct) ? '\r' : prev;
        else
        {
            cursorxy(last_x, last_y);
			ex = get_data(sb->item_type, edit_mask, bfptr);
            prev = (ex != HELP && ex != DITTO) ? ex : '\r';
			s_[scrn_no-1].f_[fld_ptr-1].din |= !spaces(bfptr);

		}
        edit_funct = sb->editfptr;
        help_funct = sb->helpfptr;
#ifdef IDI_ON
		/* IDI-hook */
		if (IDI_SWITCH && IDI_CF)
		{
			(*END_FP)(IDI_CF, bfptr, ex);
			gotowin(IDI_WN);
			_curr_crt = scrn_no;
		}
#endif
        if (ex == ESC)
			break;

		edit_result = OK;
        if (!sb->protect && ex != HELP && edit_funct)
        {
            edit_result = (*edit_funct)(bfptr,ex);
            if (scrn_no != _curr_crt)
			{   /* Rebuild the screen, if proc_scrn was re-entered */
                gotowin(s_[scrn_no-1].wn);
                _curr_crt = scrn_no;
            }
        }
        if (edit_result == OK)
            switch (ex)
            {
                case HELP:
					if (help_funct)
                    {
                        char1 = (*help_funct)();
                        if (*char1)
                            strcpy(bfptr, char1);
					}
                    break;
                case DITTO:
                    if (sb->dit_ptr)
                    {
                        strcpy(bfptr, sb->dit_ptr);
						cursorxy(last_x, last_y);
                        backfill(scrn_no, fld_ptr, scrn_data);
                    }
                /* NOTE: DITTO falls through to \r */
                case '\0':
                case '\r':
                    if (go_last && fld_ptr == fld_ct)
                    {
                        done = TRUE;
						break;
                    }
                    /* NOTE: \r falls through to fwd */
                    case '\t':
					case FWD :
                        if (fld_ptr == fld_ct)
                            fld_ptr = 1;
                        else
                            fld_ptr++;
                        break;
                    case '\b':
						if (fld_ptr == 1)
                            fld_ptr = fld_ct;
                        else
                            fld_ptr--;
                        break;
                    case UP:
                        f = fld_ptr;
                        this_y = last_y;
                        this_x = last_x;
						while (last_y == this_y && f > 1)
                            find_field(scrn_no, --f);
                        if (last_y == this_y)
						{
                            find_field(scrn_no, fld_ptr++);
                            break;
                        }
                        this_y = last_y;
                        while (last_x > this_x &&
                                this_y == last_y && f > 1)
                            find_field(scrn_no, --f);
						if (this_y != last_y)
                            f++;
                        fld_ptr = f;
                        break;
                    case '\n':
                        f = fld_ptr;
                        this_y = last_y;
                        this_x = last_x;
                        while (last_y == this_y &&
								f < s_[scrn_no-1].no_flds)
                            find_field(scrn_no, ++f);
						if (f == s_[scrn_no-1].no_flds)
                            prev = '\r';
                        if (last_y == this_y)
                        {
                            find_field(scrn_no, fld_ptr);
                            break;
                        }
                        this_y = last_y;
                        while (last_x < this_x && this_y == last_y &&
								f < s_[scrn_no-1].no_flds)
                            find_field(scrn_no, ++f);
                        if (this_y != last_y)
                            f--;
                        fld_ptr = f;
                        break;
                    case GO:
                    case F4:
                    case F5:
					case F6:
					case F7:
                    case F3:
                    case F9:
                    case F10:
                        done = TRUE;
                        break;
                    default:
                        break;
        }
    }
	/* all the fields are in, copy ditto fieds to
        ditto hold areas */
    for (fld_ptr = 1; fld_ptr <= fld_ct; fld_ptr++)
    {
        sb = scrn_data + fld_ptr - 1;
        if (sb->dit_ptr)
            strcpy(sb->dit_ptr, sb->item_value);
    }
    return (ex);
}

#pragma startfhdr
/******************************************************************

	Functie			:	void disp_scrn(int scrn_no,
								struct scrn_buf *scrn_data);

	Gebruik         :	Vertonen scherm.

	Prototype in    :	screen.h

	Beschrijving    :	disp_scrn verft het gehele scherm opnieuw.
						Dit is handig als de mogelijkheid bestaat
						dat het scherm door een edit- of helpfunctie
						gecorrumpeerd is. De parameters geven aan om
						welk scherm het handelt (zie ook scr_proc()).

	Terugkeerwaarde :	Geen.

*******************************************************************/
#pragma endhdr

void disp_scrn(int scrn_no, struct scrn_buf *scrn_data)
{
	register int fld_p;

	display_crt(scrn_no);
	for (fld_p = 1; fld_p <= s_[scrn_no-1].no_flds; fld_p++)
		backfill(scrn_no, fld_p, scrn_data);
}

#pragma startfhdr
/******************************************************************

	Functie			:	char get_item(struct scrn_buf *scrn_data,
							int scrn_no, int fld);

	Gebruik         :	Invoer van 1 veld op het scherm vragen.

	Prototype in    :	screen.h

	Beschrijving    :	get_item laat de gebruiker een veld invullen
						op het scherm gedefinieerd door scrn_data en
						scrn_no (zie ook scnr_proc()). Het veld wordt
						gespecifieerd door fld, en is een 1-relatieve
						index in de array scrn_data.

	Terugkeerwaarde :	Zie scrn_proc().

*******************************************************************/
#pragma endhdr

char get_item(struct scrn_buf *sc, int crt, int fld)
{
    register char *m;
    char c;

    sc += (fld-1);
    m = find_field(crt,fld);
	cursorxy(last_x, last_y);
    c = get_data(sc->item_type, m, sc->item_value);
    s_[crt-1].f_[fld-1].din |= !spaces(sc->item_value);
    return c;
}

#pragma startfhdr
/******************************************************************

	Functie			:	void backfill(int crt, int item
							struct scrn_buf *screen_data);

	Gebruik         :	Vertonen van een scherm-veld.

	Prototype in    :	screen.h

	Beschrijving    :	backfill kan vanuit een edit-functie gebruikt
						worden om ge-edite data opnieuw op het scherm
						te zetten. Het scherm wordt bepaald door crt en
						screen_data, het veld door item (1-relatief).

	Terugkeerwaarde :	Geen.

*******************************************************************/
#pragma endhdr

void backfill(int crt, int item, struct scrn_buf *screen_data)
{
    register char *str, *mask;
    struct scrn_buf *sptr;

    cursor(CURS_OFF);
    sptr = screen_data + item - 1;
    str = sptr->item_value;
    if (spaces(str) && s_[crt-1].f_[item-1].din == FALSE && !sptr->protect)
    {
        cursor(CURS_SMALL);
        insert_line();
        return;
    }
	s_[crt-1].f_[item-1].din = !spaces(str);
    mask = find_field(crt, item);
    cursorxy(last_x, last_y);
	if (sptr->protect)
		high_intensity(FALSE);
	else
		high_intensity(TRUE);

	underline(TRUE);
	disp_fld(str, mask);
	underline(FALSE);
	high_intensity(FALSE);
	cursor(CURS_SMALL);
	insert_line();
}

#pragma startfhdr
/******************************************************************

	Functie			:	void err_msg(const char *s);

	Gebruik         :	Zet foutmelding op het scherm

	Prototype in    :	screen.h

	Beschrijving    :	err_msg laat een pieptoon horen, en zet de
						foutmelding, aangegeven door s, op de status-
						regel onderin het scherm.

	Terugkeerwaarde :	geen.

*******************************************************************/
#pragma endhdr

void err_msg(const char *s)
{
    save_gr();
    put_byte(BELL);
    notice(s);
    rstr_gr();
}

/* status line buffer */
static char statline[81] =
"                                        "
"                                        ";
#pragma startvhdr
/******************************************************************

	Variabele		:	char *default_status;

	Gebruik         :	Standaard status-regel.

	Declaratie in   :	screen.h

	Beschrijving    :	De gebruiker kan default_status laten wijzen
						naar een boodschap die op het scherm gezet
						moet worden na een aanroep van clrmsg().
						De standaard-instelling is NULL.

*******************************************************************/
#pragma endhdr

char *default_status = NULL;

#pragma startfhdr
/******************************************************************

	Functie			:	void clrmsg(void);

	Gebruik         :	Wissen van de status-regel.

	Prototype in    :	screen.h

	Beschrijving    :	Deze functie maakt de status-regel schoon,
						en zet, indien default_status <> NULL, de
						standaard status-regel weer op het scherm.

	Terugkeerwaarde :	Geen.

*******************************************************************/
#pragma endhdr

void clrmsg(void)
{
    register int i;

    if (msg_up)
	{
        save_gr();
        /* copy default statusline, if available */
        i = 0;
        if (default_status)
            for (; i < 65 && default_status[i] != '\0'; i++)
                statline[i] = default_status[i];
        for (; i < 65; i++)
            statline[i] = ' ';
        makestat(statline);
        msg_up = FALSE;
        rstr_gr();
    }
}

#pragma startfhdr
/******************************************************************

	Functie			:	void notice(const char *s);

	Gebruik         :	Zet een boodschap op het scherm.

	Prototype in    :	screen.h

	Beschrijving    :	Deze functie is analoog aan err_msg, met het
						verschil dat hier geen geluidssignaal wordt
						gegeven.

	Terugkeerwaarde :	Geen.

*******************************************************************/
#pragma endhdr

void notice(const char *s)
{
    int i = 0;
    save_gr();
    while (*s && i < 65)
        statline[i++] = *s++;
    for (; i < 65; i++)
		statline[i] = ' ';
    makestat(statline);
    rstr_gr();
    msg_up = TRUE;
}

static void insert_line()
{
    save_gr();
	movmem(inserting ? "<INSERT>" : "<OVRWRT>", statline + 70, 8);
    cursor(inserting ? CURS_BIG : CURS_SMALL);
    makestat(statline);
    rstr_gr();
}

static void display_crt(int crt)
{
    register char *cptr;
    char c;
    int x = 0, filling = FALSE;

	/* Set-up a window for the crt, if neccesary */
	if (!s_[crt-1].wn)
	{
		s_[crt-1].wn = makewin(s_[crt-1].xt, s_[crt-1].yt,
					s_[crt-1].xs, s_[crt-1].ys, s_[crt-1].title,
					ATT_NORM, ATT_NORM, ATT_HIGH, FST_GSING,1);
		open_seq[seq_no++] = crt - 1;
	}
	else
	{
		gotowin(s_[crt-1].wn);
		return;
	}
	if (win_error)
		fatal(10);

    clr_scrn();
    cptr = s_[crt-1].s_mptr;
    while ((c = *cptr++) != SCRNID)
        if (c)
        {
            if (c == FLDFILL)
            {
                c = ' ';
                if (!filling)
                {
                    filling = TRUE;
                    high_intensity(TRUE);
                    underline(TRUE);
                }
            }
            else if (filling)
            {
                filling = FALSE;
                high_intensity(FALSE);
				underline(FALSE);
            }
            if (c != '\t')
            {
                put_byte(c);
            }
            else
            {
                put_byte(' ');
                while (++x % HT)
                    put_byte(' ');
            }
            if (c == '\n' || c == '\r')
                x = 0;
        }
    insert_line();
}

static char *find_field(int crt, int f)
{
	last_x = s_[crt-1].f_[f-1].f_x;
    last_y = s_[crt-1].f_[f-1].f_y;
    return s_[crt-1].f_[f-1].f_mptr;
}

static char get_data(char type, char *msk, char *bf)
{
    char c, *b;
    register char *mask, *buff;
    int depart, digits, chok, x, i, minus;


    buff = bf;
    mask = msk;
    depart = digits = minus = FALSE;
    high_intensity(TRUE);
    underline(TRUE);
	/* Save start of field */
    x = last_x;

	if (type == 'C')
    {
        while (*mask != '.')
		{
            if (!(punct(*mask)))
                buff++;
			mask++;
			last_x++;
		}
		last_x--;
		cursorxy(last_x, last_y);
		buff--;

		/* wait for the . or a key to get us out of the number */

		while ((c = get_byte()) != '\r' && c != ESC && c != '.' &&
				c != '\t' && c != UP && c != '\n' && c != FWD &&
				c != '\b' && c != GO)
		{
			/* Backspacing allowed */
			if (c == RUBOUT)
			{
				/* Move buffer 1 to the right */
				movmem(bf, bf+1, buff-bf);
				*bf = ' ';
				cursorxy(x, last_y);
				disp_fld(bf, msk);
				cursorxy(last_x, last_y);
				continue;
			}
			i = (c ==' ') ? TRUE : isnumr(c);
			if (i)
			{
				movmem(bf+1, bf, buff-bf);
				*buff = c;
				cursorxy(x, last_y);
				disp_fld(bf, msk);
				cursorxy(last_x, last_y);
			}
		}
		b = bf;
		while (b <= buff && (*b == ' ' || *b == '\0'))
			*b++ = ' ';
		cursorxy(x, last_y);
		disp_fld(bf, msk);
		cursorxy(last_x,last_y);
		if (c == '\r' || c == GO || c == '\b' || c == UP ||
			c == '\n' || c == ESC)
		{
			high_intensity(FALSE);
			underline(FALSE);
			return(c);
		}
		mask++;
		buff++;
		bf = buff;
		msk = mask;
		put_byte(FWD);
		put_byte(FWD);
		last_x += 2;
		first_key = FALSE;
		/* the decimal past is treated further down as a num. entry */
    }

    /* Read in a data field from the keyboard */
    while (TRUE)
    {
        c = get_byte();
        clrmsg();
        switch(c)
        {
            case '\r':
            case '\n':
            case '\t':
            case DITTO:
            case ESC:
            case GO:
            case HELP:
            case F3:
            case F4:
            case F5:
            case F6:
            case F7:
            case F9:
            case F10:
            case UP:
                depart = TRUE;
                break;
			case HOME: /* Go to start of field */
				if (type == 'A')
				{
					buff = bf;
					mask = msk;
					last_x = x;
					cursorxy(last_x, last_y);
					break;
				}
				else
				{
					depart = TRUE;
					break;
				}
			case END:  /* Go to end of field */
				if (type == 'A')
				{
					/* Go to start of field */
					buff = bf;
					/* Find end of buffer */
					while (*buff != '\0')
					{
						/* Increment all pointers */
						buff++, mask++, last_x++;
						/* Go past punctuation */
						if (punct(*mask))
							mask++, last_x++;
					}
					/* Find last non-space */
					while ((*buff == ' ' || *buff == '\0') && buff > bf)
					{
						/* Decrement the pointers */
						buff--, mask--, last_x--;
						/* Skip punctuation */
						if (punct(*mask))
							mask--, last_x--;
					}
					/* Correction: buff is pointing to the character */
					if (*(buff+1))
					{
						buff++, mask++, last_x++;
						/* Skip punctuation */
						if (punct(*mask))
							mask++, last_x++;
					}
					cursorxy(last_x, last_y);
					break;

				}
				else
				{
					depart = TRUE;
					break;
				}
            case '\b':
                if (buff == bf)
                {
                    depart = TRUE;
                    break;
                }
            case RUBOUT:
                if (buff == bf)
                    break;
                --buff;
                --mask;
                if (punct(*mask))
                {
                    put_byte('\b');
                    last_x--;
                    --mask;
                }
                put_byte('\b');
                last_x--;
                if (c != RUBOUT)
                {
                    if (type == 'N' && (*(buff-1) == ' ' || buff == bf))
                        digits = minus = FALSE;
                    break;
                }
            case DELETE:
                for (b = buff; *b != '\0'; b++)
                    *b = *(b+1);
                *--b = ' ';
                disp_fld(buff, mask);
                if (type == 'N' && buff > bf && *(buff-1) != ' ')
                    digits = TRUE;
                break;
            case FWD:
                put_byte(FWD);
                last_x++;
                mask++;
                if (punct(*mask))
                {
                    put_byte(FWD);
                    last_x++;
                    mask++;
                }
                buff++;
                break;
            case INSERT:
                inserting ^= TRUE;
                insert_line();
                break;
			default:		/* Normale toetsaanslag */
				if (first_key == TRUE)
				{
					/* Wis het veld eerst */
					buff = bf;
					while (*buff)
						*buff++ = ' ';
					buff = bf;
					mask = msk;
					last_x = x;
					disp_fld(buff, mask);
					cursorxy(last_x, last_y);
				}
                switch(type)
                {
                    case 'A':
						if ((chok = isprint(c)) == 0)
                            err_msg("Ongeldige aanslag");
                        break;
                    case 'C':
                    case 'N':
                        if ((chok = (!digits && c == ' ')) == TRUE)
                            break;
						else if ((chok = (!digits && !minus && c == '-')) == TRUE)
						{
                            minus = TRUE;
							break;
						}
                    case 'D':
                    case 'Z':
                        chok = isnumr(c);
                        break;
                    default:
                        break;
                }
                if (chok)
                {
                    if (inserting)
                    {
                        for (b = buff; *b != '\0'; b++)
                            ;
                        b--;
                        while (b != buff)
                        {
                            *b = *(b-1);
                            b--;
                        }
                        disp_fld(buff,mask);
                    }
                    if (type == 'N' && c != ' ')
                        digits = TRUE;
                    *buff++ = c;
                    put_byte(c);
                    last_x++;
                    mask++;
                    if (punct(*mask))
                    {
                        put_byte(FWD);
                        last_x++;
                        mask++;
                    }
                }
		}
		/* Einde veld bereikt */
		if (*mask == '\0')
			depart = first_key = TRUE;

#ifdef IDI_ON
		/* Roep de IDI-functie aan als aan de voorwaarden in voldaan */
		if (IDI_SWITCH && IDI_CF && !depart)
		{
			save_gr();
			cursor(CURS_OFF);
			(*IDI_FP)(IDI_CF, bf);
			gotowin(IDI_WN);
			cursor(inserting ? CURS_BIG : CURS_SMALL);
			rstr_gr();
		}
#endif
        if (depart)
        {
            if (type != 'D' || c == ESC)
                break;
            if (edit_date(bf) == OK)
                break;
            depart = FALSE;
            while (buff > bf)
            {
                --buff;
                --mask;
                if (punct(*mask))
                {
                    put_byte('\b');
                    last_x--;
                    --mask;
                }
                put_byte('\b');
                last_x--;
            }
		}
		first_key = FALSE;
    }
    if (c != ESC && c != HELP)
    {
        if (*mask == '\0' && c != FWD)
            c = '\0';
        if (type == 'N' || type == 'Z' || type == 'C')
        {
            if (type == 'N')
                r_just(bf);
            else if (type == 'Z')
                z_just(bf);
            else
                zl_just(bf);
            while (mask-- != msk)
                put_byte('\b');
            disp_fld(bf, msk);
        }
	}
	if (c == '\r')
		first_key = TRUE;
    high_intensity(FALSE);
    underline(FALSE);
    return(c);
}

static int isnumr(char c)
{
    int r;

	if ((r = isdigit(c)) == 0)
        err_msg("Alleen cijfers");
    return r;
}

/* Right justify, space fill */
static void r_just(char *s)
{
    register int len;

    len = strlen(s);
    while (*s == ' ' || *s == '0' && len)
    {
        len--;
        /* Strip leading zero's */
        *s++ = ' ';
    }
    if (len)
    {
        while (*(s + len-1) == ' ')
        {
            movmem(s, s+1, len-1);
            *s = ' ';
        }
    }
}

/* Right justify, zero fill */
static void z_just(char *s)
{
    register int len;

    if (spaces(s))
        return;
    len = strlen(s);
    while (*(s+len-1) == ' ')
    {
        movmem(s, s+1, len-1);
        *s = '0';
    }
}

/* Zero fill on the right */
static void zl_just(char *s)
{
    while (*s)
    {
        if (*s == ' ')
            *s = '0';
        s++;
    }
}

#pragma startfhdr
/******************************************************************

	Functie			:   int spaces(const char *s);

	Gebruik         :	Test of string ingevuld is.

	Prototype in    :	screen.h

	Beschrijving    :	spaces kijkt of een string geheel uit spaties
						bestaat. Deze functie kan in editing-functies
						gebruikt worden om te kijken of een veld is
						ingevuld.

	Terugkeerwaarde :	TRUE als er alleen spaties in het veld staan,
						anders FALSE.

*******************************************************************/
#pragma endhdr

int spaces(const char *c)
{
    while (*c == ' ')
        c++;
    if (!*c)
        return TRUE;
    else
        return FALSE;
}

static int edit_date(char *date)
{
    static int days[12] =
    {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    int mo, da, yr;
    static char mm[] = "00", dd[] = "00";

    if (spaces(date))
        return OK;
    yr = atoi(date+4);
    days[1] = yr % 4 ? 28 : 29;

    sprintf(dd, "%02.2s", date);
    sprintf(mm, "%02.2s", date + 2);

    mo = atoi(mm);
    if (!mo || mo > 12)
    {
        err_msg("Ongeldige maand");
        return ERROR;
    }

    da = atoi(dd);
    if (da && da <= days[mo-1])
        return OK;
    err_msg("Ongeldige dag");
    return ERROR;
}

static void disp_fld(char *b, char *mask)
{
    register char *m;

    m = mask;
    while (*m)
    {
        if (punct(*m))
        {
            m++;
            put_byte(FWD);
            if (!*m)
                break;
        }
        if (!isprint(*b))
            put_byte('^');
        else
            put_byte(*b);
        b++;
        m++;
    }
    while ((m--) - mask)
        put_byte('\b');
}


static int punct(char c)
{
    return (c == '/' || c == '.' || c == '-' || c == ':' ||
            c == '(' || c == ')' || c == ',');
}
