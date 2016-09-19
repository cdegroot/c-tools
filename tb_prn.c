/*
	TB_PRN.C


	Service-routines voor printer I/O
*/
/*
	$Id: tb_prn.c,v 1.2 1993/02/05 13:02:49 cg Rel $
*/
#include <stdio.h>
#include <string.h>
#include <bios.h>
#include <stdarg.h>
#include <alloc.h>
#include <io.h>
#include <toolset.h>
#include <printer.h>

/* Pagelength settings type */
#define LINES   1
#define INCHES  2
#define ASCII	0x10
#define VALUE	0x20
#define LPI		6
/*
	Statische opslag commando's
*/
static char	*ui[4], *uo[4],	/* Underline */
			*bi[4], *bo[4],	/* Boldface */
			*ci[4], *co[4],	/* Compressed */
			*ni[4], *no[4],	/* NLQ */
			*tof[4], *ff[4],
			*pl[4],			/* Pagelength */
			*si[4], *so[4];	/* Select */
static int	plt[4], plp[4];	/* Pagelength: type of setting, arg-offset */
static int  cr[4];			/* TRUE = auto append, FALSE = no auto append */
static FILE	*output[4];		/* Direction of output */
static char *port[4];		/* Portname */
static char *hf[4];			/* Half linefeed */
/*
	Algemene buffer
*/
#define BUFSIZE 256
static char buffer[BUFSIZE];

/*
	Overige variabelen
*/
static int init, defined, cp, pls, lm = 0, ccpi = 10, uof = FALSE;


