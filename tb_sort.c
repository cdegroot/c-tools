/* -------------- sort.c ------------------ */
/*
	$Id: tb_sort.c,v 1.2 1993/02/05 13:02:54 cg Rel $
*/
#include <stdio.h>
#include <sys\stat.h>
#include <ctype.h>
#include <string.h>
#include <io.h>
#include <mem.h>
#include <alloc.h>
#include <fcntl.h>
#include <toolset.h>
#include <sort.h>

#define MOSTMEM  40*1024        /* max. alloc'd memory */
#define LEASTMEM 10*1024        /* min. alloc'd memory */

struct bp                       /* one of these for each sequence */
{
    char    *rc;                /* ->record in merge buffer */
    int     rbuf;               /* #recs left in buffer */
    int     rdsk;               /* #recs left on disk */
};

static  struct s_prm    *sp;        /* sort parameters */
static  int         totrcd;         /* total #recs */
static  int         no_seq;         /* counts sequences */
static  int         no_seq1;
static  unsigned    h;              /* amount of buffer space avail. */
static  int         nrcds;          /* constant #recs in buffer */
static  int         nrcds1;
static  char        *bf, *bf1;      /* ->sort buffer */
static  int         inbf;           /* variable #recs in buffer */
static  char        **sptr;         /* ->array of buffer pointers */
static  char        *init_sptr;     /* ->appropriated buffer */
static  int         n;              /* #recs/sequens in merge buffer */
static  int         fd, f2;         /* sort work files */
static  char        fdname[15];     /* sort work name */
static  char        f2name[15];     /* sort work name */

static void qsort_ts(char **p, int n, int (*comp)(char *a, char *b));
static void prep_merge(void);
static void merge(void);
static void dumpbuff(void);
static int wopen(char *name, int n);
static void *appr_mem(unsigned *h);
static int std_comp(char *a, char *b);
static int (*comp)(char *a, char *b);

int init_sort(struct s_prm *prms)
{
    sp = prms;
    if ((bf = appr_mem(&h)) != NULL)
    {
        nrcds1 = nrcds = h / (sp->rc_len + sizeof(char *));
        init_sptr = bf;
        sptr = (char **) bf;
        bf += nrcds * sizeof(char *);
        fd = f2 = totrcd = no_seq = inbf = 0;
		if (prms->comp)
			comp = prms->comp;
		else
			comp = std_comp;
        return OK;
    }
    else
        return ERROR;
}

void sort(const void *s_rcd)
{
    if (inbf == nrcds)                  /* if the sort buffer is full */
    {
        qsort_ts((char **) init_sptr, inbf, comp);  /* sort the buffer */
        if (s_rcd)                      /* if there are more records */
        {
            dumpbuff();                 /* dump buffer to a work file */
            no_seq++;                   /* count sequences */
        }
    }
    if (s_rcd)                          /* if this is a record */
    {
        totrcd++;                       /* count records */
        *sptr = bf + inbf * sp->rc_len; /* put a rcd addr in the ptr array */
        inbf++;                         /* counts rcds in buffer */
        movmem(s_rcd, *sptr, sp->rc_len);/* move the rcd to the buffer */
        sptr++;                         /* point to the next entry */
    }
    else                                /* no more recs */
    {
        if (inbf)                       /* recs in buffer ? */
        {
            qsort_ts((char **) init_sptr, inbf, comp);/* sort them */
            if (no_seq)                 /* if this isn't the only seq */
                dumpbuff();
            no_seq++;
        }
        no_seq1 = no_seq;
        if (no_seq > 1)
            prep_merge();
    }
}

