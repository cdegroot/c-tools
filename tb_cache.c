#pragma startmhdr
/******************************************************************

	C Standaarddocumentatie - (c)1990 C.A. de Groot

	Project      :  TOOLBOX

	Module       :  tb_cache.c

	Beschrijving :  In deze module bevinden zich de disk-caching
					routines. Deze routines vormen een cache tussen
					het operating system en de high-level routines
					in tb_btree en tb_files.

*******************************************************************/
/*
	$Id: tb_cache.c,v 1.2 1993/02/05 13:02:42 cg Rel $
*/
#pragma endhdr

/*#define DEBUG*/

#include <io.h>
#include <stdio.h>
#include <alloc.h>
#include <fcntl.h>
#include <mem.h>

#include <toolset.h>

#define DEFNODES 64 /* 32k cache-memory */
#define FREE 0
#define INUSE 1
#define RELEASED 2
#define ln fprintf(stdprn,"%s:%d\n",__FILE__, __LINE__);

#pragma startvhdr
/******************************************************************

	Variabele               :       extern int _mxnodes;

	Gebruik         :       Grootte van de cache/

	Declaratie in   :       cache.h

	Beschrijving    :       De variabele _mxnodes geeft het aantal
						blokken aan dat de cache maximaal kan
						bevatten. De gebruiker kan deze waarde
						instellen voor een aanroep van init_cache.

*******************************************************************/
#pragma endhdr
int _mxnodes = DEFNODES;

/* Function in SUBS to ask for extended handles */
void _extend_handles(void);

/* Cache nodes */
static struct cnode
{
    struct cnode *nxt_out;      /* pointer to next node in list */
    struct cnode *prev_out;     /* pointer to prev. node in list */
    int nd_chgd;                /* 0 = data not changed, 1 = changed */
    void *n_ptr;                /* data space of node */
    ADDR nd_sector;             /* relative sector number */
    int nd_fd;                  /* fd of file */
} *nodes_in;

/* Cache linked list heads */
static struct
{
    struct cnode *first_out;    /* pointer to first node on list */
    struct cnode *last_out;     /* pointer to last node on list */
} list_head[3];                 /* three lists, see defines */

static void add_node(int list, struct cnode *n_node);
static struct cnode *lru(int list);
static struct cnode *new_node(int list, int fd, ADDR t);
static void add_node(int list, struct cnode *node);
static void remove_node(int list, struct cnode *node);
static void remove_node(int list_no, struct cnode *n_node);

static int cache_init = FALSE;
/* static var, to protect from changing on the way */
static int no_nodes;

#pragma startfhdr
/******************************************************************

	Functie                 :       void init_cache(void);

	Gebruik         :       Initialiseren cache-routines.

	Prototype in    :       cache.h

	Beschrijving    :       Deze functie alloceert geheugen voor de
						cache-buffer en initialiseert de geketende
						lijsten voor de cache-administratie. Indien
						er een fout optreedt, wordt het programma
						via een aanroep van fatal verlaten.

	Terugkeerwaarde :       Geen.

*******************************************************************/
#pragma endhdr
#ifdef DEBUG
#include <stdio.h>
FILE *stdlog;
#endif


void init_cache(void)
{
    register int i;

    if (cache_init)
		return;
#ifdef DEBUG
	stdlog = fopen("vdb.log", "a");
#endif
    cache_init = TRUE;

    /* copy number of nodes to local storage */
    no_nodes = _mxnodes;

    /* Ask DOS for extended handles */
    _extend_handles();

    /* allocate memory for nodes */
	nodes_in = calloc(no_nodes,sizeof(*nodes_in));

    /* build the linked list listhead */

    list_head[FREE].first_out = &nodes_in[0];
    list_head[FREE].last_out  = &nodes_in[no_nodes-1];
    for (i = 1; i < 3; i++)
    {
	list_head[i].first_out = NULL;
	list_head[i].last_out = NULL;
    }

    /* initialize the nodes in the linked list */

    for (i = 0; i < no_nodes; i++)
    {
	nodes_in[i].nxt_out = &nodes_in[i+1];
	nodes_in[i].prev_out = i ? &nodes_in[i-1] : NULL;
	nodes_in[i].nd_chgd = FALSE;
		if ((nodes_in[i].n_ptr = malloc(NODELEN)) == NULL)
	    fatal(1);
	nodes_in[i].nd_sector = 0;
	nodes_in[i].nd_fd = 0;
	setmem(nodes_in[i].n_ptr,NODELEN,'\0');
    }
    nodes_in[no_nodes-1].nxt_out = NULL;
    /* binairy files only */
    _fmode = O_BINARY;
}