int _Cdecl init_prn(char *pathtodriver)
{
	FILE *sf;
	int i;
	int error;
	int dupflag, fd;
	if (init)
		return OK;

	/* Stel bestandsnaam samen */
	if (pathtodriver)
		sprintf(buffer, "%s\\PRINTER.DAT", pathtodriver);
	else
		strcpy(buffer,"PRINTER.DAT");

	/* Open bestand */
	if ((sf = fopen(buffer, "rb")) == NULL)
		return ERROR;

	/* Lees aantal gedefinieerde printers: 1 char */
    defined = getc(sf);

	error = TRUE;
	/* Lees alle definities in via buffer */
	for (i = 0; i < defined; i++)
	{
		char *t;

		/* Zoek beginpositie, 1k voor elke printer */
		fseek(sf, (i+1)*1024, SEEK_SET);
#define replnl(s) {char *__t;if((__t = strrchr((s)[i],'\n')) != NULL)*__t='\0';}
		/* Poortnaam */
		fgets(buffer, BUFSIZE, sf);
		if ((port[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(port[i], buffer);replnl(port);
		t = strchr(port[i], ' ');
		*t = '\0';

		/* Open poort */
		dupflag = FALSE;
			/* Standaardprinter ? */
		if (strncmp(strupr(port[i]), "LPT1", 4) == 0 ||
			strncmp(strupr(port[i]), "PRN",  3) == 0)
		{
			dupflag = TRUE;
			fd = dup(fileno(stdprn));
		}
			/* Standaard seriele poort ? */
		else if (strncmp(strupr(port[i]), "COM1", 4) == 0 ||
			strncmp(strupr(port[i]), "AUX",  3) == 0)
		{
			dupflag = TRUE;
			fd = dup(fileno(stdaux));
		}
		if (dupflag)
		{
			if ((output[i] = fdopen(fd, "ab")) == NULL)
				continue;
		}
			/* Nee: open het bestand/device */
		else if ((output[i] = fopen(port[i],"wb")) == NULL)
			continue;

		/* Pagelength settings & CR/LF */
		plt[i] = getw(sf);
		plp[i] = getw(sf);
		cr[i] = getw(sf);
		fgets(buffer, BUFSIZE, sf);;
		if ((pl[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(pl[i], buffer);replnl(pl);


		/* Underlining */
		fgets(buffer, BUFSIZE, sf);;
		if ((ui[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(ui[i], buffer);replnl(ui);
		fgets(buffer, BUFSIZE, sf);;
		if ((uo[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(uo[i], buffer);replnl(uo);

		/* Boldface */
		fgets(buffer, BUFSIZE, sf);;
		if ((bi[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(bi[i], buffer);replnl(bi);
		fgets(buffer, BUFSIZE, sf);;
		if ((bo[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(bo[i], buffer);replnl(bo);

        /* Pitch */
		fgets(buffer, BUFSIZE, sf);;
		if ((ci[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(ci[i], buffer);replnl(ci);
		fgets(buffer, BUFSIZE, sf);;
		if ((co[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(co[i], buffer);replnl(co);

		/* Letterstyle */
		fgets(buffer, BUFSIZE, sf);;
		if ((ni[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(ni[i], buffer);replnl(ni);
		fgets(buffer, BUFSIZE, sf);;
		if ((no[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(no[i], buffer);replnl(no);

		/* Select */
		fgets(buffer, BUFSIZE, sf);;
		if ((si[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(si[i], buffer);replnl(si);
		fgets(buffer, BUFSIZE, sf);;
		if ((so[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(so[i], buffer);replnl(so);

		/* Forms */
		fgets(buffer, BUFSIZE, sf);;
		if ((tof[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(tof[i], buffer);replnl(tof);
		fgets(buffer, BUFSIZE, sf);;
		if ((ff[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(ff[i], buffer);replnl(ff);

		/* Half linefeed */
		fgets(buffer, BUFSIZE, sf);
		if ((hf[i] = malloc(strlen(buffer))) == NULL)
			continue;
		strcpy(hf[i], buffer);replnl(hf);
		error = FALSE;

	}
	/* Sluit bestand */
	fclose(sf);

	/* Keer terug */
	if (error)
		return ERROR;
	else
	{
		init = TRUE;
		pls = FALSE;
		cp = 1;
		return OK;
	}
}

/* Selecteer printer */
void _Cdecl select_prn(int pn)
{
	/* Bestaat de printer ? */
	if (pn > defined)
		pn = 1;

	/* Selecteer printer */
	cp = pn-1;
}

/* Stuur code naar printer */
int _Cdecl prn_code(int c)
{
	/* Zijn we met een pagelength bezig ? */
	if (pls)
	{
		/* Schrijf paginalengte weg */
		if (plt[cp] & LINES)	/* Reken inches naar regels */
			c *= LPI;
		c /= 10;				/* Naar hele inches */
    	if (plt[cp] & ASCII)
		{
			/* Maak ascii string */
			sprintf(buffer,"%d",c);
			/* Stuur string */
			fprintf(output[cp],buffer);
		}
		else
			putc((unsigned char) c, output[cp]);

		/* Schrijf eventuele tweede deel */
		if (plp[cp] < strlen(pl[cp]))
			fputs(&(pl[cp][plp[cp]]),output[cp]);

		pls = FALSE;
		return OK;
	}

	/* Reset uitvoeren */
	if (c & P_RESET)
	{
		/* Alleen parallelle poort */
		if (strncmp(strupr(port[cp]),"PRN",3) == 0)
			biosprint(1, 0, 0);
		else if (strncmp(strupr(port[cp]),"LPT",3) == 0)
				biosprint(1, 0, port[cp][3] - '0' - 1);
		ccpi = 10;
		uof = FALSE;
    }
	/* Buffer flushen */
	if (c & P_FLUSH)
		fflush(output[cp]);
	/* Underline */
	if (c & P_UND_ON)
	{
		fputs(ui[cp], output[cp]);
		uof = TRUE;
	}
	if (c & P_UND_OFF)
	{
		fputs(uo[cp], output[cp]);
		uof = FALSE;
	}
	/* Bold */
	if (c & P_BOLD_ON)
		fputs(bi[cp], output[cp]);
	if (c & P_BOLD_OFF)
		fputs(bo[cp], output[cp]);
	/* Pitch */
	if (c & P_P15)
	{
		fputs(ci[cp], output[cp]);
		ccpi = 15;
	}
	if (c & P_P10)
	{
		fputs(co[cp], output[cp]);
        ccpi = 10;
	}
	/* LQ */
	if (c & P_NLQ)
		fputs(ni[cp], output[cp]);
	if (c & P_DRAFT)
		fputs(no[cp], output[cp]);
	/* Form */
	if (c & P_FF)
		fputs(ff[cp], output[cp]);
	if (c & P_TOF)
		fputs(tof[cp], output[cp]);
	if (c & P_PAGELEN)
	{
		/* Schrijf eerste deel weg */
		strnset(buffer,0,BUFSIZE); /* Wel even de buffer wissen.. */
		fputs(strncpy(buffer,pl[cp],plp[cp]), output[cp]);
		pls = TRUE;
	}
	if (c & P_DW_ON)
		fputs(si[cp], output[cp]);
	if (c & P_DW_OFF)
		fputs(so[cp], output[cp]);

    if (c & P_HF)
		fputs(hf[cp], output[cp]);

	return OK;
}

/* Stel linker marge in */
void set_lm(int left_margin)
{
	lm = left_margin;
}

/* Formatted output */
int _Cdecl pprintf(const char *format, ...)
{
	char *s;
	va_list arg;
	int retval;

	/* Print naar buffer */
	va_start(arg, format);
	retval = vsprintf(buffer, format, arg);
	va_end(arg);
    if (retval == EOF)
		return ERROR;
	/* Print buffer, char voor char */
	s = buffer;
	while (*s)
		if (pputc(*s++) == ERROR)
			return ERROR;

	/* Alles OK */
	return retval;
}

/* Character output */
int _Cdecl pputc(char ch)
{
	/* Ondervang linefeed */
	if (ch == '\n')
	{
		int i, uf;

     	if (putc('\r', output[cp]) == EOF)
			return ERROR;
		if (cr[cp] == FALSE)
			if (putc('\n', output[cp]) == EOF)
				return ERROR;

		/* Zet underline uit */
		if (uof)
		{
			prn_code(P_UND_OFF);
			uf = TRUE;
		}
		else
			uf = FALSE;

		/* Print spaties voor linker marge incl. correctie voor 15 cpi */
        for (i = 0; i < (ccpi == 10 ? lm : ((lm * 15 + 5) / 10)); i++)
			if (putc(' ', output[cp]) == EOF)
				return ERROR;

		/* Underline weer aan */
		if (uf)
			prn_code(P_UND_ON);
	}
	/* Schrijf character */
	else if (putc(ch, output[cp]) == EOF)
		return ERROR;

	return ch;
}

#if 0
/* TESTFUNCTIE */
void main(void)
{
	if (init_prn(NULL) == ERROR)
		return;

	select_prn(1);

    prn_code(P_RESET);
/*	pprintf("Testing\n\n\n\n\n");
	prn_code(P_UND_ON);
	pprintf("Underline\n\n\n\n\n");
	prn_code(P_UND_OFF | P_BOLD_ON);
	pprintf("Boldface\n\n\n\n");
	prn_code(P_BOLD_OFF | P_P15);
	pprintf("Compressed\n\n\n\n");
	prn_code(P_P10);
	prn_code(P_NLQ);
	pprintf("NLQ\n\n\n\n");
	prn_code(P_DRAFT | P_DW_ON);
	pprintf("Double Width\n\n\n\n");
	prn_code(P_DW_OFF);
	pprintf("Reset printer\n\n\n\n");
	prn_code(P_RESET);*/
	pprintf("Pagelength 4 inch\n\n\n\n");
	prn_code(P_PAGELEN);
	prn_code(40);
	prn_code(P_FF);
	pprintf("Einde test");
}
#endif
