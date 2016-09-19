#pragma startmhdr
/******************************************************************

	C Standaarddocumentatie - (c)1990 C.A. de Groot

	Project      :	TOOLBOX

	Module       :	tb_files.c

	Beschrijving :	In deze module bevinden zich de routines voor
					I/O naar random access bestanden met een vaste
					recordgrootte.

*******************************************************************/
/*
	$Id: tb_files.c,v 1.3 1993/02/05 13:02:43 cg Rel $
*/
#pragma endhdr

#include <io.h>
#include <mem.h>
#include <stdio.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <alloc.h>
#include <dir.h>
#include <string.h>
#include <dos.h>
#include <toolset.h>
#include <filhdr.h>
#include <file.h>
#include <cache.h>

#define MAXFILES 30     /* max open files */

static int r_len[MAXFILES]; /* rec lengths */
static int f_fd[MAXFILES];      /* file descriptors */
static ADDR nxav[MAXFILES]; /* next available addresses */


static void new_file(int len, int fd);
static void locate(ADDR rcd_no, int len, ADDR *sector, int *pos);



#pragma startfhdr
/******************************************************************

	Functie			:   #include <toolset.h>
						int rcd_length(int fd);

	Gebruik         :	Opvragen recordlengte

	Prototype in    :	file.h

	Beschrijving    :	Deze functie geeft de recordlengte terug
						van het bestand dat met de file-handle
						fd geassocieerd is.

	Terugkeerwaarde :	Een recordlengte, of ERROR indien het
						bestand niet via opn_file geopend is.

*******************************************************************/
#pragma endhdr

int rcd_length(int fd)
{
    return (fd > MAXFILES || !f_fd[fd] ? ERROR : r_len[fd]);
}

#pragma startfhdr
/******************************************************************

	Functie			:	int opn_file(const char *name, int len);

	Gebruik         :	Openen van een random access bestand.

	Prototype in    :	file.h

	Beschrijving    :	De functie opent het bestand met de naam
						name indien de recordlengte gelijk aan nul
						is, en creeert het bestand bij een record-
						lengte groter dan nul.

	Terugkeerwaarde :	De file-handle of ERROR indien het bestand
						niet bestaat of er te veel bestanden open
						zijn.

*******************************************************************/
#pragma endhdr

int opn_file(const char *name, int len)
{
	struct fil_hdr *b;
	int i;
	char full_name[MAXPATH];

#ifdef DEBUG
fprintf(stdprn,"Opening %s\n",name);
#endif
	/*
		Create the full pathname
	*/
	strcpy(full_name, DATA_FILES);
	strcat(full_name, name);

	/* Search free slot */
	for (i = 0; i < MAXFILES; i++)
		if (f_fd[i] == 0)
			break;
	if (i == MAXFILES)
		return(ERROR);
	init_cache();
	if (len)        /* len > 0 => create file */
	{
		f_fd[i] = _creat(full_name, FA_ARCH);
		_close (f_fd[i]);
		f_fd[i] = _open(full_name, O_RDWR);
		new_file(len,i);
	}
	else
		if ((f_fd[i] = _open(full_name, O_RDWR)) == ERROR)
			return(ERROR);
	b = (struct fil_hdr *) get_node(f_fd[i], (ADDR) 1);
	r_len[i] = b->rcd_len;
	nxav[i] = b->nxt_rcd;
	release_node(f_fd[i], (ADDR) 1, 0);
	return i;
}

#pragma startfhdr
/******************************************************************

	Functie			:	void cls_file(int fd);
						void cls_file_all(void);

	Gebruik         :	Sluiten van een random access bestand.

	Prototype in    :	file.h

	Beschrijving    :   cls_file sluit het bestand met handle fd
						nadat de bijbehorende cache-buffers ge-
						flushed zijn.

						cls_file_all sluit alle open bestanden.
						Deze functie is te gebruiken bij een fatale
						fout.

	Terugkeerwaarde :	Geen.

*******************************************************************/
#pragma endhdr

void cls_file(int fd)
{
	/*
		To be sure, we check the fd. If we close stdin, we're
		in trouble !
	*/
	if (fd != ERROR && fd != 0)
	{
		flush_cache(f_fd[fd]);
		_close(f_fd[fd]);
		f_fd[fd] = 0;
	}
}

#pragma startfhdr
/******************************************************************

	Functie			:	void cls_file_all(void);

	Gebruik         :	Sluiten alle random access bestanden.

	Prototype in    :	file.h

	Beschrijving    :	Zie cls_file().

*******************************************************************/
#pragma endhdr

void cls_file_all(void)
{
	int i;

	for (i = 0; i < MAXFILES; i++)
	{
		if (f_fd[i])
		{
		    flush_cache(f_fd[i]);
		    _close(f_fd[i]);
		    f_fd[i] = 0;
		}
	}
}

static void new_file(int len, int fd)
{
    struct fil_hdr *b;

    b = (struct fil_hdr *) get_node(f_fd[fd], (ADDR) 1);
    b->nxt_rcd = 1;
    b->fst_rcd = 0;
    b->rcd_len = len;
    release_node(f_fd[fd], (ADDR) 1, 1);
}

#pragma startfhdr
/******************************************************************

	Functie			:	#include <toolset.h>
						ADDR new_record(int fd, const void *buff);

	Gebruik         :	Toevoegen nieuw record.

	Prototype in    :	file.h

	Beschrijving    :	De functie voegt een nieuw record toe aan
						het bestand met handle fd. buff moet naar
						de gegevens voor het record wijzen. De
						terugkeerwaarde kan gebruikt worden om een
						index mee op te bouwen.

	Terugkeerwaarde :	Het logische adres van het record.

*******************************************************************/
#pragma endhdr

