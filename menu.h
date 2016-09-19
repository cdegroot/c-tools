/* -------------- menu.h --------------- */
/*
	$Id: menu.h,v 1.2 1993/02/05 13:02:30 cg Rel $
*/

#define MXSELS 10 			/* Maximum aantal selecties in een menu */

#if !defined(__WINDOW_DEF)
#include <window.h>
#endif

#if !defined(OK)
#define OK 0
#define ERROR -1
#endif

#if __STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif

typedef void mfptr(void);

struct popm
{
	char	 *title;	/* Menu-titel */
	int		def_keuze;	/* Standaard-keuze */
	int		x,y,ns; /* (x,y) en aantal keuzes */
	struct menu_item
	{
		char		 *description; 	/* Beschrijving */
		int			hv_pos;		/* Positie die in high-video moet */
		mfptr		 *trigfunc;		/* Pointer naar trigger-functie */
		struct popm  *trigmenu;	/* Pointer naar trigger-submenu */
		int			t_type;			/* Type trigger (zie beneden */
	} mi[MXSELS];				/* MXSELS items per menu */
};

struct linem
{
	int					def_keuze;
	int					ns;
	struct menu_item	mi[MXSELS];
};

/* Definities voor trigger-types */
#define TR_MENU		0			/* Trigger wijst naar ander popm-struct */
#define TR_FUNC		1			/* Trigger wijst naar een functie */
					/* (alleen bij pop-up menu's) */
#define TR_PROT		2			/* Geen trigger, protected entry */
#define TR_FCHC		3			/* Functie, die de huidige entry verandert */
#define TR_FCHA		4			/* Funcite, die het hele menu verandert */

int       _Cdecl menu_exec(struct linem  *lm);
int       _Cdecl popmenu_struct(struct popm  *pm);