static void prep_merge(void)
{
    register int i;
    int hd;
    register struct bp *rr;
    unsigned n_bfsz;

    n_bfsz = h - no_seq * sizeof(struct bp); /*merge buffer size */
    n = n_bfsz / no_seq / sp->rc_len;   /* #rcds/seq in mrg buff */
    if (n < 2)
    {
        f2 = wopen(f2name,2);
        while (n < 2)
        {
            merge();
            hd = fd;
            fd = f2;
            f2 = hd;
            nrcds *= 2;
            no_seq = (no_seq + 1) / 2;
            n_bfsz = h - (no_seq * sizeof(struct bp));
            n = n_bfsz / no_seq / sp->rc_len;
        }
    }
    bf1 = init_sptr;
    rr = (struct bp *) init_sptr;
    bf1 += no_seq * sizeof(struct bp);
    bf = bf1;

    /* Fill the merge buffer with records from all sequences */

    for (i = 0; i < no_seq; i++)
    {
        lseek(fd, (long) i * ((long) nrcds * sp->rc_len), 0);
        read(fd, bf1, n * sp->rc_len);
        rr->rc = bf1;
        if (i == no_seq - 1)
        {
            if (totrcd % nrcds > n)
            {
                rr->rbuf = n;
                rr->rdsk = (totrcd % nrcds) - n;
            }
            else
            {
                rr->rbuf = totrcd % nrcds;
                rr->rdsk = 0;
            }
        }
        else
        {
            rr->rbuf = n;
            rr->rdsk = nrcds - n;
        }
        rr++;
        bf1 += n * sp->rc_len;
    }
}

static void merge(void)
{
    register int i;
    int needy, needx;   /* logical switches. TRUE = need rec from x/y */
    int xcnt, ycnt;     /* #recs left each sequence */
    int x, y;           /* seq counters */
    long adx, ady;      /* disk addresses */

    /* The two sets of sequences are thought of */
    /* as X and Y */
    lseek(f2, 0, 0);
    for (i = 0; i < no_seq; i += 2)
    {
        x = y = i;
        y++;
        ycnt = (y == no_seq) ? 0 :
                ((y == no_seq - 1) ? (totrcd % nrcds) : nrcds);
        xcnt = (y == no_seq) ? (totrcd % nrcds) : nrcds;
        adx = (long) x * (long) (nrcds * sp->rc_len);
        ady = adx + (long) (nrcds * sp->rc_len);
        needy = needx = TRUE;
        while (xcnt || ycnt)
        {
            if (needx && xcnt)  /* need a record from x ? */
            {
                lseek(fd, adx, 0);
                adx += (long) sp->rc_len;
                read(fd, init_sptr, sp->rc_len);
                needx = FALSE;
            }
            if (needy && ycnt) /* need a record from y ? */
            {
                lseek(fd, ady, 0);
                ady += (long) sp->rc_len;
                read(fd, init_sptr + sp->rc_len, sp->rc_len);
                needy = FALSE;
            }
            if (xcnt || ycnt)   /* if anything is left */
            {
                /* compare data between sequences */
                if (!ycnt ||
                    (xcnt && (comp(init_sptr, init_sptr + sp->rc_len)) < 0))
                {
                    /* data from x is lower */
                    write(f2, init_sptr, sp->rc_len);
                    --xcnt;
                    needx = TRUE;
                }
                else if (ycnt)
                {
                    /* data from y is lower */
                    write(f2, init_sptr + sp->rc_len, sp->rc_len);
                    --ycnt;
                    needy = TRUE;
                }
            }
        }
    }
}

static void dumpbuff(void)
{
    register int i;

    if (!fd)
        fd = wopen(fdname, 1);
    sptr = (char **) init_sptr;
    for (i = 0; i < inbf; i++)
    {
        write(fd, *(sptr+i), sp->rc_len);
        *(sptr+i) = 0;
    }
    inbf = 0;
}

static int wopen(char *name, int n)
{
    int fd;

    *name = '\0';
    if (sp->sort_drive)
    {
        strcpy(name, "A:");
        *name += (sp->sort_drive - 1);
    }
    strcat(name, "sortwork.000");
    name[strlen(name)-1] += n;
    fd = creat(name, S_IWRITE | S_IREAD);
    close(fd);
    return open(name, O_RDWR | O_BINARY);
}

