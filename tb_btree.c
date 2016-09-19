#pragma startmhdr
/******************************************************************

	C Standaarddocumentatie - (c)1990 C.A. de Groot

	Project      : 	TOOLBOX

	Module       : 	tb_btree.c

	Beschrijving : 	In deze module bevinden zich de functies voor
					het werken met balanced tree indices.

*******************************************************************/
/*
	$Id: tb_btree.c,v 1.4 1993/02/05 13:49:26 cg Rel $
*/
#pragma endhdr

#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <ctype.h>
#include <io.h>
#include <alloc.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <mem.h>
#include <string.h>
#include <dir.h>

#include <window.h>
#include <terminal.h>
#include <toolset.h>
#include <btree.h>
#include <cache.h>

/* For debugging */
#define ln fprintf(stdprn,"%s:%d\n",__FILE__,__LINE__)
#define test(x) ((x) > 31 ? (x) : '.')


static int b_fd;    /* b-tree file descriptor */

static int fds[MAXNDX];         /* fds of indexes */
static ADDR b_nbr[MAXNDX];        /* node number curr key */
static int k_ptr[MAXNDX];           /* key number curr. key */

static int build_b(const char *name, int len);
static int search_tree(ADDR *t, struct b_node **pp,
                struct b_hdr *hp, const char *k, char **a);
static ADDR new_bnode(struct b_hdr *hp);
static int find_node(const char *k, struct b_node *pp, struct b_hdr *hp, char **a);
static int keycmp(const char *a, const char *b, int len);
static ADDR find_leaf(ADDR *t, struct b_node **pp, struct b_hdr *hp,
				char **a, int *p);
static ADDR get_addr(ADDR t, struct b_node **pp, struct b_hdr *hp, char *a);
static void combine_nodes(int tree, struct b_node *left,
                struct b_node *right, struct b_hdr *hp);
static void even_dist(int tree, struct b_node *left,
                struct b_node *right, struct b_hdr *hp);
static void reparent(ADDR *ad, int kct, ADDR newp, struct b_hdr *hp);
static ADDR next_key(ADDR *p, struct b_node **pp, struct b_hdr *hp, char **a);
static ADDR prev_key(ADDR *p, struct b_node **pp, struct b_hdr *hp, char **a);
static void parent(ADDR left, ADDR parent, struct b_node **pp,
                char **c, struct b_hdr *hp);


#pragma startfhdr
/******************************************************************

	Functie			:   #include <toolset.h>
						int init_b(const char *ndx_name, int len);

	Gebruik         :	Openen/initialiseren index bestand

	Prototype in    :	btree.h

	Beschrijving    :	Deze functie opent een al bestaande index,
						of maakt een nieuwe aan (indien ndx_name
						niet gevonden wordt). De parameter len
						geeft de lengte van de sleutel in de index
						aan.

	Terugkeerwaarde :	De file-handle bij succes, anders ERROR.

*******************************************************************/
#pragma endhdr

int init_b(const char *ndx_name, int len)
{
	struct b_hdr *hp;
	char full_name[MAXPATH];
    int i;

#ifdef DEBUG
fprintf(stdprn,"Init B*tree %s\n",ndx_name);
#endif
    for (i = 0; i < MAXNDX; i++)
        if (fds[i] == 0)
            break;
    if (i == MAXNDX)
		return ERROR;
	init_cache();

	/*
		Move the data files to the DATAFILES subdirectory
	*/
	strcpy(full_name, DATA_FILES);
	strcat(full_name, ndx_name);
	if ((b_fd = _open(full_name, O_RDWR)) == ERROR)
		b_fd = build_b(full_name, len);

	hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
	if (hp->in_use)
	{
		char *msg = "\n    Index:\n\r"
					  "       %s\n\r"
					  "    is niet goed gesloten.\n\n\r"
					  "             Stoppen (J/N) ? ";
		len = strlen(full_name) + 10;
		if (len > 80)
			len = 80;
		if (len < 34)
			len = 34;
		makewinc(len,9,"Waarschuwing",ATT_INVERSE,
			ATT_INVERSE, ATT_INVERSE, FST_GDOUB,1);
		cprintf(msg, full_name);
		if (toupper(get_byte()) == 'J')
		{
			closewin();
			release_node(b_fd, (ADDR) 1, 0);
			flush_cache(b_fd);
			_close(b_fd);
			printf("\nIndex file %s",full_name);
			fatal(3);
		}
		else
			closewin();
	}
	hp->in_use = TRUE;
	release_node(b_fd, (ADDR) 1, 1);
	flush_cache(b_fd);
	fds[i] = b_fd;
	b_nbr[i] = 0;
	return i;
}

#pragma startfhdr
/******************************************************************

	Functie			:	#include <toolset.h>
						int close_b(int tree);
						void close_b_all(void);

	Gebruik         :	Sluiten van een index.

	Prototype in    :	btree.h

	Beschrijving    :	close_b sluit een index die met init_b
						geopend is.
						close_b_all sluit alle indices die open
						zijn. Deze functie is te gebruiken bij
						een fatale fout.

	Terugkeerwaarde :   OK bij success, ERROR bij een mislukking.
						close_b_all geeft niets terug.

*******************************************************************/
#pragma endhdr

