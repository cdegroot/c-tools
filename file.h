/* ---------------- file.h ---------------- */
/*
	$Id: file.h,v 1.2 1993/02/05 13:02:26 cg Rel $
*/

#if __STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif

#ifndef TOOLSET
#include <toolset.h>
#endif

int        _Cdecl opn_file(const char  *name, int len);
void       _Cdecl cls_file(int fd);
void       _Cdecl cls_file_all(void); /* Close all files */
ADDR       _Cdecl new_record(int fd, const void  *buff);
int        _Cdecl delete_record(int fd, ADDR rec_no);
int        _Cdecl get_record(int fd, ADDR rec_no, void  *buf);
int        _Cdecl put_record(int fd, ADDR rec_no, const void  *buf);
int        _Cdecl rcd_length(int fd);