#pragma startfhdr
/******************************************************************

	Functie                 :       #include <toolset.h>
						void *get_node(int fd, ADDR p);

	Gebruik         :       ophalen cache-node.

	Prototype in    :       cache.h

	Beschrijving    :       get_node kijkt of de gevraagde node in het
						geheugen zit, leest het blok eventueel in,
						en geeft een pointer naar het betreffende
						blok terug. fd is de DOS file-handle van het
						betreffende bestand, en p is het adres
						van het benodigde blok (1-relatief).
						Indien het inlezen van een blok mislukt,
						treedt er een fatale fout op.

	Terugkeerwaarde :       Pointer in het cache-geheugen.

*******************************************************************/
#pragma endhdr

void *get_node(int fd,ADDR p)
{
    long sect;
    register struct cnode *cn;

    /* search released nodes for a match on the requested node */

    cn = list_head[RELEASED].first_out;
    while (cn != NULL)
    {
	if (cn->nd_sector == p && cn->nd_fd == fd)
	{
	    remove_node(RELEASED,cn);
	    add_node(INUSE,cn);
	    return(cn->n_ptr);
	}
	cn = cn->nxt_out;
    }

    /* get a node from the free list or get a released one */

    cn = new_node(INUSE, fd, p);

    /* read in the data for the node */

    sect = p - 1;
    sect *= NODELEN;
    if (lseek(fd, sect, SEEK_SET) == ERROR)
	{
		setmem(cn->n_ptr, NODELEN, '\0');
	}
    else if (_read(fd, cn->n_ptr, NODELEN) == ERROR)
	fatal(2);
    cn->nd_chgd = FALSE;
    return(cn->n_ptr);
}

#pragma startfhdr
/******************************************************************

	Functie                 :   #include <toolset.h>
						void put_node(int fd, ADDR t, void *buff);

	Gebruik         :       Terugschrijven van een blok.

	Prototype in    :       cache.h

	Beschrijving    :       put_node wordt gebruikt om een door de
						aanroepende functie opgesteld blok naar
						het bestand fd op adres t te schrijven.
						buff verwijst naar het blok dat bij de
						node behoort. Gebruik voor via de cache
						opgehaalde nodes release_node.

	Terugkeerwaarde :       Geen.

*******************************************************************/
#pragma endhdr

void put_node(int fd, ADDR t, void *buff)
{
    struct cnode *cn;

    /* search released nodes for a match on the requested node */

    cn = list_head[RELEASED].first_out;
    while (cn != NULL)
    {
	if (cn->nd_sector == t && cn->nd_fd == fd)
	    break;
	cn = cn->nxt_out;
    }

    /* if the node is not in the released list, appropiate a new node */

    if (cn == NULL)
	cn = new_node(RELEASED, fd, t);
    cn->nd_chgd = TRUE;
    movmem(buff, cn->n_ptr, NODELEN);
}

static struct cnode *new_node(int list, int fd, ADDR t)
{
    struct cnode *cn;

    if ((cn = list_head[FREE].first_out) != NULL)
    {
	remove_node(FREE,cn);
	add_node(list,cn);
    }
    else
	cn = lru(list);
    cn->nd_sector = t;
    cn->nd_fd = fd;
    return(cn);
}

#pragma startfhdr
/******************************************************************

	Functie                 :       #include <toolset.h>
						void release_node(int fd, ADDR p, int chgd);

	Gebruik         :       Vrijgeven van een cache-node.

	Prototype in    :       cache.h

	Beschrijving    :       Release node moet aangeroepen worden als men
						klaar is men een blok dat via get_node is
						verkregen. chgd is een flag (TRUE of FALSE)
						die aangeeft of de node door de gebruiker
						gewijzigd is. Indien chgd TRUE is, zal de
						node teruggeschreven worden voordat de ruimte
						hergebruikt wordt.

	Terugkeerwaarde :       Geen.

*******************************************************************/
#pragma endhdr

