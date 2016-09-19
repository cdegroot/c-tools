/*
	PRNDEF.C

	Programma voor printer-definitie
*/
/*
	$Id: prndef.c,v 1.2 1993/02/05 13:02:34 cg Rel $
*/
#include <stdio.h>
#include <alloc.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <mem.h>
#include <toolset.h>
#include <keys.h>
#include <screen.h>

/*
	Symbols
*/
/* Pagelength settings type */
#define LINES 	1
#define INCHES	2
#define ASCII	0x10
#define VALUE	0x20

#define CODMAXLEN	50
/*
	Statische opslag commando's
*/
static char	ui[4][CODMAXLEN+1], uo[4][CODMAXLEN+1],	/* Underline */
			bi[4][CODMAXLEN+1], bo[4][CODMAXLEN+1],	/* Boldface */
			ci[4][CODMAXLEN+1], co[4][CODMAXLEN+1],	/* Compressed */
			ni[4][CODMAXLEN+1], no[4][CODMAXLEN+1],	/* NLQ */
			tof[4][CODMAXLEN+1], ff[4][CODMAXLEN+1],
			pl[4][CODMAXLEN+1],			/* Pagelength */
			si[4][CODMAXLEN+1], so[4][CODMAXLEN+1];	/* Select */
static int	plt[4], plp[4];	/* Pagelength: type of setting, arg-offset */
static int  cr[4];			/* TRUE = auto append, FALSE = no auto append */
static char port[4][13];		/* Portname */
static char hf[4][CODMAXLEN+1]; /* Half linefeed */
/*
	Locale functiedeclaraties
*/
static edit_func	num_edit, poort_edit, autolf_edit,
                    pl_edit;
static void save(void);
/*
	Invoerbuffers, lengte = 48+1, statisch
*/
static char buf[14][CODMAXLEN+1];
static char num[2];
static char poort[14];
static char autolf[2];
#define clrbuf() {int i;for(i=0;i<14;i++)setmem(buf[i],CODMAXLEN+1,0);\
                  setmem(num,2,0);setmem(poort,14,0);setmem(autolf,2,0);}
/*
	Overige variabelen
*/
static struct scrn_buf sb[] =
{
	{'N', FALSE, num, num_edit, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, poort, poort_edit, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, autolf, autolf_edit, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[0], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[1], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[2], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[3], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[4], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[5], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[6], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[7], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[8], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[9], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[10], pl_edit, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[11], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[12], NULL, NULL, TRUE, FALSE, NULL},
	{'A', FALSE, buf[13], NULL, NULL, TRUE, FALSE, NULL},
};

static int defined = 0;
static FILE *sf;

/*
	Program Entry Point
*/
void main(void)
{
	int i = 0;
	char key;

	if ((sf= fopen("PRINTER.DAT", "r+b")) != NULL)
	{
		/* Bestand bestaat al, lees het in */
	    defined = getc(sf);

		/* Lees alle definities in via buffer */
		for (i = 0; i < defined; i++)
		{
			/* Zoek beginpositie, 1k voor elke printer */
			fseek(sf, (i+1)*1024, SEEK_SET);
			/* Poortnaam */
#define replnl(s) {char *__t;if((__t = strrchr((s)[i],'\n')) != NULL)*__t='\0';}
			fgets(port[i], CODMAXLEN, sf);replnl(port);

			/* Pagelength settings & CR/LF */
			plt[i] = getw(sf);
			plp[i] = getw(sf);
			cr[i] = getw(sf);
			fgets(pl[i], CODMAXLEN, sf);replnl(pl);


			/* Underlining */
			fgets(ui[i], CODMAXLEN, sf);replnl(ui);
			fgets(uo[i], CODMAXLEN, sf);replnl(uo);

			/* Boldface */
			fgets(bi[i], CODMAXLEN, sf);replnl(bi);
			fgets(bo[i], CODMAXLEN, sf);replnl(bo);

	        /* Pitch */
			fgets(ci[i], CODMAXLEN, sf);replnl(ci);
			fgets(co[i], CODMAXLEN, sf);replnl(co);

			/* Letterstyle */
			fgets(ni[i], CODMAXLEN, sf);replnl(ni);
			fgets(no[i], CODMAXLEN, sf);replnl(no);

			/* Select */
			fgets(si[i], CODMAXLEN, sf);replnl(si);
			fgets(so[i], CODMAXLEN, sf);replnl(so);


			/* Forms */
			fgets(tof[i], CODMAXLEN, sf);replnl(tof);
			fgets(ff[i], CODMAXLEN, sf);replnl(ff);
			fgets(hf[i], CODMAXLEN, sf);replnl(hf);
#undef replnl
		}
    }
	else
	{
		int i;
		sf = fopen("PRINTER.DAT","w+b");
		/* Schrijf bestand met nullen */
		for (i = 0; i < 5*1024; i++)
			putc(0,sf);
		defined = 0;
	}
	if (ferror(sf))
	{
		fprintf(stdprn,"Error on input file\n");
		clearerr(sf);
    }

	/* Wis overige buffers */
	for (; i < 4; i++)
	{
 #define CLR(x) setmem((x)[i], sizeof((x)[i]),0)
    	CLR(ui), CLR(uo), CLR(bi);
		CLR(bo), CLR(ci), CLR(co);
		CLR(ni), CLR(no), CLR(tof);
		CLR(ff), CLR(pl), CLR(si);
		CLR(so), CLR(port), CLR(hf);
#undef CLR
		plt[i] = plp[i] = cr[i] = 0;
	}
	clrbuf();

	/* Invoer */
	read_screens("PRNDEF.CRT");
	key = 0;
	while (key != ESC)
	{
		key = scrn_proc(1,sb,FALSE);

		if (key == F1)
			save();
	}
    reset_screens();

	/* Sluit bestand */
	fclose(sf);
}

