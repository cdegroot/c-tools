/* ---------------------------------------- */
/* Include file for Datascreen management   */
/* ---------------------------------------- */
/*
	$Id: screen.h,v 1.2 1993/02/05 13:02:37 cg Rel $
*/

#if __STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif

#define SCRNID '*'      /* starts a screen template */
#define SCBUFF 1920     /* most chars. needed for a screen */
#define MAXSCRNS 5      /* max. screens in a file */
#define MAXFIELDS 30    /* max. fields on a screen */
#define FLDFILL '_'     /* filed filler character */

typedef int _Cdecl edit_func(char  *s, char k);   /* s->entered data */
                                            /* k = last keystroke */
                                            /* returns ERROR or OK */
typedef char * _Cdecl help_func(void);   /* returns data or ptr to "\0" */

struct scrn_buf
{
    char item_type;         /* 'A' = alpha-numeric */
                            /* 'N' = numeric, space filled */
                            /* 'Z' = numeric, zero filled */
                            /* 'C' = currency */
                            /* 'D' = date */
    int protect;            /* TRUE = protected field */
	char  *item_value;       /* ->string for data */
	edit_func ( *editfptr);  /* edit function */
	help_func ( *helpfptr);  /* ->help function */
    int fill_data;          /* TRUE = backfill, FALSE = clear */
    int ditto;              /* TRUE for automatic ditto */
	char  *dit_ptr;          /* ->ditto value */
};

void        _Cdecl read_screens(const char  *screen_file);
char        _Cdecl scrn_proc(int scrn_no,
					struct scrn_buf  *scrn_data, int go_last);
void        _Cdecl err_msg(const char  *mess);
void        _Cdecl notice(const char  *mess);
void        _Cdecl clrmsg(void);
void        _Cdecl backfill(int scrn_no, int item,
						struct scrn_buf  *data);
char        _Cdecl get_item(struct scrn_buf  *sc, int crt, int fld);
void        _Cdecl reset_screens(void);
void        _Cdecl disp_scrn(int scrn_no, struct scrn_buf  *scrn_data);
int         _Cdecl spaces(const char  *test);

extern char  *default_status;

#define IDI_ON /* | IDI_OFF */
#ifdef IDI_ON
extern int IDI_SWITCH;				/* Hoofdschakelaar van IDI-hooks */
extern int IDI_FLAGS[MAXFIELDS];	/* Flags voor ind. velden */
extern void	( *IDI_FP)(int IDI_CF, const char  *c);
									/* ->Hook-functies */
extern void ( *END_FP)(int IDI_CF, char  *c, char k);
#endif