int close_b(int tree)
{
    struct b_hdr *hp;

	if (tree >= MAXNDX || fds[tree] == 0 || fds[tree] == -1)
        return ERROR;
	b_fd = fds[tree];
    hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
    hp->in_use = FALSE;
    release_node(b_fd, (ADDR) 1, 1);
    flush_cache(b_fd);
    _close(b_fd);
    fds[tree] = 0;
    return OK;
}

#pragma startfhdr
/******************************************************************

	Functie			:	void close_b_all(void);

	Gebruik         :	Sluiten van alle indices.

	Prototype in    :	btree.h

	Beschrijving    :	zie close_b.

	Terugkeerwaarde :	geen.

*******************************************************************/
#pragma endhdr

/* Sluit alle B-trees */
void close_b_all(void)
{
	int i;
    struct b_hdr *hp;

	for (i = 0; i < MAXNDX; i++)
	{
		if (fds[i] != 0 && fds[i] != ERROR)
		{
			b_fd = fds[i];
			hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
			hp->in_use = FALSE;
			release_node(b_fd, (ADDR) 1, 1);
			flush_cache(b_fd);
			_close(b_fd);
			fds[i] = 0;
		}
	}
}

static int build_b(const char *name, int len)
{
    struct b_hdr *bh;
    int fd;

    if ((bh = (struct b_hdr *) malloc(NODELEN)) == NULL)
        fatal(9);
    setmem(bh, NODELEN, '\0');
    bh->klen = len;
    bh->m = (NODELEN - (sizeof(struct b_node) - 1)) /
                        (len + sizeof(ADDR));
    bh->nxt_avail = 2;
    fd = _creat(name, FA_ARCH);
    if (fd == ERROR)
        fatal(2);
    _close(fd);
    fd = _open(name, O_RDWR);
    if (_write(fd, bh, NODELEN) == ERROR)
        fatal(2);
    free(bh);
    return fd;
}

#pragma startfhdr
/******************************************************************

	Functie			:   #include <toolset.h>
						int find_key(int tree, const char *k,
							ADDR *ad);

	Gebruik         :	Opzoeken van een sleutel.

	Prototype in    :	btree.h

	Beschrijving    :	find_key zoekt naar sleutel k in index
						tree. Indien de sleutel gevonden wordt,
						geeft find_key via ad het adres van het
						bijbehorende record terug. Indien de
						sleutel niet gevonden wordt, wijst ad
						naar het adres van de volgende sleutel.
						Is de index leeg of is k groter dan de
						laatste sleutel, dan wordt ad op nul
						gezet.

	Terugkeerwaarde :	TRUE als de sleutel gevonden is, FALSE
						als de sleutel niet gevonden is.

*******************************************************************/
#pragma endhdr

int find_key(int tree, const char *k, ADDR *ad)
{
    struct b_node *pp;
    struct b_hdr *hp;
	int fnd;
	int i;
    ADDR ans, t, q;
    char *a;

    if (tree >= MAXNDX || fds[tree] == 0)
        return FALSE;
    b_fd = fds[tree];
    hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
    t = hp->root;
    if (t)
    {
        pp = (struct b_node *) get_node(b_fd, t);
        fnd = search_tree(&t, &pp, hp, k, &a);
        ans = find_leaf(&t, &pp, hp, &a, &i);
        q = t;
        if (i == pp->nkeys + 1)
        {
            i = 0;
            t = pp->r_bro;
            if (t)
            {
                release_node(b_fd, q, 0);
                pp = (struct b_node *) get_node(b_fd, t);
                ans = pp->faddr;
                q = t;
            }
            else
                ans = 0;
        }
        b_nbr[tree] = t;
        k_ptr[tree] = i;
        *ad = ans;
        release_node(b_fd, q, 0);
    }
    else
    {
        *ad = 0;
        fnd = FALSE;
    }
    release_node(b_fd, (ADDR) 1, 0);
    return fnd;
}

static int search_tree(ADDR *t, struct b_node **pp,
                        struct b_hdr *hp, const char *k, char **a)
{
    int nl;
    ADDR *a_ptr;
    ADDR p, q;

    p = *t;
    q = 0;
    do
    {
        if (find_node(k, *pp, hp, a))
        {
            while (!keycmp(*a, k, hp->klen))
                if (!prev_key(&p, pp, hp, a))
                    break;
            if (keycmp(*a, k, hp->klen))
                next_key(&p, pp, hp, a);
            *t = p;
            return TRUE;
        }
        q = p;
        a_ptr = (ADDR *) (*a - sizeof(ADDR));
        p = *a_ptr;
        nl = (*pp)->isnode;
        if (nl)
        {
            release_node(b_fd, q, 0);
            *pp = (struct b_node *) get_node(b_fd, p);
        }
    } while (nl);
    *t = q;
    return FALSE;
}

static int find_node(const char *k, struct b_node *pp, struct b_hdr *hp, char **a)
{
    register int i;
    int cm;

    *a = pp->kdat;
    for (i = 0; i < pp->nkeys; i++)
    {
        cm = keycmp(k, *a, hp->klen);
        if (!cm)
            return TRUE;
        if (cm < 0)
            return FALSE;
        *a += (hp->klen + sizeof(ADDR));
    }
    return FALSE;
}

static int keycmp(const char *a, const char *b, int len)
{
    return memcmp(a, b, len);
}



static ADDR get_addr(ADDR t, struct b_node **pp, struct b_hdr *hp, char *a)
{
    ADDR cn, ti;
	int i;

    ti = t;
    cn = find_leaf(&ti, pp, hp, &a, &i);
    release_node(b_fd, ti, 0);
    *pp = (struct b_node *) get_node(b_fd, t);
    return cn;
}