ADDR new_record(int fd, const void *buff)
{
    ADDR rcd_no;
    struct fil_hdr *r, *c;

    r = (struct fil_hdr *) get_node(f_fd[fd], (ADDR) 1);
    if (r->fst_rcd)                 /* any deleted rcds to reuse ? */
    {
        rcd_no = r->fst_rcd;        /* yes, use the deleted one */
		if ((c = (struct fil_hdr *) malloc(r_len[fd])) == NULL)
            fatal(8);
        release_node(f_fd[fd], (ADDR) 1, 0);
        get_record(fd,rcd_no, c);   /* get the old copy of it */
        r = (struct fil_hdr *) get_node(f_fd[fd], (ADDR) 1);
        r->fst_rcd = c->nxt_rcd;    /* puts its nxt into hdrs lst */
        free(c);
    }
    else                            /* no deleted records to reuse */
    {
        rcd_no = r->nxt_rcd++;
        nxav[fd] = rcd_no+1;
    }
    release_node(f_fd[fd], (ADDR) 1, 1);
    put_record(fd, rcd_no, buff);
    return(rcd_no);
}

#pragma startfhdr
/******************************************************************

	Functie			:	#include <toolset.h>
						int delete_record(int fd, ADDR rcd_no);

	Gebruik         :	Wissen record uit random access bestand.

	Prototype in    :	file.h

	Beschrijving    :	delete_record wist het record met het logische
						adres rcd_no uit het bestand fd. De ruimte
						die het record innam wordt gemarkeerd als vrij,
						en wordt bij de volgende new_record weer
						gebruikt.

	Terugkeerwaarde :	OK bij success, ERROR bij mislukking.

*******************************************************************/
#pragma endhdr

int delete_record(int fd, ADDR rcd_no)
{
    struct fil_hdr *b, *buff;

    if (rcd_no > nxav[fd] || rcd_no == (ADDR) 0)
        return ERROR;
    if ((buff = (struct fil_hdr *) malloc(r_len[fd])) == NULL)
        fatal(8);
    setmem(buff, r_len[fd], '\0');
    b = (struct fil_hdr *) get_node(f_fd[fd], (ADDR) 1);
    buff->nxt_rcd = b->fst_rcd;
    buff->fst_rcd = (-1);
    b->fst_rcd = rcd_no;
    release_node(f_fd[fd], (ADDR) 1, 1);
    put_record(fd, rcd_no, buff);
    free(buff);
    return OK;
}

#pragma startfhdr
/******************************************************************

	Functie			:	#include <toolset.h>
						int get_record(int fd, ADDR rcd_no,
								void *buffer);

	Gebruik         :	Ophalen record uit random access bestand.

	Prototype in    :	file.h

	Beschrijving    :	get_record probeert het record met nummer
						rcd_no uit bestand fd op te halen. Indien
						dit lukt, wordt buffer gevuld met de record-
						inhoud. buffer dient groot genoeg te zijn om
						het record te bevatten.

						Indien het record gewist is, zal de eerste
						positie van buffer een integer met de waarde
						-1 zijn. Dit kan men gebruiken om een bestand
						sequentieel te lezen waarbij de gewiste
						records overgeslagen worden.

	Terugkeerwaarde :	OK bij succes, ERROR bij mislukking.

*******************************************************************/
#pragma endhdr

int get_record(int fd, ADDR rcd_no, void *buffer)
{
    int pos, d, len;
    ADDR sector;
    char *d_ptr;
    char *buff;

    buff = (char *) buffer;

    if (rcd_no >= nxav[fd] || rcd_no == (ADDR) 0)
        return ERROR;
    len = r_len[fd];
    locate(rcd_no, len, &sector, &pos);     /* compute sector, position */

    while (len)
    {
        d_ptr = get_node(f_fd[fd], sector);
        for (d = pos; d < NODELEN && len; d++)
        {
            *buff++ = *(d_ptr+d);
            len--;
        }
        release_node(f_fd[fd],sector, 0);
        pos = 0;
        sector++;
    }
    return(OK);
}

#pragma startfhdr
/******************************************************************

	Functie			:	#include <toolset.h>
						int put_record(int fd, ADDR rcd_no,
							const void *buffer);


	Gebruik         :   Herschrijven gewijzigd record naar random
						access bestand.

	Prototype in    :	file.h

	Beschrijving    :	Deze functie schrijft de inhoud van buffer
						terug naar bestand fd op de logische positie
						aangegeven door rcd_no. Er wordt gecontroleerd
						of het recordnummer ooit toegewezen is, en de
						functie mislukt indien dit niet het geval is.

	Terugkeerwaarde :	OK bij succes, ERROR bij een fout.
*******************************************************************/
#pragma endhdr

int put_record(int fd, ADDR rcd_no, const void *buffer)
{
    int pos, d, len;
    ADDR sector;
    char *d_ptr;
    char *buff;

    buff = (char *) buffer;
    if (rcd_no > nxav[fd] || rcd_no == (ADDR) 0)
        return ERROR;
    len = r_len[fd];
    locate(rcd_no, len, &sector, &pos);

    while(len)
    {
        d_ptr = get_node(f_fd[fd], sector);
        for (d = pos; d < NODELEN && len; d++)
        {
            *(d_ptr + d) = *buff++;
            len--;
        }
            release_node(f_fd[fd], sector, 1);
            pos = 0;
            sector++;
    }
    return OK;
}

static void locate(ADDR rcd_no, int len, ADDR *sector, int *pos)
{
    long byte_ct;

    byte_ct = rcd_no -1;
    byte_ct *= len;
    byte_ct += (sizeof(ADDR) *2) + sizeof(int);
    *sector = (ADDR) (byte_ct / NODELEN + 1);
    *pos = (int) (byte_ct % NODELEN);
}
