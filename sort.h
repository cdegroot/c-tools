/* ---------------------------------------- */
/* Include file for Quick-Sort/Merge        */
/* Also include (previous to this one) the  */
/* TOOLSET.H file, for ADDR size            */
/* ---------------------------------------- */
/*
	$Id: sort.h,v 1.2 1993/02/05 13:02:38 cg Rel $
*/

#if __STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif

#ifndef TOOLSET
#include <toolset.h>
#endif

#define NOFLDS 5                /* maximum number of fields to sort */

struct s_prm                    /* sort parameters */
{
    int     rc_len;             /* record length */
    char    sort_drive;         /* disk drive for work files */
                                /* 1, 2, ... = A, B, etc. */
                                /* 0 = current drive */
	int		( *comp)(char  *a, char  *b);
	struct
    {
        int     f_pos;          /* field position (rel. to 1) */
        int     f_len;          /* field length */
        char    az;             /* A = ascending; Z = descending */
    } s_fld[NOFLDS];            /* one per field */
};

int         _Cdecl init_sort(struct s_prm  *params);
void        _Cdecl sort(const void  *sort_rec);
void  *  _Cdecl sort_op(void);
void        _Cdecl sort_stats(void);