static ADDR find_leaf(ADDR *t, struct b_node **pp, struct b_hdr *hp,
						char **a, int *p)
{
    ADDR ti, *b, *i;

    b = (ADDR *) (*a + hp->klen);
    if (!(*pp)->isnode)
    {
		*p = (*a - (*pp)->kdat) /
            (hp->klen + sizeof(ADDR)) + 1;
        return(*b);
    }
    *p = 0;
    i = b;
    release_node(b_fd, *t, 0);
    *t = *i;
    *pp = (struct b_node *) get_node(b_fd, *t);
    *a = (*pp)->kdat;
    while ((*pp)->isnode)
    {
        ti = *t;
        *t = (*pp)->faddr;
        release_node(b_fd, ti, 0);
        *pp = (struct b_node *) get_node(b_fd, *t);
        *a = (*pp)->kdat;
    }
    return (*pp)->faddr;
}


#pragma startfhdr
/******************************************************************

	Functie			:	#include <toolset.h>
						int delete_key(int tree, const char *x,
							ADDR ad);

	Gebruik         :	Wissen van een sleutel uit een index.

	Prototype in    :	btree.h

	Beschrijving    :	delete_key probeert de entry met waarde
						x uit index tree te wissen. Als controle
						moet het juiste adres meegegeven worden.
						De entry wordt pas gewist als beide gegevens
						kloppen.

	Terugkeerwaarde :   OK bij succes, ERROR bij een fout.

*******************************************************************/
#pragma endhdr

int delete_key(int tree, const char *x, ADDR ad)
{
    struct b_node *pp, *qp, *yp;
	int rt_len;
	int comb;
    ADDR p, adr, q, *b, q1, y, z;
    ADDR get_addr(), next_key();
    struct b_hdr *hp;
    char *a;

    if (tree >= MAXNDX || fds[tree] == 0)
        return ERROR;
    b_fd = fds[tree];
    hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
    p = hp->root;

    /* First, locate the key, address to delete */

    if (!p)
    {
        release_node(b_fd, (ADDR) 1, 0);
        return OK;
    }
    pp = (struct b_node *) get_node(b_fd, p);
    if (!search_tree(&p, &pp, hp, x, &a))
    {
        release_node(b_fd, (ADDR) 1, 0);
        release_node(b_fd, p, 0);
        return OK;
    }
    /* key is on file, addr must match */
    adr = get_addr(p, &pp, hp, a);
    while (adr != ad)
    {
        adr = next_key(&p, &pp, hp, &a);
        if (keycmp(a, x, hp->klen))
        {
            release_node(b_fd, (ADDR) 1, 0);
            release_node(b_fd, p, 0);
            return OK;
        }
    }

    /* p, pp, a, all point to a matching key.
       now get down to the leaf if this isn't one */

    if (pp->isnode)
    {
        b = (ADDR *) (a + hp->klen);
        q = *b;
        qp = (struct b_node *) get_node(b_fd, q);
        while (qp->isnode)
        {
            q1 = qp->faddr;
            release_node(b_fd, q, 0);
            q = q1;
            qp = (struct b_node *) get_node(b_fd, q);
        }
        movmem(qp->kdat, a, hp->klen);
        release_node(b_fd, p, 1);
        p = q;
        a = qp->kdat;
        b = (ADDR *) (a + hp->klen);
        qp->faddr = *b;
        pp = qp;
    }
    b_nbr[tree] = p;
	k_ptr[tree] = (a - pp->kdat) / (hp->klen + sizeof(ADDR));

    /* Delete the key from the leaf */

	rt_len = (pp->kdat + ((hp->m - 1) * (hp->klen + sizeof(ADDR)))) - a;
    movmem(a + (hp->klen + sizeof(ADDR)), a, rt_len);
    setmem(a + rt_len, hp->klen + sizeof(ADDR), '\0');
    pp->nkeys--;
    if (k_ptr[tree] > pp->nkeys)
    {
        if (pp->r_bro)
        {
            b_nbr[tree] = pp->r_bro;
            k_ptr[tree] = 0;
        }
        else
            k_ptr[tree]--;
    }

    /* If the node is under-populated, adjust for that */

    while (pp->nkeys <= (hp->m-1) / 2 && p != hp->root)
    {
        comb = FALSE;
        z = pp->bu.prnt;
        if (pp->r_bro)
        {
            y = pp->r_bro;
            yp = (struct b_node *) get_node(b_fd, y);
            if (yp->nkeys + pp->nkeys < hp->m-1 && yp->bu.prnt == z)
            {
                comb = TRUE;
                combine_nodes(tree, pp, yp, hp);
            }
            else
                release_node(b_fd, y, 0);
        }
        if (!comb && pp->l_bro)
        {
            y = pp->l_bro;
            yp = (struct b_node *) get_node(b_fd, y);
            if (yp->bu.prnt == z)
            {
                if (yp->nkeys + pp->nkeys < hp->m-1)
                {
                    comb = TRUE;
                    combine_nodes(tree, yp, pp, hp);
                }
                else
                {
                    even_dist(tree, yp, pp, hp);
                    release_node(b_fd, p, 1);
                    release_node(b_fd, y, 1);
                    release_node(b_fd, (ADDR) 1, 0);
                    return OK;
                }
            }
        }
        if (!comb)
        {
            y = pp->r_bro;
            yp = (struct b_node *) get_node(b_fd, y);
            even_dist(tree, pp, yp, hp);
            release_node(b_fd, y, 1);
            release_node(b_fd, p, 1);
            release_node(b_fd, (ADDR) 1, 0);
            return OK;
        }
        p = z;
        pp = (struct b_node *) get_node(b_fd, p);
    }
    if (!pp->nkeys)
    {
        hp->root = pp->faddr;
        pp->isnode = 0;
        pp->faddr = 0;
        pp->bu.dlptr = hp->nxt_free;
        hp->nxt_free = p;
    }
    if (!hp->root)
        hp->rmost = hp->lmost = 0;
    release_node(b_fd, p, 1);
    release_node(b_fd, (ADDR) 1, 1);
    return OK;
}

