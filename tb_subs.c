/* ------------------- subs.c ------------------ */
/*
	$Id: tb_subs.c,v 1.3 1993/02/05 13:02:57 cg Rel $
*/
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <mem.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>

#include <toolset.h>
#include <btree.h>
#include <file.h>
#include <window.h>

#define DOS_EXTRA_HANDLES

/* Extended file-handles */
#ifdef DOS_EXTRA_HANDLES
int _openfd[EXTRA_HANDLES] =
{
	O_RDONLY | O_DEVICE | O_TEXT,		/* stdin */
	O_WRONLY | O_DEVICE | O_TEXT,		/* stdout */
	O_WRONLY | O_DEVICE | O_TEXT,		/* stderr */
	O_RDWR   | O_DEVICE | O_BINARY,		/* stdprn */
	O_WRONLY | O_DEVICE | O_BINARY		/* stdaux */
};
#endif

void fatalfl(int n, char *file, int line)
{
	static char *errm[] =
	{
		"Insufficient memory for cache",
		"Disk I/O error",
		"Index file not properly closed",
		"Cannot find template file",
		"Insufficient memory for screens",
		"Cannot start program",
		"Screen buffer is too small",
		"Insufficient memory for files",
		"Insufficient memory for B-trees",
		"Window subsystem error",
		"File I/O error"
	};
	int i;

	printf("\n\n\n\n\n\n\n\n\n\n\n\nError ts%03d: %s (%s:%d)\n",
			n,errm[n-1],file,line);
	if (n == 10)
		printf("\tWindows reports %d: %s\nTerminating.\n",
				win_error,win_errmsg[win_error]);
	else
	{
		perror("\tDos reports");
		printf("\tTC-Errno: %d DOS-Errno: %d\nTerminating.\n",errno,_doserrno);
	}
	close_b_all();
	cls_file_all();
	for (i = 0; i < 255; i++)
		fclose(&_streams[i]);
	for (i = 0; i < 255; i++)
		close(i);
	abort();
}

/* Ask DOS for 255 file-handles */
void _extend_handles(void)
{
#ifdef DOS_EXTRA_HANDLES
	extern unsigned _nfile;
	union REGS inregs, outregs;

	/*
		Increase _nfile, so the library
		doesn't generate unnecessary error messages
	*/
	_nfile = EXTRA_HANDLES;

	/*
		Set DOS allocation to high-mem. If we don't do this, DOS
		allocates memory for the extra handles just above our program
		space, so future malloc() calls will fail.
	*/
	inregs.h.ah = 0x58;         /* DOS function set/get allocation strategy */
	inregs.h.al = 0x01;         /* Set strategy */
	inregs.x.bx = 0x02;         /* Allocate from high to low */
	intdos(&inregs, &outregs);	/* Call DOS */

	/*
		Ask for extra handles
	*/
	inregs.h.ah = 0x67;         /* DOS function request extra handles */
	inregs.x.bx = EXTRA_HANDLES;/* # of handles */
	intdos(&inregs, &outregs);	/* Call DOS */

	/*
		Reset DOS allocation strategy to default
	*/
	inregs.h.ah = 0x58;
	inregs.h.al = 0x01;
	inregs.x.bx = 0x0;          /* Allocate from low to high */
	intdos(&inregs, &outregs);
#endif
}