/*
	Locale functiedefinities
*/
static void parse(void), unparse(void);

static edit_func	num_edit
{
	int n;

	if (k == ESC)
		return OK;
    n = atoi(s);
	if (n == 0 || n > 4 || n > (defined + 1))
	{
		err_msg("Nummer incorrect");
		return ERROR;
	}
	else
	{
		int i;
		/* Zet gegevens in buffers */
		unparse();

		/* Zet gegevens op scherm */
		sb[0].protect = TRUE;
		for (i = 1; i <= 17; i++)
			backfill(1,i,sb);

		return OK;
	}
}

static edit_func	poort_edit
{
	FILE *f;
	char *t;
	if (k == ESC)
		return OK;
	if (spaces(s))
	{
		err_msg("Poortnaam verplicht");
		return ERROR;
	}
	t = strchr(s,' ');
	*t = '\0';
	if ((f = fopen(s,"wb")) == NULL)
	{
		err_msg("Kan geen toegang verkrijgen tot poort");
		*t = ' ';
		return ERROR;
	}
	else
	{
		fclose(f);
		*t = ' ';
		return OK;
	}
}

static edit_func	autolf_edit
{
	char t;
	if (k == ESC)
		return OK;

	if ((t = toupper(*s)) != 'J' && t != 'N')
	{
		err_msg("\'J\' of \'N\' invullen");
		return ERROR;
	}
	else
	{
		*s = t;
		backfill(1, 3, sb);
		return OK;
	}
}

static edit_func	pl_edit
{
	if (k == ESC)
		return OK;
	if ((strstr(s, "<A,L>")) != NULL)
		plt[atoi(num)-1] = ASCII | LINES;
	else if ((strstr(s, "<A,I>")) != NULL)
		plt[atoi(num)-1] = ASCII | INCHES;
	else if ((strstr(s, "<V,L>")) != NULL)
		plt[atoi(num)-1] = VALUE | LINES;
	else if ((strstr(s, "<V,I>")) != NULL)
		plt[atoi(num)-1] = VALUE | INCHES;
	else
	{
		err_msg("Geen code-invoegplaats gevonden");
		return ERROR;
	}
	return OK;
}

/* Bewaar een printer-instelling */
static void 		save(void)
{
	int i;

	if (spaces(num))
		return;

	/* Welk printernummer ? */
	i = atoi(num)-1;

	/*
		Bewaar printer
	*/
	/* Vertaal buffers */
	parse();

	/* Zoek beginpositie, 1k voor elke printer */
	fseek(sf, (i+1)*1024, SEEK_SET);

#define nl putc('\n',sf);
	/* Poortnaam */
	fputs(port[i], sf);nl;

	/* Pagelength settings & CR/LF */
	putw(plt[i], sf);
	putw(plp[i], sf);
	putw(cr[i], sf);
	fputs(pl[i], sf);nl;

	/* Underlining */
	fputs(ui[i], sf);nl;
	fputs(uo[i], sf);nl;

	/* Boldface */
	fputs(bi[i], sf);nl;
	fputs(bo[i], sf);nl;

    /* Pitch */
	fputs(ci[i], sf);nl;
	fputs(co[i], sf);nl;

	/* Letterstyle */
	fputs(ni[i], sf);nl;
	fputs(no[i], sf);nl;

	/* Select */
	fputs(si[i], sf);nl;
	fputs(so[i], sf);nl;


    /* Forms */
	fputs(tof[i], sf);nl;
	fputs(ff[i], sf);nl;
	fputs(hf[i], sf);nl;

	if ((i+1) > defined)
	{
		defined = (i+1);
		fseek(sf, 0, 0);
		putc(i+1, sf);
	}

#undef nl
	clrbuf();
	sb[0].protect = FALSE;
}