static void combine_nodes(int tree, struct b_node *left,
                struct b_node *right, struct b_hdr *hp)
{
    ADDR lf, rt, p;
	int lf_len;
	int rt_len;
    char *a;
    ADDR *b;
    struct b_node *par;
    ADDR c;
    char *j;

    lf = right->l_bro;
    rt = left->r_bro;

    /* Find the parent of these siblings */
    p = left->bu.prnt;
    parent(lf, p, &par, &j, hp);

    /* Move key from parent to end of left sibling */
    lf_len = left->nkeys * (hp->klen + sizeof(ADDR));
    a = left->kdat + lf_len;
    movmem(j, a, hp->klen);
    setmem(j, hp->klen + sizeof(ADDR), '\0');

    /* Move keys from right sibling to left */
    b = (ADDR *) (a + hp->klen);
    *b = right->faddr;
    rt_len = right->nkeys * (hp->klen + sizeof(ADDR));
    a = (char *) (b + 1);
    movmem(right->kdat, a, rt_len);

    /* Point lower nodes to their new parent */
    if (left->isnode)
        reparent(b, right->nkeys + 1, lf, hp);

    /* If global key pointers -> to the right sibling, change -> to left */
    if (b_nbr[tree] == left->r_bro)
    {
        b_nbr[tree] = right->l_bro;
        k_ptr[tree] += left->nkeys + 1;
    }

    /* Update control values in left sibling node */
    left->nkeys += right->nkeys + 1;
    c = hp->nxt_free;
    hp->nxt_free = left->r_bro;
    if (hp->rmost == left->r_bro)
        hp->rmost = right->l_bro;
    left->r_bro = right->r_bro;
    setmem(right, NODELEN, '\0');
    right->bu.dlptr = c;

    /* Point the deleted node's right brother to this left brother */
    if (left->r_bro)
    {
        right = (struct b_node *) get_node(b_fd, left->r_bro);
        right->l_bro = lf;
        release_node(b_fd, left->r_bro, 1);
    }

    /* Remove key from parent node */
    par->nkeys--;
    if (!par->nkeys)
        left->bu.prnt = 0;
    else
    {
		rt_len = par->kdat + (par->nkeys *
			(hp->klen + sizeof(ADDR))) - j;
        movmem(j + hp->klen + sizeof(ADDR), j, rt_len);
    }
    release_node(b_fd, lf, 1);
    release_node(b_fd, rt, 1);
    release_node(b_fd, p, 1);
}

#pragma startfhdr
/******************************************************************

	Functie			:	#include <toolset.h>
						int insert_key(int tree, const char *x,
							ADDR ad, int unique);

	Gebruik         :	Toevoegen van een verwijzing.

	Prototype in    :	btree.h

	Beschrijving    :   Met deze functie kunnen er verwijzingen aan
						index tree worden toegevoegd. De verwijzing
						krijgt een sleutel-waarde x en een adres-
						verwijzing ad. Indien unique gelijk aan TRUE
						is, controleert de functie of de sleutel al
						bestaat alvorens hem toe te voegen.

	Terugkeerwaarde :	OK als alles goed gaat, ERROR bij ongeldige
						parameter of een dubbele sleutel bij
						unique = TRUE.

*******************************************************************/
#pragma endhdr

