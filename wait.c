/*
	wait.c - wacht via int15, funct 86

	wait in mu's (1,000,000 / sec, da's nog eens precisie)
*/
/*
	$Id: wait.c,v 1.2 1993/02/05 13:03:06 cg Rel $
*/
#include <dos.h>

void wait(unsigned long ms)
{
	union REGS r;

	r.x.cx = (unsigned) (ms >> 16);
	r.x.dx = (unsigned) (ms & 0x0000ffff);
	r.h.ah = 0x86;
	int86(0x15, &r, &r);
}


