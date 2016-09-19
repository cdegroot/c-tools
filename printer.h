/*
	PRINTER.H

	Include-file voor algemene printerfuncties
*/
/*
	$Id: printer.h,v 1.2 1993/02/05 13:02:33 cg Rel $
*/

#if __STDC__
#define _Cdecl
#else
#define _Cdecl cdecl
#endif

int       _Cdecl init_prn(char  *pathtodriver);
void      _Cdecl select_prn(int printernumber);
int       _Cdecl prn_code(int attribute);
void      _Cdecl set_lm(int margin);
int       _Cdecl pprintf(const char  *format, ...);
int       _Cdecl pputc(char ch);

#define P_UND_ON	0x0001 	/* Underline */
#define P_UND_OFF	0x0100
#define P_BOLD_ON	0x0002	/* Boldface */
#define P_BOLD_OFF	0x0200
#define P_P15		0x0004	/* Pitch */
#define P_P10		0x0400
#define P_NLQ		0x0008	/* Letter quality */
#define P_DRAFT		0x0800
#define P_DW_ON		0x1000	/* Select printer remotely */
#define P_DW_OFF	0x2000	/* Deselect printer remotely */
/* Papierbewegingen */
#define P_TOF		0x0010  /* Set Top-of-form */
#define P_FF		0x0020	/* Perform formfeed */
#define P_PAGELEN	0x0040  /* Next attr is pagelen in /10 inches */
/* Reset printer */
#define P_RESET		0x0080	/* Hardware-reset via printer port */
#define P_FLUSH		0x4000	/* Flush associated stream */
#define P_HF		0x8000	/* Half line-feed */


