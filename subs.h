/* Subs.h */
/*
	$Id: subs.h,v 1.2 1993/02/05 13:02:39 cg Rel $
*/

#if __STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif


/* File handle type */
#if defined(STREAMS)
#ifndef __STDIO_DEF_
#include <stdio.h>
#endif
typedef FILE* FILETYPE;
#else
typedef int FILETYPE;
#endif

#define S_START 0
#define S_CURR  1
#define S_END   2

#define M_WRITE 1
#define M_READ  2
#define M_RDWR  3

int       _Cdecl execute(char  *p, char  *arg1, char  *arg2);
FILETYPE  _Cdecl _createfile(char  *name, int mode);
int       _Cdecl _seekfile(FILETYPE file, long offset, int fromwhere);
FILETYPE  _Cdecl _openfile(char  *name, int mode);
int       _Cdecl _closefile(FILETYPE file);
int       _Cdecl _readfile(FILETYPE file, void  *ptr, int nitems);
int       _Cdecl _writefile(FILETYPE file, void  *ptr, int nitems);

