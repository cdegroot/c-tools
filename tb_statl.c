/********************************************************
*   void makestat(char *status);            *
*                           *
*   Zet status neer op statusregel          *
*                           *
*   void nostat(void);              *
*                           *
*   Leegt statusregel               *
********************************************************/
/*
	$Id: tb_statl.c,v 1.2 1993/02/05 13:02:56 cg Rel $
*/
#include <window.h>
#include <conio.h>
#include <string.h>

static int buffer[80];

void makestat(const char *s)
{
    register int i;
    int len,  start;

    len = strlen(s);
    start = 40 - (len / 2);
    for (i = 0; i < start; i++)
        buffer[i] = 0x7020;
    for (i = start; i < (start + len); i++)
        buffer[i] = 0x7000 | s[i-start];
    for (i = (start + len); i <80; i++)
        buffer[i] = 0x7020;
    puttext(1, 25, 80, 25, buffer);
}

void nostat(void)
{
    int i;

    for (i = 0; i < 80; i++)
        buffer[i] = 0x0720;
    puttext(1, 25, 80, 25, buffer);
}

#if 0
/********************
*********************
MAIN: TESTFUNCTIE
*********************
********************/

void main(void) {
    WIN *wn;

    wn = makewin(1,1,80,24,"",7,7,7,FST_GSING);
    cprintf("Returned from makewin: %p, win_error = %d\n",wn, win_error);
        makestat("Press any key to wis statusregel (dit is een statusregel van 80 karakters.....)");
    while (!kbhit());
    getch();
    nostat();
    cprintf("Press any key to exit");
    while (!kbhit());
    getch();
    closewin();
}

#endif