/* Heen en weer vertalen */
static void do_parse(char *s, char *d);
static void parse(void) /* Vertaal van invoerbuffers -> bestandsbuffers */
{
	int i;

	/* Printernummer */
	i = atoi(num)-1;

	strcpy(port[i],poort);
	cr[i] = *autolf == 'J' ? 1 : 0;
	/* Ga alle buffers af */
	do_parse(buf[0], ui[i]);
	do_parse(buf[1], uo[i]);
	do_parse(buf[2], bi[i]);
	do_parse(buf[3], bo[i]);
	do_parse(buf[4], ci[i]);
	do_parse(buf[5], co[i]);
	do_parse(buf[6], ni[i]);
	do_parse(buf[7], no[i]);
	do_parse(buf[8], tof[i]);
	do_parse(buf[9], ff[i]);
	do_parse(buf[10], pl[i]);
	do_parse(buf[11], si[i]);
	do_parse(buf[12], so[i]);
	do_parse(buf[13], hf[i]);
}

static void do_unparse(char *d, char *s);
static int page = FALSE;
static void unparse(void) /* Vertaal van bestandsbuffers -> invoerbuffers */
{
	int i;

	/* Printernummer */
	i = atoi(num)-1;

	strcpy(poort,port[i]);
	*autolf = cr[i] == 1 ? 'J' : 'N';
	/* Ga alle buffers af */
	do_unparse(buf[0], ui[i]);
	do_unparse(buf[1], uo[i]);
	do_unparse(buf[2], bi[i]);
	do_unparse(buf[3], bo[i]);
	do_unparse(buf[4], ci[i]);
	do_unparse(buf[5], co[i]);
	do_unparse(buf[6], ni[i]);
	do_unparse(buf[7], no[i]);
	do_unparse(buf[8], tof[i]);
	do_unparse(buf[9], ff[i]);
	page = TRUE;
	do_unparse(buf[10], pl[i]);
	page = FALSE;
	do_unparse(buf[11], si[i]);
	do_unparse(buf[12], so[i]);
	do_unparse(buf[13], hf[i]);
}

static void do_parse(char *s, char *d)
{
	char *b;
	/* Ga source af */
	b = d;
	while (*s)
	{
		/* Code ? */
		if (*s == '<')
		{
			/* Karakter na het haakje */
            s++;
			/* Mogelijkheid 1: <NNN> -> ASCII Waarde */
			if (isdigit(*s))
			{
				/* Zet de waarde neer */
	            *d = (char) atoi(s);
				/* Zoek het slothaakje */
				while (*s++ != '>');
				d++;
			}
			/* Mogelijkheid 2: <C,C> -> Pagelength code */
			else
			{
				plp[atoi(num)-1] = d - b;
/*				plt[atoi(num)-1] = (*s == 'A' ? ASCII : VALUE);*/
				s++, s++;
/*				plt[atoi(num)-1] |= (*s == 'I' ? INCHES : LINES);*/
				s++, s++;
			}
		}
		/* Vervang spaties door nullen */
		else if (*s == ' ')
			*d++ = '\0', s++;
		else
		{
			*d = *s;
			d++, s++;
		}
	}
}

static void do_unparse(char *d, char *s)
{
	char *b;
	b = s;

    while (*s || (page && *(s-1)))
	{
		if (page && s - b == plp[atoi(num)-1])
		{
			/* Voeg paginalengte-code in */
			movmem(d, d + 5, strlen(d) - 5);
			d[0] = '<';
			d[1] = plt[atoi(num)-1] & ASCII ? 'A' : 'V';
			d[2] = ',';
			d[3] = plt[atoi(num)-1] & LINES ? 'L' : 'I';
			d[4] = '>';
			d += 5;
			page = FALSE;
		}
		if (isprint(*s))
			*d++ = *s++;
		else if (*s)
		{
			*d++ = '<';
			d += sprintf(d, "%03d", *s++);
			*d++ = '>';
		}
	}
}