int insert_key(int tree, const char *x, ADDR ad, int unique)
{
    struct b_hdr *hp;
    char k[MXKEY+1], *a;
    struct b_node *pp, *yp;
    struct b_node *bp;
	int nl_flag;
	int rt_len, j;
    ADDR t, p, sv;
    ADDR *b;
    int left_shift, right_shift;

    if (tree >= MAXNDX || fds[tree] == 0)
        return ERROR;
    b_fd = fds[tree];
    hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
    p = 0;
    sv = 0;
    nl_flag = 0;
    movmem(x, k, hp->klen);
    t = hp->root;

    /* Find the place and make the insertion */
    if (t)
    {
        pp = (struct b_node *) get_node(b_fd, t);
        if (search_tree(&t, &pp, hp, k, &a))
        {
            if (unique)
            {
                release_node(b_fd, t, 0);
                release_node(b_fd, 1, 0);
                return ERROR;
            }
            else
            {
                find_leaf(&t, &pp, hp, &a, &j);
                k_ptr[tree] = j;
            }
        }
        else
        {
			k_ptr[tree] = ((a - pp->kdat) /
                    (hp->klen + sizeof(ADDR))) + 1;
        }
        b_nbr[tree] = t;
    }

    /* Insert the new key into the leaf node */
    while (t)
    {
        nl_flag = 1;
		rt_len = (pp->kdat + ((hp->m - 1) *
                (hp->klen + sizeof(ADDR)))) - a;
        movmem(a, a + hp->klen + sizeof(ADDR), rt_len);
        movmem(k, a, hp->klen);
        b = (ADDR *) (a + hp->klen);
        *b = ad;
        if (!pp->isnode)
        {
            b_nbr[tree] = t;
			k_ptr[tree] = ((a - pp->kdat) /
                        (hp->klen + sizeof(ADDR))) + 1;
        }
        pp->nkeys++;
        if (pp->nkeys < hp->m)
        {
            release_node(b_fd, t, 1);
            release_node(b_fd, (ADDR) 1, 1);
            return OK;
        }

        /* Redistribute the keys across two siblings */
        left_shift = right_shift = 0;
        if (pp->l_bro)
        {
            yp = (struct b_node *) get_node(b_fd, pp->l_bro);
            if (yp->nkeys < hp->m - 1 && yp->bu.prnt == pp->bu.prnt)
            {
                left_shift = 1;
                even_dist(tree, yp, pp, hp);
            }
            release_node(b_fd, pp->l_bro, left_shift);
        }
        if (!left_shift && pp->r_bro)
        {
            yp = (struct b_node *) get_node(b_fd, pp->r_bro);
            if (yp->nkeys < hp->m - 1 && yp->bu.prnt == pp->bu.prnt)
            {
                right_shift = 1;
                even_dist(tree, pp, yp, hp);
            }
            release_node(b_fd, pp->r_bro, right_shift);
        }
        if (left_shift || right_shift)
        {
            release_node(b_fd, t, 1);
            release_node(b_fd, (ADDR) 1, 1);
            return OK;
        }
        p = new_bnode(hp);

        /* Split the node into two nodes */
        if ((bp = (struct b_node *) malloc(NODELEN)) == NULL)
            fatal(9);
        setmem(bp, NODELEN, '\0');
        pp->nkeys = hp->m / 2;
        b = (ADDR *) (pp->kdat + (((hp->m / 2) + 1) *
            (hp->klen + sizeof(ADDR))) - sizeof(ADDR));
        bp->faddr = *b;
        bp->nkeys = hp->m - ((hp->m / 2) + 1);
        rt_len = (((hp->m + 1) / 2) - 1) *
                    (hp->klen + sizeof(ADDR));
        a = (char *) (b + 1);
        movmem(a, bp->kdat, rt_len);
        bp->r_bro = pp->r_bro;
        pp->r_bro = p;
        bp->l_bro = t;
        bp->isnode = pp->isnode;
        a -= hp->klen + sizeof(ADDR);
        movmem(a, k, hp->klen);
        setmem(a, rt_len + hp->klen + sizeof(ADDR), '\0');
        if (hp->rmost == t)
            hp->rmost = p;
        if (t == b_nbr[tree] && k_ptr[tree] > pp->nkeys)
        {
            b_nbr[tree] = p;
            k_ptr[tree] -= pp->nkeys + 1;
        }
        ad = p;
        sv = t;
        t = pp->bu.prnt;
        if (t)
            bp->bu.prnt = t;
        else
        {
            p = new_bnode(hp);
            pp->bu.prnt = p;
            bp->bu.prnt = p;
        }
        put_node(b_fd, ad, bp);

        /* If the pre-split node had a right brother, that node must
            now point to the new node as its left brother */
        if (bp->r_bro)
        {
            yp = (struct b_node *) get_node(b_fd, bp->r_bro);
            yp->l_bro = ad;
            release_node(b_fd, bp->r_bro, 1);
        }

        /* If this is not a leaf, point the children to this parent */
        if (bp->isnode)
            reparent(&bp->faddr, bp->nkeys + 1, ad, hp);
        release_node(b_fd, sv, 1);
        if (t)
        {
            pp = (struct b_node *) get_node(b_fd, t);
            a = pp->kdat;
            b = &pp->faddr;
            while (*b != bp->l_bro)
            {
                a += hp->klen + sizeof(ADDR);
                b = (ADDR *) (a - sizeof(ADDR));
            }
        }
        free(bp);
    }

    /* Create a new root */
    if (!p)
        p = new_bnode(hp);
    if ((bp = (struct b_node *) malloc(NODELEN)) == NULL)
        fatal(9);
    setmem(bp, NODELEN, '\0');
    bp->isnode = nl_flag;
    bp->bu.prnt = 0;
    bp->r_bro = 0;
    bp->l_bro = 0;
    bp->nkeys = 1;
    bp->faddr = sv;
    a = bp->kdat + hp->klen;
    b = (ADDR *) a;
    *b = ad;
    movmem(k, bp->kdat, hp->klen);
    put_node(b_fd, p, bp);
    free(bp);
    hp->root = p;
    if (!nl_flag)
    {
        hp->rmost = p;
        hp->lmost = p;
        b_nbr[tree] = p;
        k_ptr[tree] = 1;
    }
    release_node(b_fd, (ADDR) 1, 1);
    return(OK);
}

static void even_dist(int tree, struct b_node *left,
        struct b_node *right, struct b_hdr *hp)
{
    int n1, n2, len;
    ADDR z;
    char *c, *d, *e;
    struct b_node *zp;

