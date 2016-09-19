/***************************************************
*   ED.EXE, programma voor het enig document       *
*   (C)1989, C.A. de Groot                         *
*                                                  *
*   MODULE: RECORDIO.H                             *
*       bevat controle-structure                   *
***************************************************/
/*
	$Id: recordio.h,v 1.2 1993/02/05 13:02:35 cg Rel $
*/

#ifndef TOOLSET
#include <toolset.h>
#endif

#if __STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif

#define MAXIKEYS 2

typedef void _Cdecl rio_func(void);

typedef struct
{
    int     df;                 /* datafile */
    int     len;                /* record length */
    ADDR    adr;                /* record address */
    int     exfl;               /* TRUE if existing record */
	void	 *data;          /* data */
	void	 *hold;          /* hold */
    struct key_info
    {
        int     bt;             /* btree */
        int     ky_off;         /* key offset */
        int     unq;            /* UNIQUE flag */
		int      *prot;      /* ->protect flag */
        int     used;           /* TRUE if this entry is used */
    } ndx[MAXIKEYS];
	rio_func    ( *clr_fptr);    /* Called when clearing records */
	rio_func    ( *get_fptr);    /* Called after retrieving records */
} rcd_ctl;                      /* Record I/O control structure */


/* Record I/O functies */
void        _Cdecl rtn_rcd(rcd_ctl  *control);
void        _Cdecl clr_rcd(rcd_ctl  *control);
void        _Cdecl get_rcd(rcd_ctl  *control);
void        _Cdecl get_nxt(rcd_ctl  *control, int key_no);
void        _Cdecl get_prv(rcd_ctl  *control, int key_no);
void        _Cdecl process_records(rcd_ctl  *control,
						struct scrn_buf  *screen,
						int main_key, int scrn_no);

