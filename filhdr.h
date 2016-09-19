/* --------------- filhdr.h --------------- */
/*
	$Id: filhdr.h,v 1.2 1993/02/05 13:02:28 cg Rel $
*/

#ifndef TOOLSET
#include <toolset.h>
#endif

struct fil_hdr
{
    ADDR fst_rcd;       /* first avail. deleted record */
    ADDR nxt_rcd;       /* next avail record position beyond EOF */
    int rcd_len;        /* record length */
};