    n1 = (left->nkeys + right->nkeys) / 2;
    if (n1 == left->nkeys)
        return;
    n2 = (left->nkeys + right->nkeys) - n1;
    z = left->bu.prnt;
    parent(right->l_bro, z, &zp, &c, hp);
    if (left->nkeys < right->nkeys)
    {
        d = left->kdat + (left->nkeys * (hp->klen + sizeof(ADDR)));
        movmem(c, d, hp->klen);
        d += hp->klen;
        e = right->kdat - sizeof(ADDR);
        len = ((right->nkeys - n2 - 1) *
            (hp->klen + sizeof(ADDR))) + sizeof(ADDR);
        movmem(e,d,len);
        if (left->isnode)
            reparent((ADDR *) d, right->nkeys - n2, right->l_bro, hp);
        e += len;
        movmem(e, c, hp->klen);
        e += hp->klen;
        d = right->kdat - sizeof(ADDR);
        len = (n2 * (hp->klen + sizeof(ADDR))) + sizeof(ADDR);
        movmem(e, d, len);
		setmem(d + len, e - d, '\0');
        if (!right->isnode && left->r_bro == b_nbr[tree])
            if (k_ptr[tree] < right->nkeys - n2)
            {
                b_nbr[tree] = right->l_bro;
                k_ptr[tree] += n1 + 1;
            }
            else
                k_ptr[tree] -= right->nkeys - n2;
    }
    else
    {
        e = right->kdat + ((n2 - right->nkeys) *
            (hp->klen + sizeof(ADDR))) - sizeof(ADDR);
        movmem(right->kdat - sizeof(ADDR), e, (right->nkeys *
            (hp->klen + sizeof(ADDR))) + sizeof(ADDR));
        e -= hp->klen;
        movmem(c, e, hp->klen);
        d = left->kdat + (n1 * (hp->klen + sizeof(ADDR)));
        movmem(d, c, hp->klen);
        setmem(d, hp->klen, '\0');
        d += hp->klen;
        len = ((left->nkeys - n1 - 1) *
            (hp->klen + sizeof(ADDR))) + sizeof(ADDR);
        movmem(d, right->kdat - sizeof(ADDR), len);
        setmem(d, len, '\0');
        if (right->isnode)
            reparent((ADDR *) right->kdat - sizeof(ADDR),
                left->nkeys - n1, left->r_bro, hp);
        if (!left->isnode)
            if (right->l_bro == b_nbr[tree] &&
                k_ptr[tree] > n1)
            {
                b_nbr[tree] = left->r_bro;
                k_ptr[tree] -= n1 + 1;
            }
            else if (left->r_bro == b_nbr[tree])
                k_ptr[tree] += left->nkeys - n1;
    }
    right->nkeys = n2;
    left->nkeys = n1;
    release_node(b_fd, z, 1);
}

static void reparent(ADDR *ad, int kct, ADDR newp, struct b_hdr *hp)
{
    char *cp;
    struct b_node *tmp;

    while (kct--)
    {
        tmp = (struct b_node *) get_node(b_fd, *ad);
        tmp->bu.prnt = newp;
        release_node(b_fd, *ad, 1);
        cp = (char *) ad;
        cp += (hp->klen + sizeof(ADDR));
        ad = (ADDR *) cp;
    }
}

static ADDR new_bnode(struct b_hdr *hp)
{
    ADDR p;
    struct b_node *pp;

    if (hp->nxt_free)
    {
        p = hp->nxt_free;
        pp = (struct b_node *) get_node(b_fd, p);
        hp->nxt_free = pp->bu.dlptr;
        release_node(b_fd, p, 0);
    }
    else
        p = hp->nxt_avail++;
    return p;
}

#pragma startfhdr
/******************************************************************

	Functie			:   #include <toolbox.h>
						ADDR get_next(int tree);
						ADDR get_prev(int tree);
						ADDR find_first(int tree);
						ADDR find_last(int tree);

	Gebruik         :	Sequentieel doorlopen van een index.

	Prototype in    :	btree.h

	Beschrijving    :	get_next en get_prev geven het adres terug
						bij de hoger resp. lager sleutel in index
						tree. Indien de interne record-pointer nog
						niet via een aanroep van find_key was inge-
						steld, wordt de eerste, dan wel laatste
						verwijzing teruggegeven.

						find_first en find_last geven de verwijzing
						bij de eerste, resp. de laatste sleutel in
						de index terug.

	Terugkeerwaarde :	Alle vier de functies geven de betreffende
						adresverwijzing terug, en 0 in geval van
						een lege index of een ongeldige handle.

*******************************************************************/
#pragma endhdr

ADDR get_next(int tree)
{
    struct b_node *pp;
    struct b_hdr *hp;
    ADDR *a, b, f;

    b_fd = fds[tree];
    b = b_nbr[tree];
    if (!b)
        return find_first(tree);

    pp = (struct b_node *) get_node(b_fd, b);
    hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
    if (k_ptr[tree] == pp->nkeys)
    {
        if (!pp->r_bro)
        {
            release_node(b_fd, b, 0);
            release_node(b_fd, (ADDR) 1, 0);
            return (ADDR) 0;
        }
        b_nbr[tree] = pp->r_bro;
        k_ptr[tree] = 0;
        release_node(b_fd, b, 0);
        b = b_nbr[tree];
        pp = (struct b_node *) get_node(b_fd, b);
    }
    else
        k_ptr[tree]++;
    a = (ADDR *) (pp->kdat + (k_ptr[tree] *
            (hp->klen + sizeof(ADDR))) - sizeof(ADDR));
    f = *a;
    release_node(b_fd, b, 0);
    release_node(b_fd, (ADDR) 1, 0);
    return f;
}