void *sort_op(void)
{
    int j = 0;
    int nrd, i, k, l, m;
    register struct bp *rr;
    static int r1 = 0;
    register char *rtn;
    long ad, tr;

    sptr = (char **) init_sptr;
    if (no_seq < 2)
    {
        if (r1 == totrcd)
        {
            free(init_sptr);
            r1 = 0;
            return NULL;
        }
        return (void *) *(sptr + r1++);
    }
    rr = (struct bp *) init_sptr;
    for (i = 0; i < no_seq; i++)
        j |= ((rr+i)->rbuf | (rr+i)->rdsk);

    /* j will be true if any seq still has records */

    if (!j)
    {
        close(fd);
        unlink(fdname);
        if (f2)
        {
            close(f2);
            unlink(f2name);
        }
        free(init_sptr);
        r1 = 0;
        return NULL;
    }
    k = 0;

    /* Find the sequence in the merge buffer with the lowest record */

    for (i = 0; i < no_seq; i++)
    {
        m = ((comp((rr+k)->rc, (rr+i)->rc) < 0) ? k : i);
        k = m;
    }

    /* K is an int seq number that offsets to the */
    /* sequence with the lowest record */

    (rr+k)->rbuf--;
    rtn = (rr+k)->rc;
    (rr+k)->rc += sp->rc_len;
    if ((rr+k)->rbuf == 0)
    {
        rtn = bf + (k * n * sp->rc_len);
        movmem((rr+k)->rc - sp->rc_len, rtn, sp->rc_len);
        (rr+k)->rc = rtn + sp->rc_len;
        if ((rr+k)->rdsk != 0)
        {
            l = ((n-1) < (rr+k)->rdsk) ? (n - 1) : (rr+k)->rdsk;
            nrd = (k == no_seq - 1) ? (totrcd % nrcds) : nrcds;
            tr = (long) ((k * nrcds) + (nrd - (rr+k)->rdsk));
            ad = tr * sp->rc_len;
            lseek(fd, ad, 0);
            read(fd, rtn + sp->rc_len, l * sp->rc_len);
            (rr+k)->rbuf = l;
            (rr+k)->rdsk -= l;
        }
        else
            setmem((rr+k)->rc, sp->rc_len, 127);
    }
    return (void *) rtn;
}

void sort_stats(void)
{
    FILE *out;

    out = stdout;
    fprintf(out,"\n\nRecord Length = %d",sp->rc_len);
    fprintf(out,"\n%d records sorted",totrcd);
    fprintf(out,"\n%d sequences",no_seq1);
    fprintf(out,"\n%u characters of sort buffer",h);
    fprintf(out,"\n%d records per buffer\n\n",nrcds1);
}

static void *appr_mem(unsigned *h)
{
    void *buff = NULL;

    *h = MOSTMEM + 1024;
    while (buff == 0 && *h > LEASTMEM)
    {
        *h -= 1024;
        buff = malloc(*h);
    }
    return buff;
}

static int std_comp(char *a, char *b)
{
    register int i, j;
    int k;

    if (*a == 127 || *b == 127)
        return *a - *b;
    for (i = 0; i < NOFLDS; i++)
        for (j = 0; j < sp->s_fld[i].f_len; j++)
            if ((k = *(a + sp->s_fld[i].f_pos - 1 + j) -
                    *(b + sp->s_fld[i].f_pos - 1 + j)) != 0)
                return (toupper(sp->s_fld[i].az) == 'Z') ? -k : k;
    return 0;
}

static void qsort_ts(char **p, int n, int (*comp)(char *a, char *b))
{
    int i = 0, j;
    register char *k;
    register char *l;
    if (n < 2)
        return;
    j = n;
    k = p[j/2];
    p[j/2] = p[0];
    p[0] = k;
    while (TRUE)
    {
        while ((*comp)(p[++i], k) < 0 && i < j - 1)
            ;
        while ((*comp)(p[--j], k) > 0);
            ;
        if (i < j)
        {
            l = p[j];
            p[j] = p[i];
            p[i] = l;
        }
        else
            break;
    }
    p[0] = p[j];
    p[j] = k;
    qsort_ts(p, j, comp);
    qsort_ts(p+j+1, n-j-1, comp);
}