void release_node(int fd, ADDR p, int chgd)
{
    struct cnode *cn;

    cn = list_head[INUSE].first_out;
    while (cn != NULL)
    {
	if (cn->nd_sector == p && cn->nd_fd == fd)
	{
	    cn->nd_chgd |= chgd;
	    remove_node(INUSE,cn);
	    add_node(RELEASED,cn);
	    return;
	}
	cn = cn->nxt_out;
    }
}

#pragma startfhdr
/******************************************************************

	Functie                 :       void flush_cache(int fd);

	Gebruik         :       Vrijgeven van cache-ruimte

	Prototype in    :       cache.h

	Beschrijving    :       flush_cache forceert het schrijven van alle
						bij fd behorende cache-buffers naar het
						bestand. Indien fd nul is, dan worden alle
						buffers van alle bestanden geschreven, en
						wordt het cache-gegeugen vrijgegeven. Al-
						vorens de cache weer te gebruiken, is dan
						een aanroep van init_cache nodig.

	Terugkeerwaarde :       Geen.

*******************************************************************/
#pragma endhdr

void flush_cache(int fd)
{
    struct cnode *cn, *j;
    long sect;
    int i;

    cn = list_head[RELEASED].first_out;
    while (cn != NULL)
    {
	j = cn->nxt_out;
	if (fd == cn->nd_fd || !fd)
	{
	    if (cn->nd_chgd)
	    {
		sect = cn->nd_sector-1;
		sect *= NODELEN;
#ifdef DEBUF
if (sect == 0)
	fprintf(stdlog, "Write header of file %d: %08lX %08lX\n",
		(ADDR) *(cn->n_ptr), (ADDR) *(cn->n_ptr + 4));
#endif
		lseek(fd, sect, 0);
		_write(fd,cn->n_ptr,NODELEN);
	    }
	    cn->nd_chgd = FALSE;
	    cn->nd_sector = 0;
	    cn->nd_fd = 0;
	    remove_node(RELEASED, cn);
	    add_node(FREE, cn);
	}
	cn = j;
    }
    if (!fd) {
	cache_init = FALSE;
	for (i = 0; i < no_nodes; i++)
	    free(nodes_in[i].n_ptr);
	free(nodes_in);
    }
}

static struct cnode *lru(int list)
{
    struct cnode *cn;
    long sect;

    cn = list_head[RELEASED].first_out;
    if (cn->nd_chgd)
    {
	sect = cn->nd_sector -1;
		sect *= NODELEN;
#ifdef DEBUF
if (sect == 0)
	fprintf(stdlog, "Write header of file %d: %08lX %08lX\n",
		(ADDR) *(cn->n_ptr), (ADDR) *(cn->n_ptr + 4));
#endif
	lseek(cn->nd_fd, sect, 0);
	_write(cn->nd_fd, cn->n_ptr, NODELEN);
    }
    remove_node(RELEASED, cn);
    add_node(list, cn);
    return(cn);
}

static void add_node(int list, struct cnode *n_node)
{
    if (list_head[list].first_out == NULL)
	list_head[list].first_out = n_node;
    if (list_head[list].last_out != NULL)
	list_head[list].last_out->nxt_out = n_node;
    n_node->prev_out = list_head[list].last_out;
    list_head[list].last_out = n_node;
    n_node->nxt_out = NULL;
#ifdef DEBUG
{struct cnode *cn;
fprintf(stdprn,"ADDED NODE %p to list %d\nList now:",n_node, list);
cn = list_head[list].first_out;
while (cn != NULL)      {
fprintf(stdprn,"%p ",cn); cn = cn->nxt_out;}}
#endif
}

static void remove_node(int list_no, struct cnode *n_node)
{
    if (list_head[list_no].first_out == n_node)
	list_head[list_no].first_out = n_node->nxt_out;
    else
	n_node->prev_out->nxt_out = n_node->nxt_out;
    if (list_head[list_no].last_out == n_node)
	list_head[list_no].last_out = n_node->prev_out;
    else
	n_node->nxt_out->prev_out = n_node->prev_out;
    n_node->nxt_out = NULL;
    n_node->prev_out = NULL;
#ifdef DEBUG
{struct cnode *cn;
fprintf(stdprn,"REMOVED NODE %p from list %d\nList now:",n_node, list_no);
cn = list_head[list_no].first_out;
while (cn != NULL)      {
fprintf(stdprn,"%p ",cn); cn = cn->nxt_out;}}
#endif
}