#pragma startfhdr
/******************************************************************

	Functie			:	#include <toolset.h>
						ADDR get_prev(int tree);

	Gebruik         :	Sequentieel doorlopen van een index.

	Prototype in    :	btree.h

	Beschrijving    :	zie get_next.

*******************************************************************/
#pragma endhdr

ADDR get_prev(int tree)
{
    struct b_node *pp;
    struct b_hdr *hp;
	ADDR *a, b, f;

    b_fd = fds[tree];
    b = b_nbr[tree];
    if (!b)
        return find_last(tree);
    pp = (struct b_node *) get_node(b_fd, b);
    hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
    if (!k_ptr[tree])
    {
        if (!pp->l_bro)
        {
            release_node(b_fd, b, 0);
			release_node(b_fd, (ADDR) 1, 0);
            return (ADDR) 0;
        }
        b_nbr[tree] = pp->l_bro;
        release_node(b_fd, b, 0);
        b = b_nbr[tree];
        pp = (struct b_node *) get_node(b_fd, b);
        k_ptr[tree] = pp->nkeys;
    }
    else
        k_ptr[tree]--;
    a = (ADDR *) (pp->kdat + (k_ptr[tree] *
        (hp->klen + sizeof(ADDR))) - sizeof(ADDR));
	f = *a;
    release_node(b_fd, b, 0);
    release_node(b_fd, (ADDR) 1, 0);
    return f;
}

#pragma startfhdr
/******************************************************************

	Functie			:	#include <toolset.h>
						ADDR find_first(int tree);

	Gebruik         :	Sequentieel doorlopen van een index.

	Prototype in    :	btree.h

	Beschrijving    :	zie get_next.

*******************************************************************/
#pragma endhdr

ADDR find_first(int tree)
{
    ADDR b, *c, p;
    struct b_node *pp;
    struct b_hdr *hp;

    b_fd = fds[tree];
    hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
    p = hp->lmost;
    if (!p)
    {
        release_node(b_fd, (ADDR) 1, 0);
        return (ADDR) 0;
    }
    pp = (struct b_node *) get_node(b_fd, p);
    c = (ADDR *) (pp->kdat + hp->klen);
    b = *c;
    b_nbr[tree] = p;
	k_ptr[tree] = 1;
    release_node(b_fd, p, 0);
    release_node(b_fd, (ADDR) 1, 0);
    return b;
}

#pragma startfhdr
/******************************************************************

	Functie			:	#include <toolset.h>
						ADDR find_last(int tree);

	Gebruik         :	Sequentieel doorlopen van een index.

	Prototype in    :	btree.h

	Beschrijving    :	zie get_next.

*******************************************************************/
#pragma endhdr

ADDR find_last(int tree)
{
    ADDR b, *c, p;
    struct b_node *pp;
    struct b_hdr *hp;

    b_fd = fds[tree];
    hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
    p = hp->rmost;
    if (!p)
    {
        release_node(b_fd, (ADDR) 1, 0);
        return (ADDR) 0;
    }
    pp = (struct b_node *) get_node(b_fd, p);
    c = (ADDR *) (pp->kdat + (pp->nkeys *
            (hp->klen + sizeof(ADDR))) - sizeof(ADDR));
    b = *c;
	b_nbr[tree] = p;
    k_ptr[tree] = pp->nkeys;
    release_node(b_fd, p, 0);
    release_node(b_fd, (ADDR) 1, 0);
    return b;
}


static ADDR next_key(ADDR *p, struct b_node **pp,
            struct b_hdr *hp, char **a)
{
    ADDR cn;
    ADDR *b;

    if ((*pp)->isnode)     /* Als het een node is: */
    {
        cn = *p;                       /* current node = p */
        b = (ADDR *) (*a + hp->klen);  /* b -> volgende key */
        *p = *b;                        /* p = volgende key */
        release_node(b_fd, cn, 0);      /* geef huidige node vrij */
        *pp = (struct b_node *) get_node(b_fd, *p); /* haal child op */
        while ((*pp)->isnode)           /* zolang het een node is */
        {
            cn = *p;                    /* haal child op */
            *p = (*pp)->faddr;
            release_node(b_fd, cn, 0);
			*pp = (struct b_node *) get_node(b_fd, *p);
        }
        *a = (*pp)->kdat;               /* a -> eerste key in leaf */
        b = (ADDR *) (*a + hp->klen);   /* b = bijbehorend address */
        return *b;
    }
    *a += hp->klen + sizeof(ADDR);      /* geen node: a wordt volgende key */
    while (-1)
    {
        if (((*pp)->kdat + ((*pp)->nkeys)   /* als a niet de laatste key is :*/
                * (hp->klen + sizeof(ADDR))) != *a)
            return get_addr(*p, pp, hp, *a);       /* haal address op */
        if (!(*pp)->bu.prnt || !(*pp)->r_bro) /* verder niks te zoeken: */
			return (ADDR) 0;                    /* stuur nul terug */
		cn = *p;                                /* ga terug naar ouder */
        *p = (*pp)->bu.prnt;
        release_node(b_fd, cn, 0);
        *pp = (struct b_node *) get_node(b_fd, *p); /* haal ouder op */
        *a = (*pp)->kdat;                      /* a -> eerste keyval */
        b = (ADDR *) (*a - sizeof(ADDR));      /* b -> bijbeh. adres */
        while (*b != cn)                        /* loop sleutels af ?*/
        {
            *a += hp->klen + sizeof(ADDR);
            b = (ADDR *) (*a - sizeof(ADDR));
        }
	}
	/* Om warning te onderdrukken */
	/* return (ADDR) 0; */
}

