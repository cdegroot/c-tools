/********************************************************
*   void fieldattr(int x, int y, int len,       *
*           char attr);                     *
*                           *
*   Zet het veld, beginnend op (x,y), met lengte    *
*   len, op attribute attr              *
********************************************************/
/*
	$Id: tb_fldat.c,v 1.2 1993/02/05 13:02:45 cg Rel $
*/

#include <window.h>
#include <conio.h>
#include <dos.h>

char buffer[160];

void fieldattr(int x, int y, int len, register char attribute)
{
	register i;
	int realx, realy;
        struct text_info ti;

	if ((x + len) > 81)
        return;
    gettextinfo(&ti);
    realx = x + ti.winleft - 1;
    realy = y + ti.wintop - 1;
    gettext(realx, realy, realx + len - 1, realy, buffer);
    for (i = 1; i < 2 * len; i += 2)
        buffer[i] = attribute;
    puttext(realx, realy, realx + len - 1, realy, buffer);
}

static int ct = CURS_SMALL;
void cursor(int type)
{
    char top,bot;

    switch(type)
    {
        case CURS_SMALL: top = 11;
                         bot = 12;
                         break;
        case CURS_BIG:   top = 0;
                         bot = 12;
                         break;
		case CURS_OFF:   top = 0xff;
						 bot = 0xff;
                         break;
		default:
						 return;
    }
	ct = type;
    _CH = top;
    _CL = bot;
    _AH = 1;
    geninterrupt(0x10);
}

int getcursor(void)
{
	return ct;
}
