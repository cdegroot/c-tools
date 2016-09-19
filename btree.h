/* ---------------- btree.h --------------- */
/*
	$Id: btree.h,v 1.2 1993/02/05 13:02:24 cg Rel $
*/

#if __STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif

#ifndef TOOLSET
#include <toolset.h>
#endif

int        _Cdecl init_b(const char  *ndx_name, int keylen);
int        _Cdecl close_b(int tree);
void       _Cdecl close_b_all(void);
int        _Cdecl find_key(int tree, const char  *key, ADDR  *ad);
int        _Cdecl insert_key(int tree, const char  *key,
								ADDR ad, int unique);
int        _Cdecl delete_key(int tree, const char  *key, ADDR ad);
ADDR       _Cdecl get_next(int tree);
ADDR       _Cdecl get_prev(int tree);
ADDR       _Cdecl find_first(int tree);
ADDR       _Cdecl find_last(int tree);
void       _Cdecl currkey(int tree, char  *ky);
ADDR       _Cdecl get_curr(int tree);

#define MXKEY   30      /* maximum key length */
#define MAXNDX  25      /* maximum open trees */

struct b_node
{
	int isnode;         /* 0 if leaf, 1 if node */
	union
    {
        ADDR prnt;      /* node# of parent */
        ADDR dlptr;     /* linked list ptr for deleted nodes */
    } bu;
    ADDR l_bro;         /* left sibling */
    ADDR r_bro;         /* right sibling */
    int nkeys;          /* number of key in this node */
    ADDR faddr;         /* node # of keys */
    char kdat[1];       /* 1st byte of 1st key */
};

struct b_hdr
{
    ADDR root;          /* node # of root */
    int klen;           /* key length */
    int m;              /* max # of keys in one node */
    ADDR nxt_free;      /* node# of next free node */
    ADDR nxt_avail;     /* node# of next available node */
    int in_use;         /* TRUE when tree is in use */
    ADDR lmost;         /* left most node among leaves */
    ADDR rmost;         /* right most node */
};