static ADDR prev_key(ADDR *p, struct b_node **pp,
            struct b_hdr *hp, char **a)
{
    ADDR cn;
    ADDR *b;

    if ((*pp)->isnode)
    {
        cn = *p;
		b = (ADDR *) (*a - sizeof(ADDR));
        *p = *b;
        release_node(b_fd, cn, 0);
        *pp = (struct b_node *) get_node(b_fd, *p);
        while ((*pp)->isnode)
        {
            cn = *p;
            b = (ADDR *) (*pp)->kdat + (((*pp)->nkeys *
                (hp->klen + sizeof(ADDR))) - sizeof(ADDR));
            *p = *b;
            release_node(b_fd, cn, 0);
            *pp = (struct b_node *) get_node(b_fd, *p);
        }
		*a = (*pp)->kdat + (((*pp)->nkeys - 1) *
                (hp->klen + sizeof(ADDR)));
        b = (ADDR *) (*a + hp->klen);
        return *b;
    }
    /* Current node is a leaf */
    *a -= (hp->klen + sizeof(ADDR));
    while (TRUE)
    {
        if (*a >= (*pp)->kdat)
            return get_addr(*p, pp, hp, *a);
        if (!(*pp)->bu.prnt || !(*pp)->l_bro)
            return (ADDR) 0;
		cn = *p;
        *p = (*pp)->bu.prnt;
        release_node(b_fd, cn, 0);
        *pp = (struct b_node *) get_node(b_fd, *p);
        *a = (*pp)->kdat + (((*pp)->nkeys - 1) *
                (hp->klen + sizeof(ADDR)));
        b = (ADDR *) (*a + hp->klen);
        while (*b != cn)
        {
            *a -= (hp->klen + sizeof(ADDR));
            b = (ADDR *) (*a + hp->klen);
        }
	}
}


static void parent(ADDR left, ADDR parent, struct b_node **pp,
                char **c, struct b_hdr *hp)
{
    ADDR *b;

    *pp = (struct b_node *) get_node(b_fd, parent);
    *c = (*pp)->kdat;
    b = (ADDR *) (*c - sizeof(ADDR));
    while (*b != left)
    {
        *c += (hp->klen + sizeof(ADDR));
        b = (ADDR *) (*c - sizeof(ADDR));
    }

}

#pragma startfhdr
/******************************************************************

	Functie			:	void currkey(int tree, char *ky);

	Gebruik         :	Opvragen huidige sleutel.

	Prototype in    :	btree.h

	Beschrijving    :	currkey geeft via key de huidige sleutelwaarde
						terug van index tree. Dit is handig om
						sequentieel door een bestand heen te wandelen
						zonder steeds de records op te moeten halen om
						de sleutelwaarde te verkrijgen.

	Terugkeerwaarde :   geen.

*******************************************************************/
#pragma endhdr

void currkey(int tree, char *ky)
{
    struct b_node *pp;
    struct b_hdr *hp;
    ADDR b, p, *a;
    char *k;
    int i;

    b_fd = fds[tree];
    b = b_nbr[tree];
    if (b)
    {
        hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
        pp = (struct b_node *) get_node(b_fd, b);
        i = k_ptr[tree];
        k = pp->kdat + ((i - 1) * (hp->klen + sizeof(ADDR)));
        while (i == 0)
        {
            p = b;
            b = pp->bu.prnt;
            release_node(b_fd, p, 0);
            pp = (struct b_node *) get_node(b_fd, b);
            for (; i <= pp->nkeys; i++) {
                k = pp->kdat + ((i - 1) * (hp->klen + sizeof(ADDR)));
                a = (ADDR *) (k + hp->klen);
                if (*a == p)
                    break;
            }
        }
        movmem(k, ky, hp->klen);
        release_node(b_fd, b, 0);
        release_node(b_fd, (ADDR) 1, 0);
    }
}

#pragma startfhdr
/******************************************************************

	Functie			:	#include <toolset.h>
						ADDR get_curr(int tree);

	Gebruik         :	Ophalen huidige adres.

	Prototype in    :	btree.h

	Beschrijving    :	get_curr geeft het adres van de huidige
						indexverwijzing in index tree.

	Terugkeerwaarde :	Het adres of 0 als de index leeg of nog
						niet gebruikt is.

*******************************************************************/
#pragma endhdr

ADDR get_curr(int tree)
{
    struct b_node *pp;
    struct b_hdr *hp;
    ADDR *a, b, f = (ADDR) 0;

    b_fd = fds[tree];
    b = b_nbr[tree];
    if (b)
    {
        pp = (struct b_node *) get_node(b_fd, b);
        hp = (struct b_hdr *) get_node(b_fd, (ADDR) 1);
        a = (ADDR *) (pp->kdat + (k_ptr[tree] *
            (hp->klen + sizeof (ADDR))) - sizeof(ADDR));
        f = *a;
        release_node(b_fd, b, 0);
        release_node(b_fd, (ADDR) 1, 0);
    }
    return f;
}
