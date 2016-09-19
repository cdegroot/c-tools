/* -------------- toolset.h ---------------- */
/*
	$Id: toolset.h,v 1.3 1993/02/05 13:03:03 cg Rel $
*/

#define TOOLSET

#ifndef ERROR
#define ERROR -1
#define OK 0
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define EXTRA_HANDLES 100   /* Number of handles requested to DOS */

#define NODELEN 512         /* For opt. performance, mult. of sectorsize */
#define ADDR unsigned long	/* Address size */

#define SAVE_SECS 300		/* Screen saver: 5 minutes */

#define DATA_FILES	".\\"	/* Where the data files are */
#define SCRN_FILES  ".\\"	/* Where the .crt files are */

void fatalfl(int error, char  *src_file, int line_no);
#define fatal(n) fatalfl((n),__FILE__,__LINE__)
