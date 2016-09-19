/* -------------- cache.h ----------------- */
/*
	$Id: cache.h,v 1.2 1993/02/05 13:02:25 cg Rel $
*/

#if __STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif

#ifndef TOOLSET
#include <toolset.h>
#endif

void         _Cdecl init_cache(void);
void  *   _Cdecl get_node(int filedescriptor, ADDR nodeaddress);
void         _Cdecl put_node(int filedescriptor, ADDR nodeaddress,
								const void  *buffer);
void        _Cdecl release_node(int filedescriptor, ADDR nodeaddress,
								int changed);
void        _Cdecl flush_cache(int filedescriptor);

extern int _mxnodes;
