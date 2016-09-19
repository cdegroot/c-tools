C Toolbox for MS-DOS
====================

One for the computing museum, but this one is dear to me.

When I started programming, for realz (which was not: write machine code
on a piece of paper for the C64 and hope your friend, who owns one, will
let you type it in to see what it does), it was in C. A fellow student landed
me a gig which had to be done in C, so I learned C.

Given that I was at a business school, there was a dearth of books about
computer programming. Luckily, there were two books about C: the K&R
book, and [C development tools for the IBM PC](http://www.worldcat.org/title/c-development-tools-for-the-ibm-pc/oclc/12286591). It was full of code, basically
explaining how to drive the screen, store data on disk, index it
using B-Trees, etcetera. In the beginning, I used especially the source
code appendix a lot to see how stuff got done (this was before the Interwebs,
so we got our knowledge from books...). Later on, I set out to typing in
the whole source code appendix because I was landing some gigs where it
could be helpful ("whole systems" written for MS-DOS; one of them got
delivered in '89 or so and kept running until I got "the call" 10 years
later: whether my software was Y2K compliant. I had to disappoint them).

As I was using the [DeSmet C Toolkit](http://www.desmet-c.com/) back then,
and C wasn't really standardized on MS-DOS, I vaguely remember having to
tweak here and there, possibly because of newer compiler and libraries. I
may also have extended the author's original library; so code ownership
here is a bit murky; most is Mr. Stevens', some is mine, and the only way
to find out which is which is by doing a line-for-line comparison with the
book by now :-).

Note the CVS tags (alas, I lost the original repo, and in the beginning
a code repository versioning scheme was just "make a copy of the source
code floppy") and the fact that I seem to have used comments in the Dutch
language, something I hope I'll never repeat again.

When I have some time, I'm planning to see what's needed to make this run on
a modern system. With the Internet of Things looming, there might be renewed
interest in libraries that are 6 KLOC and compile down to tiny object code.

Al Stevens, the author, graciously gave me permission to throw this out here.
Please don't contact him on any of this - he's retired and not programming anymore.

