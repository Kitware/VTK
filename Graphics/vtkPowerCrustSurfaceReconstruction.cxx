/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPowerCrustSurfaceReconstruction.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPowerCrustSurfaceReconstruction.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

vtkCxxRevisionMacro(vtkPowerCrustSurfaceReconstruction, "1.3");
vtkStandardNewMacro(vtkPowerCrustSurfaceReconstruction);

vtkPowerCrustSurfaceReconstruction::vtkPowerCrustSurfaceReconstruction()
{
    this->medial_surface = vtkPolyData::New();
}

vtkPowerCrustSurfaceReconstruction::~vtkPowerCrustSurfaceReconstruction()
{
    this->medial_surface->Delete();
}
 
//=================================================================================================

/* -----------------------------------------------------------------

  "Porting PowerCrust to VTK"
     or
  "The Things We Do for Fun These Days"

  Tim J. Hutton 28/6/2002
  T.Hutton@eastman.ucl.ac.uk

  Some history:
  Nina Amenta et al. came up with a lovely algorithm for surface
  reconstruction - the PowerCrust. With humbling generosity they made
  their code available to the world, under the GNU Public License. 
  To make it easier to use for everyone, I (with much encouragement 
  and help from many others) decided to port it to VTK. This is that story.

  The original code can be found at:

  http://www.cs.utexas.edu/users/amenta/powercrust/welcome.html

  Getting it to compile and run on my Windows box was fun. Notes on how to 
  do this can be found at the bottom of this file.

  Getting it to compile under VTK was a battle and a half. The word 'beast'
  doesn't begin to describe it. A lot of little changes were necessary, many
  of them concerned with getting rid of the use of files as temporary
  storage. More details are at the bottom of this file. At the time of writing 
  there are still a lot of problems, including memory leaks and the fact that 
  the same instance of the filter cannot be used twice for surface reconstruction.

  The powercrust code includes robust routines for computing voronoi triangularizations. 
  I wanted to make this available for other routines in VTK but was beaten back 
  by the thicket. It would take a better person than I to make this useful elsewhere. 
  It is a sign of defeat that I've munged the source files into one - it does makes 
  it neater from a VTK point of view since there is only one file.

  Known problems:
   - leaks memory like a upside-down bucket
   - cannot reuse an instance of the filter for doing surface reconstruction again
     since a lot of global variables don't get reset.

------------------------------------------------------------------- */

// these globals are here so we can access them from anywhere in the powercrust code
// if you can find a neat way to improve this then please feel free
vtkDataSet* vtk_input;
vtkPolyData* vtk_output;
vtkPolyData* vtk_medial_surface;

// some hacks to enable us to have useful error reporting
vtkPowerCrustSurfaceReconstruction *our_filter;
void ASSERT(int b,const char* message="") { 
    if(!b) 
        our_filter->Error(message); 
}
void vtkPowerCrustSurfaceReconstruction::Error(const char *message)
{
    vtkErrorMacro(<<"ASSERT:"<<message);
}

int pcFALSE = (1==0);
int pcTRUE = (1==1);


//========points.h=============================================================
/*
 * Power Crust software, by Nina Amenta, Sunghee Choi and Ravi Krishna Kolluri.
 * Copyright (c) 2000 by the University of Texas
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee under the GNU Public License is hereby granted, 
 * provided that this entire notice  is included in all copies of any software 
 * which is or includes a copy or modification of this software and in all copies 
 * of the supporting documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */


#ifndef PNTSH
#define PNTSH 1


typedef double Coord;
typedef Coord* point;
extern int  pdim;  /* point dimension */

#endif
//========pointsites.h=============================================================
/*
 * Power Crust software, by Nina Amenta, Sunghee Choi and Ravi Krishna Kolluri.
 * Copyright (c) 2000 by the University of Texas
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee under the GNU Public License is hereby granted, 
 * provided that this entire notice  is included in all copies of any software 
 * which is or includes a copy or modification of this software and in all copies 
 * of the supporting documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#ifndef PNTSTSH
#define PNTSTSH 1


#define MAXBLOCKS 10000

typedef point site;
typedef Coord* normalp;
point  site_blocks[MAXBLOCKS];
int  num_blocks;

#endif

//========stormacs.h=============================================================
/*
 * Ken Clarkson wrote this.  Copyright (c) 1995 by AT&T..
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#define max_blocks 10000
#define Nobj 10000

#define STORAGE_GLOBALS(X)    \
          \
extern size_t X##_size;      \
extern X *X##_list;      \
extern X *new_block_##X(int);    \
extern void flush_##X##_blocks(void);  \
void free_##X##_storage(void);    \


#define INCP(X,p,k) ((X*) ( (char*)p + (k) * X##_size)) /* portability? */


#define STORAGE(X)            \
                \
size_t  X##_size;            \
X  *X##_list = 0;            \
                \
X *new_block_##X(int make_blocks)        \
{  int i;              \
  static  X *X##_block_table[max_blocks];      \
  X *xlm, *xbt;          \
  static int num_##X##_blocks;        \
  if (make_blocks) {          \
    assert(num_##X##_blocks<max_blocks);    \
        DEB(0, before) DEBEXP(0, Nobj * X##_size)      \
                \
    xbt = X##_block_table[num_##X##_blocks++] =  (X*)malloc(Nobj * X##_size); \
     memset(xbt,0,Nobj * X##_size);  \
    if (!xbt) {          \
      DEBEXP(-10,num_##X##_blocks)    \
    }            \
    assert(xbt);          \
                \
    xlm = INCP(X,xbt,Nobj);        \
    for (i=0;i<Nobj; i++) {        \
      xlm = INCP(X,xlm,(-1));      \
      xlm->next = X##_list;      \
      X##_list = xlm;        \
    }            \
                \
    return X##_list;        \
  }              \
                \
  for (i=0; i<num_##X##_blocks; i++)      \
    free(X##_block_table[i]);      \
  num_##X##_blocks = 0;          \
  X##_list = 0;            \
  return 0;            \
}                \
                \
void free_##X##_storage(void) {new_block_##X(0);}    \
/*end of STORAGE*/

#define NEWL(X,p)            \
{                \
   p = X##_list ? X##_list : new_block_##X(1);    \
  assert(p);            \
   X##_list = p->next;          \
}                \



#define NEWLRC(X,p)            \
{                \
  p = X##_list ? X##_list : new_block_##X(1);    \
  assert(p);            \
  X##_list = p->next;          \
  p->ref_count = 1;          \
}                \


#define FREEL(X,p)            \
{                \
  memset((p),0,X##_size);          \
  (p)->next = X##_list;          \
  X##_list = p;            \
}                \


#define dec_ref(X,v)  {if ((v) && --(v)->ref_count == 0) FREEL(X,(v));}
#define inc_ref(X,v)  {if (v) v->ref_count++;}
#define NULLIFY(X,v)  {dec_ref(X,v); v = NULL;}



#define mod_refs(op,s)          \
{              \
  int i;            \
  neighbor *mrsn;          \
              \
  for (i=-1,mrsn=s->neigh-1;i<cdim;i++,mrsn++)  \
    op##_ref(basis_s, mrsn->basis);    \
}

#define free_simp(s)        \
{  mod_refs(dec,s);      \
  FREEL(basis_s,s->normal);    \
  FREEL(simplex, s);      \
}            \


#define copy_simp(new,s)      \
{  NEWL(simplex,new);      \
  memcpy(new,s,simplex_size);    \
  mod_refs(inc,s);      \
}            \





#if 0
STORAGE_GLOBALS(type)
    STORAGE(type)
    NEWL(type,xxx)
    FREEL(type,xxx)
    dec_ref(type,xxxx)
    inc_ref(type,xxxx)
    NULLIFY(type,xxxx)
#endif
//========hull.h=============================================================
/* hull.h */

// Some small changes made by Tim J. Hutton trying to port the code into VTK...

/*
 * Power Crust software, by Nina Amenta, Sunghee Choi and Ravi Krishna Kolluri.
 * Copyright (c) 2000 by the University of Texas
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee under the GNU Public License is hereby granted, 
 * provided that this entire notice  is included in all copies of any software 
 * which is or includes a copy or modification of this software and in all copies 
 * of the supporting documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

/* 
 * This file is a significant modification of Ken Clarkson's file hull.h
 * We include his copyright notice in accordance with its terms.
 *                                                                     - Nina, Sunghee and Ravi
 */

/*
 * Ken Clarkson wrote this.  Copyright (c) 1995 by AT&T..
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#ifndef HDR
#define HDR 1

//#include "points.h"   TJH: we've munged this file into this one
//#include "stormacs.h"  TJH: we've munged this file into this one

#define MAXDIM 8
#define BLOCKSIZE 100000
//#define MAXBLOCKS 1000 TJH: defined above
#define DEBUG -7
#define CHECK_OVERSHOOT 1
#define EXACT 1 /* sunghee */
#define NRAND 5  /* number of random points chosen to check orientation */
#define SMALL_ENOUGH .0001
#define MAXNF 100 /* max number of conv hull triangles adjacent to a vertex */
#define MAXTA 100000
#define MAXTEMPA 100000
#define CNV 0 /* sunghee : status of simplex, if it's on convex hull */
#define VV 1 /* sunghee :    if it's regular simplex  */
#define SLV -1 /* sunghee : if orient3d=0, sliver simplex */
#define AV 2 /* if av contains the averaged pole vector */
#define PSLV -2 /* probably sliver */
#define POLE_OUTPUT 3 /* VV is pole and it's ouput */
#define SQ(a) ((a)*(a)) /* sunghee */

#define BAD_POLE -1

// next two lines added by TJH to avoid name collision
#undef IN
#undef OUT

#define IN 2
#define OUT 1
#define INIT 0
#define NONE -1

#define FIRST 0
#define NO 1
#define DEG -1
#define NORM 2
#define VOR 3
#define VOR_NORM 4
#define SLVT 7
#define PSLVT 8
#define SURF 5
#define OPP 6

#define FIRST_EDGE 0
#define POW 1
#define NOT_POW 2
#define VISITED 3

/*RAVI */

#define VALIDEDGE 24 
#define INVALIDEDGE 23
#define INEDGE 25
#define OUTEDGE 26
#define ADDAXIS 13
#define PRESENT 19
#define FIXED 20
#define REMOVED 21  /* for the thinning  stuff */ 

/* for priority queue */
#define LEFT(i) ((i)*2)
#define RIGHT(i) ((i)*2+1)
#define PARENT(i) ((i)/2)


extern char tmpfilenam[L_tmpnam];

extern short check_overshoot_f;

// TJH: stripping out all file use
//FILE* efopen(char *, char *);
//void  efclose(FILE* file);



extern FILE *DFILE;

#define DEBS(qq)  {if (DEBUG>qq) {
#define EDEBS }}
#define DEBOUT DFILE
#define DEB(ll,mes)  DEBS(ll) if(DEBOUT){fprintf(DEBOUT,#mes "\n");fflush(DEBOUT);} EDEBS
#define DEBEXP(ll,exp) DEBS(ll) if(DEBOUT){fprintf(DEBOUT,#exp "=%G\n", (double) exp); fflush(DEBOUT);} EDEBS
#define DEBTR(ll) DEBS(ll) if(DEBOUT){fprintf(DEBOUT, __FILE__ " line %d \n" ,__LINE__);fflush(DEBOUT);} EDEBS
#define warning(lev, x)                     \
    {static int messcount;                  \
        if (++messcount<=10) {DEB(lev,x) DEBTR(lev)}    \
        if (messcount==10) DEB(lev, consider yourself warned) \
    }                           \


#define SBCHECK(s) /*                               \
                                                    {double Sb_check=0;                             \
                                                    int i;                                      \
                                                    for (i=1;i<cdim;i++) if (s->neigh[i].basis)             \
                                                    Sb_check+=s->neigh[i].basis->sqb;       \
                                                    if ((float)(Sb_check - s->Sb) !=0.0)                            \
                                                    {DEBTR DEB(bad Sb); DEBEXP(s->Sb) DEBEXP(Sb_check);print_simplex(s); exit(1);}}*/\




typedef point site;

extern site p;          /* the current site */

extern Coord infinity[10];  /* point at infinity for Delaunay triang */

extern int
rdim,   /* region dimension: (max) number of sites specifying region */
    cdim,   /* number of sites currently specifying region */
    site_size, /* size of malloc needed for a site */
    point_size;  /* size of malloc needed for a point */



typedef struct basis_s {
    struct basis_s *next; /* free list */
    int ref_count;  /* storage management */
    int lscale;    /* the log base 2 of total scaling of vector */
    Coord sqa, sqb; /* sums of squared norms of a part and b part */
    Coord vecs[1]; /* the actual vectors, extended by malloc'ing bigger */
} basis_s;
STORAGE_GLOBALS(basis_s)


    typedef struct neighbor {
        site vert; /* vertex of simplex */
        /*        short edgestatus[3];  FIRST_EDGE if not visited
                  NOT_POW if not dual to powercrust faces
                  POW if dual to powercrust faces */
        struct simplex *simp; /* neighbor sharing all vertices but vert */
        basis_s *basis; /* derived vectors */
    } neighbor;

typedef struct simplex {
    struct simplex *next;   /* used in free list */
    short mark;
    site vv; /* Voronoi vertex of simplex ; sunghee */
    double sqradius; /* squared radius of Voronoi ball */
    /*        site av; */ /* averaged pole */
    /*        double cond; */
    /*    float Sb; */
    short status;/* sunghee : 0(CNV) if on conv hull so vv contains normal vector;
                    1(VV) if vv points to circumcenter of simplex;
                    -1(SLV) if cond=0 so vv points to hull 
                    2(AV) if av contains averaged pole */
    long poleindex; /* for 1st DT, if status==POLE_OUTPUT, contains poleindex; for 2nd, contains vertex index for powercrust output for OFF file format */
    short edgestatus[6]; /* edge status :(01)(02)(03)(12)(13)(23)
                            FIRST_EDGE if not visited
                            VISITED
                            NOT_POW if not dual to powercrust faces
                            POW if dual to powercrust faces */  
    /*  short tristatus[4];   triangle status :     
        FIRST if not visited
        NO   if not a triangle
        DEG  if degenerate triangle
        SURF if surface triangle
        NORM if fails normal test
        VOR  if falis voronoi edge test
        VOR_NORM if fails both test */ 
    /* NOTE!!! neighbors has to be the LAST field in the simplex stucture, 
       since it's length gets altered by some tricky Clarkson-move. 
       Also peak has to be the one before it. 
       Don't try to move these babies!! */
    long visit;     /* number of last site visiting this simplex */
    basis_s* normal;    /* normal vector pointing inward */
    neighbor peak;      /* if null, remaining vertices give facet */ 
    neighbor neigh[1];  /* neighbors of simplex */
} simplex;
STORAGE_GLOBALS(simplex)


    /* Ravi:  for the thinning stuff */

/* represent a node in the graph */

    typedef  struct  spole { /* simple version to rep neighbors */
        long index;
        struct spole *next;
    } snode;

typedef   struct vpole{
    long index; /* index of the node */
    long pindex; /* index in the actual list of poles */
    double px;
    double py;
    double pz;
    double pr;  /* the radius of the ball centered here */
    double perpma; /* perpendicular distance from sample to medial axis */
    double pw;
    snode  *adj;
    int status;  /* for thinning */
    int label;  /* might be useful for computing the crust again */
    long substitute; /* if removed points to the substitute node */
    double estlfs; /* the estimated lfs of each ball */
} vnode ;

/* edges in the powerface */

typedef struct enode {
    long sindex;
    long dindex;
}   edge;

typedef struct fnode {
    long index1;
    long index2;
    long index3;
} face;



/* end defn for medial axis thinning */
       

/* structure for list of opposite poles, opplist. */
typedef struct plist {
    long pid;
    double angle;
    struct plist *next;
} plist;

/* regular triangulation edge, between pole pid to center of simp? */
typedef struct edgesimp {
    short kth;
    double angle;   /* angle between balls */
    struct simplex *simp;
    long pid;
    struct edgesimp *next;
} edgesimp;
 
/* additional info about poles: label for pole, pointer to list of regular 
   triangulation edges, squared radius of  polar ball. adjlist is an
   array of polelabels. */
typedef struct polelabel {
    struct edgesimp *eptr;
    short bad;
    short label;
    double in; /* 12/7/99 Sunghee for priority queue */
    double out; /* 12/7/99 Sunghee for priority queue */
    int hid; /* 0 if not in the heap, otherwise heap index 1..heap_size*/
    double sqradius;
    double oppradius; /* minimum squared radius of this or any opposite ball */
    double samp_distance;
    int grafindex; /* index in thinning graph data structure */
} polelabel;

typedef struct queuenode {
    long pid; 
    struct queuenode *next;
} queuenode;

typedef struct temp {
    struct simplex *simp;
    int vertptr[3];  
    int novert; 
    /* 0,1,2,3 : 3 vertices but ts->neigh[ti].vert are vertices of triangle */
} temp;

typedef struct tarr {
    int tempptr[50];
    int num_tempptr;
    long vert;
} tarr;

/*
typedef struct tlist {
  int tempptr;
  struct tlist *next;
} tlist;
*/

typedef struct fg_node fg;
typedef struct tree_node Tree;
struct tree_node {
    Tree *left, *right;
    site key;
    int size;   /* maintained to be the number of nodes rooted here */
    fg *fgs;
    Tree *next; /* freelist */
};

STORAGE_GLOBALS(Tree)


    typedef struct fg_node {
        Tree *facets;
        double dist, vol;   /* of Voronoi face dual to this */
        fg *next;       /* freelist */
        short mark;
        int ref_count;
    } fg_node;
    
STORAGE_GLOBALS(fg)


    typedef void* visit_func(simplex *, void *);
    typedef int test_func(simplex *, int, void *);
typedef void out_func(point *, int, FILE*, int);


/* Ravi thin axis */

void thinaxis();
void printaxis();
void initialize();

/* from driver, e.g., hullmain.c */

typedef site gsitef(void);

extern gsitef *get_site;    

typedef long site_n(site);
extern site_n *site_num;

site get_site_offline(long); /* sunghee */

extern double bound[8][3];

void read_bounding_box(long);

extern double mult_up;

extern Coord mins[MAXDIM], maxs[MAXDIM];

typedef short zerovolf(simplex *);

extern double Huge;

extern double bound[8][3];

void read_bounding_box(long);
void construct_face(simplex *, short);

/* from segt.c or ch.c */

simplex *build_convex_hull(gsitef*, site_n*, short, short);

void free_hull_storage(void);

int sees(site, simplex *);

void get_normal(simplex *s);

int out_of_flat(simplex*, site);

void set_ch_root(simplex*);

void print_site(site, FILE*);

void print_normal(simplex*);

visit_func check_marks;

double find_alpha(simplex*);
test_func alph_test;
void* visit_outside_ashape(simplex*, visit_func);

void get_basis_sede(simplex *);

void compute_distance(simplex**,int,double*);

    /* for debugging */
int check_perps(simplex*);

void find_volumes(fg*, FILE*);

#define MAXPOINTS 10000
extern short mi[MAXPOINTS], mo[MAXPOINTS];


/* from hull.c */


void *visit_triang_gen(simplex *, visit_func, test_func);
void *visit_triang(simplex *, visit_func);
void* visit_hull(simplex *, visit_func);

neighbor *op_simp(simplex *a, simplex *b);

neighbor *op_vert(simplex *a, site b);

simplex *new_simp(void);

void buildhull(simplex *);


/* from io.c */

void panic(char *fmt, ...);

typedef void print_neighbor_f(FILE*, neighbor*);
extern print_neighbor_f
print_neighbor_full,
    print_neighbor_snum;

void check_triang(simplex*);

short is_bound(simplex *);

void check_new_triangs(simplex *);

void print_extra_facets(void);

void *print_facet(FILE*, simplex*, print_neighbor_f*);

void print_basis(FILE*, basis_s*);

void *print_simplex_f(simplex*, FILE*, print_neighbor_f*);

void *print_simplex(simplex*, void*);

void print_triang(simplex*, FILE*, print_neighbor_f*);


out_func vlist_out, ps_out, cpr_out, mp_out, off_out, vv_out; 
/* sunghee : added vlist_out */
/* functions for different formats */

/* added compute axis RAVI */
visit_func facets_print, afacets_print, ridges_print, 
    compute_vv, compute_pole1, compute_pole2, test_surface, 
    compute_2d_power_vv, compute_3d_power_vv, compute_3d_power_edges,compute_axis; 
/* to print facets, alpha facets, ridges */
/* Sunghee added compute_cc, compute_pole1, compute_pole2, test_surface */

void test_temp();

/* Nina's functions in crust.c */
short is_bound(simplex *);
int close_pole(double*,double*,double);
int antiLabel(int);
int cantLabelAnything(int);
void labelPole(int,int);
void newOpposite(int, int, double);
double computePoleAngle(simplex*, simplex*, double*);
void outputPole(/* TJH FILE*, FILE*, */simplex*, int, double*, int*,double);

void print_edge_dat(fg *, FILE *);


/* from pointops.c */

void print_point(FILE*, int, point);
void print_point_int(FILE*, int, point);
Coord maxdist(int,point p1, point p2);



/* from rand.c */

double double_rand(void);
void init_rand(long seed);


/* from fg.c, for face graphs */

fg *build_fg(simplex*);

void print_fg(fg*, FILE *);

void print_fg_alt(fg*, FILE *, int);

void print_hist_fg(simplex *, fg*, FILE *);

/*  void arena_check(void); */  /* from hobby's debugging malloc  */

/* from predicates.c, math.c */
void normalize(double*);
double sqdist(double*, double*);
void dir_and_dist(double*, double*, double*, double*);
double dotabac(double*, double*, double*);
double maxsqdist(double*, double*, double*, double*);
double dotabc(double*, double*, double*);
void crossabc(double*, double*, double*, double*);
void tetcircumcenter(double*, double*, double*, double*, double*,double*);
void tricircumcenter3d(double*, double*, double*, double*,double*);
void exactinit();
double orient3d(double*, double*, double*, double*);
double orient2d(double*, double*, double*);

/* heap.c */
typedef struct heap_array {
    int pid;
    double pri;
} heap_array;

void init_heap(int);
void heapify(int);
int extract_max();
int insert_heap(int , double);
void update(int , double);

/* label.c */
void opp_update(int);
void sym_update(int);
void update_pri(int,int);
int propagate();
void label_unlabeled(int);


/*power.c */
int correct_orientation(double*,double*,double*,double*,double*);

#endif
//========tim_defs.h=============================================================

#define random rand
#define srandom srand

/* TJH: old attempts at solving compilation problem (don't work on other OS's)
int random() { return rand(); }
void srandom(int s) { srand(s); }

/* TJH: these were put here to allow compiling under windows, getting problems on other OS's now
#define popen _popen
#define pclose _pclose*/
//========rand48.h=============================================================
/*
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 *
 *  $OpenBSD: rand48.h,v 1.2 1996/08/19 08:33:45 tholo Exp $
 */

#ifndef _RAND48_H_
#define _RAND48_H_

#include <math.h>
#include <stdlib.h>

void _dorand48(unsigned short xseed[3]);
//void __dorand48(unsigned short xseed[3]);
//void    __dorand48 __P((unsigned short[3]));

#define  RAND48_SEED_0  (0x330e)
#define  RAND48_SEED_1  (0xabcd)
#define  RAND48_SEED_2  (0x1234)
#define  RAND48_MULT_0  (0xe66d)
#define  RAND48_MULT_1  (0xdeec)
#define  RAND48_MULT_2  (0x0005)
#define  RAND48_ADD  (0x000b)

#endif /* _RAND48_H_ */
//========hullmain.c=============================================================
// Some modifications made by Tim J. Hutton (T.Hutton@eastman.ucl.ac.uk) as TJH

/*
 * Power Crust software, by Nina Amenta, Sunghee Choi and Ravi Krishna Kolluri.
 * Copyright (c) 2000 by the University of Texas
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee under the GNU Public License is hereby granted, 
 * provided that this entire notice  is included in all copies of any software 
 * which is or includes a copy or modification of this software and in all copies 
 * of the supporting documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

/* 
 * This file is a significant modification of Ken Clarkson's file hullmain.c. 
 * We include his copyright notice in accordance with its terms.
 *                                                                     - Nina, Sunghee and Ravi
 */


/*
 * Ken Clarkson wrote this.  Copyright (c) 1995 by AT&T..
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#include <float.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
//#include <getopt.h>  TJH: we no longer need option handling
#include <ctype.h>
  
#define POINTSITES 1

//#include "hull.h"   TJH: this file is above

//#include "tim_defs.h" TJH: this file is above

double bound[8][3], omaxs[3], omins[3];  /* 8 vertices for bounding box */
//point   site_blocks[MAXBLOCKS];  TJH: this is declared above
//int num_blocks; TJH: this is declared above
extern int numfaces;
struct queuenode *queue;
struct queuenode *qend;
int num_vtxs=0, num_faces=0;

/* Data structures for poles */ 
struct simplex **pole1, **pole2;  /* arrays of poles - per sample*/
struct polelabel *adjlist;  /* array of additional info - per pole */
struct plist **opplist; /* opposite pid and angle between poles - per pole */
double* lfs_lb;  /*  array of lower bounds for lfs of each sample */
double  est_r = 0.6;   /* estimated value of r - user input */


int num_poles=0,num_axedgs=0,num_axfaces=0; 


double *pole1_distance,*pole2_distance;


/* for priority queue */
extern int heap_size;   
    
/* int  getopt(int, char**, char*); */
extern char *optarg;
extern int optind;
extern int opterr;
extern int scount;
extern int v1[6], v2[6], v3[6], v4[6];
long num_sites;

// TJH: vd declared elsewhere, renamed throughout this file
//static short vd = 1;
static short vd_new = 1;

short power_diagram = 0; /* 1 if power diagram */

static int dim;
static long s_num = 0; /* site number */

double theta = 0.0; /* input argument - angle defining deep intersection */
double deep = 0.0; /* input argument.. same as theta for labeling unlabled pole */
int defer = 0; /* input argument -D 1 if you don't want to propagate bad poles */

int poleInput=0; /* are the poles given as input */

// TJH: we have bypassed file handling, so we don't need this
//FILE *INFILE, *OUTFILE=NULL,*DFILE, *TFILE, *SPFILE, *POLE, *PC, *PNF, *INPOLE, *INPBALL, *INVBALL, *AXIS, *AXISFACE;
FILE *DFILE; // just to avoid having to rewrite tons of code

// TJH: if you ever want to see the output of powercrust as it works, uncomment the lines
//      that contain DFILE - this used to be stderr

int *rverts;


int* select_random_points(int Nv) /* for orientation testing */
{ /* Nv : Number of vertices (sites) */
    int i,j;
    int *rverts;
 
    rverts = (int*) malloc (NRAND*sizeof(int)); 
   
    srandom(Nv);  /* seed the random number generator */
    for (i=0; i<NRAND; i++) {
        j = random() % Nv;
        rverts[i] = j;
    }
    return(rverts); 
}  

long site_numm(site p) {

    long i,j;

    if (( vd_new || power_diagram) && p==infinity) return -1;
    if (!p) return -2;
    for (i=0; i<num_blocks; i++) {
        if ((j=p-site_blocks[i])>=0 && j < BLOCKSIZE*dim) 
            return j/dim+BLOCKSIZE*i;
    }
    return -3;
}


site new_site (site p, long j) {

    assert(num_blocks+1<MAXBLOCKS);
    if (0==(j%BLOCKSIZE)) {
        assert(num_blocks < MAXBLOCKS);
        return(site_blocks[num_blocks++]=(site)malloc(BLOCKSIZE*site_size));
    } else
        return p+dim;
}


void read_bounding_box(long j)
{
    int i,k;
    double center[3],width;

    omaxs[0] = maxs[0];
    omins[0] = mins[0];
    omaxs[1] = maxs[1];
    omins[1] = mins[1];
    omaxs[2] = maxs[2];
    omins[2] = mins[2];

    center[0] = (maxs[0] - mins[0])/2;
    center[1] = (maxs[1] - mins[1])/2;
    center[2] = (maxs[2] - mins[2])/2;
    if ((maxs[0] - mins[0])>(maxs[1] - mins[1])) {
        if ((maxs[2] - mins[2]) > (maxs[0] - mins[0]))
            width = maxs[2] - mins[2];
        else width = maxs[0] - mins[0];
    }
    else {
        if ((maxs[1] - mins[1]) > (maxs[2] - mins[2]))
            width = maxs[1] - mins[1];
        else width = maxs[2] - mins[2];
    }

    width = width * 4;

    bound[0][0] = center[0] + width;
    bound[1][0] = bound[0][0];
    bound[2][0] = bound[0][0];
    bound[3][0] = bound[0][0];
    bound[0][1] = center[1] + width;
    bound[1][1] = bound[0][1]; 
    bound[4][1] = bound[0][1];  
    bound[5][1] = bound[0][1]; 
    bound[0][2] = center[2] + width; 
    bound[2][2] = bound[0][2];
    bound[4][2] = bound[0][2];
    bound[6][2] = bound[0][2];
    bound[4][0] = center[0] - width; 
    bound[5][0] = bound[4][0]; 
    bound[6][0] = bound[4][0];
    bound[7][0] = bound[4][0];
    bound[2][1] = center[1] - width;
    bound[3][1] = bound[2][1];
    bound[6][1] = bound[2][1]; 
    bound[7][1] = bound[2][1];
    bound[1][2] = center[2] - width;
    bound[3][2] = bound[1][2];
    bound[5][2] = bound[1][2];
    bound[7][2] = bound[1][2];

    for (i=0;i<8;i++)
    {
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE, "%f %f %f\n",
                bound[i][0]/mult_up, bound[i][1]/mult_up, bound[i][2]/mult_up);
    }

    for (k=0;k<3;k++) {
        p[k] = bound[0][k];
    }

    for (i=1;i<8;i++) {
        p=new_site(p,j+i);
        for (k=0;k<3;k++) {
            p[k] = bound[i][k];
        }
    }
    maxs[0] = bound[0][0];
    mins[0] = bound[4][0];
    maxs[1] = bound[0][1];
    mins[1] = bound[2][1];
    maxs[2] = bound[0][2];
    mins[2] = bound[1][2];
}

// TJH: trying to replace file use
site vtk_read_next_site(long j)
{
    ASSERT(j>=0);
    p = new_site(p,j);
    
    for(int i=0;i<dim;i++)
    {
       p[i] = (double)vtk_input->GetPoint(j)[i];
       p[i] = floor(mult_up*p[i]+0.5);
       mins[i] = (mins[i]<p[i]) ? mins[i] : p[i];
       maxs[i] = (maxs[i]>p[i]) ? maxs[i] : p[i];
    }

    return p;
}
     
// TJH: trying to replace file use
site vtk_pole_read_next_site(long j)
{
    ASSERT(j>=0);
    p = new_site(p,j);
    
    for(int i=0;i<dim;i++)
    {
       if(i<3)
         p[i] = (double)vtk_medial_surface->GetPoint(j)[i];
       else
         p[i] = (double)vtk_medial_surface->GetPointData()->GetScalars()->GetTuple1(j);
       p[i] = floor(mult_up*p[i]+0.5);
       mins[i] = (mins[i]<p[i]) ? mins[i] : p[i];
       maxs[i] = (maxs[i]>p[i]) ? maxs[i] : p[i];
    }

    return p;
}
     
// TJH: we've bypassed the file handling, so we don't need this function
// (added some comments to try to make a little clearer what this code does
// in overview it reads x y z coords from a file, space separated, each on its own line)
/*site read_next_site(long j){

    int i=0, k=0;
    static char buf[1000], *s;

    // TJH: allocate space for a new site unless just testing
    // (this code gets used with j=-1 to find the dimensions of the data (2 or 3))
    // p is a global (blech) storing the current 'site' or point
    if (j!=-1) p = new_site(p,j);
    
    // TJH: hack alert! (Ok, so maybe it's efficient)
    // if j==0 then we've previously called this function with j==-1 to read the first
    // line, so this time we don't need to read in the line
    if (j!=0) {
        // TJH: read each line of the file in turn until we find some data
        while ((s=fgets(buf,sizeof(buf),INFILE))) 
        {
            // TJH: ignore lines that begin with a %, these are comments
            if (buf[0]=='%') continue;
            // TJH: ignore all the whitespace at the start of the line
            for (k=0; buf[k] && isspace(buf[k]); k++);
            // TJH: if we've found a character then we assume the data starts here
            if (buf[k]) break;
        }
    }

    // TJH: if the end of the file was reached without finding any data then early return
    // no problem with s being undefined *if* j==-1 call was made before j==0
    if (!s) return 0;

    // TJH: output the line to TFILE
    // TFILE is a temporary file 'temp'
    // TJH: we have bypassed the file handling so don't need this bit
    //if (j!=0) {
    //    assert(TFILE != NULL);
    //    fprintf(TFILE, "%s", &(buf[k]) );fflush(TFILE);
    //}

    // TJH: parse the line that was read
    while (buf[k]) {
        // TJH: zip through any whitespace
        while (buf[k] && isspace(buf[k])) k++;
        // TJH: if still non-empty buffer and not in special sizing call
        if (buf[k] && j!=-1) {
            // TJH: read in the i'th coordinate (x=0,y=1,z=2)
            if (sscanf(buf+k,"%lf",p+i)==EOF) {
                // TJH: DFILE is stderr or whatever error output stream you want
                // TJH: added this if statement
                if(DFILE)
                    fprintf(DFILE, "bad input line: %s\n", buf);
                ASSERT(pcFALSE);
            }
            // TJH: we multiply the coordinates by some large user-supplied value
            // to get everything into integer values (for speed later!)
            p[i] = floor(mult_up*p[i]+0.5);
            // TJH: find the bounding box (min and max of each axis)
            mins[i] = (mins[i]<p[i]) ? mins[i] : p[i];
            maxs[i] = (maxs[i]>p[i]) ? maxs[i] : p[i];
        }
        // TJH: get ready for the next coordinate
        if (buf[k]) i++;
        // TJH: zip through until we find the next whitespace (allows for commas or other
        // text before the separating space(s) )
        while (buf[k] && !isspace(buf[k])) {
            k++;
        }
    }

    // TJH: the dimensions of the data (2 or 3) come from the how many coordinates were read
    // dim is a global variable
    if (!dim) dim = i;
    // TJH: if any of the lines gave a different number of coordinates then file is invalid
    if (i!=dim) {
        // make a debug call
        DEB(-10,inconsistent input);
        DEBTR(-10); 
        ASSERT(pcFALSE,"inconsistent input");
    }  

    return p;
}*/


/* reads a site from storage we're managing outselves */
site get_site_offline(long i) {

    if (i>=num_sites) return NULL;
    else {
        return site_blocks[i/BLOCKSIZE]+(i%BLOCKSIZE)*dim;
    }
}


long *shufmat;

void make_shuffle(void){
    long i,t,j;
    static long mat_size = 0;

    if (mat_size<=num_sites) {
        mat_size = num_sites+1;
        shufmat = (long*)malloc(mat_size*sizeof(long));
    }
    for (i=0;i<=num_sites;i++) {
        shufmat[i] = (double)i;  // TJH added this cast
    }
    for (i=0;i<num_sites;i++){
        t = shufmat[i];
        j = i + (num_sites-i)*double_rand();
        shufmat[i] = shufmat[j];
        shufmat[j] = t;
    }
}

static long (*shuf)(long);
long noshuffle(long i) {return i;}
long shufflef(long i) {
    return shufmat[i];
}

static site (*get_site_n)(long);

/* returns shuffled, offline sites or reads an unshuffled site, depending on 
   how get_site_n and shuf are set up. */
site get_next_site(void) {
    /*  static long s_num = 0; */
    return (*get_site_n)((*shuf)(s_num++));
}


/* TJH: we aren't using the command line anymore, so we don't need this
void errline(char *s) {fprintf(stderr, s); fprintf(stderr,"\n"); return;}
void tell_options(void) {

    errline("options:");
    errline( "-m mult  multiply by mult before rounding;");
    errline( "-s seed  shuffle with srand(seed);");
    errline( "-i<name> read input from <name>;");
    errline( "-X<name> chatter to <name>;");
    errline( "-oF<name>  prefix of output files is <name>;");
    errline( "-t min cosine of allowed dihedral angle btwn polar balls");
    errline( "-w same as -t, but for trying to label unlabled poles, the second time around.");
    errline( "-D no propagation for 1st pole of non-manifold cells");
    errline( "-B throw away both poles for non-manifold cells");
    errline( "-R guess for value of r, used to eliminate bad second poles");
}*/


void echo_command_line(FILE *F, int argc, char **argv) {
    fprintf(F,"%%");
    while (--argc>=0) 
        fprintf(F, "%s%s", *argv++, (argc>0) ? " " : "");
    fprintf(F,"\n");
}

char *output_forms[] = {"vn", "ps", "mp", "cpr", "off"};

out_func *out_funcs[] = {&vlist_out, &ps_out, &mp_out, &cpr_out, &off_out};


int set_out_func(char *s) {

    int i;

    for (i=0;i< sizeof(out_funcs)/(sizeof (out_func*)); i++)
        if (strcmp(s,output_forms[i])==0) return i;
    /* TJH: we aren't using the command line anymore
    tell_options();*/
    return 0;
}

void make_output(simplex *root,
                 void *(*visit_gen)(simplex*, visit_func* visit),
                 visit_func* visit,
                 out_func* out_funcp/*,
                 FILE *F*/)
{
    FILE *F=NULL; // TJH: dummy file pointer

    out_funcp(0,0,F,-1);
    visit(0, out_funcp);
    visit_gen(root, visit);
    out_funcp(0,0,F,1);
    /*  efclose(F); */
}

//---------------------------------------------------------------------------------
// the function below is a modified version of the main() function
// it is called from vtkPowerCrustSurfaceReconstruction::Execute()
// the original main() function is commented out beneath it
//---------------------------------------------------------------------------------

void adapted_main() 
{
    long    seed = 0, poleid=0;
    short   shuffle = 1,
        output = 0,   // TJH: set to 1 if you want output reported to stdout, 0 otherwise
        hist = 0,
        vol = 0,
        ofn = 0,
        ifn = 0,
        bad = 0 /* for -B */
        ;
    int /* TJH option, */num_poles=0;
    double  pole_angle;
    char    ofile[50] = "",
        ifile[50] = "",
        ofilepre[50] = "";

    /* TJH: we've bypassed all the file handling so we don't need this bit
    FILE *INPOLE, *OUTPOLE,*HEAD,*POLEINFO;*/

    int main_out_form=0, i,k;

    simplex *root;

    struct edgesimp *eindex;
    double samp[3];
    double tmp_pt[3];
    int numbadpoles=0;
    // TJH double x,y,z,r,d;
    // TJH int l;
    out_func *mof;
    visit_func *pr;


    /* some default values */
    mult_up = 1000000; 
    est_r = 1;

    // TJH: in this VTK port we don't want anything piped to stderr
//    DFILE = stderr;
    DFILE = NULL;

    /* TJH: no input options any more, can skip all this

    while ((option = getopt(argc, argv, "i:m:rs:DBo:X::f:t:w:R:p")) != EOF) {
        switch (option) {
        case 'm' :
            sscanf(optarg,"%lf",&mult_up);
            DEBEXP(-4,mult_up);
            break;
        case 's':
            seed = atol(optarg);
            shuffle = 1;
            break;
        case 'D':
            defer = 1; 
            break;
        case 'B':
            bad = 1; 
            break;
        case 'i' :
            strcpy(ifile, optarg);
            break;
        case 'X' : 
            DFILE = efopen(optarg, "w");
            break;
        case 'f' :
            main_out_form = set_out_func(optarg);
            break;
        case 'o': switch (optarg[0]) {
        case 'o': output=1; break;
        case 'N': output=1; break; // output is never set to zero
        case 'v': vd_new = vol = 1; break;
        case 'h': hist = 1; break;
        case 'F': strcpy(ofile, optarg+1); break;
        default: errline("illegal output option");
            exit(1);
        }
        break;
        case 't':sscanf(optarg,"%lf",&theta);
            break;
        case 'w':sscanf(optarg,"%lf",&deep);
            break;   
        case 'R':sscanf(optarg,"%lf",&est_r);
            break;   
        case 'p':poleInput=1;
            break; 

        default :
            tell_options();
            exit(1);
        }
    }*/

    /* TJH: we read the data directly from our vtkPolyData now instead of from file
    // old TJH: first we output out vtk structure to file to blend in with powercrust's normal operation
    {
        strcpy(ifile,"in");
        FILE *vtk_input_file = fopen(ifile,"w");
        if(vtk_input_file)
        {
            float p[3];
            for(int i=0;i<vtk_input->GetNumberOfPoints();i++)
            {
                vtk_input->GetPoint(i,p);
                fprintf(vtk_input_file,"%f %f %f\n",p[0],p[1],p[2]);
            }
            fclose(vtk_input_file);
        }
    }*/

    /* TJH: we have bypassed the file routines, so we don't need this bit

    AXIS=fopen("axis","w");
    AXISFACE=fopen("axisface","w");
    POLE=fopen("pole","w");

    // old TJH: replace use of temporary file with concrete temp file for easier deletion later
    //TFILE = efopen(tmpnam(tmpfilenam), "w");
    strcpy(tmpfilenam,"temp");

    TFILE = efopen(tmpfilenam, "w");
    
    */

    // TJH: poleInput is a global (blech) and is 0 by default, so this code is run
    if (!poleInput)  
    {
        /* TJH: we've bypassed file handling so we don't need this bit
        ifn = (strlen(ifile)!=0); 
        INFILE = ifn ? efopen(ifile, "r") : stdin;
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE, "reading from %s\n", ifn ? ifile : "stdin");

        ofn = (strlen(ofile)!=0);

        strcpy(ofilepre, ofn ? ofile : (ifn ? ifile : "hout") );

        if (output) {
            if (ofn && main_out_form > 0) {
                strcat(ofile, "."); 
                strcat(ofile, output_forms[main_out_form]);
            }
            OUTFILE = ofn ? efopen(ofile, "w") : stdout;
            // TJH: added this if statement
            if(DFILE)
                fprintf(DFILE, "main output to %s\n", ofn ? ofile : "stdout");
        } else 
        {
            // TJH: added this if statement
            if(DFILE)
                fprintf(DFILE, "no main output\n");
        }*/

        
        // TJH: we've replaced the old style file handling with our own vtkPolyData reading
        //read_next_site(-1);
        dim=3;

        // TJH: added this if statement
        if(DFILE)
        {
            fprintf(DFILE,"dim=%d\n",dim);fflush(DFILE); 
        }
        if (dim > MAXDIM) panic("dimension bound MAXDIM exceeded"); 

        point_size = site_size = sizeof(Coord)*dim;

        // TJH: read in the points, find their bounding box, randomise their order
    
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE, "reading sites...");

        // TJH: we've replaced this file-reading loop with the loop below
        /*for(num_sites=0;read_next_site(num_sites);num_sites++);*/
        for(num_sites=0;num_sites<vtk_input->GetNumberOfPoints();num_sites++)
        {
            vtk_read_next_site(num_sites);
        }
        num_sites--;

        // TJH: if we made our own input file then it can be closed and deleted here
        //fclose(INFILE);
        //system("del in");

        // TJH: added this if statement
        if(DFILE)
        {
            fprintf(DFILE,"done; num_sites=%ld\n", num_sites);fflush(DFILE);
        }
        read_bounding_box(num_sites);
        num_sites += 8; 
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE,"shuffling...");
        init_rand(seed);
        make_shuffle();
        shuf = &shufflef;
        get_site_n = get_site_offline;
    

        /* Step 1: compute DT of input point set */
        root = build_convex_hull(get_next_site, site_numm, dim, vd_new);

        /* Step 2: Find poles */
        pole1 = (struct simplex **) calloc(num_sites, sizeof(struct simplex *));    
        pole2 = (struct simplex **) calloc(num_sites, sizeof(struct simplex *));    
        lfs_lb = (double*) calloc(num_sites, sizeof(double));

        // TJH: added this if statement
        if(DFILE)
        {
            fprintf(DFILE, "done\n"); fflush(DFILE);
        }

        /*  
            rverts = select_random_points(num_sites); 
            fprintf(DFILE, "selecing random points\n");
        */

        mof = out_funcs[main_out_form];
        pr = facets_print;
        
        // TJH: no command-line arguments no more
        //if(main_out_form==0) echo_command_line(OUTFILE,argc,argv);

        exactinit(); /* Shewchuk's exact arithmetic initialization */

        pr = compute_vv;
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE, "Computing Voronoi vertices and 1st poles....\n");
        make_output(root, visit_hull, pr, mof/*, OUTFILE*/);
               
        pr = compute_pole2;
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE, "\n\n\ncomputing 2nd poles....\n");
        make_output(root, visit_hull, pr, mof/*, OUTFILE*/);
     

        /* poles with weights. Input to regular triangulation */
        /* TJH: we've bypassed the file handling, so we don't need this bit
        SPFILE = fopen("sp","w"); */

        /*  fprintf(POLE,"%s \n","OFF"); */
      
        /* initialize the sample distance info for the poles */

        pole1_distance=(double *) malloc(num_sites*sizeof(double));
        pole2_distance=(double *) malloc(num_sites*sizeof(double));

        compute_distance(pole1,num_sites-8,pole1_distance);
        compute_distance(pole2,num_sites-8,pole2_distance);




        /* intialize list of lists of pointers to opposite poles */
        opplist = (struct plist**) calloc(num_sites*2, sizeof(struct plist *));

        /* data about poles; adjacencies, labels, radii */
        adjlist = (struct polelabel *) calloc(num_sites*2, sizeof(struct polelabel));
     
        /* loop through sites, writing out poles */
        for (i=0;i<num_sites-8;i++) { 

            /* rescale the sample to real input coordinates */ 
            for (k=0; k<3; k++) 
                samp[k] = get_site_offline(i)[k]/mult_up;

            /* output poles, both to debugging file and for weighted DT */
            /* remembers sqaured radius */
            if ((pole1[i]!=NULL)&&(pole1[i]->status != POLE_OUTPUT)) {
                /* if second pole is closer than we think it should be... */
                if ((pole2[i]!=NULL) && bad &&
                    close_pole(samp,pole2[i]->vv,lfs_lb[i])) {
                    numbadpoles++;
                } 
                else {
                    outputPole(/* TJH POLE,SPFILE,*/pole1[i],poleid++,samp,&num_poles,pole1_distance[i]);
                }
            }

            if ( (pole2[i]!=NULL) && (pole2[i]->status != POLE_OUTPUT)) {

                /* if pole is closer than we think it should be... */
                if (close_pole(samp,pole2[i]->vv,lfs_lb[i])) {
                    /* remember opposite bad for late labeling */
                    if (!bad) adjlist[pole1[i]->poleindex].bad = BAD_POLE;
                    numbadpoles++;
                    continue;
                }
   
                /* otherwise... */
                outputPole(/* TJH POLE,SPFILE,*/pole2[i],poleid++,samp,&num_poles,pole2_distance[i]);
            }

            /* keep list of opposite poles for later coloring */
            if ((pole1[i]!=NULL)&&(pole2[i]!=NULL)&&
                (pole1[i]->status == POLE_OUTPUT) &&
                (pole2[i]->status == POLE_OUTPUT)) {

                pole_angle = 
                    computePoleAngle(pole1[i],pole2[i],samp);

                newOpposite(pole1[i]->poleindex,
                            pole2[i]->poleindex,pole_angle);
                newOpposite(pole2[i]->poleindex,
                            pole1[i]->poleindex,pole_angle);
            }
        }
        /* TJH: we've bypassed the file handling so we don't need this bit
        efclose(POLE);
        efclose(SPFILE);*/
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE,"bad poles=%d\n",numbadpoles);

        free_hull_storage(); 
  
    } /* do this if the input was a set of samples not poles */

    /*  mult_up = mult_up1;  set the multiplier for the 2nd Delaunay */
    /*  TFILE = fopen("temp", "w"); */ 
    /*    TFILE = efopen(tmpnam(tmpfilenam), "w"); */

    else { 
        
        // TJH: poleInput is not going to be 1, so this code shouldn't get run

        /*

        // read data from the input file and put it in sp 
        // initialize adjlist to the right size and initialize labels 
        INFILE=fopen(ifile,"r");
        SPFILE=fopen("sp","w");
      

        fprintf(DFILE,"%s",ifile);
        mof = out_funcs[main_out_form];
        pr = facets_print;
        OUTFILE=stdout;

        fscanf(INFILE,"%d",&num_poles);// get the number of poles ..
        adjlist = (struct polelabel *) calloc(num_poles, sizeof(struct polelabel));
        for(i=0;i<num_poles;i++) {
     
            //   fscanf(INFILE,"%e\s%e\s%e\s%e\s%d\s%e\n",&x,&y,&z,&r,&l,&d); 
     
            fscanf(INFILE,"%le ",&x);
            fscanf(INFILE,"%le ",&y);
            fscanf(INFILE,"%le ",&z);
            fscanf(INFILE,"%le ",&r);
            fscanf(INFILE,"%d ",&l);
            fscanf(INFILE,"%le ",&d);
        
            // we  have the square of the radius 

            fprintf(SPFILE,"%f %f %f %f\n",x,y,z,SQ(x)+SQ(y)+SQ(z)-r);
            fprintf(POLE,"%f %f %f \n",x,y,z);
            //  fprintf(DFILE,"%f %f %f %d\n",x,y,z,num_poles);
            adjlist[i].sqradius=r;
            adjlist[i].label=l;

        }

        efclose(INFILE);
        efclose(SPFILE);
        efclose(POLE);
        
        */
    }

    

    power_diagram = 1;
    vd_new = 0; 
    dim = 4; 
      
    // TJH: we've bypassed the file handling
    //INFILE = fopen("sp","r");

    // TJH: added this if statement
    if(DFILE)
        fprintf(DFILE,"num_blocks = %d\n",num_blocks);
    
    // TJH: uncommented this line, seemed like we need to free this memory before num_blocks gets reset
    for (i=0;i<num_blocks;i++) free(site_blocks[i]);

    num_blocks = 0;  
    s_num = 0; 
    scount = 0;

    // TJH: we've bypassed the file handling
    //read_next_site(-1);

    // TJH: added this if statement
    if(DFILE)
    {
        fprintf(DFILE,"dim=%d\n",dim); fflush(DFILE); 
    }
    /* if (dim > MAXDIM) panic("dimension bound MAXDIM exceeded"); */
    
    point_size = site_size = sizeof(Coord)*dim; 
    /* save points in order read */
    // TJH: we've bypassed the file handling
    //for (num_sites=0; read_next_site(num_sites); num_sites++);
    for(num_sites=0;num_sites<vtk_medial_surface->GetNumberOfPoints();num_sites++)
    {
        vtk_pole_read_next_site(num_sites);
    }
    //num_sites--;
     
    // TJH: added this if statement
    if(DFILE)
    {
        fprintf(DFILE,"done; num_sites=%ld\n", num_sites);fflush(DFILE);
    }

    // TJH: moved to new version of code
    //efclose(INFILE);

    /* set up the shuffle */
    // TJH: added this if statement
    if(DFILE)
        fprintf(DFILE,"shuffling...");
    init_rand(seed);
    make_shuffle();
    shuf = &shufflef;
    get_site_n = get_site_offline;  /* returns stored points, unshuffled */
      
    /* Compute weighted DT  */
    root = build_convex_hull(get_next_site, site_numm, dim, vd_new);
     
    // TJH: added this if statement
    if(DFILE)
        fprintf(DFILE,"scount=%d, s_num=%ld\n",scount,s_num);
    
    // TJH: added this if statement
    if(DFILE)
    {
        fprintf(DFILE, "done\n"); fflush(DFILE);
    }
     
    /* file of faces */
    /* TJH: we have bypassed the file routines, so we don't need this bit
    PNF = fopen("pnf","w"); */
    /* file of points */
    /* TJH: we have bypassed the file routines, so we don't need this bit
    PC = fopen("pc","w"); */
    
    /* compute adjacencies and find angles of ball intersections */
    queue = NULL;
    pr = compute_3d_power_vv;
    make_output(root, visit_hull, pr, mof/*, OUTFILE*/);
      
    
    
    /* Begin by labeling everything outside a big bounding box as outside */
       
    /* labeling */
    if(!poleInput) { /* if we dont have the labels */
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE,"num_poles=%d\n",num_poles);
        init_heap(num_poles);
        for (i=0;i<num_poles;i++) { 
            if ((get_site_offline(i)[0]>(2*omaxs[0]-omins[0]))||
                (get_site_offline(i)[0]<(2*omins[0]-omaxs[0]))||
                (get_site_offline(i)[1]>(2*omaxs[1]-omins[1]))||
                (get_site_offline(i)[1]<(2*omins[1]-omaxs[1]))||
                (get_site_offline(i)[2]>(2*omaxs[2]-omins[2]))||
                (get_site_offline(i)[2]<(2*omins[2]-omaxs[2])))
            {
                adjlist[i].hid = insert_heap(i,1.0);
                adjlist[i].out = 1.0;
                adjlist[i].label = OUT;
            }
        }
    
        while (heap_size != 0) propagate();
      
        label_unlabeled(num_poles);

    }
     

      
    /* Enough labeling; let's look at the poles and output a crust!  */  
    /* TJH: we have bypassed the file routines, so we don't need this bit
    INPOLE = fopen("inpole","w");
    OUTPOLE = fopen("outpole","w");*/

    /* TJH: we have bypassed the file routines, so we don't need this bit
    // for visualization of polar balls: 
    INPBALL = fopen("inpball","w");  // inner poles with radii
    POLEINFO = fopen("tpoleinfo","w");*/
    

    for (i=0;i<num_poles;i++) {

        for (k=0; k<3; k++) 
            tmp_pt[k] = get_site_offline(i)[k]/mult_up;

        /* TJH: we have bypassed the file routines, so we don't need this bit
        fprintf(POLEINFO,"%f %f %f %f %d %f \n ",tmp_pt[0],tmp_pt[1],tmp_pt[2],
                adjlist[i].sqradius,adjlist[i].label,adjlist[i].samp_distance);*/
            
        if ((adjlist[i].label != IN) && (adjlist[i].label != OUT)) {
            // TJH: added this if statement
            if(DFILE)
                fprintf(DFILE,"pole %d label %d\n",i,adjlist[i].label);
        }
        else { 
         
            if (adjlist[i].label == IN) {
                /* TJH: we have bypassed the file routines, so we don't need this bit
                fprintf(INPOLE,"%f %f %f\n",tmp_pt[0],tmp_pt[1],tmp_pt[2]); 
                fprintf(INPBALL,"%f %f %f %f \n",
                        tmp_pt[0],tmp_pt[1],tmp_pt[2],sqrt(adjlist[i].sqradius)); */
            }
            else if (adjlist[i].label == OUT) 
            {
                /* TJH: we have bypassed the file routines, so we don't need this bit
                fprintf(OUTPOLE,"%f %f %f\n",tmp_pt[0],tmp_pt[1],tmp_pt[2]); */
            }
        
            eindex = adjlist[i].eptr;
            while (eindex!=NULL) {
                if ((i < eindex->pid) && 
                    (antiLabel(adjlist[i].label) == adjlist[eindex->pid].label))
                {
                    construct_face(eindex->simp,eindex->kth);
                }
                eindex = eindex->next;
            }
        }
    }
     
    /* TJH: we have bypassed the file routines, so we don't need this bit
    efclose(PC);
    efclose(PNF); 
    efclose(POLEINFO);*/

    /* powercrust output done... */

    /* TJH: we have bypassed the file routines, so we don't need this bit
    HEAD = fopen("head","w");
    fprintf(HEAD,"OFF\n"); 
    fprintf(HEAD,"%d %d %d\n",num_vtxs,num_faces,0);
    efclose(HEAD);
    system("type head pc pnf > pc.off"); // TJH: 'type' was 'cat', changed for Windows compilation
    system("del head pc pnf"); // TJH: 'del' was 'rm', changed for Windows compilation
    */

 
    /* compute the medial axis */
    pr=compute_axis;
    // TJH: added this if statement
    if(DFILE)
        fprintf(DFILE,"\n\n computing the medial axis ....\n");
    make_output(root,visit_hull,pr,mof/*,OUTFILE*/);
    /* TJH: we have bypassed the file routines, so we don't need this bit
    efclose(AXIS);*/
      
    /* TJH: we have bypassed the file routines, so we don't need this bit
    HEAD = fopen("head","w");
    fprintf(HEAD,"OFF\n"); 
    fprintf(HEAD,"%d %d %d\n",num_poles,num_axedgs,0);
    efclose(HEAD);*/
    //system("type head pole axis > axis.off"); // TJH: 'type' was 'cat', changed for Windows compilation
      
    /* TJH: we have bypassed the file routines, so we don't need this bit
    HEAD=fopen("head","w");
    fprintf(HEAD,"%d %d \n", num_poles,num_axedgs);
    efclose(HEAD);*/

    //system("type head tpoleinfo axis > poleinfo"); // TJH: 'type' was 'cat', changed for Windows compilation          

    
    /* TJH: we have bypassed the file routines, so we don't need this bit
    HEAD = fopen("head","w");
    fprintf(HEAD,"OFF\n"); 
    fprintf(HEAD,"%d %d %d\n",num_poles,num_axfaces,0);
    efclose(HEAD);
    efclose(AXISFACE);*/


    //system("type head pole axisface > axisface.off"); // TJH: 'type' was 'cat', changed for Windows compilation
    //system("del head axis axisface tpoleinfo sp"); // TJH: 'del' was 'rm', changed for Windows compilation

    /* power shape output done */
    
    /* TJH: we have bypassed the file routines, so we don't need this bit
    efclose(INPOLE);
    efclose(OUTPOLE);
    efclose(INPBALL);

    efclose(TFILE);*/

    // TJH: delete the files that were left
    //system("del temp inpole outpole inpball poleinfo axis.off axisface.off pc.off");

    free(adjlist);
     
    // TJH: shouldn't we also free tons of other things?
    // (trying to solve memory leaks)
    free(opplist);
    free(pole1_distance);
    free(pole2_distance);
    free(pole1);
    free(pole2);
    free(shufmat);
    free(lfs_lb);
        
    
    free_hull_storage();
    // TJH: removed file handling
    //efclose(DFILE);

    // TJH: copied this line from earlier - how else does site_blocks get deallocated?
    // (trying to solve memory leaks)
    for (i=0;i<num_blocks;i++) free(site_blocks[i]);


    // TJH: we don't need to crash out here...
    //exit(1);
}

// TJH: the main function is now in an adapted form above, this is here for posterity. May she rest in peace.
/*int main(int argc, char **argv) {

    long    seed = 0, poleid=0;
    short   shuffle = 1,
        output = 1,
        hist = 0,
        vol = 0,
        ofn = 0,
        ifn = 0,
        bad = 0 // for -B
        ;
    int option, num_poles=0;
    double  pole_angle;
    char    ofile[50] = "",
        ifile[50] = "",
        ofilepre[50] = "";
    FILE *INPOLE, *OUTPOLE, *HEAD,*POLEINFO;
    int main_out_form=0, i,k;

    simplex *root;

    struct edgesimp *eindex;
    double samp[3];
    double tmp_pt[3];
    int numbadpoles=0;
    double x,y,z,r,d;
    int l;
    out_func *mof;
    visit_func *pr;


    // some default values
    mult_up = 100000; 
    est_r = 1;
    DFILE = stderr;

    while ((option = getopt(argc, argv, "i:m:rs:DBo:X::f:t:w:R:p")) != EOF) {
        switch (option) {
        case 'm' :
            sscanf(optarg,"%lf",&mult_up);
            DEBEXP(-4,mult_up);
            break;
        case 's':
            seed = atol(optarg);
            shuffle = 1;
            break;
        case 'D':
            defer = 1; 
            break;
        case 'B':
            bad = 1; 
            break;
        case 'i' :
            strcpy(ifile, optarg);
            break;
        case 'X' : 
            DFILE = efopen(optarg, "w");
            break;
        case 'f' :
            main_out_form = set_out_func(optarg);
            break;
        case 'o': switch (optarg[0]) {
        case 'o': output=1; break;
        case 'N': output=1; break; // output is never set to zero
        case 'v': vd_new = vol = 1; break;
        case 'h': hist = 1; break;
        case 'F': strcpy(ofile, optarg+1); break;
        default: errline("illegal output option");
            exit(1);
        }
        break;
        case 't':sscanf(optarg,"%lf",&theta);
            break;
        case 'w':sscanf(optarg,"%lf",&deep);
            break;   
        case 'R':sscanf(optarg,"%lf",&est_r);
            break;   
        case 'p':poleInput=1;
            break; 

        default :
            tell_options();
            exit(1);
        }
    }
    AXIS=fopen("axis","w");
    AXISFACE=fopen("axisface","w");
    POLE=fopen("pole","w");
    TFILE = efopen(tmpnam(tmpfilenam), "w");

    if (!poleInput)  {
        ifn = (strlen(ifile)!=0); 
        INFILE = ifn ? efopen(ifile, "r") : stdin;
        fprintf(DFILE, "reading from %s\n", ifn ? ifile : "stdin");

        ofn = (strlen(ofile)!=0);

        strcpy(ofilepre, ofn ? ofile : (ifn ? ifile : "hout") );

        if (output) {
            if (ofn && main_out_form > 0) {
                strcat(ofile, "."); 
                strcat(ofile, output_forms[main_out_form]);
            }
            OUTFILE = ofn ? efopen(ofile, "w") : stdout;
            fprintf(DFILE, "main output to %s\n", ofn ? ofile : "stdout");
        } else fprintf(DFILE, "no main output\n");

        
        read_next_site(-1);
        fprintf(DFILE,"dim=%d\n",dim);fflush(DFILE); 
        if (dim > MAXDIM) panic("dimension bound MAXDIM exceeded"); 

        point_size = site_size = sizeof(Coord)*dim;

    
        fprintf(DFILE, "reading sites...");
        for (num_sites=0; read_next_site(num_sites); num_sites++);
        fprintf(DFILE,"done; num_sites=%ld\n", num_sites);fflush(DFILE);
        read_bounding_box(num_sites);
        num_sites += 8; 
        fprintf(DFILE,"shuffling...");
        init_rand(seed);
        make_shuffle();
        shuf = &shufflef;
        get_site_n = get_site_offline;
    

        // Step 1: compute DT of input point set
        root = build_convex_hull(get_next_site, site_numm, dim, vd_new);

        // Step 2: Find poles
        pole1 = (struct simplex **) calloc(num_sites, sizeof(struct simplex *));    
        pole2 = (struct simplex **) calloc(num_sites, sizeof(struct simplex *));    
        lfs_lb = (double*) calloc(num_sites, sizeof(double));

        fprintf(DFILE, "done\n"); fflush(DFILE);

        //    rverts = select_random_points(num_sites); 
        //    fprintf(DFILE, "selecing random points\n");

        mof = out_funcs[main_out_form];
        pr = facets_print;
        
        if  (main_out_form==0) echo_command_line(OUTFILE,argc,argv);

        exactinit(); // Shewchuk's exact arithmetic initialization

        pr = compute_vv;
        fprintf(DFILE, "Computing Voronoi vertices and 1st poles....\n");
        make_output(root, visit_hull, pr, mof, OUTFILE);
               
        pr = compute_pole2;
        fprintf(DFILE, "\n\n\ncomputing 2nd poles....\n");
        make_output(root, visit_hull, pr, mof, OUTFILE);
     

        // poles with weights. Input to regular triangulation
        SPFILE = fopen("sp","w");  

        //  fprintf(POLE,"%s \n","OFF");
      
        // initialize the sample distance info for the poles

        pole1_distance=(double *) malloc(num_sites*sizeof(double));
        pole2_distance=(double *) malloc(num_sites*sizeof(double));

        compute_distance(pole1,num_sites-8,pole1_distance);
        compute_distance(pole2,num_sites-8,pole2_distance);




        // intialize list of lists of pointers to opposite poles
        opplist = (struct plist**) calloc(num_sites*2, sizeof(struct plist *));

        // data about poles; adjacencies, labels, radii
        adjlist = (struct polelabel *) calloc(num_sites*2, sizeof(struct polelabel));
     
        // loop through sites, writing out poles
        for (i=0;i<num_sites-8;i++) { 

            // rescale the sample to real input coordinates
            for (k=0; k<3; k++) 
                samp[k] = get_site_offline(i)[k]/mult_up;

            // output poles, both to debugging file and for weighted DT
            // remembers sqaured radius
            if ((pole1[i]!=NULL)&&(pole1[i]->status != POLE_OUTPUT)) {
                // if second pole is closer than we think it should be...
                if ((pole2[i]!=NULL) && bad &&
                    close_pole(samp,pole2[i]->vv,lfs_lb[i])) {
                    numbadpoles++;
                } 
                else {
                    outputPole(POLE,SPFILE,pole1[i],poleid++,samp,&num_poles,pole1_distance[i]);
                }
            }

            if ( (pole2[i]!=NULL) && (pole2[i]->status != POLE_OUTPUT)) {

                // if pole is closer than we think it should be...
                if (close_pole(samp,pole2[i]->vv,lfs_lb[i])) {
                    // remember opposite bad for late labeling
                    if (!bad) adjlist[pole1[i]->poleindex].bad = BAD_POLE;
                    numbadpoles++;
                    continue;
                }
   
                // otherwise...
                outputPole(POLE,SPFILE,pole2[i],poleid++,samp,&num_poles,pole2_distance[i]);
            }

            // keep list of opposite poles for later coloring
            if ((pole1[i]!=NULL)&&(pole2[i]!=NULL)&&
                (pole1[i]->status == POLE_OUTPUT) &&
                (pole2[i]->status == POLE_OUTPUT)) {

                pole_angle = 
                    computePoleAngle(pole1[i],pole2[i],samp);

                newOpposite(pole1[i]->poleindex,
                            pole2[i]->poleindex,pole_angle);
                newOpposite(pole2[i]->poleindex,
                            pole1[i]->poleindex,pole_angle);
            }
        }
        efclose(POLE);
        efclose(SPFILE);
        fprintf(DFILE,"bad poles=%d\n",numbadpoles);

        free_hull_storage(); 
  
    } // do this if the input was a set of samples not poles

    //  mult_up = mult_up1;  set the multiplier for the 2nd Delaunay
    //  TFILE = fopen("temp", "w"); 
    //    TFILE = efopen(tmpnam(tmpfilenam), "w");

    else { 
        // read data from the input file and put it in sp
        // initialize adjlist to the right size and initialize labels
        INFILE=fopen(ifile,"r");
        SPFILE=fopen("sp","w");
      

        fprintf(DFILE,"%s",ifile);
        mof = out_funcs[main_out_form];
        pr = facets_print;
        OUTFILE=stdout;

        fscanf(INFILE,"%d",&num_poles);// get the number of poles ..
        adjlist = (struct polelabel *) calloc(num_poles, sizeof(struct polelabel));
        for(i=0;i<num_poles;i++) {
     
            //   fscanf(INFILE,"%e\s%e\s%e\s%e\s%d\s%e\n",&x,&y,&z,&r,&l,&d);
     
            fscanf(INFILE,"%le ",&x);
            fscanf(INFILE,"%le ",&y);
            fscanf(INFILE,"%le ",&z);
            fscanf(INFILE,"%le ",&r);
            fscanf(INFILE,"%d ",&l);
            fscanf(INFILE,"%le ",&d);
        
            // we  have the square of the radius

            fprintf(SPFILE,"%f %f %f %f\n",x,y,z,SQ(x)+SQ(y)+SQ(z)-r);
            fprintf(POLE,"%f %f %f \n",x,y,z);
            //  fprintf(DFILE,"%f %f %f %d\n",x,y,z,num_poles);
            adjlist[i].sqradius=r;
            adjlist[i].label=l;

        }

        efclose(INFILE);
        efclose(SPFILE);
        efclose(POLE);
    }

    

    power_diagram = 1;
    vd_new = 0; 
    dim = 4; 
      
    INFILE = fopen("sp","r");
    fprintf(DFILE,"num_blocks = %d\n",num_blocks);
    // for (i=0;i<num_blocks;i++) free(site_blocks[i]);

    num_blocks = 0;  
    s_num = 0; 
    scount = 0;
    read_next_site(-1);
    fprintf(DFILE,"dim=%d\n",dim); fflush(DFILE); 
    // if (dim > MAXDIM) panic("dimension bound MAXDIM exceeded");
    
    point_size = site_size = sizeof(Coord)*dim; 
    // save points in order read
    for (num_sites=0; read_next_site(num_sites); num_sites++);
     
    fprintf(DFILE,"done; num_sites=%ld\n", num_sites);fflush(DFILE);
    efclose(INFILE);

    // set up the shuffle
    fprintf(DFILE,"shuffling...");
    init_rand(seed);
    make_shuffle();
    shuf = &shufflef;
    get_site_n = get_site_offline;  // returns stored points, unshuffled
      
    // Compute weighted DT 
    root = build_convex_hull(get_next_site, site_numm, dim, vd_new);
     
    fprintf(DFILE,"scount=%d, s_num=%ld\n",scount,s_num);
    
    fprintf(DFILE, "done\n"); fflush(DFILE);
     
    // file of faces
    PNF = fopen("pnf","w");
    // file of points
    PC = fopen("pc","w");
    
    // compute adjacencies and find angles of ball intersections
    queue = NULL;
    pr = compute_3d_power_vv;
    make_output(root, visit_hull, pr, mof, OUTFILE);
      
    
    
    // Begin by labeling everything outside a big bounding box as outside
       
    // labeling
    if(!poleInput) { // if we dont have the labels
        fprintf(DFILE,"num_poles=%d\n",num_poles);
        init_heap(num_poles);
        for (i=0;i<num_poles;i++) { 
            if ((get_site_offline(i)[0]>(2*omaxs[0]-omins[0]))||
                (get_site_offline(i)[0]<(2*omins[0]-omaxs[0]))||
                (get_site_offline(i)[1]>(2*omaxs[1]-omins[1]))||
                (get_site_offline(i)[1]<(2*omins[1]-omaxs[1]))||
                (get_site_offline(i)[2]>(2*omaxs[2]-omins[2]))||
                (get_site_offline(i)[2]<(2*omins[2]-omaxs[2])))
            {
                adjlist[i].hid = insert_heap(i,1.0);
                adjlist[i].out = 1.0;
                adjlist[i].label = OUT;
            }
        }
    
        while (heap_size != 0) propagate();
      
        label_unlabeled(num_poles);

    }
     

      
    // Enough labeling; let's look at the poles and output a crust! 
    INPOLE = fopen("inpole","w");
    OUTPOLE = fopen("outpole","w"); 

    // for visualization of polar balls:
    INPBALL = fopen("inpball","w");  // inner poles with radii
    POLEINFO = fopen("tpoleinfo","w");
    

    for (i=0;i<num_poles;i++) {

        for (k=0; k<3; k++) 
            tmp_pt[k] = get_site_offline(i)[k]/mult_up;

        fprintf(POLEINFO,"%f %f %f %f %d %f \n ",tmp_pt[0],tmp_pt[1],tmp_pt[2],
                adjlist[i].sqradius,adjlist[i].label,adjlist[i].samp_distance);
            
        if ((adjlist[i].label != IN) && (adjlist[i].label != OUT)) {
            fprintf(DFILE,"pole %d label %d\n",i,adjlist[i].label);
        }
        else { 
         
            if (adjlist[i].label == IN) {
                fprintf(INPOLE,"%f %f %f\n",tmp_pt[0],tmp_pt[1],tmp_pt[2]); 
                fprintf(INPBALL,"%f %f %f %f \n",
                        tmp_pt[0],tmp_pt[1],tmp_pt[2],sqrt(adjlist[i].sqradius)); 
            }
            else if (adjlist[i].label == OUT) 
                fprintf(OUTPOLE,"%f %f %f\n",tmp_pt[0],tmp_pt[1],tmp_pt[2]); 
        
            eindex = adjlist[i].eptr;
            while (eindex!=NULL) {
                if ((i < eindex->pid) && 
                    (antiLabel(adjlist[i].label) == adjlist[eindex->pid].label))
                {
                    construct_face(eindex->simp,eindex->kth);
                }
                eindex = eindex->next;
            }
        }
    }
      
    efclose(PC);
    efclose(PNF);
    efclose(POLEINFO);

    // powercrust output done...
    HEAD = fopen("head","w");
    fprintf(HEAD,"OFF\n"); 
    fprintf(HEAD,"%d %d %d\n",num_vtxs,num_faces,0);
    efclose(HEAD);
    system("type head pc pnf > pc.off"); // TJH: 'type' was 'cat', changed for Windows compilation
    system("del head pc pnf"); // TJH: 'del' was 'rm', changed for Windows compilation

 
    // compute the medial axis
    pr=compute_axis;
    fprintf(DFILE,"\n\n computing the medial axis ....\n");
    make_output(root,visit_hull,pr,mof,OUTFILE);
      
    HEAD = fopen("head","w");
    fprintf(HEAD,"OFF\n"); 
    fprintf(HEAD,"%d %d %d\n",num_poles,num_axedgs,0);
    efclose(HEAD);
    efclose(AXIS);
    system("type head pole axis > axis.off"); // TJH: 'type' was 'cat', changed for Windows compilation
      
    HEAD=fopen("head","w");
    fprintf(HEAD,"%d %d \n", num_poles,num_axedgs);
    efclose(HEAD);

    system("type head tpoleinfo axis > poleinfo"); // TJH: 'type' was 'cat', changed for Windows compilation          

    
    HEAD = fopen("head","w");
    fprintf(HEAD,"OFF\n"); 
    fprintf(HEAD,"%d %d %d\n",num_poles,num_axfaces,0);
    efclose(HEAD);
    efclose(AXISFACE);
    system("type head pole axisface > axisface.off"); // TJH: 'type' was 'cat', changed for Windows compilation
    system("del head pole axis axisface tpoleinfo sp"); // TJH: 'del' was 'rm', changed for Windows compilation

    // power shape output done
    
    efclose(INPOLE);
    efclose(OUTPOLE);
    efclose(INPBALL);
    efclose(TFILE);
    free(adjlist);
     
        
    
    free_hull_storage();
    efclose(DFILE);
    exit(0);
}*/






/* for each pole array, compute the maximum of the distances on the sample */


void compute_distance(simplex** poles,int size,double* distance) {

    int i,j,k,l;
    double indices[4][3]; /* the coords of the four vertices of the simplex*/
    point v[MAXDIM]; 
    simplex* currSimplex;

 
 
    double maxdistance=0;
    double currdistance;

    for(l=0;l<size;l++) {  /* for each pole do*/

        if(poles[l]!=NULL) {
            currSimplex=poles[l];
 
   


            /* get the coordinates of the  four endpoints */
            for(j=0;j<4;j++) {
                v[j]=currSimplex->neigh[j].vert;
      
                for(k=0;k<3;k++)
                    indices[j][k]=v[j][k]/mult_up; 

    
            }

            /* now compute the actual distance  */
            maxdistance=0;

            for(i=0;i<4;i++) {
                for(j=i+1;j<4;j++) {
                    currdistance= SQ(indices[i][0]-indices[j][0]) +
                        SQ(indices[i][1]-indices[j][1])+ SQ(indices[i][2]-indices[j][2]); 
                    currdistance=sqrt(currdistance);
                    if(maxdistance<currdistance) 
                        maxdistance=currdistance;
                }
            }

            distance[l]=maxdistance;

        } 

    }
}
//========power.c=============================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <float.h>
#include <math.h>

/*
 * Power Crust software, by Nina Amenta, Sunghee Choi and Ravi Krishna Kolluri.
 * Copyright (c) 2000 by the University of Texas
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee under the GNU Public License is hereby granted, 
 * provided that this entire notice  is included in all copies of any software 
 * which is or includes a copy or modification of this software and in all copies 
 * of the supporting documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

//#include "hull.h"   TJH: this file is now above

extern double theta;
extern FILE /* TJH *PC, *PNF,*/ *INFPOLE/*,*AXIS TJH ,*AXISFACE*/; /* *RT, *PS, *PR; */
int numvtxs=0, numfaces=0;
extern int num_vtxs,num_faces;

extern double theta;
extern struct polelabel *adjlist;
extern struct queuenode *queue;
extern long site_numm(site p);
extern void triorthocenter(double a[], double b[], double c[],
                           double orthocenter[], double* cnum);
extern void tetorthocenter(double a[], double b[], double c[], double d[],
                           double orthocenter[], double* cnum);

/* some new variables */

extern int num_poles,num_axedgs,num_axfaces;

int v1[6]={0,0,0,1,1,2};
int v2[6]={1,2,3,2,3,3};
int v3[6]={2,3,1,3,0,0};
int v4[6]={3,1,2,0,2,1};


void *compute_2d_power_vv(simplex *s, void *p) { 
    /* computes Voronoi vertices  */

  static out_func *out_func_here;
  point v[MAXDIM];
  int j,k,inf=0, index;
    double cc[2], cond, ta[3][3];
    
  if (p) {out_func_here = (out_func*)p; if (!s) return NULL;}

  index = 0;
  for (j=0;j<3;j++) {
        v[j] = s->neigh[j].vert; 
        /* v[j] stores coordinates of j'th vertex of simplex s; j=0..3 */ 
        if (v[j]==infinity) { /* means simplex s is on the convex hull */
            inf=1;
            continue; /* skip the rest of the for loop; process next vertex */
        }  
        /*i=(site_num)(v[j]); i is the index of the vertex v[j] */
        for (k=0;k<3;k++) {
            ta[index][k] = v[j][k]/mult_up;
            /* restore original coords   */
            /* if inf=1, ta[0],ta[1] are non-infinite vertices of s*/
            /*    inf=0, ta[0],ta[1],ta[2] are 3 vertices of s     */
        }
        index++;
  }
  printf("\n");
  if (!inf) { /* if not faces on convex hull, compute circumcenter*/
        for (k=0;k<3;k++)
            /*    printf("%f %f %f\n",ta[k][0],ta[k][1],ta[k][2]);*/
            triorthocenter(ta[0], ta[1], ta[2], cc, &cond);   
        /* cc is the displacement of orthocenter from ta[0] */
        /* cond is the denominator ( orient2d ) value        */
        if (cond!=0) { /* ignore them if cond = 0 */
      s->vv = (Coord*) malloc(sizeof(Coord)*2);
      for (k=0;k<2;k++) {
                s->vv[k] = ta[0][k]+cc[k];  
      }
      s->status = VV;
      
        }
        else { /* if cond=0, s is SLIVER */
            fprintf(DFILE,"sliver!\n");
            s->vv = NULL;
            s->status = SLV;
        }
  }
  else { /* if on conv hull, ignore */
    s->vv = NULL;
    s->status = CNV;
  }   

  return NULL;
}

/*
void *reg_triang(simplex *s, void *p) { 

  static out_func *out_func_here;
  point v[MAXDIM];
  int j,k,vnum;
        double cc[3], cond, ta[4][4];
    
  if (p) {out_func_here = (out_func*)p; if (!s) return NULL;}

  for (j=0;j<cdim;j++) {
    v[j] = s->neigh[j].vert; 
      
    if (v[j]==infinity) { 
      return NULL; 
    }  
  }

  for (j=0;j<cdim;j++) {
                vnum=0;
                for (k=0;k<cdim;k++) {
                        if (k==j) continue;
                        v[vnum++] = (s->neigh[k].vert);
                }
           fprintf(RT,"3 %d %d %d\n",(site_num)(v[0]),(site_num)(v[1]),(site_num)(v[2])); 
        }
        return NULL; 
}
*/


void *compute_3d_power_vv(simplex *s, void *p) { 

  static out_func *out_func_here;
  point v[MAXDIM];
  int j,k,inf=0, index, visited_edge;
    double cc[3], cond, ta[4][4], d, r1, r2, e;
  struct edgesimp *newplist, *pindex;

  if (p) {
        out_func_here = (out_func*)p; 
        if (!s) return NULL;
  }

  index = 0;
  for (j=0;j<cdim;j++) {
        v[j] = s->neigh[j].vert; 
        /* v[j] stores coordinates of j'th vertex of simplex s; j=0..3 */ 
        if (v[j]==infinity) { /* means simplex s is on the convex hull */
            inf=1;
            continue; /* skip the rest of the for loop; process next vertex */
        }  
        /*i=(site_num)(v[j]);  i is the index of the vertex v[j] */
        for (k=0;k<4;k++) {
            ta[index][k] = v[j][k]/mult_up; /* restore original coords   */
            /* if inf=1, ta[0],ta[1],ta[2] are non-infinite vertices of s*/
            /*    inf=0, ta[0],ta[1],ta[2],ta[3] are 4 vertices of s     */
        }
        index++;
  }
 
  /* if not faces on convex hull, process */
  if (!inf) { 

        /* build structure for each edge, including angle of intersection */
        for (k=0;k<6;k++) { 
            if (s->edgestatus[k]==FIRST_EDGE) { /* not visited edge */
                pindex = adjlist[site_numm(v[v1[k]])].eptr;
                visited_edge = 0;
                while (pindex!= NULL) {
                    if (pindex->pid == site_numm(v[v2[k]])) { /* already in the list */
                        visited_edge = 1;
                        break;
                    }
                    pindex = pindex->next;
                }
  
                if (!visited_edge) {
                    d = sqdist(ta[v1[k]],ta[v2[k]]);
                    r1 = SQ(ta[v1[k]][0])+SQ(ta[v1[k]][1])+SQ(ta[v1[k]][2])-ta[v1[k]][3];
                    r2 = SQ(ta[v2[k]][0])+SQ(ta[v2[k]][1])+SQ(ta[v2[k]][2])-ta[v2[k]][3];
                    e = 2 * sqrt(r1) * sqrt(r2);

                    newplist = (struct edgesimp *) malloc(sizeof(struct edgesimp));
                    newplist->simp = s;
                    newplist->kth = k;
                    newplist->angle = (r1+r2-d)/e;
                    newplist->pid = site_numm(v[v1[k]]);
                    newplist->next = adjlist[site_numm(v[v2[k]])].eptr;
                    adjlist[site_numm(v[v2[k]])].eptr = newplist;

                    newplist = (struct edgesimp *) malloc(sizeof(struct edgesimp));
                    newplist->simp = s;
                    newplist->kth = k;
                    newplist->angle = (r1+r2-d)/e;
                    newplist->pid = site_numm(v[v2[k]]);
                    newplist->next = adjlist[site_numm(v[v1[k]])].eptr;
                    adjlist[site_numm(v[v1[k]])].eptr = newplist;

                    s->edgestatus[k] = VISITED;
                }
            }
        }

        tetorthocenter(ta[0], ta[1], ta[2], ta[3], cc, &cond);   
        /* cc is the displacement of orthocenter from ta[0] */
        /* cond is the denominator ( orient2d ) value        */
        if (cond!=0) { /* ignore them if cond = 0 */
      s->vv = (Coord*) malloc(sizeof(Coord)*3);
      for (k=0;k<3;k++) {
                s->vv[k] = ta[0][k]+cc[k];  
      }
      s->status = VV;      
      
        }
        else { /* if cond=0, s is SLIVER */
            fprintf(DFILE,"sliver!\n");
            s->vv = NULL;
            s->status = SLV;
        }
  }
  else { /* if on conv hull, ignore */
    s->vv = NULL;
    s->status = CNV;
  }   

  return NULL;
}

void *compute_3d_power_edges(simplex *s, void *p) { 

  static out_func *out_func_here;
  point v[MAXDIM];
  int j, k, inf=0, numedges, ns, l, nedge0, nedge1, nremv, nnextv, l1, l2, nk;
  site edge0, edge1, nextv, remv, prevv;
    double ta[4][4], r1, r2, d, e;
  simplex *prevs, *nexts;
        
  if (p) {out_func_here = (out_func*)p; if (!s) return NULL;}

  
  if ((s->status == CNV)||(s->status == SLV)) return NULL; /* skip inf faces */
  for (j=0;j<cdim;j++) {
        v[j] = s->neigh[j].vert; 
        for (k=0;k<4;k++) {
            ta[j][k] = v[j][k]/mult_up; /* restore original coords   */
        }
  }
 
  if (!inf) {
        for (k=0;k<6;k++) { /* for each edge */
            if (s->edgestatus[k]==FIRST_EDGE) { /* not visited edge */
         
                /* check the dihedral angle */
                d = sqdist(ta[v1[k]],ta[v2[k]]);
                r1 = SQ(ta[v1[k]][0])+SQ(ta[v1[k]][1])+
                    SQ(ta[v1[k]][2])-ta[v1[k]][3];
                r2 = SQ(ta[v2[k]][0])+SQ(ta[v2[k]][1])+
                    SQ(ta[v2[k]][2])-ta[v2[k]][3];
                e = 2 * sqrt(r1) * sqrt(r2);
                if ((d >= (r1+r2+e)) || ((d-r1-r2)/e > theta )) {
                    /* fprintf(DFILE,"%f\n",(d-r1-r2)/e);*/
                    /* edge0, edge1 are the vertices of the edge */
                    edge0 = s->neigh[v1[k]].vert;
                    edge1 = s->neigh[v2[k]].vert;
                    nextv = s->neigh[v3[k]].vert;
                    /* nextv is the opposite vtx of the next simplex */
                    remv = s->neigh[v4[k]].vert;
                    /* remv is a vtx of the next simplex with edge0, edge1 */
                    prevv = remv;
                    /* prevv is the vtx shared by prevs and nexts besides edge0, edge1 */

                    /* construct its dual power face */
                    s->edgestatus[k]=POW;
      
                    /* visit the next simplex */
                    /* print orthocenter of s->neigh[v3[k]].simp ...*/
                    prevs = s;
                    nexts = s->neigh[v3[k]].simp;
      
                    ns = v3[k];
                    numedges=0;
                    while (nexts != s) {
                        if (nexts->status == CNV) {
                            fprintf(DFILE,"inf reg face\n");
                            break;
                        }
                        else {
                            // TJH: PC contains the points for the powercrust surface
                            // so we hijack the data and pipe it to our structure
                            {
                                float vp[3];
                                vp[0]=prevs->vv[0];
                                vp[1]=prevs->vv[1];
                                vp[2]=prevs->vv[2];
                                vtk_output->GetPoints()->InsertNextPoint(vp);
                            }
                            /* TJH: we have bypassed the file routines, so we don't need this bit
                            fprintf(PC,"%f %f %f\n",prevs->vv[0],prevs->vv[1],prevs->vv[2]);*/
                            numedges++;numvtxs++;
                            /* find edgenumber k of nexts for this edge */
                            for (l=0;l<4;l++) {
                                if (nexts->neigh[l].vert==edge0) {
                                    /* l == v1[k] */
                                    nedge0 = l;continue;
                                }
                                else if (nexts->neigh[l].vert==edge1) {
                                    /* l == v2[k] */
                                    nedge1 = l;continue;
                                }
                                else if (nexts->neigh[l].vert==prevv) {
                                    nremv = l;continue;
                                }
                                else if (nexts->neigh[l].vert==nextv) {
                                    nnextv = l;
                                    continue; 
       
                                }
                                else {
                                    nnextv = l;
                                }
                            }
          
                            if (nedge0 > nedge1) { l1 = nedge1; l2 = nedge0; }
                            else { l2 = nedge1; l1 = nedge0; }
                            if (l1==0) {
                                if (l2==1) nk = 0;
                                else if (l2==2) nk = 1;
                                else nk = 2;
                            }
                            else if (l1==1) {
                                if (l2==2) nk = 3;
                                else nk = 4;
                            }
                            else nk = 5;  
                            /* found nk for the edge */
                            nexts->edgestatus[nk]=POW; /* record that it's visited */
                            /* visit next simplex (opposite vertex ns )*/
                            prevs = nexts;
                            prevv = nexts->neigh[nnextv].vert;
                            nexts = nexts->neigh[nremv].simp;
                        }
                    }
                    // TJH: PC contains the points for the powercrust surface
                    // so we hijack the data and pipe it to our structure
                    {
                        float vp[3];
                        vp[0]=prevs->vv[0];
                        vp[1]=prevs->vv[1];
                        vp[2]=prevs->vv[2];
                        vtk_output->GetPoints()->InsertNextPoint(vp);
                    }
                    /* TJH: we have bypassed the file routines, so we don't need this bit
                    fprintf(PC,"%f %f %f\n", prevs->vv[0], prevs->vv[1], prevs->vv[2]);*/
                    numedges++;numvtxs++;

                    // TJH: PNF contains the polygons (indices of their vertices)
                    // so we hijack the data and pipe it to our waiting vtkPolyData
                    // create the array of indices
                    vtk_output->GetPolys()->InsertNextCell(numedges);

                    /* TJH: we have bypassed the file routines, so we don't need this bit
                    fprintf(PNF,"%d ",numedges);*/
                    for (l=numedges;l>0;l--) {
                        /* TJH: we have bypassed the file routines, so we don't need this bit
                        fprintf(PNF, "%d ",numvtxs-l);*/
                        // TJH: insert the next vertex index
                        vtk_output->GetPolys()->InsertCellPoint(numvtxs-l);
                    }
                    /* TJH: we have bypassed the file routines, so we don't need this bit
                    fprintf(PNF,"\n");numfaces++;*/
                }
                else { 
                    s->edgestatus[k]=NOT_POW;
                }
            }        /* skip if the edge is visited before */
        }
  }
  /* ignore inf faces */

  return NULL;

}

/* the function for computing the medial axis */

void *compute_axis (simplex *s, void *p) { 

  static out_func *out_func_here;
  point v[MAXDIM];
  point  point1,point2;
  int pindex,qindex;
  int edgedata[6];
  int indices[6]; /* store the indices */
    int j, k, inf=0;
  
    double ta[4][4];
        
  if (p) {out_func_here = (out_func*)p; if (!s) return NULL;}

  
  if ((s->status == CNV)||(s->status == SLV)) return NULL; /* skip inf faces */
  for (j=0;j<cdim;j++) {
        v[j] = s->neigh[j].vert; 
        for (k=0;k<4;k++) {
            ta[j][k] = v[j][k]/mult_up; /* restore original coords   */
        }
  }
 
  if (!inf) {
        for (k=0;k<6;k++) { /* for each edge */
            edgedata[k]=0;
            if ((s->edgestatus[k]!=POW) ) { /* not dual to a power  face  */
         
  
         
                point1 = v[v1[k]];
                point2 = v[v2[k]];
                pindex=site_numm(point1);
                qindex=site_numm(point2);
                if(adjlist[pindex].label==IN && adjlist[qindex].label==IN) 
                {
                    if(s->edgestatus[k]!=ADDAXIS) {
                        num_axedgs++;  
                        /* TJH: we have bypassed the file routines, so we don't need this bit
                        fprintf(AXIS,"2 %d %d \n ",pindex,qindex);*/
                    }
                    edgedata[k]=VALIDEDGE;
                    indices[v1[k]]=pindex ;
                    indices[v2[k]]=qindex ;
                    s->edgestatus[k]=ADDAXIS;
        
      
                }
                /* now start adding triangles if present */
            }
        }
   
        if((edgedata[0]==VALIDEDGE)&& (edgedata[1]==VALIDEDGE)
           && (edgedata[3]==VALIDEDGE))
        {

            /* TJH: we have bypassed the file routines, so we don't need this bit
            fprintf(AXIS,"3 %d %d %d \n",indices[v1[0]],
                    indices[v2[1]],indices[v1[3]]);*/

            /* TJH: we have bypassed the file routines, so we don't need this bit
            fprintf(AXISFACE,"3 %d %d %d \n",indices[v1[0]],
                    indices[v2[1]],indices[v1[3]]);*/
            // TJH: instead we pipe the data straight to our vtk structure
            {
                vtk_medial_surface->GetPolys()->InsertNextCell(3);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v1[0]]);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v2[1]]);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v1[3]]);
            }

            num_axedgs++;
            num_axfaces++;
        }
        if((edgedata[1]==VALIDEDGE)&& (edgedata[2]==VALIDEDGE)
           && (edgedata[5]==VALIDEDGE))
        {
            /* TJH: we have bypassed the file routines, so we don't need this bit
            fprintf(AXIS,"3 %d %d %d \n",indices[v1[1]],
                    indices[v2[2]],indices[v1[5]]);*/

            /* TJH: we have bypassed the file routines, so we don't need this bit
            fprintf(AXISFACE,"3 %d %d %d \n",indices[v1[1]],
                    indices[v2[2]],indices[v1[5]]);*/
            // TJH: instead we pipe the data straight to our vtk structure
            {
                vtk_medial_surface->GetPolys()->InsertNextCell(3);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v1[1]]);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v2[2]]);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v1[5]]);
            }

            num_axedgs++;
            num_axfaces++;

        }
        if((edgedata[0]==VALIDEDGE)&& (edgedata[2]==VALIDEDGE)
           && (edgedata[4]==VALIDEDGE))
        {
            /* TJH: we have bypassed the file routines, so we don't need this bit
            fprintf(AXIS,"3 %d %d %d \n",indices[v1[0]],
                    indices[v2[2]],indices[v1[4]]);*/

            /* TJH: we have bypassed the file routines, so we don't need this bit
            fprintf(AXISFACE,"3 %d %d %d \n",indices[v1[0]],
                    indices[v2[2]],indices[v1[4]]);*/
            // TJH: instead we pipe the data straight to our vtk structure
            {
                vtk_medial_surface->GetPolys()->InsertNextCell(3);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v1[0]]);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v2[2]]);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v1[4]]);
            }

            num_axedgs++;
            num_axfaces++;
        }
        if((edgedata[3]==VALIDEDGE)&& (edgedata[4]==VALIDEDGE)
           && (edgedata[5]==VALIDEDGE))
        {
            /* TJH: we have bypassed the file routines, so we don't need this bit
            fprintf(AXIS,"3 %d %d %d \n",indices[v1[3]],
                    indices[v2[4]],indices[v1[5]]);*/

            /* TJH: we have bypassed the file routines, so we don't need this bit
            fprintf(AXISFACE,"3 %d %d %d \n",indices[v1[3]],
                    indices[v2[4]],indices[v1[5]]);*/
            // TJH: instead we pipe the data straight to our vtk structure
            {
                vtk_medial_surface->GetPolys()->InsertNextCell(3);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v1[3]]);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v2[4]]);
                vtk_medial_surface->GetPolys()->InsertCellPoint(indices[v1[5]]);
            }

            num_axedgs++;
            num_axfaces++;
        }
    
       
         
    

  }
  return NULL;
}



  
  


/* To print out powercrust faces */ 
void construct_face(simplex *s, short k)
{
    site edge0, edge1, nextv, remv, prevv,outsite,insite; 
    simplex *prevs, *nexts;
    int j, numedges, l1, l2, nk, l, ns, nedge0, nedge1, nremv, nnextv, i;
    char cface[200];
    char indface[1024][32];  /* the indices of the face */

    double plane[3][3];
    double outpole[3],inpole[3];

    cface[0] = '\0';
    edge0 = s->neigh[v1[k]].vert;
    edge1 = s->neigh[v2[k]].vert;

    if(adjlist[site_numm(edge0)].label==OUT) {
        outsite=edge0;
        insite=edge1;
    }
    else {
        outsite=edge1;
        insite=edge0;
    }

    for(j=0;j<3;j++){
        outpole[j]=outsite[j]/mult_up;
        inpole[j]=insite[j]/mult_up;
    }
    

    nextv = s->neigh[v3[k]].vert;
    /* nextv is the opposite vtx of the next simplex */
    remv = s->neigh[v4[k]].vert;
    /* remv is a vtx of the next simplex with edge0, edge1 */
    prevv = remv;
    /* prevv is the vtx shared by prevs and nexts besides edge0, edge1 */

    /* construct its dual power face */
    s->edgestatus[k]=POW;
      
    /* visit the next simplex */
    /* print orthocenter of s->neigh[v3[k]].simp ...*/
    prevs = s;
    nexts = s->neigh[v3[k]].simp;
      
    ns = v3[k];
    numedges=0;
    while (nexts != s) {
        if (nexts->status == CNV) {
            fprintf(DFILE,"inf reg face\n");
            break;
        }
        else {
            if (prevs->status != POLE_OUTPUT) {
                /* this vertex is not yet output */
                prevs->status = POLE_OUTPUT;
                prevs->poleindex = num_vtxs++;
                // TJH: PC contains the points for the powercrust surface
                // so we hijack the data and pipe it to our structure
                {
                    float vp[3];
                    vp[0]=prevs->vv[0];
                    vp[1]=prevs->vv[1];
                    vp[2]=prevs->vv[2];
                    vtk_output->GetPoints()->InsertNextPoint(vp);
                }
                /* TJH: we have bypassed the file routines, so we don't need this bit
                fprintf(PC,"%f %f %f\n",prevs->vv[0],prevs->vv[1],prevs->vv[2]);*/
            }

            if(numedges<3){
                plane[numedges][0]=prevs->vv[0];
                plane[numedges][1]=prevs->vv[1];
                plane[numedges][2]=prevs->vv[2];
            }

            sprintf(indface[numedges],"%ld ",prevs->poleindex);
            /*   strcat(cface,tempface);*/
            numedges++;
            /* find edgenumber k of nexts for this edge */
            for (l=0;l<4;l++) {
                if (nexts->neigh[l].vert==edge0) {
                    /* l == v1[k] */
                    nedge0 = l;continue;
                }
                else if (nexts->neigh[l].vert==edge1) {
                    /* l == v2[k] */
                    nedge1 = l;continue;
                }
                else if (nexts->neigh[l].vert==prevv) {
                    nremv = l;continue;
                }
                else if (nexts->neigh[l].vert==nextv) {
                    /*  if (nexts->neigh[nremv].simp == s) { */
                    nnextv = l;
                    continue; 
                    /*}
                      else fprintf(DFILE,"cannot happen l=%d!!\n",l); */
                }
                else {
                    nnextv = l;
                }
            }
        
            if (nedge0 > nedge1) { l1 = nedge1; l2 = nedge0; }
            else { l2 = nedge1; l1 = nedge0; }
            if (l1==0) {
                if (l2==1) nk = 0;
                else if (l2==2) nk = 1;
                else nk = 2;
            }
            else if (l1==1) {
                if (l2==2) nk = 3;
                else nk = 4;
            }
            else nk = 5;  
            /* found nk for the edge */
            nexts->edgestatus[nk]=POW; /* record that it's visited */
            /* visit next simplex (opposite vertex ns )*/
            prevs = nexts;
            prevv = nexts->neigh[nnextv].vert;
            nexts = nexts->neigh[nremv].simp;
        }
    }
      
    if (prevs->status != POLE_OUTPUT) {
        prevs->status = POLE_OUTPUT;
        prevs->poleindex = num_vtxs++;
        // TJH: PC contains the points for the powercrust surface
        // so we hijack the data and pipe it to our structure
        {
            float vp[3];
            vp[0]=prevs->vv[0];
            vp[1]=prevs->vv[1];
            vp[2]=prevs->vv[2];
            vtk_output->GetPoints()->InsertNextPoint(vp);
        }
        /* TJH: we have bypassed the file routines, so we don't need this bit
        fprintf(PC,"%f %f %f\n", prevs->vv[0], prevs->vv[1], prevs->vv[2]);*/
    }
      
    if(numedges<3) {
        plane[numedges][0]=prevs->vv[0];
        plane[numedges][1]=prevs->vv[1];
        plane[numedges][2]=prevs->vv[2];
        
    }
    sprintf(indface[numedges],"%ld ",prevs->poleindex);
     
    numedges++;

    // TJH: PNF contains the polygons (indices of their vertices)
    // so we hijack the data and pipe it to our waiting vtkPolyData
    // create the array of indices
    vtk_output->GetPolys()->InsertNextCell(numedges);

    /* TJH: we have bypassed the file routines, so we don't need this bit
    fprintf(PNF,"%d ",numedges);*/

    if(!correct_orientation(plane[0],plane[1],plane[2],inpole,outpole))
        for(i=numedges-1;i>=0;i--)
        {
            /* TJH: we have bypassed the file routines, so we don't need this bit
            fprintf(PNF,"%s ",indface[i]);*/
            // TJH: get the index of the vertex
            {
                // TJH: convert indface[i] to an integer (drove me mad for ages till I spotted this!)
                vtk_output->GetPolys()->InsertCellPoint(atoi(indface[i]));
            }
        }
    else
    {
        for(i=0;i<numedges;i++)
        {
            /* TJH: we have bypassed the file routines, so we don't need this bit
            fprintf(PNF,"%s ",indface[i]);*/
            // TJH: get the index of the vertex
            {
                // TJH: convert indface[i] to an integer (drove me mad for ages till I spotted this!)
                vtk_output->GetPolys()->InsertCellPoint(atoi(indface[i]));
            }
        }

    }
        
    /* TJH: we have bypassed the file routines, so we don't need this bit
    fprintf(PNF,"\n");*/
      
    num_faces++;
}



int correct_orientation(double *p1,double *p2,double *p3,double *inp,double *outp) {

    double normal[3];
    double v1[3],v2[3];
    double xcross,ycross,zcross;
    int numplus=0,numminus=0;

    normal[0]=outp[0]-inp[0];
    normal[1]=outp[1]-inp[1];
    normal[2]=outp[2]-inp[2];

    v1[0]=p2[0]-p1[0];
    v1[1]=p2[1]-p1[1];
    v1[2]=p2[2]-p1[2];
  
    v2[0]=p3[0]-p2[0];
    v2[1]=p3[1]-p2[1];
    v2[2]=p3[2]-p2[2];

    xcross=v1[1]*v2[2]-v1[2]*v2[1];
    ycross=v1[2]*v2[0]-v1[0]*v2[2];
    zcross=v1[0]*v2[1]-v1[1]*v2[0];

    if((xcross*normal[0]) > 0)
        numplus++;
    else
        numminus++;
  
  
    if((ycross*normal[1]) > 0)
        numplus++;
    else
        numminus++;

  
    if((zcross*normal[2]) > 0)
        numplus++;
    else
        numminus++;
  
    if(numplus > numminus)
        return 1;
    else
        return 0;

}
//========ch.c=============================================================
/* ch.c : numerical functions for hull computation */

/*
 * Ken Clarkson wrote this.  Copyright (c) 1995 by AT&T..
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */



#include <float.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>


//#include "hull.h"   TJH: this files is now above

extern short power_diagram;

short check_overshoot_f=0;


simplex *ch_root;

#define NEARZERO(d) ((d) < FLT_EPSILON && (d) > -FLT_EPSILON)
#define SMALL (100*FLT_EPSILON)*(100*FLT_EPSILON)

#define SWAP(X,a,b) {X t; t = a; a = b; b = t;}

#define DMAX 

double Huge;

#define check_overshoot(x)                          \
    {if (CHECK_OVERSHOOT && check_overshoot_f && ((x)>9e15))        \
        warning(-20, overshot exact arithmetic)}            \


#define DELIFT 0
int basis_vec_size;


#define lookupshort(a,b,whatb,c,whatc)                  \
{                                   \
    int i;                              \
    neighbor *x;                            \
    c = NULL;                           \
    for (i=-1, x = a->neigh-1; (x->whatb != b) && (i<cdim) ; i++, x++);\
    if (i<cdim) c = x->whatc;                   \
}                                   \

    
Coord Vec_dot(point x, point y) {
    int i;
    Coord sum = 0;
    for (i=0;i<rdim;i++) sum += x[i] * y[i];
    return sum;
}

Coord Vec_dot_pdim(point x, point y) {
    int i;
    Coord sum = 0;
    for (i=0;i<pdim;i++) sum += x[i] * y[i];
    /*  check_overshoot(sum); */
    return sum;
}

Coord Norm2(point x) {
    int i;
    Coord sum = 0;
    for (i=0;i<rdim;i++) sum += x[i] * x[i];
    return sum;
}

void Ax_plus_y(Coord a, point x, point y) {
    int i;
    for (i=0;i<rdim;i++) {
        *y++ += a * *x++;
    }
}

void Ax_plus_y_test(Coord a, point x, point y) {
    int i;
    for (i=0;i<rdim;i++) {
        check_overshoot(*y + a * *x);
        *y++ += a * *x++;
    }
}

void Vec_scale(int n, Coord a, Coord *x)
{
    register Coord *xx = x,
        *xend = xx + n;
    while (xx!=xend) *xx++ *= a;
}

void Vec_scale_test(int n, Coord a, Coord *x)
{
    register Coord *xx = x,
        *xend = xx + n  ;

    check_overshoot(a);

    while (xx!=xend) {
        *xx *= a;
        check_overshoot(*xx);
        xx++;
    }
}



int exact_bits;
float b_err_min, b_err_min_sq;

double logb(double); /* on SGI machines: returns floor of log base 2 */

static short vd;
static basis_s  tt_basis = {0,1,-1,0,0,{0}};
static basis_s *tt_basisp = &tt_basis, *infinity_basis;


STORAGE(basis_s)

    typedef Coord site_struct;

    Coord   infinity[10]={57.2,0,0,0,0}; /* point at infinity for vd; value not used */
    
    void print_site(site p, FILE* F)
{print_point(F, pdim,p);fprintf(F, "\n");}

#define VA(x) ((x)->vecs+rdim)
#define VB(x) ((x)->vecs)




/* tables for runtime stats */
int A[100]={0}, B[100] ={0}, C[100] = {0}, D[100] ={0};
int tot =0, totinf=0, bigt=0; 

#define two_to(x) ( ((x)<20) ? 1<<(x) : ldexp(1,(x)) )



double sc(basis_s *v,simplex *s, int k, int j) {
    /* amount by which to scale up vector, for reduce_inner */

    double      labound;
    static int  lscale;
    static double   max_scale,
        ldetbound,
        Sb;

    if (j<10) {
        labound = logb(v->sqa)/2;
        max_scale = exact_bits - labound - 0.66*(k-2)-1  -DELIFT;
        if (max_scale<1) {
            warning(-10, overshot exact arithmetic);
            max_scale = 1;

        }

        if (j==0) {
            int i;
            neighbor *sni;
            basis_s *snib;

            ldetbound = DELIFT;

            Sb = 0;
            for (i=k-1,sni=s->neigh+k-1;i>0;i--,sni--) {
                snib = sni->basis;
                Sb += snib->sqb;
                ldetbound += logb(snib->sqb)/2 + 1;
                ldetbound -= snib->lscale;
            }
        }
    }
    if (ldetbound - v->lscale + logb(v->sqb)/2 + 1 < 0) {
        DEBS(-2)
            DEBTR(-2) DEBEXP(-2, ldetbound)
            print_simplex_f(s, DFILE, &print_neighbor_full);
        print_basis(DFILE,v);
        EDEBS           
            return 0;                   
    } else {
        lscale = logb(2*Sb/(v->sqb + v->sqa*b_err_min))/2;  
        if (lscale > max_scale) {
            lscale = max_scale;
        } else if (lscale<0) lscale = 0;
        v->lscale += lscale;
        return two_to(lscale);
    }
}


double lower_terms(basis_s* v) {

    point vp = v->vecs;
    int i,j,h,hh=0;
    int facs[6] = {2,3,5,7,11,13};
    double out = 1;

    DEBTR(-10) print_basis(DFILE, v); printf("\n");
    DEBTR(0)

        for (j=0;j<6;j++) do {
            for (i=0; i<2*rdim && facs[j]*floor(vp[i]/facs[j])==vp[i];i++);
            if ((h = (i==2*rdim))!=0) {
                hh=1;
                out *= facs[j];
                for (i=0;i<2*rdim; i++) vp[i]/=facs[j];
            }
        } while (h);
    /*  if (hh) {DEBTR(-10)  print_basis(DFILE, v);} */
    return out;
}

double lower_terms_point(point vp) {

    int i,j,h,hh=0;
    int facs[6] = {2,3,5,7,11,13};
    double out = 1;

    for (j=0;j<6;j++) do {
        for (i=0; i<2*rdim && facs[j]*floor(vp[i]/facs[j])==vp[i];i++);
        if ((h = (i==2*rdim))!=0) {
            hh=1;
            out *= facs[j];
            for (i=0;i<2*rdim; i++) vp[i]/=facs[j];
        }
    } while (h);
    return out;
}


int reduce_inner(basis_s *v, simplex *s, int k) {

    point   va = VA(v),
        vb = VB(v);
    int i,j;
    double  dd;
    double  scale;
    basis_s *snibv;
    neighbor *sni;
    static int failcount;

    /*  lower_terms(v); */
    v->sqa = v->sqb = Norm2(vb);
    if (k<=1) {
        memcpy(vb,va,basis_vec_size);
        return 1;
    }
    /*  if (vd) {
        snibv = s->neigh[1].basis;
        scale = floor(sqrt(snibv->sqa/v->sqa));
        if (scale > 1) Vec_scale(rdim,scale,va);
        dd = Vec_dot(VA(snibv),va)/snibv->sqa;
        dd = -floor(0.5+dd);
        Ax_plus_y( dd, VA(snibv), va);
        }
    */      
    for (j=0;j<250;j++) {

        memcpy(vb,va,basis_vec_size);
        for (i=k-1,sni=s->neigh+k-1;i>0;i--,sni--) {
            snibv = sni->basis;
            dd = -Vec_dot(VB(snibv),vb)/ snibv->sqb;
            Ax_plus_y( dd, VA(snibv), vb);      
        }
        v->sqb = Norm2(vb);     
        v->sqa = Norm2(va);
        
        if (2*v->sqb >= v->sqa) {B[j]++; return 1;}

        Vec_scale_test(rdim,scale = sc(v,s,k,j),va);
        
        for (i=k-1,sni=s->neigh+k-1;i>0;i--,sni--) {
            snibv = sni->basis;
            dd = -Vec_dot(VB(snibv),va)/snibv->sqb; 
            dd = floor(dd+0.5);
            Ax_plus_y_test( dd, VA(snibv), va);
        }       
    }
    if (failcount++<10) {
        DEB(-8, reduce_inner failed on:)
            DEBTR(-8) print_basis(DFILE, v); 
        print_simplex_f(s, DFILE, &print_neighbor_full);
    }
    return 0;
}

#define trans(z,p,q) {int i; for (i=0;i<pdim;i++) z[i+rdim] = z[i] = p[i] - q[i];}
#define lift(z,s) {if (vd) z[2*rdim-1] =z[rdim-1]= ldexp(Vec_dot_pdim(z,z), -DELIFT);}
/*not scaling lift to 2^-DELIFT */



int reduce(basis_s **v, point p, simplex *s, int k) {

    point   z;
    point   tt = s->neigh[0].vert;

    if (!*v) NEWLRC(basis_s,(*v))
                 else (*v)->lscale = 0;
    z = VB(*v);
    if (vd || power_diagram ) {
        if (p==infinity) memcpy(*v,infinity_basis,basis_s_size);
        else {trans(z,p,tt); lift(z,s);}
    } else trans(z,p,tt);
    return reduce_inner(*v,s,k);
}


void get_basis_sede(simplex *s) {

    int k=1;
    neighbor *sn = s->neigh+1,
        *sn0 = s->neigh;

    if (( vd || power_diagram) && sn0->vert == infinity && cdim >1) {
        SWAP(neighbor, *sn0, *sn );
        NULLIFY(basis_s,sn0->basis);
        sn0->basis = tt_basisp;
        tt_basisp->ref_count++;
    } else {
        if (!sn0->basis) {
            sn0->basis = tt_basisp;
            tt_basisp->ref_count++;
        } else while (k < cdim && sn->basis) {k++;sn++;}
    }
    while (k<cdim) {
        NULLIFY(basis_s,sn->basis);
        reduce(&sn->basis,sn->vert,s,k);
        k++; sn++;
    }
}


int out_of_flat(simplex *root, point p) {

    static neighbor p_neigh={0,0,0};

    if (!p_neigh.basis) p_neigh.basis = (basis_s*) malloc(basis_s_size);

    p_neigh.vert = p;
    cdim++;
    root->neigh[cdim-1].vert = root->peak.vert;
    NULLIFY(basis_s,root->neigh[cdim-1].basis);
    get_basis_sede(root);
    if ((vd || power_diagram)&& root->neigh[0].vert == infinity) return 1;
    reduce(&p_neigh.basis,p,root,cdim);
    if (p_neigh.basis->sqa != 0) return 1;
    cdim--;
    return 0;
}


double cosangle_sq(basis_s* v,basis_s* w)  {
    double dd;
    point   vv=v->vecs,
        wv=w->vecs;
    dd = Vec_dot(vv,wv);
    return dd*dd/Norm2(vv)/Norm2(wv);
}


int check_perps(simplex *s) {

    static basis_s *b = NULL;
    point   z,y;
    point   tt;
    double  dd;
    int i,j;

    for (i=1; i<cdim; i++) if (NEARZERO(s->neigh[i].basis->sqb)) return 0;
    if (!b) {
        b = (basis_s*)malloc(basis_s_size);
        assert(b);
    }
    else b->lscale = 0;
    z = VB(b);
    tt = s->neigh[0].vert;
    for (i=1;i<cdim;i++) {
        y = s->neigh[i].vert;
        if ( (vd || power_diagram)&& y==infinity) memcpy(b, infinity_basis, basis_s_size);
        else {trans(z,y,tt); lift(z,s);}
        if (s->normal && cosangle_sq(b,s->normal)>b_err_min_sq) {DEBS(0)
                                                                     DEB(0,bad normal) DEBEXP(0,i) DEBEXP(0,dd)
                                                                     print_simplex_f(s, DFILE, &print_neighbor_full);
        EDEBS
            return 0;
        }
        for (j=i+1;j<cdim;j++) {
            if (cosangle_sq(b,s->neigh[j].basis)>b_err_min_sq) {
                DEBS(0)
                    DEB(0,bad basis)DEBEXP(0,i) DEBEXP(0,j) DEBEXP(0,dd)
                    DEBTR(-8) print_basis(DFILE, b);
                print_simplex_f(s, DFILE, &print_neighbor_full);
                EDEBS
                    return 0;
            }
        }
    }
    return 1;
}


void get_normal_sede(simplex *s) {

    neighbor *rn;
    int i,j;

    get_basis_sede(s);
    if (rdim==3 && cdim==3) {
        point   c,
            a = VB(s->neigh[1].basis),
            b = VB(s->neigh[2].basis);
        NEWLRC(basis_s,s->normal);
        c = VB(s->normal);
        c[0] = a[1]*b[2] - a[2]*b[1];
        c[1] = a[2]*b[0] - a[0]*b[2];
        c[2] = a[0]*b[1] - a[1]*b[0];
        s->normal->sqb = Norm2(c);
        for (i=cdim+1,rn = ch_root->neigh+cdim-1; i; i--, rn--) {
            for (j = 0; j<cdim && rn->vert != s->neigh[j].vert;j++);
            if (j<cdim) continue;
            if (rn->vert==infinity) {
                if (c[2] > -b_err_min) continue;
            } else  if (!sees(rn->vert,s)) continue;
            c[0] = -c[0]; c[1] = -c[1]; c[2] = -c[2];
            break;
        }
        DEBS(-1) if (!check_perps(s)) /* TJH exit(1);*/ ASSERT(pcFALSE); EDEBS
                                                   return;
    }   
        
    for (i=cdim+1,rn = ch_root->neigh+cdim-1; i; i--, rn--) {
        for (j = 0; j<cdim && rn->vert != s->neigh[j].vert;j++);
        if (j<cdim) continue;
        reduce(&s->normal,rn->vert,s,cdim);
        if (s->normal->sqb != 0) break;
    }

    DEBS(-1) if (!check_perps(s)) {DEBTR(-1) /* TJH exit(1);*/ ASSERT(pcFALSE);} EDEBS

                                                           }


void get_normal(simplex *s) {get_normal_sede(s); return;}

int sees(site p, simplex *s) {

    static basis_s *b = NULL;
    point   tt,zz;
    double  dd,dds;
    int i;


    if (!b) {
        b = (basis_s*)malloc(basis_s_size);
        assert(b);
    }
    else b->lscale = 0;
    zz = VB(b);
    if (cdim==0) return 0;
    if (!s->normal) {
        get_normal_sede(s);
        for (i=0;i<cdim;i++) NULLIFY(basis_s,s->neigh[i].basis);
    }
    tt = s->neigh[0].vert;
    if (vd || power_diagram) {
        if (p==infinity) memcpy(b,infinity_basis,basis_s_size);
        else {trans(zz,p,tt); lift(zz,s);}
    } else trans(zz,p,tt);
    for (i=0;i<3;i++) {
        dd = Vec_dot(zz,s->normal->vecs);   
        if (dd == 0.0) {
            DEBS(-7) DEB(-6,degeneracy:); DEBEXP(-6,site_num(p));
            print_site(p, DFILE); print_simplex_f(s, DFILE, &print_neighbor_full); EDEBS
                                                                                       return 0;
        } 
        dds = dd*dd/s->normal->sqb/Norm2(zz);
        if (dds > b_err_min_sq) return (dd<0);
        get_basis_sede(s);
        reduce_inner(b,s,cdim);
    }
    DEBS(-7) if (i==3) {
        DEB(-6, looped too much in sees);
        DEBEXP(-6,dd) DEBEXP(-6,dds) DEBEXP(-6,site_num(p));
        print_simplex_f(s, DFILE, &print_neighbor_full); /* TJH exit(1);*/ ASSERT(pcFALSE);}
    EDEBS
        return 0;
}





double radsq(simplex *s) {

    point n;
    neighbor *sn;
    int i;

    /* square of ratio of circumcircle radius to
       max edge length for Delaunay tetrahedra */

    
    for (i=0,sn=s->neigh;i<cdim;i++,sn++)
        if (sn->vert == infinity) return Huge;

    if (!s->normal) get_normal_sede(s);

    /* compute circumradius */
    n = s->normal->vecs;

    if (NEARZERO(n[rdim-1])) {return Huge;}

    return Vec_dot_pdim(n,n)/4/n[rdim-1]/n[rdim-1];
}


void *zero_marks(simplex * s, void *dum) { s->mark = 0; return NULL; }

void *one_marks(simplex * s, void *dum) {s->mark = 1; return NULL;}

void *show_marks(simplex * s, void *dum) {printf("%d",s->mark); return NULL;}


#define swap_points(a,b) {point t; t=a; a=b; b=t;}

int alph_test(simplex *s, int i, void *alphap) {
    /*returns 1 if not an alpha-facet */

    simplex *si;
    double  rs,rsi,rsfi;
    neighbor *scn, *sin;
    int k, nsees, ssees;
    static double alpha;

    if (alphap) {alpha=*(double*)alphap; if (!s) return 1;}
    if (i==-1) return 0;

    si = s->neigh[i].simp;
    scn = s->neigh+cdim-1;
    sin = s->neigh+i;
    nsees = 0;

    for (k=0;k<cdim;k++) if (s->neigh[k].vert==infinity && k!=i) return 1;
    rs = radsq(s);
    rsi = radsq(si);

    if (rs < alpha &&  rsi < alpha) return 1;

    swap_points(scn->vert,sin->vert);
    NULLIFY(basis_s, s->neigh[i].basis);
    cdim--;
    get_basis_sede(s);
    reduce(&s->normal,infinity,s,cdim);
    rsfi = radsq(s);

    for (k=0;k<cdim;k++) if (si->neigh[k].simp==s) break;

    ssees = sees(scn->vert,s);
    if (!ssees) nsees = sees(si->neigh[k].vert,s);
    swap_points(scn->vert,sin->vert);
    cdim++;
    NULLIFY(basis_s, s->normal);
    NULLIFY(basis_s, s->neigh[i].basis);

    if (ssees) return alpha<rs;
    if (nsees) return alpha<rsi;

    assert(rsfi<=rs+FLT_EPSILON && rsfi<=rsi+FLT_EPSILON);

    return alpha<=rsfi;
}


void *conv_facetv(simplex *s, void *dum) {
    int i;
    for (i=0;i<cdim;i++) if (s->neigh[i].vert==infinity) {return s;}
    return NULL;
}

short mi[MAXPOINTS], mo[MAXPOINTS];

void *mark_points(simplex *s, void *dum) {
    int i, snum;
    neighbor *sn;

    for  (i=0,sn=s->neigh;i<cdim;i++,sn++) {
        if (sn->vert==infinity) continue;
        snum = site_num(sn->vert);
        if (s->mark) mo[snum] = 1;
        else mi[snum] = 1;
    }
    return NULL;
}

void* visit_outside_ashape(simplex *root, visit_func visit) {
    // change by TJH, compiler choked
//    return visit_triang_gen(visit_hull(root, conv_facetv), visit, alph_test);
    return visit_triang_gen((simplex*)visit_hull(root, conv_facetv), visit, alph_test);
}

int check_ashape(simplex *root, double alpha) {

    int i;

    for (i=0;i<MAXPOINTS;i++) {mi[i] = mo[i] = 0;}

    visit_hull(root, zero_marks);

    alph_test(0,0,&alpha);
    visit_outside_ashape(root, one_marks);

    visit_hull(root, mark_points);

    for (i=0;i<MAXPOINTS;i++) if (mo[i] && !mi[i]) {return 0;}

    return 1;
}

double find_alpha(simplex *root) {

    int i;
    float al=0,ah,am;

    for (ah=i=0;i<pdim;i++) ah += (maxs[i]-mins[i])*(maxs[i]-mins[i]);
    int check_ashape_returns = check_ashape(root,ah);
    assert(check_ashape_returns);
    for (i=0;i<17;i++) {
        if (check_ashape(root, am = (al+ah)/2)) ah = am;
        else al = am;
        if ((ah-al)/ah<.5) break;
    }
    return 1.1*ah;
}

void vols(fg *f, Tree *t, basis_s* n, int depth) {

    static simplex *s;
    static neighbor *sn;
    int tdim = cdim;
    basis_s *nn = 0;
    int signum;
    point nnv;
    double sqq;


    if (!t) {return;}

    if (!s) {NEWL(simplex,s); sn = s->neigh;}
    cdim = depth;
    s->normal = n;
    if (depth>1 && sees(t->key,s)) signum = -1; else signum = 1;
    cdim = tdim;

    if (t->fgs->dist == 0) {
        sn[depth-1].vert = t->key;
        NULLIFY(basis_s,sn[depth-1].basis);
        cdim = depth; get_basis_sede(s); cdim = tdim;
        reduce(&nn, infinity, s, depth);
        nnv = nn->vecs;
        if (t->key==infinity || f->dist==Huge || NEARZERO(nnv[rdim-1]))
            t->fgs->dist = Huge;
        else
            t->fgs->dist = Vec_dot_pdim(nnv,nnv)
                /4/nnv[rdim-1]/nnv[rdim-1];
        if (!t->fgs->facets) t->fgs->vol = 1;
        else vols(t->fgs, t->fgs->facets, nn, depth+1);
    }

    assert(f->dist!=Huge || t->fgs->dist==Huge);
    if (t->fgs->dist==Huge || t->fgs->vol==Huge) f->vol = Huge;
    else {
        sqq = t->fgs->dist - f->dist;
        if (NEARZERO(sqq)) f->vol = 0;
        else f->vol += signum
                 *sqrt(sqq)
                 *t->fgs->vol
                 /(cdim-depth+1);
    }
    vols(f, t->left, n, depth);
    vols(f, t->right, n, depth);

    return;
}


void find_volumes(fg *faces_gr, FILE *F) {
    if (!faces_gr) return;
    vols(faces_gr, faces_gr->facets, 0, 1);
    print_fg(faces_gr, F);
}


gsitef *get_site;
site_n *site_num;


void set_ch_root(simplex *s) {ch_root = s; return;}
/* set root to s, for purposes of getting normals etc. */


simplex *build_convex_hull(gsitef *get_s, site_n *site_numm, short dim, short vdd) {

    /*
      get_s     returns next site each call;
      hull construction stops when NULL returned;
      site_numm returns number of site when given site;
      dim       dimension of point set;
      vdd       if (vdd) then return Delaunay triangulation


    */

    simplex *s, *root;

    // TJH: this line caused a warning: overflow in floating point arithmetic (unsurprisingly)
    //if (!Huge) Huge = DBL_MAX*DBL_MAX;
    if(!Huge) Huge = DBL_MAX;

    cdim = 0;
    get_site = get_s;
    site_num = site_numm;
    pdim = dim;
    vd = vdd;

    exact_bits = DBL_MANT_DIG*log(FLT_RADIX)/log(2);
    b_err_min = DBL_EPSILON*MAXDIM*(1<<MAXDIM)*MAXDIM*3.01;
    b_err_min_sq = b_err_min * b_err_min;

    assert(get_site!=NULL); assert(site_num!=NULL);

    rdim = vd ? pdim+1 : pdim;
    if (rdim > MAXDIM)
        panic("dimension bound MAXDIM exceeded; rdim=%d; pdim=%d\n",
              rdim, pdim);
    /*  fprintf(DFILE, "rdim=%d; pdim=%d\n", rdim, pdim); fflush(DFILE);*/

    point_size = site_size = sizeof(Coord)*pdim;
    basis_vec_size = sizeof(Coord)*rdim;
    basis_s_size = sizeof(basis_s)+ (2*rdim-1)*sizeof(Coord);
    simplex_size = sizeof(simplex) + (rdim-1)*sizeof(neighbor); 
    Tree_size = sizeof(Tree);
    fg_size = sizeof(fg);

    root = NULL;
    if (vd || power_diagram ) {
        p = infinity;
        NEWLRC(basis_s, infinity_basis);
        infinity_basis->vecs[2*rdim-1]
            = infinity_basis->vecs[rdim-1]
            = 1;
        infinity_basis->sqa
            = infinity_basis->sqb
            = 1;
    } else if (!(p = (*get_site)())) return 0;

    NEWL(simplex,root);

    ch_root = root;

    copy_simp(s,root);
    root->peak.vert = p;
    root->peak.simp = s;
    s->peak.simp = root;

    buildhull(root);
    return root;
}


void free_hull_storage(void) {
    free_basis_s_storage();
    free_simplex_storage();
    free_Tree_storage();
    free_fg_storage();
}

//========crust.c=============================================================
/*
 * Power Crust software, by Nina Amenta, Sunghee Choi and Ravi Krishna Kolluri.
 * Copyright (c) 2000 by the University of Texas
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee under the GNU Public License is hereby granted, 
 * provided that this entire notice  is included in all copies of any software 
 * which is or includes a copy or modification of this software and in all copies 
 * of the supporting documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <float.h>
#include <math.h>

//#include "hull.h"  TJH: this file is now above

extern struct simplex **pole1, **pole2; 
extern double* lfs_lb; /* array of estimated lower bounds on lfs */
extern double est_r;  /* guess for r */
extern struct polelabel *adjlist;
extern struct plist **opplist;
extern int *rverts;
extern double bound[8][3], omins[3], omaxs[3];
extern long num_sites;
extern double mult_up;
extern double  thinthreshold;

/* variables for tracking infinite loop */
/* (This should never occur, but it did in early versions) */

int loopStart = -1;
int count = 0;
int lastCount = 0; 

/* TJH: moved these up
int pcFALSE = (1==0);
int pcTRUE = (1==1);*/

short is_bound(simplex *s) {
  
    int i;

    for (i=0;i<4;i++) 
        if ((s->neigh[i].vert[0] > omaxs[0]) || (s->neigh[i].vert[0] < omins[0])
            ||(s->neigh[i].vert[1] > omaxs[1]) || (s->neigh[i].vert[1] < omins[1])
            ||(s->neigh[i].vert[2] > omaxs[2]) || (s->neigh[i].vert[2] < omins[2]))
            return 1;
    return 0;
}


void *compute_vv(simplex *s, void *p) { 
    /* computes Voronoi vertices  */

  static out_func *out_func_here;
  point v[MAXDIM];
  int i,j,k,inf=0;
    double cc[3], cond, ta[4][3], slvnum, sqrad;
    
  if (p) {out_func_here = (out_func*)p; if (!s) return NULL;}

  for (j=0;j<cdim;j++) {
        v[j] = s->neigh[j].vert; 
        /* v[j] stores coordinates of j'th vertex of simplex s; j=0..3 */ 
        if (v[j]==infinity) { /* means simplex s is on the convex hull */
            inf=1;
            break; /* skip the rest of the for loop,
                      ignore convex hull faces (= bounding box ) */
        }  
        i=(site_num)(v[j]); /* i is the index of the vertex v[j] */
        for (k=0;k<cdim-1;k++) {
            ta[j][k] = v[j][k]/mult_up; /* restore original coords   */
            /*    inf=0, ta[0],ta[1],ta[2],ta[3] are 4 vertices of s     */
        }
  }
 
  if (!inf) { /* if not faces on convex hull, compute circumcenter*/
        tetcircumcenter(ta[0], ta[1], ta[2], ta[3], cc, &cond);   
        /* cc is the displacement of circumcenter from ta[0] */
        /* cond is the denominator ( orient3d ) value        */
        sqrad = SQ(cc[0])+SQ(cc[1])+SQ(cc[2]);
        slvnum = SQ(cond)/(sqrad*sqrad*sqrad);  
  
        /*    fprintf(DFILE,"%f %f %f\n",cond,slvnum,sqrad);*/
        /*  sqd = 4*maxsqdist(ta[0],ta[1],ta[2],ta[3]); */
        if (cond!=0) { /* ignore them if cond = 0 */
      s->vv = (Coord*) malloc(sizeof(Coord)*3);
      for (k=0;k<cdim-1;k++) {
                s->vv[k] = ta[0][k]+cc[k];  
      }
      /*  if (slvnum<0.00000000001) s->status = PSLV; 
                else */  
      s->status = VV;
          /*fprintf(CC,"%f %f %f\n",s->vv[0],s->vv[1],s->vv[2]); */
        }
        else { /* if cond=0, s is SLIVER */
            /*      fprintf(DFILE,"cond=%f sliver!\n", cond); */
            s->vv = NULL;
            s->status = SLV; 
            /* modification (no longer used) : set the previous vv as the new vv 
               s->vv = prevsimp->vv;
               s->status = prevsimp->status; */
        }
  }
    else { /* if on conv hull */
        s->status = CNV;
        /* 
           s->vv = (Coord*) malloc(sizeof(Coord)*3);
           crossabc(ta[0],ta[1],ta[2],norm); 
           cross product of 3 non-infinite vertices 
           check that this normal is the right sign  
           pointing_in = 0;
           for (k=0; k<NRAND; k++) {
           if (dotabc(ta[0],get_site_offline(rverts[k]),norm) > SMALL_ENOUGH)
           pointing_in = 1;
           }
           if (pointing_in) {
           norm[0] *= -1; norm[1] *= -1; norm[2] *= -1;
           }
           s->status = CNV;
           s->vv[0]=norm[0];
           s->vv[1]=norm[1];
           s->vv[2]=norm[2];
        */
  }
  /*      prevsimp = s;  */
       
  /* computing poles */
    for (j=0;j<cdim;j++) {  /* compute 1st pole for vertex j */ 
        i=(site_num)(s->neigh[j].vert);
        if (i==-1) continue; 

        /* Ignore poles that are too far away to matter - a relic of the 
           original California-style crust. Probably no longer needed */
        if ((s->neigh[j].vert[0] > omaxs[0])||
            (s->neigh[j].vert[0] < omins[0])||
            (s->neigh[j].vert[1] > omaxs[1])||
            (s->neigh[j].vert[1] < omins[1])||
            (s->neigh[j].vert[2] > omaxs[2])||
            (s->neigh[j].vert[2] < omins[2])) {
    
            /* if (i > (num_sites - 8) ) { /if bounding box vertex */
            pole1[i]=NULL; 
            continue;
        }
  
        else { 

            if (pole1[i]==NULL) {
                /* the vertex i is encountered for the 1st time */ 
                if (s->status==VV) { /* we don't store infinite poles */
                    pole1[i]=s; 
                    continue;
                }
            }

            if (( s->status == VV) && ( pole1[i]->status == VV)) {
                if ( sqdist(pole1[i]->vv,ta[j]) < sqdist(s->vv,ta[j]) ) {
                    pole1[i]=s; /* update 1st pole */
                }
            }
        }
  }
         
  return NULL;
}

  

void *compute_pole2(simplex *s, void *p) { 

    static out_func *out_func_here;
    point v[MAXDIM];
    int i,j,k,inf=0;
    double a[3];
    site t;
    double dir_s[3], dir_p[3], dist_s,dist_p, cos_sp, est_lfs;
    double cos_2r;

    if (p) {out_func_here = (out_func*)p; if (!s) return NULL;}

    for (j=0;j<cdim;j++) { 
        v[j] = s->neigh[j].vert;
        i=(site_num)(v[j]);
        if (i==-1) inf=1;
    }
         
    cos_2r = cos(2* est_r);

    for (j=0;j<cdim;j++) {  /* compute 2nd poles */

        t=s->neigh[j].vert;
        i=(site_num)(t);
        if (i<0) continue; /* not a vertex */
        if (inf) { /* on conv hull */  
            if (s->status == CNV)
            {  
                continue;
            }
            else {
                // TJH: added this if statement
                if(DFILE)
                    fprintf(DFILE,"cannot happen7\n");
            }
        }
        if (!pole1[i]) {
            /*      fprintf(DFILE, "no 1st pole\n"); */
            continue;
        }

        if (pole1[i]->vv==NULL) {  
            // TJH: added this if statement
            if(DFILE)
                fprintf(DFILE,"cannot happen8\n");
            continue;
        }

        if (!s->vv) {
            if (s->status != SLV) fprintf(DFILE,"cannot happen3\n");
            continue;
        }

        for (k=0;k<cdim-1;k++)  { /* a stores orig vertex coord */
            a[k]=t[k]/mult_up;
        }

        /* compute direction and length of vector from 
           sample to first pole */

        dir_and_dist(a,pole1[i]->vv,dir_p,&dist_p);
    
        /* We have a vertex, and there is a good first pole. */
        if ((s->status==VV)&&(pole1[i]->status==VV)) {
      
            /* make direction vector from sample to this Voronoi vertex */
            dir_and_dist(a, s->vv, dir_s, &dist_s);

            /* cosine of angle between angle to vertex and angle to pole */
            cos_sp = dir_s[0]*dir_p[0] + dir_s[1]*dir_p[1] + dir_s[2]*dir_p[2];

            /* if there is an estimate for r, use it to estimate lfs */
            if (est_r < 1.0) {

                /* near vertices - should be close to sample (a) */
                if ((cos_sp < cos_2r) && (cos_sp > -cos_2r)) {
                    /* use to get lower bound on lfs */
                    est_lfs = dist_s /est_r * ((sqrt(1- cos_sp*cos_sp)) - est_r);
                    if (est_lfs > lfs_lb[i]) lfs_lb[i] = est_lfs;
                }
      
            } else {
                lfs_lb[i] = 0;
            }
   
            if (cos_sp > 0) { 
                /* s->vv is in the same side of pole1  */
                continue;
            } 
    
            /* s->vv is a candidate for pole2 */

            if (!pole2[i]) { 
                /* 1st pole2 candidate for vertex i */

                pole2[i]=s;
                continue;
            }
    
            else if (!pole2[i]->vv) { /* 2nd pole points null */
                // TJH: added this if statement
                if(DFILE)
                    fprintf(DFILE,"cannot happen4\n");
                continue;
            }

            else if ((pole2[i]->status == VV) && 
                     (sqdist(a,pole2[i]->vv)<sqdist(a,s->vv)) ) 
                pole2[i]=s; /* update 2nd pole */

        }
    }

    /*  out_func_here(v,cdim,DFILE,0); */

    return NULL;
}

/* tests pole to see if it's farther than estimated local feature size.
   v is a sample, p is a pole. */

int close_pole(double* v, double* p, double lfs_lb) {
    return (sqdist(v,p) < lfs_lb * lfs_lb);
}


int antiLabel(int label) {
    if (label == IN) return(OUT);
    if (label == OUT) return(IN);
    return(label);
}

/* labels a pole */
void labelPole(int pid,int label) {
    adjlist[pid].label = label;
    if (pid == loopStart) {
        loopStart = -1;
    }
}

/* checks to see if list of unlabeled poles is looping infinitely */
int cantLabelAnything(int pid) {

    if (loopStart == -1) {
        loopStart = pid;
        count = 0;
        lastCount = 0;
        return(pcFALSE);
    }

    if (pid == loopStart) {
        if (count == lastCount) {
            /* infinite loop! */
            // TJH: added this if statement
            if(DFILE)
                fprintf(DFILE,"Can't label any more! \n");
            return(pcTRUE);
        } else {
            /* we labeled something last time through */
            lastCount = count;
            count = 0;
        }
    } else {
        /* in the middle of the loop */
        count++;
    }
    return(pcFALSE);
}


/* computes angle between two opposite poles */
double computePoleAngle(simplex* pole1, simplex* pole2, double* samp) {

    return ( ((pole1->vv[0]-samp[0])*(pole2->vv[0]-samp[0]) +
              (pole1->vv[1]-samp[1])*(pole2->vv[1]-samp[1]) +
              (pole1->vv[2]-samp[2])*(pole2->vv[2]-samp[2]) )/
             (sqrt(SQ(pole1->vv[0]-samp[0])+
                   SQ(pole1->vv[1]-samp[1])+
                   SQ(pole1->vv[2]-samp[2]))* 
              sqrt(SQ(pole2->vv[0]-samp[0])+
                   SQ(pole2->vv[1]-samp[1])+
                   SQ(pole2->vv[2]-samp[2]))) );
}

/* Adds a new pair of opposite poles to each other's lists */
void newOpposite(int p1index, int p2index, double pole_angle) {
    plist  *newplist;

    newplist = (struct plist *) malloc(sizeof(struct plist));
    newplist->pid = p2index;
    newplist->angle = pole_angle;
    newplist->next = opplist[p1index];
    opplist[p1index] = newplist;
    if (adjlist[p1index].oppradius > adjlist[p2index].sqradius) {
        assert(adjlist[p2index].sqradius > 0.0); 
        adjlist[p1index].oppradius = adjlist[p2index].sqradius;
    }
}


/* Outputs a pole, saving it's squared radius in adjlist */
void outputPole(/*TJH FILE* POLE, FILE* SPFILE, */simplex* pole, int poleid, 
                double* samp, int* num_poles,double distance) {
    double r2, weight;

    r2 = SQ(pole->vv[0]-samp[0])
        + SQ(pole->vv[1]-samp[1])
        + SQ(pole->vv[2]-samp[2]);

    weight = SQ(pole->vv[0])+SQ(pole->vv[1])+SQ(pole->vv[2])- r2;

    pole->status = POLE_OUTPUT;
    pole->poleindex = poleid;

    /* debugging file */
    /* TJH: we have bypassed the file routines, so we don't need this bit
    fprintf(POLE,"%f %f %f\n",pole->vv[0], 
            pole->vv[1], pole->vv[2]);*/
    vtk_medial_surface->GetPoints()->InsertNextPoint(pole->vv[0],pole->vv[1], pole->vv[2]);

    /* for computing powercrust */
    /* TJH: we have bypassed the file routines, so we don't need this bit
    fprintf(SPFILE,"%f %f %f %f\n",pole->vv[0], 
            pole->vv[1], pole->vv[2],
            weight);*/
    vtk_medial_surface->GetPointData()->GetScalars()->InsertNextTuple1(weight);

    /* remember squared radius */
    adjlist[poleid].sqradius = r2;
    adjlist[poleid].samp_distance=distance; 

  /* initialize perp dist to MA */
    adjlist[poleid].oppradius = r2;

    /* initialize */
    adjlist[poleid].grafindex = -1;

    /* keep count! */
    (*num_poles)++; 

}
  
//========fg.c=============================================================
/* fg.c : face graph of hull, and splay trees */

/*
 * Ken Clarkson wrote this.  Copyright (c) 1995 by AT&T..
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */


#include <float.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

//#include "hull.h"   TJH: this file is now above

Tree* insert(site, Tree*);

// TJH: um, delete is, like, a keyword, um.
// but this function isn't used anyway, so commenting out
//Tree* delete(site, Tree*);

void printtree(Tree*,int);

void printtree_flat(Tree*);


/* splay tree code */

/*
Fri Oct 21 21:15:01 EDT 1994
    style changes, removed Sedgewickized...
    Ken Clarkson

*/
/*
           An implementation of top-down splaying with sizes
             D. Sleator <sleator@cs.cmu.edu>, January 1994.

  This extends top-down-splay.c to maintain a size field in each node.
  This is the number of nodes in the subtree rooted there.  This makes
  it possible to efficiently compute the rank of a key.  (The rank is
  the number of nodes to the left of the given key.)  It it also
  possible to quickly find the node of a given rank.  Both of these
  operations are illustrated in the code below.  The remainder of this
  introduction is taken from top-down-splay.c.

  "Splay trees", or "self-adjusting search trees" are a simple and
  efficient data structure for storing an ordered set.  The data
  structure consists of a binary tree, with no additional fields.  It
  allows searching, insertion, deletion, deletemin, deletemax,
  splitting, joining, and many other operations, all with amortized
  logarithmic performance.  Since the trees adapt to the sequence of
  requests, their performance on real access patterns is typically even
  better.  Splay trees are described in a number of texts and papers
  [1,2,3,4].

  The code here is adapted from simple top-down splay, at the bottom of
  page 669 of [2].  It can be obtained via anonymous ftp from
  spade.pc.cs.cmu.edu in directory /usr/sleator/public.

  The chief modification here is that the splay operation works even if the
  item being splayed is not in the tree, and even if the tree root of the
  tree is NULL.  So the line:

                              t = splay(i, t);

  causes it to search for item with key i in the tree rooted at t.  If it's
  there, it is splayed to the root.  If it isn't there, then the node put
  at the root is the last one before NULL that would have been reached in a
  normal binary search for i.  (It's a neighbor of i in the tree.)  This
  allows many other operations to be easily implemented, as shown below.

  [1] "Data Structures and Their Algorithms", Lewis and Denenberg,
       Harper Collins, 1991, pp 243-251.
  [2] "Self-adjusting Binary Search Trees" Sleator and Tarjan,
       JACM Volume 32, No 3, July 1985, pp 652-686.
  [3] "Data Structure and Algorithm Analysis", Mark Weiss,
       Benjamin Cummins, 1992, pp 119-130.
  [4] "Data Structures, Algorithms, and Performance", Derick Wood,
       Addison-Wesley, 1993, pp 367-375
*/


STORAGE(Tree)


#define compare(i,j) (site_num(i)-site_num(j))
    /* This is the comparison.                                       */
    /* Returns <0 if i<j, =0 if i=j, and >0 if i>j                   */
 
#define node_size(x) ((x) ? ((x)->size) : 0 )
    /* This macro returns the size of a node.  Unlike "x->size",     */
    /* it works even if x=NULL.  The test could be avoided by using  */
    /* a special version of NULL which was a real node with size 0.  */
 
    Tree * splay (site i, Tree *t) 
    /* Splay using the key i (which may or may not be in the tree.) */
    /* The starting root is t, and the tree used is defined by rat  */
    /* size fields are maintained */
{
    Tree N, *l, *r, *y;
    int comp, root_size, l_size, r_size;
    
    if (!t) return t;
    N.left = N.right = NULL;
    l = r = &N;
    root_size = node_size(t);
    l_size = r_size = 0;
 
    for (;;) {
        comp = compare(i, t->key);
        if (comp < 0) {
            if (!t->left) break;
            if (compare(i, t->left->key) < 0) {
                y = t->left;                           /* rotate right */
                t->left = y->right;
                y->right = t;
                t->size = node_size(t->left) + node_size(t->right) + 1;
                t = y;
                if (!t->left) break;
            }
            r->left = t;                               /* link right */
            r = t;
            t = t->left;
            r_size += 1+node_size(r->right);
        } else if (comp > 0) {
            if (!t->right) break;
            if (compare(i, t->right->key) > 0) {
                y = t->right;                          /* rotate left */
                t->right = y->left;
                y->left = t;
                t->size = node_size(t->left) + node_size(t->right) + 1;
                t = y;
                if (!t->right) break;
            }
            l->right = t;                              /* link left */
            l = t;
            t = t->right;
            l_size += 1+node_size(l->left);
        } else break;
    }
    l_size += node_size(t->left);  /* Now l_size and r_size are the sizes of */
    r_size += node_size(t->right); /* the left and right trees we just built.*/
    t->size = l_size + r_size + 1;

    l->right = r->left = NULL;

    /* The following two loops correct the size fields of the right path  */
    /* from the left child of the root and the right path from the left   */
    /* child of the root.                                                 */
    for (y = N.right; y != NULL; y = y->right) {
        y->size = l_size;
        l_size -= 1+node_size(y->left);
    }
    for (y = N.left; y != NULL; y = y->left) {
        y->size = r_size;
        r_size -= 1+node_size(y->right);
    }
 
    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;

    return t;
}

Tree * insert(site i, Tree * t) {
    /* Insert key i into the tree t, if it is not already there. */
    /* Return a pointer to the resulting tree.                   */

    // TJH: changed new to new_tree in this function to avoid keyword collision
    //Tree * new;
    Tree *new_tree;

    if (t != NULL) {
        t = splay(i,t);
        if (compare(i, t->key)==0) {
            return t;  /* it's already there */
        }
    }
    NEWL(Tree,new_tree)
        if (!t) {
            new_tree->left = new_tree->right = NULL;
        } else if (compare(i, t->key) < 0) {
            new_tree->left = t->left;
            new_tree->right = t;
            t->left = NULL;
            t->size = 1+node_size(t->right);
        } else {
            new_tree->right = t->right;
            new_tree->left = t;
            t->right = NULL;
            t->size = 1+node_size(t->left);
        }
    new_tree->key = i;
    new_tree->size = 1 + node_size(new_tree->left) + node_size(new_tree->right);
    return new_tree;
}

/* commented out by TJH - not used and causes problems because delete is a keyword
Tree * delete(site i, Tree *t) {
    // Deletes i from the tree if it's there.               
    // Return a pointer to the resulting tree.              
    Tree * x;
    int tsize;

    if (!t) return NULL;
    tsize = t->size;
    t = splay(i,t);
    if (compare(i, t->key) == 0) {               // found it 
        if (!t->left) {
            x = t->right;
        } else {
            x = splay(i, t->left);
            x->right = t->right;
        }
        FREEL(Tree,t);
        if (x) x->size = tsize-1;
        return x;
    } else {
        return t;                         // It wasn't there
    }
}*/

Tree *find_rank(int r, Tree *t) {
    /* Returns a pointer to the node in the tree with the given rank.  */
    /* Returns NULL if there is no such node.                          */
    /* Does not change the tree.  To guarantee logarithmic behavior,   */
    /* the node found here should be splayed to the root.              */
    int lsize;
    if ((r < 0) || (r >= node_size(t))) return NULL;
    for (;;) {
        lsize = node_size(t->left);
        if (r < lsize) {
            t = t->left;
        } else if (r > lsize) {
            r = r - lsize -1;
            t = t->right;
        } else {
            return t;
        }
    }
}

void printtree_flat_inner(Tree * t) {
    if (!t) return;

    printtree_flat_inner(t->right);
    printf("%f ", *(t->key));
    fflush(stdout);
    printtree_flat_inner(t->left);
}

void printtree_flat(Tree * t) {
    if (!t) {
        printf("<empty tree>");
        return;
    }
    printtree_flat_inner(t);
}


void printtree(Tree * t, int d) {
    int i;
    if (!t) return;

    printtree(t->right, d+1);
    for (i=0; i<d; i++) printf("  ");
    printf("%f(%d)\n", *(t->key), t->size);
    fflush(stdout);
    printtree(t->left, d+1);
}






fg *faces_gr_t;

STORAGE(fg)

#define snkey(x) site_num((x)->vert)

    fg *find_fg(simplex *s,int q) {

    fg *f;
    neighbor *si, *sn = s->neigh;
    Tree *t;

    if (q==0) return faces_gr_t;
    if (!faces_gr_t) NEWLRC(fg, faces_gr_t);
    f = faces_gr_t;
    for (si=sn; si<sn+cdim; si++) if (q & (1<<(si-sn))) {
        t = f->facets = insert(si->vert,f->facets);
        if (!t->fgs) NEWLRC(fg, (t->fgs))
                         f = t->fgs;
    }
    return f;
}

void *add_to_fg(simplex *s, void *dum) {

    neighbor t, *si, *sj, *sn = s->neigh;
    fg *fq;
    int q,m,Q=1<<cdim;
    /* sort neigh by site number */
    for (si=sn+2;si<sn+cdim;si++)
        for (sj=si; sj>sn+1 && snkey(sj-1) > snkey(sj); sj--)
        {t=*(sj-1); *(sj-1) = *sj; *sj = t;}

    NULLIFY(basis_s,s->normal);
    NULLIFY(basis_s,s->neigh[0].basis);

    /* insert subsets */
    for (q=1; q<Q; q++) find_fg(s,q);

    /* include all superset relations */
    for (q=1; q<Q; q++) {
        fq = find_fg(s,q);
        assert(fq);
        for (m=1,si=sn;si<sn+cdim;si++,m<<=1) if (!(q&m)) {
            fq->facets = insert(si->vert,fq->facets);
            fq->facets->fgs = find_fg(s, q|m);
        }
    }
    return NULL;    
}

fg *build_fg(simplex *root) {
    faces_gr_t= 0;
    visit_hull(root, add_to_fg);
    return faces_gr_t;
}

void visit_fg_i(   void (*v_fg)(Tree *, int, int),
                   Tree *t, int depth, int vn, int boundary) {
    int boundaryc = boundary;

    if (!t) return;

    assert(t->fgs);
    if (t->fgs->mark!=vn) {
        t->fgs->mark = vn;
        if (t->key!=infinity && !mo[site_num(t->key)]) boundaryc = 0; 
        v_fg(t,depth, boundaryc);
        visit_fg_i(v_fg, t->fgs->facets,depth+1, vn, boundaryc);
    }
    visit_fg_i(v_fg, t->left,depth,vn, boundary);
    visit_fg_i(v_fg, t->right,depth,vn,boundary);
}

void visit_fg(fg *faces_gr, void (*v_fg)(Tree *, int, int)) {
    static int fg_vn;
    visit_fg_i(v_fg, faces_gr->facets, 0, ++fg_vn, 1);
}

int visit_fg_i_far(void (*v_fg)(Tree *, int),
                   Tree *t, int depth, int vn) {
    int nb = 0;

    if (!t) return 0;

    assert(t->fgs);
    if (t->fgs->mark!=vn) {
        t->fgs->mark = vn;
        nb = (t->key==infinity) || mo[site_num(t->key)];
        if (!nb && !visit_fg_i_far(v_fg, t->fgs->facets,depth+1,vn))
            v_fg(t,depth);
    }
    nb = visit_fg_i_far(v_fg, t->left,depth,vn) || nb;
    nb = visit_fg_i_far(v_fg, t->right,depth,vn) || nb;
    return nb;
}

void visit_fg_far(fg *faces_gr, void (*v_fg)(Tree *, int)) {
    static int fg_vn;
    visit_fg_i_far(v_fg,faces_gr->facets, 0, --fg_vn);
}



FILE *FG_OUT;

void p_fg(Tree* t, int depth, int bad) {
    static int fa[MAXDIM];
    int i;
    static double mults[MAXDIM];

    if (mults[0]==0) {
        mults[pdim] = 1;
        for (i=pdim-1; i>=0; i--) mults[i] = mult_up*mults[i+1];
    }

    fa[depth] = site_num(t->key);
    for (i=0;i<=depth;i++)
        fprintf(FG_OUT, "%d ", fa[i]);
    fprintf(FG_OUT, "   %G\n", t->fgs->vol/mults[depth]);
}

int p_fg_x_depth;

void p_fg_x(Tree*t, int depth, int bad) {

    static int fa[MAXDIM];
    static point fp[MAXDIM];
    int i;

    fa[depth] = site_num(t->key);
    fp[depth] = t->key;

    if (depth==p_fg_x_depth) for (i=0;i<=depth;i++)
        fprintf(FG_OUT, "%d%s", fa[i], (i==depth) ? "\n" : " ");
}

void print_fg_alt(fg *faces_gr, FILE *F, int fd) {
    FG_OUT=F;
    if (!faces_gr) return;
    p_fg_x_depth = fd;
    visit_fg(faces_gr, p_fg_x);
    fclose(FG_OUT);
}


void print_fg(fg *faces_gr, FILE *F) {FG_OUT=F; visit_fg(faces_gr, p_fg);}


double fg_hist[100][100], fg_hist_bad[100][100],fg_hist_far[100][100];

void h_fg(Tree *t, int depth, int bad) {
    if (!t->fgs->facets) return;
    if (bad) {
        fg_hist_bad[depth][t->fgs->facets->size]++;
        return;
    }
    fg_hist[depth][t->fgs->facets->size]++;
}

void h_fg_far(Tree* t, int depth) {
    if (t->fgs->facets) fg_hist_far[depth][t->fgs->facets->size]++;
}


void print_hist_fg(simplex *root, fg *faces_gr, FILE *F) {
    int i,j,k;
    double tot_good[100], tot_bad[100], tot_far[100];
    for (i=0;i<20;i++) {
        tot_good[i] = tot_bad[i] = tot_far[i] = 0;
        for (j=0;j<100;j++) {
            fg_hist[i][j]= fg_hist_bad[i][j]= fg_hist_far[i][j] = 0;
        }
    }
    if (!root) return;

    find_alpha(root);

    if (!faces_gr) faces_gr = build_fg(root);

    visit_fg(faces_gr, h_fg);
    visit_fg_far(faces_gr, h_fg_far);

    for (j=0;j<100;j++) for (i=0;i<20;i++) {
        tot_good[i] += fg_hist[i][j];
        tot_bad[i] += fg_hist_bad[i][j];
        tot_far[i]  += fg_hist_far[i][j];
    }

    for (i=19;i>=0 && !tot_good[i] && !tot_bad[i]; i--);
    fprintf(F,"totals   ");
    for (k=0;k<=i;k++) {
        if (k==0) fprintf(F, "  ");
        else fprintf(F,"            ");
        fprintf(F, "%d/%d/%d",
                (int)tot_far[k], (int)tot_good[k], (int)tot_good[k] + (int)tot_bad[k]);
    }
        
    
    for (j=0;j<100;j++) {
        for (i=19; i>=0 && !fg_hist[i][j] && !fg_hist_bad[i][j]; i--);
        if (i==-1) continue;
        fprintf(F, "\n%d    ",j);fflush(F);
            
        for (k=0;k<=i;k++) {
            if (k==0) fprintf(F, "  ");
            else fprintf(F,"            ");
            if (fg_hist[k][j] || fg_hist_bad[k][j])
                fprintf(F,
                        "%2.1f/%2.1f/%2.1f",
                        tot_far[k] ? 100*fg_hist_far[k][j]/tot_far[k]+.05 : 0,
                        tot_good[k] ? 100*fg_hist[k][j]/tot_good[k]+.05 : 0,
                        100*(fg_hist[k][j]+fg_hist_bad[k][j])/(tot_good[k]+tot_bad[k])+.05
                    );
        }
    }
    fprintf(F, "\n");
}
//========heap.c=============================================================
/*
 * Power Crust software, by Nina Amenta, Sunghee Choi and Ravi Krishna Kolluri.
 * Copyright (c) 2000 by the University of Texas
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee under the GNU Public License is hereby granted, 
 * provided that this entire notice  is included in all copies of any software 
 * which is or includes a copy or modification of this software and in all copies 
 * of the supporting documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <float.h>
#include <math.h>

//#include "hull.h"  TJH: this file is now above

extern struct polelabel *adjlist;
struct heap_array *heap_A;
int heap_length;
int heap_size = 0;

void init_heap(int num)
{
    heap_A = (struct heap_array *)calloc(num, sizeof(struct heap_array));
    heap_size = 0;
    heap_length = num;
    // TJH: added this if statement
    if(DFILE)
        fprintf(DFILE,"heap %d initialized\n", num);
}

void heapify(int hi)
{
    int largest;
    int temp;
    double td;
 
    if ((LEFT(hi) <= heap_size) && (heap_A[LEFT(hi)].pri > heap_A[hi].pri))
        largest = LEFT(hi);
    else largest = hi;
  
    if ((RIGHT(hi) <= heap_size) && (heap_A[RIGHT(hi)].pri > heap_A[largest].pri))
        largest = RIGHT(hi);
  
    if (largest != hi) {
        temp = heap_A[hi].pid;
        heap_A[hi].pid = heap_A[largest].pid;
        adjlist[heap_A[hi].pid].hid = hi;
        heap_A[largest].pid = temp;  
        adjlist[heap_A[largest].pid].hid = largest;
        td =  heap_A[hi].pri;
        heap_A[hi].pri = heap_A[largest].pri;
        heap_A[largest].pri = td;
        heapify(largest);
    }

}

int extract_max()
{
    int max;

    if (heap_size < 1) return -1;
    max = heap_A[1].pid;
    heap_A[1].pid = heap_A[heap_size].pid;
    heap_A[1].pri = heap_A[heap_size].pri;  
    adjlist[heap_A[1].pid].hid = 1;
    heap_size--;
    heapify(1);
    return max;
}

int insert_heap(int pi, double pr)
{
    int i;

    heap_size++;
    /*printf("heap_size %d\n",heap_size); */
    i = heap_size;
    while ((i>1)&&(heap_A[PARENT(i)].pri < pr)) {
        heap_A[i].pid = heap_A[PARENT(i)].pid;
        heap_A[i].pri = heap_A[PARENT(i)].pri;
        adjlist[heap_A[i].pid].hid = i;
        i = PARENT(i);
    };
    heap_A[i].pri = pr;
    heap_A[i].pid = pi;
    adjlist[pi].hid = i;
    return i;
}

void update(int hi, double pr) 
    /* make the element heap_A[hi].pr = pr ... */
{
    int pi,i;

    heap_A[hi].pri = pr;
    pi = heap_A[hi].pid;

    if (pr > heap_A[PARENT(hi)].pri) { 
        i = hi;
        while ((i>1)&&(heap_A[PARENT(i)].pri < pr)) {
            heap_A[i].pid = heap_A[PARENT(i)].pid;
            heap_A[i].pri = heap_A[PARENT(i)].pri;
            adjlist[heap_A[i].pid].hid = i;
            i = PARENT(i);
        };
        heap_A[i].pri = pr;
        heap_A[i].pid = pi;
        adjlist[pi].hid = i;
    }
    else heapify(hi);
}



//========hull.c=============================================================
/* hull.c : "combinatorial" functions for hull computation */

/*
 * Ken Clarkson wrote this.  Copyright (c) 1995 by AT&T..
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

//#include "hull.h"  TJH: this file is now above


site p;
long pnum;

int rdim,   /* region dimension: (max) number of sites specifying region */
    cdim,   /* number of sites currently specifying region */
    site_size, /* size of malloc needed for a site */
    point_size;  /* size of malloc needed for a point */

int scount = 0; /* new power */
STORAGE(simplex)

#define push(x) *(st+tms++) = x;
#define pop(x)  x = *(st + --tms);

    void *visit_triang_gen(simplex *s, visit_func *visit, test_func *test) {
    /* 
     * starting at s, visit simplices t such that test(s,i,0) is true,
     * and t is the i'th neighbor of s;
     * apply visit function to all visited simplices;
     * when visit returns nonNULL, exit and return its value
     */
    neighbor *sn;
    void *v;
    simplex *t;
    int i;
    long tms = 0;
    static long vnum = -1;
    static long ss = 2000;
    static simplex **st;

    vnum--;
    if (!st) {
        st=(simplex**)malloc((ss+MAXDIM+1)*sizeof(simplex*));
        assert(st);
    }
    if (s) push(s);
    while (tms) {

        if (tms>ss) {
            DEBEXP(-1,tms);
            st=(simplex**)realloc(st,((ss+=ss)+MAXDIM+1)*sizeof(simplex*));
            assert(st);
        }
        pop(t);
        if (!t || t->visit == vnum) continue;
        t->visit = vnum;
        if ((v=(*visit)(t,0))!=NULL) {return v;}
        for (i=-1,sn = t->neigh-1;i<cdim;i++,sn++)
            if ((sn->simp->visit != vnum) && sn->simp && test(t,i,0))
                push(sn->simp);
    }
    return NULL;
}

int truet(simplex *s, int i, void *dum) {return 1;}

void *visit_triang(simplex *root, visit_func *visit)
    /* visit the whole triangulation */
{return visit_triang_gen(root, visit, truet);}


int hullt(simplex *s, int i, void *dummy) {return i>-1;}

void *facet_test(simplex *s, void *dummy) {return (!s->peak.vert) ? s : NULL;}

void *visit_hull(simplex *root, visit_func *visit)
    /* visit all simplices with facets of the current hull */
{
    // TJH: cast required
    //return visit_triang_gen(visit_triang(root, &facet_test),visit, hullt);
    return visit_triang_gen((simplex*)visit_triang(root, &facet_test),visit, hullt);
}


// TJH: replaced overlong #defines with plain function calls
/*neighbor* simplex_lookup(simplex *a,simplex *b) 
{
    int i;
    neighbor *x;
    for (i=0, x = a->neigh; (x->simp != b) && (i<cdim) ; i++, x++);
    if (i<cdim)
        return x; 
    else {   
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE,"adjacency failure,op_simp:\n");
        DEBTR(-10)
        print_simplex_f(a, DFILE, &print_neighbor_full);
        print_simplex(b, DFILE); 
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE,"---------------------\n"); 
        print_triang(a,DFILE, &print_neighbor_full);
        ASSERT(pcFALSE); 
        return 0; 
    }  
}*/  
/*neighbor* vert_lookup(simplex *a,site b) 
{
    int i;
    neighbor *x;
    for (i=0, x = a->neigh; (x->vert != b) && (i<cdim) ; i++, x++);
    if (i<cdim)
        return x; 
    else {   
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE,"adjacency failure,op_vert:\n");
        DEBTR(-10)
        print_simplex_f(a, DFILE, &print_neighbor_full);
        print_site(b, DFILE); 
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE,"---------------------\n"); 
        print_triang(a,DFILE, &print_neighbor_full);
        /* TJH exit(1);*/ /*ASSERT(pcFALSE,"adjacency failure!"); 
        return 0; 
    }  
}*/   

#define lookup(a,b,what,whatt)                      \
{                                   \
    int i;                              \
    neighbor *x;                            \
    for (i=0, x = a->neigh; (x->what != b) && (i<cdim) ; i++, x++); \
    if (i<cdim)                         \
        return x;                       \
    else {                              \
        if(DFILE)\
            fprintf(DFILE,"adjacency failure,op_" #what ":\n"); \
        DEBTR(-10)                      \
        print_simplex_f(a, DFILE, &print_neighbor_full);    \
        print_##whatt(b, DFILE);                \
        if(DFILE)\
            fprintf(DFILE,"---------------------\n");       \
        print_triang(a,DFILE, &print_neighbor_full);        \
        /* TJH exit(1);*/ ASSERT(pcFALSE,"adjacency failure!");                        \
        return 0;                       \
    }                               \
}                                   \


neighbor *op_simp(simplex *a, simplex *b) {lookup(a,b,simp,simplex)}
//neighbor *op_simp(simplex *a, simplex *b) { return simplex_lookup(a,b); }
/* the neighbor entry of a containing b */

neighbor *op_vert(simplex *a, site b) {lookup(a,b,vert,site)}
//neighbor *op_vert(simplex *a, site b) { return vert_lookup(a,b); }
/* the neighbor entry of a containing b */


void connect(simplex *s) {
    /* make neighbor connections between newly created simplices incident to p */

    site xf,xb,xfi;
    simplex *sb, *sf, *seen;
    int i;
    neighbor *sn;

    if (!s) return;
    assert(!s->peak.vert
           && s->peak.simp->peak.vert==p
           && !op_vert(s,p)->simp->peak.vert);
    if (s->visit==pnum) return;
    s->visit = pnum;
    seen = s->peak.simp;
    xfi = op_simp(seen,s)->vert;
    for (i=0, sn = s->neigh; i<cdim; i++,sn++) {
        xb = sn->vert;
        if (p == xb) continue;
        sb = seen;
        sf = sn->simp;
        xf = xfi;
        if (!sf->peak.vert) {   /* are we done already? */
            sf = op_vert(seen,xb)->simp;
            if (sf->peak.vert) continue;                
        } else do {
            xb = xf;
            xf = op_simp(sf,sb)->vert;
            sb = sf;
            sf = op_vert(sb,xb)->simp;
        } while (sf->peak.vert);

        sn->simp = sf;
        op_vert(sf,xf)->simp = s;

        connect(sf);
    }

}


                
simplex *make_facets(simplex *seen) {
    /*
     * visit simplices s with sees(p,s), and make a facet for every neighbor
     * of s not seen by p
     */

    simplex *n;
    static simplex *ns;
    neighbor *bn;
    int i;


    if (!seen) return NULL;
    DEBS(-1) assert(sees(p,seen) && !seen->peak.vert);
    EDEBS seen->peak.vert = p;

    for (i=0,bn = seen->neigh; i<cdim; i++,bn++) {
        n = bn->simp;
        if (pnum != n->visit) {
            n->visit = pnum;
            if (sees(p,n)) make_facets(n);
        } 
        if (n->peak.vert) continue;
        copy_simp(ns,seen);
        ns->visit = 0;
        ns->peak.vert = 0;
        ns->normal = 0;
        ns->peak.simp = seen;
        /*      ns->Sb -= ns->neigh[i].basis->sqb; */
        NULLIFY(basis_s,ns->neigh[i].basis);
        ns->neigh[i].vert = p;
        bn->simp = op_simp(n,seen)->simp = ns;
    }
    return ns;
}



simplex *extend_simplices(simplex *s) {
    /*
     * p lies outside flat containing previous sites;
     * make p a vertex of every current simplex, and create some new simplices
     */

    int i,
        ocdim=cdim-1;
    simplex *ns;
    neighbor *nsn;

    if (s->visit == pnum) return s->peak.vert ? s->neigh[ocdim].simp : s;
    s->visit = pnum;
    s->neigh[ocdim].vert = p;
    NULLIFY(basis_s,s->normal);
    NULLIFY(basis_s,s->neigh[0].basis);
    if (!s->peak.vert) {
        s->neigh[ocdim].simp = extend_simplices(s->peak.simp);
        return s;
    } else {
        copy_simp(ns,s);
        s->neigh[ocdim].simp = ns;
        ns->peak.vert = NULL;
        ns->peak.simp = s;
        ns->neigh[ocdim] = s->peak;
        inc_ref(basis_s,s->peak.basis);
        for (i=0,nsn=ns->neigh;i<cdim;i++,nsn++)
            nsn->simp = extend_simplices(nsn->simp);
    }
    return ns;
}


simplex *search(simplex *root) {
    /* return a simplex s that corresponds to a facet of the 
     * current hull, and sees(p, s) */

    simplex *s;
    static simplex **st;
    static long ss = MAXDIM;
    neighbor *sn;
    int i;
    long tms = 0;

    if (!st) st = (simplex **)malloc((ss+MAXDIM+1)*sizeof(simplex*));
    push(root->peak.simp);
    root->visit = pnum;
    if (!sees(p,root))
        for (i=0,sn=root->neigh;i<cdim;i++,sn++) push(sn->simp);
    while (tms) 
    {
        if(tms>ss) 
        {
            st=(simplex**)realloc(st,((ss+=ss)+MAXDIM+1)*sizeof(simplex*));
            assert(st);
        }
        pop(s);
        if (s->visit == pnum) continue;
        s->visit = pnum;
        if (!sees(p,s)) continue;
        if (!s->peak.vert) return s;
        for (i=0, sn=s->neigh; i<cdim; i++,sn++) push(sn->simp);
    }
    return NULL;
}


point get_another_site(void) {
  
    /*static int scount =0; */
    point pnext;

    if (!(++scount%1000)) {
        // TJH: added this if statement
        if(DFILE)
            fprintf(DFILE,"site %d...", scount);
    }
    /*  check_triang(); */
    pnext = (*get_site)();
    if (!pnext) return NULL;
    pnum = site_num(pnext)+2;
    return pnext;
}



void buildhull (simplex *root) {

    while (cdim < rdim) {
        p = get_another_site();
        if (!p) return;
        if (out_of_flat(root,p))
            extend_simplices(root);
        else
            connect(make_facets(search(root)));
    }
    while ((p = get_another_site())!=NULL)
        connect(make_facets(search(root)));
}
//========io.c=============================================================
/* io.c : input-output */

// Some changes made by Tim J. Hutton (T.Hutton@eastman.ucl.ac.uk) as TJH

/*
 * Ken Clarkson wrote this.  Copyright (c) 1995 by AT&T..
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <float.h>
#include <math.h>

//#include "hull.h"   TJH: this file is now above

//#include "tim_defs.h" TJH: this file is now above

double mult_up = 1.0;
Coord mins[MAXDIM]
= {DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX,DBL_MAX},
    maxs[MAXDIM]
    = {-DBL_MAX,-DBL_MAX,-DBL_MAX,-DBL_MAX,-DBL_MAX,-DBL_MAX,-DBL_MAX,-DBL_MAX};

void panic(char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    // TJH: added this if statement
    if(DFILE)
    {
        vfprintf(DFILE, fmt, args);
        fflush(DFILE);
    }
    va_end(args);

    /* TJH exit(1);*/ ASSERT(pcFALSE,fmt);
}


// TJH: stripping out any file use
// old TJH: uncommented these lines, they seem useful. Added dummy bodies.
//FILE * popen(char*, char*) {return NULL;}
//void pclose(FILE*) {}
//FILE* efopen(char *file, char *mode) { return NULL;}
//void efclose(FILE* file) {}
//FILE* epopen(char *com, char *mode) {return NULL;}


char tmpfilenam[L_tmpnam];

/* TJH: we're not using file handling so can lose these

FILE* efopen(char *file, char *mode) {
    FILE* fp;
    if ((fp = fopen(file, mode))!=NULL) return fp;
    // TJH: added this if statement
    if(DFILE)
        fprintf(DFILE, "couldn't open file %s mode %s\n",file,mode);
    ASSERT(pcFALSE);
    return NULL;
}

void efclose(FILE* file) {
    fclose(file);
    file = NULL;
}

FILE* epopen(char *com, char *mode) {
    FILE* fp;
    if ((fp = popen(com, mode))!=NULL) return fp;
    fprintf(stderr, "couldn't open stream %s mode %s\n",com,mode);
    ASSERT(pcFALSE);
    return 0;
}*/

// TJH: replaced this function with an empty body
void print_neighbor_snum(FILE* F, neighbor *n){}
/*void print_neighbor_snum(FILE* F, neighbor *n){
    // TJH: added this line
    if(!F) return;

    assert(site_num!=NULL);
    if (n->vert)
        fprintf(F, "%ld ", (*site_num)(n->vert));
    else
        fprintf(F, "NULL vert ");

    // TJH: fairly sure they meant F here
    //fflush(stdout);
    fflush(F);
}*/

// TJH: replaced this function with an empty body
void print_basis(FILE *F, basis_s* b) {}
/*void print_basis(FILE *F, basis_s* b) {
    // TJH: added this line
    if(!F) return;

    if (!b) {fprintf(F, "NULL basis ");fflush(F);return;}
    if (b->lscale<0) {fprintf(F, "\nbasis computed");return;}
    fprintf(F, "\n%p  %d \n b=",(void*)b,b->lscale);
    print_point(F, rdim, b->vecs);
    fprintf(F, "\n a= ");
    print_point_int(F, rdim, b->vecs+rdim); fprintf(F, "   ");
    fflush(F);
}*/

// TJH: replaced this function with an empty body
void print_simplex_num(FILE *F, simplex *s) {}
/*void print_simplex_num(FILE *F, simplex *s) {
    // TJH: added this line
    if(!F) return;

    fprintf(F, "simplex ");
    if(!s) fprintf(F, "NULL ");
    else fprintf(F, "%p  ", (void*)s);
}*/

// TJH: replaced this function with an empty body
void print_neighbor_full(FILE *F, neighbor *n){}
/*void print_neighbor_full(FILE *F, neighbor *n){
    // TJH: added this line
    if(!F) return;

    if (!n) {fprintf(F, "null neighbor\n");return;}

    print_simplex_num(F, n->simp);
    print_neighbor_snum(F, n);fprintf(F, ":  ");fflush(F);
    if (n->vert) {
        //      if (n->basis && n->basis->lscale <0) fprintf(F, "trans ");
        // else  
            print_point(F, pdim,n->vert);
        fflush(F);
    }
    print_basis(F, n->basis);
    fflush(F);
    fprintf(F, "\n");
}*/

// TJH: replaced this function with an empty body
void *print_facet(FILE *F, simplex *s, print_neighbor_f *pnfin) { return NULL; }
/*void *print_facet(FILE *F, simplex *s, print_neighbor_f *pnfin) {
    // TJH: added this line
    if(!F) return NULL;

    int i;
    neighbor *sn = s->neigh;

    //  fprintf(F, "%d ", s->mark);
    for (i=0; i<cdim;i++,sn++) (*pnfin)(F, sn);
    fprintf(F, "\n");
    fflush(F);
    return NULL;
}*/

// TJH: replaced this function with an empty body
void *print_simplex_f(simplex *s, FILE *F, print_neighbor_f *pnfin){ return NULL; }
/*void *print_simplex_f(simplex *s, FILE *F, print_neighbor_f *pnfin){
    // TJH: added this line
    if(!F) return NULL;

    static print_neighbor_f *pnf;

    if (pnfin) {pnf=pnfin; if (!s) return NULL;}

    print_simplex_num(F, s);
    fprintf(F, "\n");
    if (!s) return NULL;
    fprintf(F, "normal ="); print_basis(F, s->normal); fprintf(F, "\n");
    fprintf(F, "peak ="); (*pnf)(F, &(s->peak));
    fprintf (F, "facet =\n");fflush(F);
    return print_facet(F, s, pnf);
}*/

// TJH: replaced this function with an empty body
void *print_simplex(simplex *s, void *Fin) { return NULL; }
/*void *print_simplex(simplex *s, void *Fin) {
    static FILE *F;

    if (Fin) {F=(FILE*)Fin; if (!s) return NULL;}

    return print_simplex_f(s, F, 0);
}*/

// TJH: replaced this function with an empty body
void print_triang(simplex *root, FILE *F, print_neighbor_f *pnf) {}
/*void print_triang(simplex *root, FILE *F, print_neighbor_f *pnf) {
    print_simplex(0,F);
    print_simplex_f(0,0,pnf);
    visit_triang(root, print_simplex);
}*/

// TJH: this code isn't used anywhere
//void *p_peak_test(simplex *s) {return (s->peak.vert==p) ? (void*)s : (void*)NULL;}



void *check_simplex(simplex *s, void *dum){

    int i,j,k,l;
    neighbor *sn, *snn, *sn2;
    simplex *sns;
    site vn;

    for (i=-1,sn=s->neigh-1;i<cdim;i++,sn++) {
        sns = sn->simp;
        if (!sns) {
            // TJH: added this if statement
            if(DFILE)
                fprintf(DFILE, "check_triang; bad simplex\n");
            print_simplex_f(s, DFILE, &print_neighbor_full);
            // TJH: added this if statement
            if(DFILE)
                fprintf(DFILE, "site_num(p)=%ld\n",  site_num(p));
            return s;
        }
        if (!s->peak.vert && sns->peak.vert && i!=-1) {
            // TJH: added this if statement
            if(DFILE)
                fprintf(DFILE, "huh?\n");
            print_simplex_f(s, DFILE, &print_neighbor_full);
            print_simplex_f(sns, DFILE, &print_neighbor_full);
            /* TJH exit(1);*/ ASSERT(pcFALSE);
        }
        for (j=-1,snn=sns->neigh-1; j<cdim && snn->simp!=s; j++,snn++);
        if (j==cdim) {
            // TJH: added this if statement
            if(DFILE)
                fprintf(DFILE, "adjacency failure:\n");
            DEBEXP(-1,site_num(p))
            print_simplex_f(sns, DFILE, &print_neighbor_full);
            print_simplex_f(s, DFILE, &print_neighbor_full);
            /* TJH exit(1);*/ ASSERT(pcFALSE);
        }
        for (k=-1,snn=sns->neigh-1; k<cdim; k++,snn++){
            vn = snn->vert;
            if (k!=j) {
                for (l=-1,sn2 = s->neigh-1;
                     l<cdim && sn2->vert != vn;
                     l++,sn2++);
                if (l==cdim) {
                    // TJH: added this if statement
                    if(DFILE)
                    {
                        fprintf(DFILE, "cdim=%d\n",cdim);
                        fprintf(DFILE, "error: neighboring simplices with incompatible vertices:\n");
                    }
                    print_simplex_f(sns, DFILE, &print_neighbor_full);
                    print_simplex_f(s, DFILE, &print_neighbor_full);
                    /* TJH exit(1);*/ ASSERT(pcFALSE);
                }   
            }
        }
    }
    return NULL;
}

int p_neight(simplex *s, int i, void *dum) {return s->neigh[i].vert !=p;}

void check_triang(simplex *root){visit_triang(root, &check_simplex);}

void check_new_triangs(simplex *s){visit_triang_gen(s, check_simplex, p_neight);}


/* outfuncs: given a list of points, output in a given format */

void vv_out(point *v, int vdim, FILE *Fin, int amble) {
    /* sunghee */
    static FILE *F;
    int i,j;

    if (Fin) {F=Fin; if (!v) return;}
    for (j=0;j<vdim;j++) {
        for (i=0;i<3;i++) {
            fprintf(F, "%G ", v[j][i]/mult_up);
        }
        fprintf(F, " | ");
    } 
    fprintf(F, "\n");
    return;
}

// TJH: I *think* this is what was meant (old version below)
// OK, so the functionality isn't identical but using static like this is cheesy.
void vlist_out(point *v, int vdim, FILE *Fout, int amble) 
{
    if(!Fout) return;
    if(!v) return;

    for (int j=0;j<vdim;j++) 
        fprintf(Fout, "%ld ", (site_num)(v[j]));
    fprintf(Fout,"\n");
}
/*void vlist_out(point *v, int vdim, FILE *Fin, int amble) {

    static FILE *F;
    int j;

    if (Fin) {F=Fin; if (!v) return;}

    for (j=0;j<vdim;j++) fprintf(F, "%ld ", (site_num)(v[j]));
    fprintf(F,"\n");

    return;
}*/

void off_out(point *v, int vdim, FILE *Fin, int amble) {

    /* TJH: this code never get used, have commented it out to make this clear

    static FILE *F, *G;
    static FILE *OFFFILE;
    static char offfilenam[L_tmpnam];
    char comst[100], buf[200];
    int j,i;

    if (Fin) {F=Fin;}

    if (pdim!=3) { warning(-10, off apparently for 3d points only); return;}

    if (amble==0) {
        for (i=0;i<vdim;i++) if (v[i]==infinity) return;
        fprintf(OFFFILE, "%d ", vdim);
        for (j=0;j<vdim;j++) fprintf(OFFFILE, "%ld ", (site_num)(v[j]));
        fprintf(OFFFILE,"\n");
    } else if (amble==-1) {
        OFFFILE = efopen(tmpnam(offfilenam), "w");
    } else {
        efclose(OFFFILE);

        fprintf(F, "    OFF\n");
    
        sprintf(comst, "wc %s", tmpfilenam);
        G = epopen(comst, "r");
        fscanf(G, "%d", &i);
        fprintf(F, " %d", i);
        pclose(G);
    
        sprintf(comst, "wc %s", offfilenam);
        G = epopen(comst, "r");
        fscanf(G, "%d", &i);
        fprintf(F, " %d", i);
        pclose(G);
    
        fprintf (F, " 0\n");
    
        G = efopen(tmpfilenam, "r");
        while (fgets(buf, sizeof(buf), G)) fprintf(F, "%s", buf);
        efclose(G);
    
        G = efopen(offfilenam, "r");
    
    
        while (fgets(buf, sizeof(buf), G)) fprintf(F, "%s", buf);
        efclose(G);
    }

    */
}



void mp_out(point *v, int vdim, FILE *Fin, int amble) {

    /* should fix scaling */

    /* TJH: this code never gets used, have commented it out to make clear

    static int figno=1;
    static FILE *F;

    if (Fin) {F=Fin;}

    if (pdim!=2) { warning(-10, mp for planar points only); return;}
    if (amble==0) {
        int i;
        if (!v) return;
        for (i=0;i<vdim;i++) if (v[i]==infinity) {
            point t=v[i];
            v[i]=v[vdim-1];
            v[vdim-1] = t;
            vdim--;
            break;
        }
        fprintf(F, "draw ");
        for (i=0;i<vdim;i++) 
            fprintf(F,
                    (i+1<vdim) ? "(%Gu,%Gu)--" : "(%Gu,%Gu);\n",
                    v[i][0]/mult_up,v[i][1]/mult_up
                );
    } else if (amble==-1) {
        if (figno==1) fprintf(F, "u=1pt;\n");
        fprintf(F , "beginfig(%d);\n",figno++);
    } else if (amble==1) {
        fprintf(F , "endfig;\n");
    }

    */
}


void ps_out(point *v, int vdim, FILE *Fin, int amble) {

    /* TJH: this code never gets used, have commented out to make clear

    static FILE *F;
    static double scaler;

    if (Fin) {F=Fin;}

    if (pdim!=2) { warning(-10, ps for planar points only); return;}

    if (amble==0) {
        int i;
        if (!v) return;
        for (i=0;i<vdim;i++) if (v[i]==infinity) {
            point t=v[i];
            v[i]=v[vdim-1];
            v[vdim-1] = t;
            vdim--;
            break;
        }
        fprintf(F,
                "newpath %G %G moveto\n",
                v[0][0]*scaler,v[0][1]*scaler);
        for (i=1;i<vdim;i++) 
            fprintf(F,
                    "%G %G lineto\n",
                    v[i][0]*scaler,v[i][1]*scaler
                );
        fprintf(F, "stroke\n");
    } else if (amble==-1) {
        float len[2], maxlen;
        fprintf(F, "%%!PS\n");
        len[0] = maxs[0]-mins[0]; len[1] = maxs[1]-mins[1];
        maxlen = (len[0]>len[1]) ? len[0] : len[1];
        scaler = 216/maxlen;
    
        fprintf(F, "%%%%BoundingBox: %G %G %G %G \n",
                mins[0]*scaler,
                mins[1]*scaler,
                maxs[0]*scaler,
                maxs[1]*scaler);
        fprintf(F, "%%%%Creator: hull program\n");
        fprintf(F, "%%%%Pages: 1\n");
        fprintf(F, "%%%%EndProlog\n");
        fprintf(F, "%%%%Page: 1 1\n");
        fprintf(F, " 0.5 setlinewidth [] 0 setdash\n");
        fprintf(F, " 1 setlinecap 1 setlinejoin 10 setmiterlimit\n");
    } else if (amble==1) {
        fprintf(F , "showpage\n %%%%EOF\n");
    }

    */
}

void cpr_out(point *v, int vdim, FILE *Fin, int amble) {

    /* TJH: this code never gets used, have commented out to make clear

    static FILE *F;
    int i;

    if (Fin) {F=Fin; if (!v) return;}

    if (pdim!=3) { warning(-10, cpr for 3d points only); return;}
    
    for (i=0;i<vdim;i++) if (v[i]==infinity) return;

    fprintf(F, "t %G %G %G %G %G %G %G %G %G 3 128\n",
            v[0][0]/mult_up,v[0][1]/mult_up,v[0][2]/mult_up,
            v[1][0]/mult_up,v[1][1]/mult_up,v[1][2]/mult_up,
            v[2][0]/mult_up,v[2][1]/mult_up,v[2][2]/mult_up
        );

    */
}


/* vist_funcs for different kinds of output: facets, alpha shapes, etc. */



void *facets_print(simplex *s, void *p) {

    static out_func *out_func_here;
    point v[MAXDIM];
    int j;
 
    if (p) {out_func_here = (out_func*)p; if (!s) return NULL;} 
 
    for (j=0;j<cdim;j++) v[j] = s->neigh[j].vert;
 
    out_func_here(v,cdim,0,0);
 
    return NULL;
}  

void *ridges_print(simplex *s, void *p) {

    static out_func *out_func_here;
    point v[MAXDIM];
    int j,k,vnum;

    if (p) {out_func_here = (out_func*)p; if (!s) return NULL;}

    for (j=0;j<cdim;j++) {
        vnum=0;
        for (k=0;k<cdim;k++) {
            if (k==j) continue;
            v[vnum++] = (s->neigh[k].vert);
        }
        out_func_here(v,cdim-1,0,0);
    }
    return NULL; 
}


void *afacets_print(simplex *s, void *p) {

    static out_func *out_func_here;
    point v[MAXDIM];
    int j,k,vnum;

    if (p) {out_func_here = (out_func*)p; if (!s) return NULL;}

    for (j=0;j<cdim;j++) { /* check for ashape consistency */
        for (k=0;k<cdim;k++) if (s->neigh[j].simp->neigh[k].simp==s) break;
        if (alph_test(s,j,0)!=alph_test(s->neigh[j].simp,k,0)) {
            DEB(-10,alpha-shape not consistent)
                DEBTR(-10)
                print_simplex_f(s,DFILE,&print_neighbor_full);
            print_simplex_f(s->neigh[j].simp,DFILE,&print_neighbor_full);
            fflush(DFILE);
            /* TJH exit(1);*/ ASSERT(pcFALSE);
        }
    }
    for (j=0;j<cdim;j++) {
        vnum=0;
        if (alph_test(s,j,0)) continue;
        for (k=0;k<cdim;k++) {
            if (k==j) continue;
            v[vnum++] = s->neigh[k].vert;
        }
        out_func_here(v,cdim-1,0,0);
    }
    return NULL;
}
//========label.c=============================================================
/*
 * Power Crust software, by Nina Amenta, Sunghee Choi and Ravi Krishna Kolluri.
 * Copyright (c) 2000 by the University of Texas
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee under the GNU Public License is hereby granted, 
 * provided that this entire notice  is included in all copies of any software 
 * which is or includes a copy or modification of this software and in all copies 
 * of the supporting documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <float.h>
#include <math.h>

//#include "hull.h"   TJH: this file is now above


extern struct heap_array *heap_A;
extern int heap_length,heap_size; 
extern struct polelabel *adjlist;
extern struct plist **opplist;
extern double theta, deep;
extern int defer;

int propagate()
{
    int pid;

    pid = extract_max();
    if (adjlist[pid].in > adjlist[pid].out) 
        adjlist[pid].label = IN;
    else adjlist[pid].label = OUT;
    /*  fprintf(DFILE,"pole %d in %f out %f label %d\n",pid, adjlist[pid].in, adjlist[pid].out, adjlist[pid].label);  */
    
    if (pid != -1) {
        /*    fprintf(DFILE,"propagating pole %d..\n",pid);  */
        opp_update(pid);
        sym_update(pid);
    }
    return pid;
}

void opp_update(int pi)
{
    struct plist *pindex;
    int npi, nhi;
    double temp;

    pindex = opplist[pi];
    while (pindex!=NULL) { 
        npi = pindex->pid;
        if (defer) {
            if (adjlist[npi].bad == BAD_POLE) {
                /*  fprintf(DFILE,"found bad pole.. defer its labeling\n"); */
                pindex = pindex->next;
                continue; 
            } 
        }
        if (adjlist[npi].label == INIT) { /* not yet labeled */
            if (adjlist[npi].hid == 0) { /* not in the heap */
                if (adjlist[pi].in > adjlist[pi].out) {
                    /* propagate in*cos to out */
                    adjlist[npi].out = (-1.0) * adjlist[pi].in * pindex->angle;
                    /*  fprintf(DFILE,"pole %d.out = %f\n",npi,adjlist[npi].out); */
                    insert_heap(npi,adjlist[npi].out);
                }
                else if (adjlist[pi].in < adjlist[pi].out) {
                    /* propagate out*cos to in */
                    adjlist[npi].in = (-1.0) * adjlist[pi].out * pindex->angle;
                    /* fprintf(DFILE,"pole %d.in = %f\n",npi,adjlist[npi].in); */
                    insert_heap(npi,adjlist[npi].in);
                }
            }
            else { /* in the heap */
                nhi = adjlist[npi].hid;
                if (adjlist[pi].in > adjlist[pi].out) {
                    /* propagate in*cos to out */
                    temp = (-1.0) * adjlist[pi].in * pindex->angle;
                    if (temp > adjlist[npi].out) { 
                        adjlist[npi].out = temp;
                        update_pri(nhi,npi);
                    }
                }
                else if (adjlist[pi].in < adjlist[pi].out) {
                    /* propagate out*cos to in */
                    temp = (-1.0) * adjlist[pi].out * pindex->angle;
                    if (temp > adjlist[npi].in) {
                        adjlist[npi].in = temp;
                        update_pri(nhi,npi);
                    }
                }
            }
        }
        pindex = pindex->next;
    }
}

void sym_update(int pi)
{
    struct edgesimp *eindex;
    int npi, nhi;
    double temp;

    eindex = adjlist[pi].eptr;
    while (eindex!=NULL) {
        npi = eindex->pid;
        if (defer) {
            if (adjlist[npi].bad == BAD_POLE) {
                eindex = eindex->next;
                /*fprintf(DFILE,"found bad pole.. defer its labeling\n");*/
                continue;
            }
        }

        /* try to label deeply intersecting unlabeled neighbors */
        if  ((adjlist[npi].label==INIT) && (eindex->angle > theta)) { 
            /* not yet labeled */
            if (adjlist[npi].hid == 0) { /* not in the heap */
                if (adjlist[pi].in > adjlist[pi].out) {
                    /* propagate in*cos to in */
                    adjlist[npi].in = adjlist[pi].in * eindex->angle;
                    insert_heap(npi,adjlist[npi].in);
                }
                else if (adjlist[pi].in < adjlist[pi].out) {
                    /* propagate out*cos to out */
                    adjlist[npi].out = adjlist[pi].out * eindex->angle;
                    insert_heap(npi,adjlist[npi].out);
                }
            }
            else { /* in the heap */
                if (heap_A[adjlist[npi].hid].pid != npi) fprintf(DFILE,"ERROR\n");
                nhi = adjlist[npi].hid;
                if (adjlist[pi].in > adjlist[pi].out) {
                    /* propagate in*cos to in */
                    temp = adjlist[pi].in * eindex->angle;
                    if (temp > adjlist[npi].in) { 
                        adjlist[npi].in = temp;
                        update_pri(nhi,npi);
                    }
                }
                else if (adjlist[pi].in < adjlist[pi].out) {
                    /* propagate out*cos to out */
                    temp = adjlist[pi].out * eindex->angle;
                    if (temp > adjlist[npi].out) {
                        adjlist[npi].out = temp;
                        update_pri(nhi,npi);
                    }
                }
            }
        }
        eindex = eindex->next;
    }
}

void update_pri(int hi, int pi) 
    /* update heap_A[hi].pri using adjlist[pi].in/out */
{
    double pr;

    if ((heap_A[hi].pid != pi)||(adjlist[pi].hid != hi)) {
        fprintf(DFILE,"Error update_pri!\n");
        return;
    }
    if (adjlist[pi].in==0.0) {
        pr = adjlist[pi].out;
    }
    else if (adjlist[pi].out == 0.0) {
        pr = adjlist[pi].in;
    }
    else { /* both in/out nonzero */
        if (adjlist[pi].in > adjlist[pi].out) {
            pr =  adjlist[pi].in - adjlist[pi].out - 1;
        }
        else {
            pr = adjlist[pi].out - adjlist[pi].in - 1;
        }
    }
    update(hi,pr);
}

void label_unlabeled(int num)
{
    struct plist *pindex;
    struct edgesimp *eindex;
    int npi,i, opplabel;
    int tlabel;
    double tangle, tangle1;


    for (i=0;i<num;i++) { 
        if (adjlist[i].label == INIT) { /* pole i is unlabeled.. try to label now */
            tlabel = INIT;
            opplabel = INIT;
            pindex = opplist[i];
            if ((pindex == NULL)&&(adjlist[i].eptr==NULL)) {
                fprintf(DFILE,"no opp pole, no adjacent pole!\n");
                continue;
            }
            /* check whether there is opp pole */
            while (pindex!=NULL) { /* opp pole */
                npi = pindex->pid;
                if (adjlist[npi].label != INIT) {
                    fprintf(DFILE,"opp is labeled\n");
                    if (opplabel == INIT) opplabel = adjlist[npi].label;
                    else if (opplabel != adjlist[npi].label) {
                        /* opp poles have different labels ... inconsistency! */
                        fprintf(DFILE,"opp poles have inconsistent labels\n");
                        opplabel = INIT; /* ignore the label of opposite poles */
                    } 
                }
                pindex = pindex->next;
            }

            tangle = -3.0;
            tangle1 = -3.0;
            eindex = adjlist[i].eptr;
            while (eindex != NULL) {
                npi = eindex->pid; 
                if (adjlist[npi].label == IN) {
                    if (tangle < eindex->angle) {
                        tangle = eindex->angle;
                    }
                }
                else if (adjlist[npi].label == OUT) {
                    if (tangle1 < eindex->angle) {
                        tangle1 = eindex->angle;
                    }
                }
                eindex = eindex->next;
            }
            /* now tangle, tangle 1 are angles of most deeply interesecting in, out poles */
            if (tangle == -3.0) { /* there was no in poles */
                if (tangle1 == -3.0) { /* there was no out poles */
                    if (opplabel == INIT) { /* cannot trust opp pole or no labeled opp pole */
                        // TJH: added this if statement
                        if(DFILE)
                            fprintf(DFILE, "1: cannot label pole %d\n", i);
                    }
                    else if (opplabel == IN) {
                        adjlist[i].label = OUT;
                    }
                    else  adjlist[i].label = IN;
                }
                else if (tangle1 > deep) { /* interesecting deeply only out poles */
                    adjlist[i].label = OUT;
                }
                else { /* no deeply intersecting poles . use opp pole */
                    if (opplabel == INIT) { /* cannot trust opp pole or no labeled opp pole */
                        // TJH: added this if statement
                        if(DFILE)
                            fprintf(DFILE, "2: cannot label pole %d\n", i);
                    }
                    else if (opplabel == IN) {
                        adjlist[i].label = OUT;
                    }
                    else  adjlist[i].label = IN;
                }
            }
            else if (tangle1 == -3.0) { /* there are in pole but no out pole */
                if (tangle > deep) { /* interesecting deeply only in poles */
                    adjlist[i].label = IN;
                }
                else { /* no deeply intersecting poles . use opp pole */
                    if (opplabel == INIT) { /* cannot trust opp pole or no labeled opp pole */
                        // TJH: added this if statement
                        if(DFILE)
                            fprintf(DFILE, "2: cannot label pole %d\n", i);
                    }
                    else if (opplabel == IN) {
                        adjlist[i].label = OUT;
                    }
                    else  adjlist[i].label = IN;
                }
            }
            else { /* there are both in/out poles */
                if (tangle > deep) {
                    if (tangle1 > deep) { /* intersecting both deeply */
                        /* use opp */
                        if (opplabel == INIT) { /* cannot trust opp pole or no labeled opp pole */
                            /* then give label with bigger angle */
                            // TJH: added this if statement
                            if(DFILE)
                                fprintf(DFILE,"intersect both deeply but no opp,in %f out %f.try to label more deeply intersected label.\n", tangle, tangle1);
                            if (tangle > tangle1) {
                                adjlist[i].label = IN;
                            }
                            else adjlist[i].label = OUT;
                        }
                        else if (opplabel == IN) {
                            adjlist[i].label = OUT;
                        }
                        else  adjlist[i].label = IN;
                    }
                    else { /* intersecting only in deeply */
                        adjlist[i].label = IN;
                    }
                }
                else if (tangle1 > deep) { /* intersecting only out deeply */
                    adjlist[i].label = OUT;
                }
                else { /* no deeply intersecting poles . use opp pole */
                    if (opplabel == INIT) { /* cannot trust opp pole or no labeled opp pole */
                        // TJH: added this if statement
                        if(DFILE)
                            fprintf(DFILE, "3: cannot label pole %d\n", i); 
                    }
                    else if (opplabel == IN) {
                        adjlist[i].label = OUT;
                    }
                    else  adjlist[i].label = IN;
                }
            } 



 
            /* no labeled opp pole - label pole same as the most deeply intersecting labeled pole ... no longer needed because opp values are already propagated..
               tangle = -3.0;
               eindex = adjlist[i].eptr;
               while (eindex != NULL) {
               npi = eindex->pid; 
               if (adjlist[npi].label == IN) {
               if (tangle < eindex->angle) {
               tangle = eindex->angle;
               tlabel = IN;
               }
               }
               else if (adjlist[npi].label == OUT) {
               if (tangle < eindex->angle) {
               tangle = eindex->angle;
               tlabel = OUT;
               }
               }
               eindex = eindex->next;
               }
               fprintf(DFILE,"pole %d  max angle %f label %d\n", i,tangle,tlabel);
               adjlist[i].label = tlabel;    
            */


        }
    }
}
//========math.c=============================================================
/*
 * Power Crust software, by Nina Amenta, Sunghee Choi and Ravi Krishna Kolluri.
 * Copyright (c) 2000 by the University of Texas
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee under the GNU Public License is hereby granted, 
 * provided that this entire notice  is included in all copies of any software 
 * which is or includes a copy or modification of this software and in all copies 
 * of the supporting documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <math.h>
#include <float.h> /* used for DBL_MAX macro definition */

//#include "hull.h" /* sunghee */  TJH: this file is now above

void normalize(double a[3])
{
    double t;
  
    t =SQ(a[0])+SQ(a[1])+SQ(a[2]);
    t = sqrt(t);
    a[0]=a[0]/t;
    a[2]=a[2]/t;
    a[1]=a[1]/t;
}

double sqdist(double a[3], double b[3]) 
{
  /* returns the squared distance between a and b */ 
  return SQ(a[0]-b[0])+SQ(a[1]-b[1])+SQ(a[2]-b[2]);
}

void dir_and_dist(double a[3], double b[3], double dir[3], double* dist) {
    int k;

    for (k=0; k<3; k++) dir[k] = b[k] - a[k];
    *dist = sqrt( SQ(dir[0])+SQ(dir[1])+SQ(dir[2]));
    for (k=0; k<3; k++) dir[k] = dir[k] / (*dist);
}



void crossabc(double a[3], double b[3], double c[3], double n[3])
{
    double t;

    n[0] = (b[1]-a[1])*(c[2]-a[2]) - (b[2]-a[2])*(c[1]-a[1]);
    n[1] = (b[2]-a[2])*(c[0]-a[0]) - (b[0]-a[0])*(c[2]-a[2]);
    n[2] = (a[0]-b[0])*(a[1]-c[1]) - (a[1]-b[1])*(a[0]-c[0]);
    t =SQ(n[0])+SQ(n[1])+SQ(n[2]);
    t = sqrt(t);
    n[0]=n[0]/t;
    n[2]=n[2]/t;
    n[1]=n[1]/t;
    /* normalized */
}

double dotabac(double a[3], double b[3], double c[3])
{
    return (b[0]-a[0])*(c[0]-a[0])+(b[1]-a[1])*(c[1]-a[1])+(b[2]-a[2])*(c[2]-a[2]);
}

double dotabc(double a[3], double b[3], double c[3])
{
    return (b[0]-a[0])*c[0]+(b[1]-a[1])*c[1]+(b[2]-a[2])*c[2];
}

/*****************************************************************************/
/*                                                                           */
/*  tetcircumcenter()   Find the circumcenter of a tetrahedron.              */
/*                                                                           */
/*  The result is returned both in terms of xyz coordinates and xi-eta-zeta  */
/*  coordinates, relative to the tetrahedron's point `a' (that is, `a' is    */
/*  the origin of both coordinate systems).  Hence, the xyz coordinates      */
/*  returned are NOT absolute; one must add the coordinates of `a' to        */
/*  find the absolute coordinates of the circumcircle.  However, this means  */
/*  that the result is frequently more accurate than would be possible if    */
/*  absolute coordinates were returned, due to limited floating-point        */
/*  precision.  In general, the circumradius can be computed much more       */
/*  accurately.                                                              */
/*                                                                           */
/*  The xi-eta-zeta coordinate system is defined in terms of the             */
/*  tetrahedron.  Point `a' is the origin of the coordinate system.          */
/*  The edge `ab' extends one unit along the xi axis.  The edge `ac'         */
/*  extends one unit along the eta axis.  The edge `ad' extends one unit     */
/*  along the zeta axis.  These coordinate values are useful for linear      */
/*  interpolation.                                                           */
/*                                                                           */
/*  If `xi' is NULL on input, the xi-eta-zeta coordinates will not be        */
/*  computed.                                                                */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declarations
/*void tetcircumcenter(a, b, c, d, circumcenter, cond)
    double a[3];
    double b[3];
    double c[3];
    double d[3];
    double circumcenter[3];
    double *cond; */
void tetcircumcenter(double a[3],double b[3],double c[3],double d[3],
                     double circumcenter[3],double *cond)
{
    double xba, yba, zba, xca, yca, zca, xda, yda, zda;
    double balength, calength, dalength;
    double xcrosscd, ycrosscd, zcrosscd;
    double xcrossdb, ycrossdb, zcrossdb;
    double xcrossbc, ycrossbc, zcrossbc;
    double denominator;
    double xcirca, ycirca, zcirca;

    /* Use coordinates relative to point `a' of the tetrahedron. */
    xba = b[0] - a[0];
    yba = b[1] - a[1];
    zba = b[2] - a[2];
    xca = c[0] - a[0];
    yca = c[1] - a[1];
    zca = c[2] - a[2];
    xda = d[0] - a[0];
    yda = d[1] - a[1];
    zda = d[2] - a[2];
    /* Squares of lengths of the edges incident to `a'. */
    balength = xba * xba + yba * yba + zba * zba;
    calength = xca * xca + yca * yca + zca * zca;
    dalength = xda * xda + yda * yda + zda * zda;
    /* Cross products of these edges. */
    xcrosscd = yca * zda - yda * zca;
    ycrosscd = zca * xda - zda * xca;
    zcrosscd = xca * yda - xda * yca;
    xcrossdb = yda * zba - yba * zda;
    ycrossdb = zda * xba - zba * xda;
    zcrossdb = xda * yba - xba * yda;
    xcrossbc = yba * zca - yca * zba;
    ycrossbc = zba * xca - zca * xba;
    zcrossbc = xba * yca - xca * yba;

    /* Calculate the denominator of the formulae. */
#ifdef EXACT
    /* Use orient3d() from http://www.cs.cmu.edu/~quake/robust.html     */
        /*   to ensure a correctly signed (and reasonably accurate) result, */
        /*   avoiding any possibility of division by zero.                  */
        *cond = orient3d(b,c,d,a);
        denominator = 0.5 / (*cond);
#else
        /* Take your chances with floating-point roundoff. */
        denominator = 0.5 / (xba * xcrosscd + yba * ycrosscd + zba * zcrosscd);
#endif

        /* Calculate offset (from `a') of circumcenter. */
        xcirca = (balength * xcrosscd + calength * xcrossdb + dalength * xcrossbc) *
            denominator;
        ycirca = (balength * ycrosscd + calength * ycrossdb + dalength * ycrossbc) *
            denominator;
        zcirca = (balength * zcrosscd + calength * zcrossdb + dalength * zcrossbc) *
            denominator;
        circumcenter[0] = xcirca;
        circumcenter[1] = ycirca;
        circumcenter[2] = zcirca;  
}


/*****************************************************************************/
/*                                                                           */
/*  tricircumcenter3d()   Find the circumcenter of a triangle in 3D.         */
/*                                                                           */
/*  The result is returned both in terms of xyz coordinates and xi-eta       */
/*  coordinates, relative to the triangle's point `a' (that is, `a' is       */
/*  the origin of both coordinate systems).  Hence, the xyz coordinates      */
/*  returned are NOT absolute; one must add the coordinates of `a' to        */
/*  find the absolute coordinates of the circumcircle.  However, this means  */
/*  that the result is frequently more accurate than would be possible if    */
/*  absolute coordinates were returned, due to limited floating-point        */
/*  precision.  In general, the circumradius can be computed much more       */
/*  accurately.                                                              */
/*                                                                           */
/*  The xi-eta coordinate system is defined in terms of the triangle.        */
/*  Point `a' is the origin of the coordinate system.  The edge `ab' extends */
/*  one unit along the xi axis.  The edge `ac' extends one unit along the    */
/*  eta axis.  These coordinate values are useful for linear interpolation.  */
/*                                                                           */
/*  If `xi' is NULL on input, the xi-eta coordinates will not be computed.   */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declarations
/*void tricircumcenter3d(a, b, c, circumcenter,cond)
    double a[3];
    double b[3];
    double c[3];
    double circumcenter[3];
    double *cond;*/
void tricircumcenter3d(double a[3],double b[3],double c[3],double circumcenter[3],
                       double *cond)
{
    double xba, yba, zba, xca, yca, zca;
    double balength, calength;
    double xcrossbc, ycrossbc, zcrossbc;
    double denominator;
    double xcirca, ycirca, zcirca;
    double ta[2],tb[2],tc[2];

    /* Use coordinates relative to point `a' of the triangle. */
    xba = b[0] - a[0];
    yba = b[1] - a[1];
    zba = b[2] - a[2];
    xca = c[0] - a[0];
    yca = c[1] - a[1];
    zca = c[2] - a[2];
    /* Squares of lengths of the edges incident to `a'. */
    balength = xba * xba + yba * yba + zba * zba;
    calength = xca * xca + yca * yca + zca * zca;
  
    /* Cross product of these edges. */
#ifdef EXACT
    /* Use orient2d() from http://www.cs.cmu.edu/~quake/robust.html     */
        /*   to ensure a correctly signed (and reasonably accurate) result, */
        /*   avoiding any possibility of division by zero.                  */
        ta[0]=b[1];ta[1]=b[2];tb[0]=c[1];tb[1]=c[2];tc[0]=a[1];tc[1]=a[2];
        xcrossbc = orient2d(ta,tb,tc);
        ta[0]=b[2];ta[1]=b[0];tb[0]=c[2];tb[1]=c[0];tc[0]=a[2];tc[1]=a[0];
        ycrossbc = orient2d(ta,tb,tc);
        ta[0]=b[0];ta[1]=b[1];tb[0]=c[0];tb[1]=c[1];tc[0]=a[0];tc[1]=a[1];
        zcrossbc = orient2d(ta,tb,tc);
#else
        /* Take your chances with floating-point roundoff. */
        xcrossbc = yba * zca - yca * zba;
        ycrossbc = zba * xca - zca * xba;
        zcrossbc = xba * yca - xca * yba;
#endif

        /* Calculate the denominator of the formulae. */
        denominator = 0.5 / (xcrossbc * xcrossbc + ycrossbc * ycrossbc +
                             zcrossbc * zcrossbc);

        /* Calculate offset (from `a') of circumcenter. */
        xcirca = ((balength * yca - calength * yba) * zcrossbc -
                  (balength * zca - calength * zba) * ycrossbc) * denominator;
        ycirca = ((balength * zca - calength * zba) * xcrossbc -
                  (balength * xca - calength * xba) * zcrossbc) * denominator;
        zcirca = ((balength * xca - calength * xba) * ycrossbc -
                  (balength * yca - calength * yba) * xcrossbc) * denominator;
        circumcenter[0] = xcirca;
        circumcenter[1] = ycirca;
        circumcenter[2] = zcirca;

}

// TJH: old style function declaration
/*void tetorthocenter(a, b, c, d, orthocenter, cnum)
    double a[4];
    double b[4];
    double c[4];
    double d[4];
    double orthocenter[3];
    double *cnum;*/
void tetorthocenter(double a[4],double b[4],double c[4],double d[4],double orthocenter[3],
                    double *cnum)
{
    double xba, yba, zba, xca, yca, zca, xda, yda, zda, wba, wca, wda;
    double balength, calength, dalength;
    double xcrosscd, ycrosscd, zcrosscd;
    double xcrossdb, ycrossdb, zcrossdb;
    double xcrossbc, ycrossbc, zcrossbc;
    double denominator;
    double xcirca, ycirca, zcirca;
    double wa,wb,wc,wd;

    wa = a[0]*a[0] + a[1]*a[1] + a[2]*a[2] - a[3];
    wb = b[0]*b[0] + b[1]*b[1] + b[2]*b[2] - b[3];
    wc = c[0]*c[0] + c[1]*c[1] + c[2]*c[2] - c[3];
    wd = d[0]*d[0] + d[1]*d[1] + d[2]*d[2] - d[3];
    /* Use coordinates relative to point `a' of the tetrahedron. */
    xba = b[0] - a[0];
    yba = b[1] - a[1];
    zba = b[2] - a[2];
    wba = wb - wa;
    xca = c[0] - a[0];
    yca = c[1] - a[1];
    zca = c[2] - a[2];
    wca = wc - wa;
    xda = d[0] - a[0];
    yda = d[1] - a[1];  
    zda = d[2] - a[2];
    wda = wd - wa;
  
    /* Squares of lengths of the edges incident to `a'. */
    balength = xba * xba + yba * yba + zba * zba - wba;
    calength = xca * xca + yca * yca + zca * zca - wca;
    dalength = xda * xda + yda * yda + zda * zda - wda;
    /* Cross products of these edges. */
    xcrosscd = yca * zda - yda * zca;
    ycrosscd = zca * xda - zda * xca;
    zcrosscd = xca * yda - xda * yca;
    xcrossdb = yda * zba - yba * zda;
    ycrossdb = zda * xba - zba * xda;
    zcrossdb = xda * yba - xba * yda;
    xcrossbc = yba * zca - yca * zba;
    ycrossbc = zba * xca - zca * xba;
    zcrossbc = xba * yca - xca * yba;
  
    /* Calculate the denominator of the formulae. */
#ifdef EXACT 
    /* Use orient3d() from http://www.cs.cmu.edu/~quake/robust.html     */
        /*   to ensure a correctly signed (and reasonably accurate) result, */
        /*   avoiding any possibility of division by zero.                  */
        *cnum = orient3d(b, c, d, a);
        denominator = 0.5 / (*cnum);
#else
        /* Take your chances with floating-point roundoff. */
        denominator = 0.5 / (xba * xcrosscd + yba * ycrosscd + zba * zcrosscd);

#endif
  
        /* Calculate offset (from `a') of circumcenter. */
        xcirca = (balength * xcrosscd + calength * xcrossdb + dalength * xcrossbc) *
            denominator;
        ycirca = (balength * ycrosscd + calength * ycrossdb + dalength * ycrossbc) *
            denominator;
        zcirca = (balength * zcrosscd + calength * zcrossdb + dalength * zcrossbc) *
            denominator;
        orthocenter[0] = xcirca;
        orthocenter[1] = ycirca;
        orthocenter[2] = zcirca;
} 
  
/*****************************************************************************/
/*                                                                           */
/*  tricircumcenter()   Find the circumcenter of a triangle.                 */
/*                                                                           */
/*  The result is returned both in terms of x-y coordinates and xi-eta       */
/*  coordinates, relative to the triangle's point `a' (that is, `a' is       */
/*  the origin of both coordinate systems).  Hence, the x-y coordinates      */
/*  returned are NOT absolute; one must add the coordinates of `a' to        */
/*  find the absolute coordinates of the circumcircle.  However, this means  */
/*  that the result is frequently more accurate than would be possible if    */
/*  absolute coordinates were returned, due to limited floating-point        */
/*  precision.  In general, the circumradius can be computed much more       */
/*  accurately.                                                              */
/*                                                                           */
/*  The xi-eta coordinate system is defined in terms of the triangle.        */
/*  Point `a' is the origin of the coordinate system.  The edge `ab' extends */
/*  one unit along the xi axis.  The edge `ac' extends one unit along the    */
/*  eta axis.  These coordinate values are useful for linear interpolation.  */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declarations
/*void triorthocenter(a, b, c, orthocenter, cnum)
    double a[3];
    double b[3];
    double c[3];
    double orthocenter[2];
    double *cnum;*/
void triorthocenter(double a[3],double b[3],double c[3],double orthocenter[2],
                    double *cnum)
{
    double xba, yba, wba, xca, yca, wca;
    double balength, calength;
    double denominator;
    double xcirca, ycirca;

    /* Use coordinates relative to point `a' of the triangle. */
    xba = b[0] - a[0];
    yba = b[1] - a[1];
    wba = b[2] - a[2];
    xca = c[0] - a[0];
    yca = c[1] - a[1];
    wca = b[2] - a[2];
    /* Squares of lengths of the edges incident to `a'. */
    balength = xba * xba + yba * yba - wba;
    calength = xca * xca + yca * yca - wca;

    /* Calculate the denominator of the formulae. */
#ifdef EXACT
    /* Use orient2d() from http://www.cs.cmu.edu/~quake/robust.html     */
        /*   to ensure a correctly signed (and reasonably accurate) result, */
        /*   avoiding any possibility of division by zero.                  */
        *cnum = orient2d(b, c, a);
        denominator = 0.5 / (*cnum);
#else
        /* Take your chances with floating-point roundoff. */
        denominator = 0.5 / (xba * yca - yba * xca);
#endif

        /* Calculate offset (from `a') of circumcenter. */
        xcirca = (yca * balength - yba * calength) * denominator;  
        ycirca = (xba * calength - xca * balength) * denominator;  
        orthocenter[0] = xcirca;
        orthocenter[1] = ycirca;

}
//========pointops.c=============================================================
/*
 * Ken Clarkson wrote this.  Copyright (c) 1995 by AT&T..
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#include <float.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

//#include "points.h"  TJH: this file is now above

int  pdim;  /* point dimension */

#define NEARZERO(d)  ((d) < FLT_EPSILON && (d) > -FLT_EPSILON)
Coord maxdist(int dim, point p1, point p2) {
  Coord  x,y,
    d = 0;
  int i = dim;


  while (i--) {
    x = *p1++;
    y = *p2++;
    d += (x<y) ? y-x : x-y ;
  }

  return d;
}

void print_point(FILE *F, int dim, point p) {
  int j;
  if (!p) {
    fprintf(F, "NULL");
    return;
  }
  for (j=0;j<dim;j++) fprintf(F, "%g  ", *p++);
}

void print_point_int(FILE *F, int dim, point p) {
  int j;
  if (!p) {
    fprintf(F, "NULL");
    return;
  }
  for (j=0;j<dim;j++) fprintf(F, "%.20g  ", *p++);
}


int scale(int dim, point p) {
  Coord max = 0;
  int i;
  Coord abs,val;
  for (i=0;i<dim;i++) {
    val = p[i];
    abs = (val > 0) ? val: -val;
    max = (abs > max) ? abs : max;
  }


  if (max< 100*DBL_EPSILON) {
    fprintf(stderr, "fails to scale: ");
    print_point(stderr, dim,p);fflush(stderr);
    fprintf(stderr, "\n");
    return 1;
  }

  for (i=0;i<dim;i++) p[i] /= max;

  return 0;
}


/*
  int normalize(int dim, point p) {
  Coord norm;
  int i;

  norm = norm2(dim,p);

  if (norm < 3*FLT_EPSILON) {
  fprintf(stderr, "fails to normalize: ");
  print_point(dim,p);fflush(stdout);
  fprintf(stderr, "\n");
  return 1;
  }


  for (i=0;i<dim;i++) p[i] /= norm;

  return 0;
  }

*/
//========predicates.c=============================================================
/*****************************************************************************/
/*                                                                           */
/*  Routines for Arbitrary Precision Floating-point Arithmetic               */
/*  and Fast Robust Geometric Predicates                                     */
/*  (predicates.c)                                                           */
/*                                                                           */
/*  May 18, 1996                                                             */
/*                                                                           */
/*  Placed in the public domain by                                           */
/*  Jonathan Richard Shewchuk                                                */
/*  School of Computer Science                                               */
/*  Carnegie Mellon University                                               */
/*  5000 Forbes Avenue                                                       */
/*  Pittsburgh, Pennsylvania  15213-3891                                     */
/*  jrs@cs.cmu.edu                                                           */
/*                                                                           */
/*  This file contains C implementation of algorithms for exact addition     */
/*    and multiplication of floating-point numbers, and predicates for       */
/*    robustly performing the orientation and incircle tests used in         */
/*    computational geometry.  The algorithms and underlying theory are      */
/*    described in Jonathan Richard Shewchuk.  "Adaptive Precision Floating- */
/*    Point Arithmetic and Fast Robust Geometric Predicates."  Technical     */
/*    Report CMU-CS-96-140, School of Computer Science, Carnegie Mellon      */
/*    University, Pittsburgh, Pennsylvania, May 1996.  (Submitted to         */
/*    Discrete & Computational Geometry.)                                    */
/*                                                                           */
/*  This file, the paper listed above, and other information are available   */
/*    from the Web page http://www.cs.cmu.edu/~quake/robust.html .           */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/*  Using this code:                                                         */
/*                                                                           */
/*  First, read the short or long version of the paper (from the Web page    */
/*    above).                                                                */
/*                                                                           */
/*  Be sure to call exactinit() once, before calling any of the arithmetic   */
/*    functions or geometric predicates.  Also be sure to turn on the        */
/*    optimizer when compiling this file.                                    */
/*                                                                           */
/*                                                                           */
/*  Several geometric predicates are defined.  Their parameters are all      */
/*    points.  Each point is an array of two or three floating-point         */
/*    numbers.  The geometric predicates, described in the papers, are       */
/*                                                                           */
/*    orient2d(pa, pb, pc)                                                   */
/*    orient2dfast(pa, pb, pc)                                               */
/*    orient3d(pa, pb, pc, pd)                                               */
/*    orient3dfast(pa, pb, pc, pd)                                           */
/*    incircle(pa, pb, pc, pd)                                               */
/*    incirclefast(pa, pb, pc, pd)                                           */
/*    insphere(pa, pb, pc, pd, pe)                                           */
/*    inspherefast(pa, pb, pc, pd, pe)                                       */
/*                                                                           */
/*  Those with suffix "fast" are approximate, non-robust versions.  Those    */
/*    without the suffix are adaptive precision, robust versions.  There     */
/*    are also versions with the suffices "exact" and "slow", which are      */
/*    non-adaptive, exact arithmetic versions, which I use only for timings  */
/*    in my arithmetic papers.                                               */
/*                                                                           */
/*                                                                           */
/*  An expansion is represented by an array of floating-point numbers,       */
/*    sorted from smallest to largest magnitude (possibly with interspersed  */
/*    zeros).  The length of each expansion is stored as a separate integer, */
/*    and each arithmetic function returns an integer which is the length    */
/*    of the expansion it created.                                           */
/*                                                                           */
/*  Several arithmetic functions are defined.  Their parameters are          */
/*                                                                           */
/*    e, f           Input expansions                                        */
/*    elen, flen     Lengths of input expansions (must be >= 1)              */
/*    h              Output expansion                                        */
/*    b              Input scalar                                            */
/*                                                                           */
/*  The arithmetic functions are                                             */
/*                                                                           */
/*    grow_expansion(elen, e, b, h)                                          */
/*    grow_expansion_zeroelim(elen, e, b, h)                                 */
/*    expansion_sum(elen, e, flen, f, h)                                     */
/*    expansion_sum_zeroelim1(elen, e, flen, f, h)                           */
/*    expansion_sum_zeroelim2(elen, e, flen, f, h)                           */
/*    fast_expansion_sum(elen, e, flen, f, h)                                */
/*    fast_expansion_sum_zeroelim(elen, e, flen, f, h)                       */
/*    linear_expansion_sum(elen, e, flen, f, h)                              */
/*    linear_expansion_sum_zeroelim(elen, e, flen, f, h)                     */
/*    scale_expansion(elen, e, b, h)                                         */
/*    scale_expansion_zeroelim(elen, e, b, h)                                */
/*    compress(elen, e, h)                                                   */
/*                                                                           */
/*  All of these are described in the long version of the paper; some are    */
/*    described in the short version.  All return an integer that is the     */
/*    length of h.  Those with suffix _zeroelim perform zero elimination,    */
/*    and are recommended over their counterparts.  The procedure            */
/*    fast_expansion_sum_zeroelim() (or linear_expansion_sum_zeroelim() on   */
/*    processors that do not use the round-to-even tiebreaking rule) is      */
/*    recommended over expansion_sum_zeroelim().  Each procedure has a       */
/*    little note next to it (in the code below) that tells you whether or   */
/*    not the output expansion may be the same array as one of the input     */
/*    expansions.                                                            */
/*                                                                           */
/*                                                                           */
/*  If you look around below, you'll also find macros for a bunch of         */
/*    simple unrolled arithmetic operations, and procedures for printing     */
/*    expansions (commented out because they don't work with all C           */
/*    compilers) and for generating random floating-point numbers whose      */
/*    significand bits are all random.  Most of the macros have undocumented */
/*    requirements that certain of their parameters should not be the same   */
/*    variable; for safety, better to make sure all the parameters are       */
/*    distinct variables.  Feel free to send email to jrs@cs.cmu.edu if you  */
/*    have questions.                                                        */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#include <sys/time.h> // TJH commented out, replaced with the below
#include <time.h>

//#include "hull.h" /* sunghee */  TJH: this file is now above
/* On some machines, the exact arithmetic routines might be defeated by the  */
/*   use of internal extended precision floating-point registers.  Sometimes */
/*   this problem can be fixed by defining certain values to be volatile,    */
/*   thus forcing them to be stored to memory and rounded off.  This isn't   */
/*   a great solution, though, as it slows the arithmetic down.              */
/*                                                                           */
/* To try this out, write "#define INEXACT volatile" below.  Normally,       */
/*   however, INEXACT should be defined to be nothing.  ("#define INEXACT".) */

#define INEXACT                          /* Nothing */
/* #define INEXACT volatile */

#define REAL double                      /* float or double */
#define REALPRINT doubleprint
#define REALRAND doublerand
#define NARROWRAND narrowdoublerand
#define UNIFORMRAND uniformdoublerand

/* Which of the following two methods of finding the absolute values is      */
/*   fastest is compiler-dependent.  A few compilers can inline and optimize */
/*   the fabs() call; but most will incur the overhead of a function call,   */
/*   which is disastrously slow.  A faster way on IEEE machines might be to  */
/*   mask the appropriate bit, but that's difficult to do in C.              */

#define Absolute(a)  ((a) >= 0.0 ? (a) : -(a))
/* #define Absolute(a)  fabs(a) */

/* Many of the operations are broken up into two pieces, a main part that    */
/*   performs an approximate operation, and a "tail" that computes the       */
/*   roundoff error of that operation.                                       */
/*                                                                           */
/* The operations Fast_Two_Sum(), Fast_Two_Diff(), Two_Sum(), Two_Diff(),    */
/*   Split(), and Two_Product() are all implemented as described in the      */
/*   reference.  Each of these macros requires certain variables to be       */
/*   defined in the calling routine.  The variables `bvirt', `c', `abig',    */
/*   `_i', `_j', `_k', `_l', `_m', and `_n' are declared `INEXACT' because   */
/*   they store the result of an operation that may incur roundoff error.    */
/*   The input parameter `x' (or the highest numbered `x_' parameter) must   */
/*   also be declared `INEXACT'.                                             */

#define Fast_Two_Sum_Tail(a, b, x, y) \
  bvirt = x - a; \
  y = b - bvirt

#define Fast_Two_Sum(a, b, x, y) \
  x = (REAL) (a + b); \
  Fast_Two_Sum_Tail(a, b, x, y)

#define Fast_Two_Diff_Tail(a, b, x, y) \
  bvirt = a - x; \
  y = bvirt - b

#define Fast_Two_Diff(a, b, x, y) \
  x = (REAL) (a - b); \
  Fast_Two_Diff_Tail(a, b, x, y)

#define Two_Sum_Tail(a, b, x, y) \
  bvirt = (REAL) (x - a); \
  avirt = x - bvirt; \
  bround = b - bvirt; \
  around = a - avirt; \
  y = around + bround

#define Two_Sum(a, b, x, y) \
  x = (REAL) (a + b); \
  Two_Sum_Tail(a, b, x, y)

#define Two_Diff_Tail(a, b, x, y) \
  bvirt = (REAL) (a - x); \
  avirt = x + bvirt; \
  bround = bvirt - b; \
  around = a - avirt; \
  y = around + bround

#define Two_Diff(a, b, x, y) \
  x = (REAL) (a - b); \
  Two_Diff_Tail(a, b, x, y)

#define Split(a, ahi, alo) \
  c = (REAL) (splitter * a); \
  abig = (REAL) (c - a); \
  ahi = c - abig; \
  alo = a - ahi

#define Two_Product_Tail(a, b, x, y) \
  Split(a, ahi, alo); \
  Split(b, bhi, blo); \
  err1 = x - (ahi * bhi); \
  err2 = err1 - (alo * bhi); \
  err3 = err2 - (ahi * blo); \
  y = (alo * blo) - err3

#define Two_Product(a, b, x, y) \
  x = (REAL) (a * b); \
  Two_Product_Tail(a, b, x, y)

/* Two_Product_Presplit() is Two_Product() where one of the inputs has       */
/*   already been split.  Avoids redundant splitting.                        */

#define Two_Product_Presplit(a, b, bhi, blo, x, y) \
  x = (REAL) (a * b); \
  Split(a, ahi, alo); \
  err1 = x - (ahi * bhi); \
  err2 = err1 - (alo * bhi); \
  err3 = err2 - (ahi * blo); \
  y = (alo * blo) - err3

/* Two_Product_2Presplit() is Two_Product() where both of the inputs have    */
/*   already been split.  Avoids redundant splitting.                        */

#define Two_Product_2Presplit(a, ahi, alo, b, bhi, blo, x, y) \
  x = (REAL) (a * b); \
  err1 = x - (ahi * bhi); \
  err2 = err1 - (alo * bhi); \
  err3 = err2 - (ahi * blo); \
  y = (alo * blo) - err3

/* Square() can be done more quickly than Two_Product().                     */

#define Square_Tail(a, x, y) \
  Split(a, ahi, alo); \
  err1 = x - (ahi * ahi); \
  err3 = err1 - ((ahi + ahi) * alo); \
  y = (alo * alo) - err3

#define Square(a, x, y) \
  x = (REAL) (a * a); \
  Square_Tail(a, x, y)

/* Macros for summing expansions of various fixed lengths.  These are all    */
/*   unrolled versions of Expansion_Sum().                                   */

#define Two_One_Sum(a1, a0, b, x2, x1, x0) \
  Two_Sum(a0, b , _i, x0); \
  Two_Sum(a1, _i, x2, x1)

#define Two_One_Diff(a1, a0, b, x2, x1, x0) \
  Two_Diff(a0, b , _i, x0); \
  Two_Sum( a1, _i, x2, x1)

#define Two_Two_Sum(a1, a0, b1, b0, x3, x2, x1, x0) \
  Two_One_Sum(a1, a0, b0, _j, _0, x0); \
  Two_One_Sum(_j, _0, b1, x3, x2, x1)

#define Two_Two_Diff(a1, a0, b1, b0, x3, x2, x1, x0) \
  Two_One_Diff(a1, a0, b0, _j, _0, x0); \
  Two_One_Diff(_j, _0, b1, x3, x2, x1)

#define Four_One_Sum(a3, a2, a1, a0, b, x4, x3, x2, x1, x0) \
  Two_One_Sum(a1, a0, b , _j, x1, x0); \
  Two_One_Sum(a3, a2, _j, x4, x3, x2)

#define Four_Two_Sum(a3, a2, a1, a0, b1, b0, x5, x4, x3, x2, x1, x0) \
  Four_One_Sum(a3, a2, a1, a0, b0, _k, _2, _1, _0, x0); \
  Four_One_Sum(_k, _2, _1, _0, b1, x5, x4, x3, x2, x1)

#define Four_Four_Sum(a3, a2, a1, a0, b4, b3, b1, b0, x7, x6, x5, x4, x3, x2, \
                      x1, x0) \
  Four_Two_Sum(a3, a2, a1, a0, b1, b0, _l, _2, _1, _0, x1, x0); \
  Four_Two_Sum(_l, _2, _1, _0, b4, b3, x7, x6, x5, x4, x3, x2)

#define Eight_One_Sum(a7, a6, a5, a4, a3, a2, a1, a0, b, x8, x7, x6, x5, x4, \
                      x3, x2, x1, x0) \
  Four_One_Sum(a3, a2, a1, a0, b , _j, x3, x2, x1, x0); \
  Four_One_Sum(a7, a6, a5, a4, _j, x8, x7, x6, x5, x4)

#define Eight_Two_Sum(a7, a6, a5, a4, a3, a2, a1, a0, b1, b0, x9, x8, x7, \
                      x6, x5, x4, x3, x2, x1, x0) \
  Eight_One_Sum(a7, a6, a5, a4, a3, a2, a1, a0, b0, _k, _6, _5, _4, _3, _2, \
                _1, _0, x0); \
  Eight_One_Sum(_k, _6, _5, _4, _3, _2, _1, _0, b1, x9, x8, x7, x6, x5, x4, \
                x3, x2, x1)

#define Eight_Four_Sum(a7, a6, a5, a4, a3, a2, a1, a0, b4, b3, b1, b0, x11, \
                       x10, x9, x8, x7, x6, x5, x4, x3, x2, x1, x0) \
  Eight_Two_Sum(a7, a6, a5, a4, a3, a2, a1, a0, b1, b0, _l, _6, _5, _4, _3, \
                _2, _1, _0, x1, x0); \
  Eight_Two_Sum(_l, _6, _5, _4, _3, _2, _1, _0, b4, b3, x11, x10, x9, x8, \
                x7, x6, x5, x4, x3, x2)

/* Macros for multiplying expansions of various fixed lengths.               */

#define Two_One_Product(a1, a0, b, x3, x2, x1, x0) \
  Split(b, bhi, blo); \
  Two_Product_Presplit(a0, b, bhi, blo, _i, x0); \
  Two_Product_Presplit(a1, b, bhi, blo, _j, _0); \
  Two_Sum(_i, _0, _k, x1); \
  Fast_Two_Sum(_j, _k, x3, x2)

#define Four_One_Product(a3, a2, a1, a0, b, x7, x6, x5, x4, x3, x2, x1, x0) \
  Split(b, bhi, blo); \
  Two_Product_Presplit(a0, b, bhi, blo, _i, x0); \
  Two_Product_Presplit(a1, b, bhi, blo, _j, _0); \
  Two_Sum(_i, _0, _k, x1); \
  Fast_Two_Sum(_j, _k, _i, x2); \
  Two_Product_Presplit(a2, b, bhi, blo, _j, _0); \
  Two_Sum(_i, _0, _k, x3); \
  Fast_Two_Sum(_j, _k, _i, x4); \
  Two_Product_Presplit(a3, b, bhi, blo, _j, _0); \
  Two_Sum(_i, _0, _k, x5); \
  Fast_Two_Sum(_j, _k, x7, x6)

#define Two_Two_Product(a1, a0, b1, b0, x7, x6, x5, x4, x3, x2, x1, x0) \
  Split(a0, a0hi, a0lo); \
  Split(b0, bhi, blo); \
  Two_Product_2Presplit(a0, a0hi, a0lo, b0, bhi, blo, _i, x0); \
  Split(a1, a1hi, a1lo); \
  Two_Product_2Presplit(a1, a1hi, a1lo, b0, bhi, blo, _j, _0); \
  Two_Sum(_i, _0, _k, _1); \
  Fast_Two_Sum(_j, _k, _l, _2); \
  Split(b1, bhi, blo); \
  Two_Product_2Presplit(a0, a0hi, a0lo, b1, bhi, blo, _i, _0); \
  Two_Sum(_1, _0, _k, x1); \
  Two_Sum(_2, _k, _j, _1); \
  Two_Sum(_l, _j, _m, _2); \
  Two_Product_2Presplit(a1, a1hi, a1lo, b1, bhi, blo, _j, _0); \
  Two_Sum(_i, _0, _n, _0); \
  Two_Sum(_1, _0, _i, x2); \
  Two_Sum(_2, _i, _k, _1); \
  Two_Sum(_m, _k, _l, _2); \
  Two_Sum(_j, _n, _k, _0); \
  Two_Sum(_1, _0, _j, x3); \
  Two_Sum(_2, _j, _i, _1); \
  Two_Sum(_l, _i, _m, _2); \
  Two_Sum(_1, _k, _i, x4); \
  Two_Sum(_2, _i, _k, x5); \
  Two_Sum(_m, _k, x7, x6)

/* An expansion of length two can be squared more quickly than finding the   */
/*   product of two different expansions of length two, and the result is    */
/*   guaranteed to have no more than six (rather than eight) components.     */

#define Two_Square(a1, a0, x5, x4, x3, x2, x1, x0) \
  Square(a0, _j, x0); \
  _0 = a0 + a0; \
  Two_Product(a1, _0, _k, _1); \
  Two_One_Sum(_k, _1, _j, _l, _2, x1); \
  Square(a1, _j, _1); \
  Two_Two_Sum(_j, _1, _l, _2, x5, x4, x3, x2)

REAL splitter;     /* = 2^ceiling(p / 2) + 1.  Used to split floats in half. */
REAL epsilon;                /* = 2^(-p).  Used to estimate roundoff errors. */
/* A set of coefficients used to calculate maximum roundoff errors.          */
REAL resulterrbound;
REAL ccwerrboundA, ccwerrboundB, ccwerrboundC;
REAL o3derrboundA, o3derrboundB, o3derrboundC;
REAL iccerrboundA, iccerrboundB, iccerrboundC;
REAL isperrboundA, isperrboundB, isperrboundC;

/*****************************************************************************/
/*                                                                           */
/*  doubleprint()   Print the bit representation of a double.                */
/*                                                                           */
/*  Useful for debugging exact arithmetic routines.                          */
/*                                                                           */
/*****************************************************************************/

/*
void doubleprint(number)
double number;
{
  unsigned long long no;
  unsigned long long sign, expo;
  int exponent;
  int i, bottomi;

  no = *(unsigned long long *) &number;
  sign = no & 0x8000000000000000ll;
  expo = (no >> 52) & 0x7ffll;
  exponent = (int) expo;
  exponent = exponent - 1023;
  if (sign) {
    printf("-");
  } else {
    printf(" ");
  }
  if (exponent == -1023) {
    printf(
      "0.0000000000000000000000000000000000000000000000000000_     (   )");
  } else {
    printf("1.");
    bottomi = -1;
    for (i = 0; i < 52; i++) {
      if (no & 0x0008000000000000ll) {
        printf("1");
        bottomi = i;
      } else {
        printf("0");
      }
      no <<= 1;
    }
    printf("_%d  (%d)", exponent, exponent - 1 - bottomi);
  }
}
*/

/*****************************************************************************/
/*                                                                           */
/*  floatprint()   Print the bit representation of a float.                  */
/*                                                                           */
/*  Useful for debugging exact arithmetic routines.                          */
/*                                                                           */
/*****************************************************************************/

/*
void floatprint(number)
float number;
{
  unsigned no;
  unsigned sign, expo;
  int exponent;
  int i, bottomi;

  no = *(unsigned *) &number;
  sign = no & 0x80000000;
  expo = (no >> 23) & 0xff;
  exponent = (int) expo;
  exponent = exponent - 127;
  if (sign) {
    printf("-");
  } else {
    printf(" ");
  }
  if (exponent == -127) {
    printf("0.00000000000000000000000_     (   )");
  } else {
    printf("1.");
    bottomi = -1;
    for (i = 0; i < 23; i++) {
      if (no & 0x00400000) {
        printf("1");
        bottomi = i;
      } else {
        printf("0");
      }
      no <<= 1;
    }
    printf("_%3d  (%3d)", exponent, exponent - 1 - bottomi);
  }
}
*/

/*****************************************************************************/
/*                                                                           */
/*  expansion_print()   Print the bit representation of an expansion.        */
/*                                                                           */
/*  Useful for debugging exact arithmetic routines.                          */
/*                                                                           */
/*****************************************************************************/

/*
void expansion_print(elen, e)
int elen;
REAL *e;
{
  int i;

  for (i = elen - 1; i >= 0; i--) {
    REALPRINT(e[i]);
    if (i > 0) {
      printf(" +\n");
    } else {
      printf("\n");
    }
  }
}
*/

/*****************************************************************************/
/*                                                                           */
/*  doublerand()   Generate a double with random 53-bit significand and a    */
/*                 random exponent in [0, 511].                              */
/*                                                                           */
/*****************************************************************************/

double doublerand()
{
    double result;
    double expo;
    long a, b, c;
    long i;

    a = random();
    b = random();
    c = random();
    result = (double) (a - 1073741824) * 8388608.0 + (double) (b >> 8);
    for (i = 512, expo = 2; i <= 131072; i *= 2, expo = expo * expo) {
        if (c & i) {
            result *= expo;
        }
    }
    return result;
}

/*****************************************************************************/
/*                                                                           */
/*  narrowdoublerand()   Generate a double with random 53-bit significand    */
/*                       and a random exponent in [0, 7].                    */
/*                                                                           */
/*****************************************************************************/

double narrowdoublerand()
{
    double result;
    double expo;
    long a, b, c;
    long i;

    a = random();
    b = random();
    c = random();
    result = (double) (a - 1073741824) * 8388608.0 + (double) (b >> 8);
    for (i = 512, expo = 2; i <= 2048; i *= 2, expo = expo * expo) {
        if (c & i) {
            result *= expo;
        }
    }
    return result;
}

/*****************************************************************************/
/*                                                                           */
/*  uniformdoublerand()   Generate a double with random 53-bit significand.  */
/*                                                                           */
/*****************************************************************************/

double uniformdoublerand()
{
    double result;
    long a, b;

    a = random();
    b = random();
    result = (double) (a - 1073741824) * 8388608.0 + (double) (b >> 8);
    return result;
}

/*****************************************************************************/
/*                                                                           */
/*  floatrand()   Generate a float with random 24-bit significand and a      */
/*                random exponent in [0, 63].                                */
/*                                                                           */
/*****************************************************************************/

float floatrand()
{
    float result;
    float expo;
    long a, c;
    long i;

    a = random();
    c = random();
    result = (float) ((a - 1073741824) >> 6);
    for (i = 512, expo = 2; i <= 16384; i *= 2, expo = expo * expo) {
        if (c & i) {
            result *= expo;
        }
    }
    return result;
}

/*****************************************************************************/
/*                                                                           */
/*  narrowfloatrand()   Generate a float with random 24-bit significand and  */
/*                      a random exponent in [0, 7].                         */
/*                                                                           */
/*****************************************************************************/

float narrowfloatrand()
{
    float result;
    float expo;
    long a, c;
    long i;

    a = random();
    c = random();
    result = (float) ((a - 1073741824) >> 6);
    for (i = 512, expo = 2; i <= 2048; i *= 2, expo = expo * expo) {
        if (c & i) {
            result *= expo;
        }
    }
    return result;
}

/*****************************************************************************/
/*                                                                           */
/*  uniformfloatrand()   Generate a float with random 24-bit significand.    */
/*                                                                           */
/*****************************************************************************/

float uniformfloatrand()
{
    float result;
    long a;

    a = random();
    result = (float) ((a - 1073741824) >> 6);
    return result;
}

/*****************************************************************************/
/*                                                                           */
/*  exactinit()   Initialize the variables used for exact arithmetic.        */
/*                                                                           */
/*  `epsilon' is the largest power of two such that 1.0 + epsilon = 1.0 in   */
/*  floating-point arithmetic.  `epsilon' bounds the relative roundoff       */
/*  error.  It is used for floating-point error analysis.                    */
/*                                                                           */
/*  `splitter' is used to split floating-point numbers into two half-        */
/*  length significands for exact multiplication.                            */
/*                                                                           */
/*  I imagine that a highly optimizing compiler might be too smart for its   */
/*  own good, and somehow cause this routine to fail, if it pretends that    */
/*  floating-point arithmetic is too much like real arithmetic.              */
/*                                                                           */
/*  Don't change this routine unless you fully understand it.                */
/*                                                                           */
/*****************************************************************************/

void exactinit()
{
    REAL half;
    REAL check, lastcheck;
    int every_other;

    every_other = 1;
    half = 0.5;
    epsilon = 1.0;
    splitter = 1.0;
    check = 1.0;
    /* Repeatedly divide `epsilon' by two until it is too small to add to    */
    /*   one without causing roundoff.  (Also check if the sum is equal to   */
    /*   the previous sum, for machines that round up instead of using exact */
    /*   rounding.  Not that this library will work on such machines anyway. */
    do {
        lastcheck = check;
        epsilon *= half;
        if (every_other) {
            splitter *= 2.0;
        }
        every_other = !every_other;
        check = 1.0 + epsilon;
    } while ((check != 1.0) && (check != lastcheck));
    splitter += 1.0;

    /* Error bounds for orientation and incircle tests. */
    resulterrbound = (3.0 + 8.0 * epsilon) * epsilon;
    ccwerrboundA = (3.0 + 16.0 * epsilon) * epsilon;
    ccwerrboundB = (2.0 + 12.0 * epsilon) * epsilon;
    ccwerrboundC = (9.0 + 64.0 * epsilon) * epsilon * epsilon;
    o3derrboundA = (7.0 + 56.0 * epsilon) * epsilon;
    o3derrboundB = (3.0 + 28.0 * epsilon) * epsilon;
    o3derrboundC = (26.0 + 288.0 * epsilon) * epsilon * epsilon;
    iccerrboundA = (10.0 + 96.0 * epsilon) * epsilon;
    iccerrboundB = (4.0 + 48.0 * epsilon) * epsilon;
    iccerrboundC = (44.0 + 576.0 * epsilon) * epsilon * epsilon;
    isperrboundA = (16.0 + 224.0 * epsilon) * epsilon;
    isperrboundB = (5.0 + 72.0 * epsilon) * epsilon;
    isperrboundC = (71.0 + 1408.0 * epsilon) * epsilon * epsilon;
}

/*****************************************************************************/
/*                                                                           */
/*  grow_expansion()   Add a scalar to an expansion.                         */
/*                                                                           */
/*  Sets h = e + b.  See the long version of my paper for details.           */
/*                                                                           */
/*  Maintains the nonoverlapping property.  If round-to-even is used (as     */
/*  with IEEE 754), maintains the strongly nonoverlapping and nonadjacent    */
/*  properties as well.  (That is, if e has one of these properties, so      */
/*  will h.)                                                                 */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int grow_expansion(elen, e, b, h)                // e and h can be the same.
    int elen;
    REAL *e;
    REAL b;
    REAL *h;*/
int grow_expansion(int elen,REAL *e,REAL b,REAL *h)
{
    REAL Q;
    INEXACT REAL Qnew;
    int eindex;
    REAL enow;
    INEXACT REAL bvirt;
    REAL avirt, bround, around;

    Q = b;
    for (eindex = 0; eindex < elen; eindex++) {
        enow = e[eindex];
        Two_Sum(Q, enow, Qnew, h[eindex]);
        Q = Qnew;
    }
    h[eindex] = Q;
    return eindex + 1;
}

/*****************************************************************************/
/*                                                                           */
/*  grow_expansion_zeroelim()   Add a scalar to an expansion, eliminating    */
/*                              zero components from the output expansion.   */
/*                                                                           */
/*  Sets h = e + b.  See the long version of my paper for details.           */
/*                                                                           */
/*  Maintains the nonoverlapping property.  If round-to-even is used (as     */
/*  with IEEE 754), maintains the strongly nonoverlapping and nonadjacent    */
/*  properties as well.  (That is, if e has one of these properties, so      */
/*  will h.)                                                                 */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int grow_expansion_zeroelim(elen, e, b, h)       // e and h can be the same.
    int elen;
    REAL *e;
    REAL b;
    REAL *h;*/
int grow_expansion_zeroelim(int elen,REAL *e,REAL b,REAL *h)
{
    REAL Q, hh;
    INEXACT REAL Qnew;
    int eindex, hindex;
    REAL enow;
    INEXACT REAL bvirt;
    REAL avirt, bround, around;

    hindex = 0;
    Q = b;
    for (eindex = 0; eindex < elen; eindex++) {
        enow = e[eindex];
        Two_Sum(Q, enow, Qnew, hh);
        Q = Qnew;
        if (hh != 0.0) {
            h[hindex++] = hh;
        }
    }
    if ((Q != 0.0) || (hindex == 0)) {
        h[hindex++] = Q;
    }
    return hindex;
}

/*****************************************************************************/
/*                                                                           */
/*  expansion_sum()   Sum two expansions.                                    */
/*                                                                           */
/*  Sets h = e + f.  See the long version of my paper for details.           */
/*                                                                           */
/*  Maintains the nonoverlapping property.  If round-to-even is used (as     */
/*  with IEEE 754), maintains the nonadjacent property as well.  (That is,   */
/*  if e has one of these properties, so will h.)  Does NOT maintain the     */
/*  strongly nonoverlapping property.                                        */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int expansion_sum(elen, e, flen, f, h)
    // e and h can be the same, but f and h cannot.
    int elen;
    REAL *e;
    int flen;
    REAL *f;
    REAL *h;*/
int expansion_sum(int elen,REAL *e,int flen,REAL *f,REAL *h)
{
    REAL Q;
    INEXACT REAL Qnew;
    int findex, hindex, hlast;
    REAL hnow;
    INEXACT REAL bvirt;
    REAL avirt, bround, around;

    Q = f[0];
    for (hindex = 0; hindex < elen; hindex++) {
        hnow = e[hindex];
        Two_Sum(Q, hnow, Qnew, h[hindex]);
        Q = Qnew;
    }
    h[hindex] = Q;
    hlast = hindex;
    for (findex = 1; findex < flen; findex++) {
        Q = f[findex];
        for (hindex = findex; hindex <= hlast; hindex++) {
            hnow = h[hindex];
            Two_Sum(Q, hnow, Qnew, h[hindex]);
            Q = Qnew;
        }
        h[++hlast] = Q;
    }
    return hlast + 1;
}

/*****************************************************************************/
/*                                                                           */
/*  expansion_sum_zeroelim1()   Sum two expansions, eliminating zero         */
/*                              components from the output expansion.        */
/*                                                                           */
/*  Sets h = e + f.  See the long version of my paper for details.           */
/*                                                                           */
/*  Maintains the nonoverlapping property.  If round-to-even is used (as     */
/*  with IEEE 754), maintains the nonadjacent property as well.  (That is,   */
/*  if e has one of these properties, so will h.)  Does NOT maintain the     */
/*  strongly nonoverlapping property.                                        */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int expansion_sum_zeroelim1(elen, e, flen, f, h)
    // e and h can be the same, but f and h cannot.
    int elen;
    REAL *e;
    int flen;
    REAL *f;
    REAL *h;*/
int expansion_sum_zeroelim1(int elen,REAL *e,int flen,REAL *f,REAL *h)
{
    REAL Q;
    INEXACT REAL Qnew;
    int index, findex, hindex, hlast;
    REAL hnow;
    INEXACT REAL bvirt;
    REAL avirt, bround, around;

    Q = f[0];
    for (hindex = 0; hindex < elen; hindex++) {
        hnow = e[hindex];
        Two_Sum(Q, hnow, Qnew, h[hindex]);
        Q = Qnew;
    }
    h[hindex] = Q;
    hlast = hindex;
    for (findex = 1; findex < flen; findex++) {
        Q = f[findex];
        for (hindex = findex; hindex <= hlast; hindex++) {
            hnow = h[hindex];
            Two_Sum(Q, hnow, Qnew, h[hindex]);
            Q = Qnew;
        }
        h[++hlast] = Q;
    }
    hindex = -1;
    for (index = 0; index <= hlast; index++) {
        hnow = h[index];
        if (hnow != 0.0) {
            h[++hindex] = hnow;
        }
    }
    if (hindex == -1) {
        return 1;
    } else {
        return hindex + 1;
    }
}

/*****************************************************************************/
/*                                                                           */
/*  expansion_sum_zeroelim2()   Sum two expansions, eliminating zero         */
/*                              components from the output expansion.        */
/*                                                                           */
/*  Sets h = e + f.  See the long version of my paper for details.           */
/*                                                                           */
/*  Maintains the nonoverlapping property.  If round-to-even is used (as     */
/*  with IEEE 754), maintains the nonadjacent property as well.  (That is,   */
/*  if e has one of these properties, so will h.)  Does NOT maintain the     */
/*  strongly nonoverlapping property.                                        */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int expansion_sum_zeroelim2(elen, e, flen, f, h)
    // e and h can be the same, but f and h cannot.
    int elen;
    REAL *e;
    int flen;
    REAL *f;
    REAL *h;*/
int expansion_sum_zeroelim2(int elen,REAL *e,int flen,REAL *f,REAL *h)
{
    REAL Q, hh;
    INEXACT REAL Qnew;
    int eindex, findex, hindex, hlast;
    REAL enow;
    INEXACT REAL bvirt;
    REAL avirt, bround, around;

    hindex = 0;
    Q = f[0];
    for (eindex = 0; eindex < elen; eindex++) {
        enow = e[eindex];
        Two_Sum(Q, enow, Qnew, hh);
        Q = Qnew;
        if (hh != 0.0) {
            h[hindex++] = hh;
        }
    }
    h[hindex] = Q;
    hlast = hindex;
    for (findex = 1; findex < flen; findex++) {
        hindex = 0;
        Q = f[findex];
        for (eindex = 0; eindex <= hlast; eindex++) {
            enow = h[eindex];
            Two_Sum(Q, enow, Qnew, hh);
            Q = Qnew;
            if (hh != 0) {
                h[hindex++] = hh;
            }
        }
        h[hindex] = Q;
        hlast = hindex;
    }
    return hlast + 1;
}

/*****************************************************************************/
/*                                                                           */
/*  fast_expansion_sum()   Sum two expansions.                               */
/*                                                                           */
/*  Sets h = e + f.  See the long version of my paper for details.           */
/*                                                                           */
/*  If round-to-even is used (as with IEEE 754), maintains the strongly      */
/*  nonoverlapping property.  (That is, if e is strongly nonoverlapping, h   */
/*  will be also.)  Does NOT maintain the nonoverlapping or nonadjacent      */
/*  properties.                                                              */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int fast_expansion_sum(elen, e, flen, f, h)           // h cannot be e or f.
    int elen;
    REAL *e;
    int flen;
    REAL *f;
    REAL *h;*/
int fast_expansion_sum(int elen,REAL *e,int flen,REAL *f,REAL *h)
{
    REAL Q;
    INEXACT REAL Qnew;
    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    int eindex, findex, hindex;
    REAL enow, fnow;

    enow = e[0];
    fnow = f[0];
    eindex = findex = 0;
    if ((fnow > enow) == (fnow > -enow)) {
        Q = enow;
        enow = e[++eindex];
    } else {
        Q = fnow;
        fnow = f[++findex];
    }
    hindex = 0;
    if ((eindex < elen) && (findex < flen)) {
        if ((fnow > enow) == (fnow > -enow)) {
            Fast_Two_Sum(enow, Q, Qnew, h[0]);
            enow = e[++eindex];
        } else {
            Fast_Two_Sum(fnow, Q, Qnew, h[0]);
            fnow = f[++findex];
        }
        Q = Qnew;
        hindex = 1;
        while ((eindex < elen) && (findex < flen)) {
            if ((fnow > enow) == (fnow > -enow)) {
                Two_Sum(Q, enow, Qnew, h[hindex]);
                enow = e[++eindex];
            } else {
                Two_Sum(Q, fnow, Qnew, h[hindex]);
                fnow = f[++findex];
            }
            Q = Qnew;
            hindex++;
        }
    }
    while (eindex < elen) {
        Two_Sum(Q, enow, Qnew, h[hindex]);
        enow = e[++eindex];
        Q = Qnew;
        hindex++;
    }
    while (findex < flen) {
        Two_Sum(Q, fnow, Qnew, h[hindex]);
        fnow = f[++findex];
        Q = Qnew;
        hindex++;
    }
    h[hindex] = Q;
    return hindex + 1;
}

/*****************************************************************************/
/*                                                                           */
/*  fast_expansion_sum_zeroelim()   Sum two expansions, eliminating zero     */
/*                                  components from the output expansion.    */
/*                                                                           */
/*  Sets h = e + f.  See the long version of my paper for details.           */
/*                                                                           */
/*  If round-to-even is used (as with IEEE 754), maintains the strongly      */
/*  nonoverlapping property.  (That is, if e is strongly nonoverlapping, h   */
/*  will be also.)  Does NOT maintain the nonoverlapping or nonadjacent      */
/*  properties.                                                              */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int fast_expansion_sum_zeroelim(elen, e, flen, f, h)  // h cannot be e or f.
    int elen;
    REAL *e;
    int flen;
    REAL *f;
    REAL *h;*/
int fast_expansion_sum_zeroelim(int elen,REAL *e,int flen,REAL *f,REAL *h)
{
    REAL Q;
    INEXACT REAL Qnew;
    INEXACT REAL hh;
    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    int eindex, findex, hindex;
    REAL enow, fnow;

    enow = e[0];
    fnow = f[0];
    eindex = findex = 0;
    if ((fnow > enow) == (fnow > -enow)) {
        Q = enow;
        enow = e[++eindex];
    } else {
        Q = fnow;
        fnow = f[++findex];
    }
    hindex = 0;
    if ((eindex < elen) && (findex < flen)) {
        if ((fnow > enow) == (fnow > -enow)) {
            Fast_Two_Sum(enow, Q, Qnew, hh);
            enow = e[++eindex];
        } else {
            Fast_Two_Sum(fnow, Q, Qnew, hh);
            fnow = f[++findex];
        }
        Q = Qnew;
        if (hh != 0.0) {
            h[hindex++] = hh;
        }
        while ((eindex < elen) && (findex < flen)) {
            if ((fnow > enow) == (fnow > -enow)) {
                Two_Sum(Q, enow, Qnew, hh);
                enow = e[++eindex];
            } else {
                Two_Sum(Q, fnow, Qnew, hh);
                fnow = f[++findex];
            }
            Q = Qnew;
            if (hh != 0.0) {
                h[hindex++] = hh;
            }
        }
    }
    while (eindex < elen) {
        Two_Sum(Q, enow, Qnew, hh);
        enow = e[++eindex];
        Q = Qnew;
        if (hh != 0.0) {
            h[hindex++] = hh;
        }
    }
    while (findex < flen) {
        Two_Sum(Q, fnow, Qnew, hh);
        fnow = f[++findex];
        Q = Qnew;
        if (hh != 0.0) {
            h[hindex++] = hh;
        }
    }
    if ((Q != 0.0) || (hindex == 0)) {
        h[hindex++] = Q;
    }
    return hindex;
}

/*****************************************************************************/
/*                                                                           */
/*  linear_expansion_sum()   Sum two expansions.                             */
/*                                                                           */
/*  Sets h = e + f.  See either version of my paper for details.             */
/*                                                                           */
/*  Maintains the nonoverlapping property.  (That is, if e is                */
/*  nonoverlapping, h will be also.)                                         */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int linear_expansion_sum(elen, e, flen, f, h)         // h cannot be e or f.
    int elen;
    REAL *e;
    int flen;
    REAL *f;
    REAL *h;*/
int linear_expansion_sum(int elen,REAL *e,int flen,REAL *f,REAL *h)
{
    REAL Q, q;
    INEXACT REAL Qnew;
    INEXACT REAL R;
    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    int eindex, findex, hindex;
    REAL enow, fnow;
    REAL g0;

    enow = e[0];
    fnow = f[0];
    eindex = findex = 0;
    if ((fnow > enow) == (fnow > -enow)) {
        g0 = enow;
        enow = e[++eindex];
    } else {
        g0 = fnow;
        fnow = f[++findex];
    }
    if ((eindex < elen) && ((findex >= flen)
                            || ((fnow > enow) == (fnow > -enow)))) {
        Fast_Two_Sum(enow, g0, Qnew, q);
        enow = e[++eindex];
    } else {
        Fast_Two_Sum(fnow, g0, Qnew, q);
        fnow = f[++findex];
    }
    Q = Qnew;
    for (hindex = 0; hindex < elen + flen - 2; hindex++) {
        if ((eindex < elen) && ((findex >= flen)
                                || ((fnow > enow) == (fnow > -enow)))) {
            Fast_Two_Sum(enow, q, R, h[hindex]);
            enow = e[++eindex];
        } else {
            Fast_Two_Sum(fnow, q, R, h[hindex]);
            fnow = f[++findex];
        }
        Two_Sum(Q, R, Qnew, q);
        Q = Qnew;
    }
    h[hindex] = q;
    h[hindex + 1] = Q;
    return hindex + 2;
}

/*****************************************************************************/
/*                                                                           */
/*  linear_expansion_sum_zeroelim()   Sum two expansions, eliminating zero   */
/*                                    components from the output expansion.  */
/*                                                                           */
/*  Sets h = e + f.  See either version of my paper for details.             */
/*                                                                           */
/*  Maintains the nonoverlapping property.  (That is, if e is                */
/*  nonoverlapping, h will be also.)                                         */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int linear_expansion_sum_zeroelim(elen, e, flen, f, h)// h cannot be e or f.
    int elen;
    REAL *e;
    int flen;
    REAL *f;
    REAL *h;*/
int linear_expansion_sum_zeroelim(int elen,REAL *e,int flen,REAL *f,REAL *h)
{
    REAL Q, q, hh;
    INEXACT REAL Qnew;
    INEXACT REAL R;
    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    int eindex, findex, hindex;
    int count;
    REAL enow, fnow;
    REAL g0;

    enow = e[0];
    fnow = f[0];
    eindex = findex = 0;
    hindex = 0;
    if ((fnow > enow) == (fnow > -enow)) {
        g0 = enow;
        enow = e[++eindex];
    } else {
        g0 = fnow;
        fnow = f[++findex];
    }
    if ((eindex < elen) && ((findex >= flen)
                            || ((fnow > enow) == (fnow > -enow)))) {
        Fast_Two_Sum(enow, g0, Qnew, q);
        enow = e[++eindex];
    } else {
        Fast_Two_Sum(fnow, g0, Qnew, q);
        fnow = f[++findex];
    }
    Q = Qnew;
    for (count = 2; count < elen + flen; count++) {
        if ((eindex < elen) && ((findex >= flen)
                                || ((fnow > enow) == (fnow > -enow)))) {
            Fast_Two_Sum(enow, q, R, hh);
            enow = e[++eindex];
        } else {
            Fast_Two_Sum(fnow, q, R, hh);
            fnow = f[++findex];
        }
        Two_Sum(Q, R, Qnew, q);
        Q = Qnew;
        if (hh != 0) {
            h[hindex++] = hh;
        }
    }
    if (q != 0) {
        h[hindex++] = q;
    }
    if ((Q != 0.0) || (hindex == 0)) {
        h[hindex++] = Q;
    }
    return hindex;
}

/*****************************************************************************/
/*                                                                           */
/*  scale_expansion()   Multiply an expansion by a scalar.                   */
/*                                                                           */
/*  Sets h = be.  See either version of my paper for details.                */
/*                                                                           */
/*  Maintains the nonoverlapping property.  If round-to-even is used (as     */
/*  with IEEE 754), maintains the strongly nonoverlapping and nonadjacent    */
/*  properties as well.  (That is, if e has one of these properties, so      */
/*  will h.)                                                                 */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int scale_expansion(elen, e, b, h)            // e and h cannot be the same.
    int elen;
    REAL *e;
    REAL b;
    REAL *h;*/
int scale_expansion(int elen,REAL *e,REAL b,REAL *h)
{
    INEXACT REAL Q;
    INEXACT REAL sum;
    INEXACT REAL product1;
    REAL product0;
    int eindex, hindex;
    REAL enow;
    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL ahi, alo, bhi, blo;
    REAL err1, err2, err3;

    Split(b, bhi, blo);
    Two_Product_Presplit(e[0], b, bhi, blo, Q, h[0]);
    hindex = 1;
    for (eindex = 1; eindex < elen; eindex++) {
        enow = e[eindex];
        Two_Product_Presplit(enow, b, bhi, blo, product1, product0);
        Two_Sum(Q, product0, sum, h[hindex]);
        hindex++;
        Two_Sum(product1, sum, Q, h[hindex]);
        hindex++;
    }
    h[hindex] = Q;
    return elen + elen;
}

/*****************************************************************************/
/*                                                                           */
/*  scale_expansion_zeroelim()   Multiply an expansion by a scalar,          */
/*                               eliminating zero components from the        */
/*                               output expansion.                           */
/*                                                                           */
/*  Sets h = be.  See either version of my paper for details.                */
/*                                                                           */
/*  Maintains the nonoverlapping property.  If round-to-even is used (as     */
/*  with IEEE 754), maintains the strongly nonoverlapping and nonadjacent    */
/*  properties as well.  (That is, if e has one of these properties, so      */
/*  will h.)                                                                 */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int scale_expansion_zeroelim(elen, e, b, h)   // e and h cannot be the same.
    int elen;
    REAL *e;
    REAL b;
    REAL *h;*/
int scale_expansion_zeroelim(int elen,REAL *e,REAL b,REAL *h)
{
    INEXACT REAL Q, sum;
    REAL hh;
    INEXACT REAL product1;
    REAL product0;
    int eindex, hindex;
    REAL enow;
    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL ahi, alo, bhi, blo;
    REAL err1, err2, err3;

    Split(b, bhi, blo);
    Two_Product_Presplit(e[0], b, bhi, blo, Q, hh);
    hindex = 0;
    if (hh != 0) {
        h[hindex++] = hh;
    }
    for (eindex = 1; eindex < elen; eindex++) {
        enow = e[eindex];
        Two_Product_Presplit(enow, b, bhi, blo, product1, product0);
        Two_Sum(Q, product0, sum, hh);
        if (hh != 0) {
            h[hindex++] = hh;
        }
        Fast_Two_Sum(product1, sum, Q, hh);
        if (hh != 0) {
            h[hindex++] = hh;
        }
    }
    if ((Q != 0.0) || (hindex == 0)) {
        h[hindex++] = Q;
    }
    return hindex;
}

/*****************************************************************************/
/*                                                                           */
/*  compress()   Compress an expansion.                                      */
/*                                                                           */
/*  See the long version of my paper for details.                            */
/*                                                                           */
/*  Maintains the nonoverlapping property.  If round-to-even is used (as     */
/*  with IEEE 754), then any nonoverlapping expansion is converted to a      */
/*  nonadjacent expansion.                                                   */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*int compress(elen, e, h)                         // e and h may be the same.
    int elen;
    REAL *e;
    REAL *h;*/
int compress(int elen,REAL *e,REAL *h)
{
    REAL Q, q;
    INEXACT REAL Qnew;
    int eindex, hindex;
    INEXACT REAL bvirt;
    REAL enow, hnow;
    int top, bottom;

    bottom = elen - 1;
    Q = e[bottom];
    for (eindex = elen - 2; eindex >= 0; eindex--) {
        enow = e[eindex];
        Fast_Two_Sum(Q, enow, Qnew, q);
        if (q != 0) {
            h[bottom--] = Qnew;
            Q = q;
        } else {
            Q = Qnew;
        }
    }
    top = 0;
    for (hindex = bottom + 1; hindex < elen; hindex++) {
        hnow = h[hindex];
        Fast_Two_Sum(hnow, Q, Qnew, q);
        if (q != 0) {
            h[top++] = q;
        }
        Q = Qnew;
    }
    h[top] = Q;
    return top + 1;
}

/*****************************************************************************/
/*                                                                           */
/*  estimate()   Produce a one-word estimate of an expansion's value.        */
/*                                                                           */
/*  See either version of my paper for details.                              */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*REAL estimate(elen, e)
    int elen;
    REAL *e;*/
REAL estimate(int elen,REAL *e)
{
    REAL Q;
    int eindex;

    Q = e[0];
    for (eindex = 1; eindex < elen; eindex++) {
        Q += e[eindex];
    }
    return Q;
}

/*****************************************************************************/
/*                                                                           */
/*  orient2dfast()   Approximate 2D orientation test.  Nonrobust.            */
/*  orient2dexact()   Exact 2D orientation test.  Robust.                    */
/*  orient2dslow()   Another exact 2D orientation test.  Robust.             */
/*  orient2d()   Adaptive exact 2D orientation test.  Robust.                */
/*                                                                           */
/*               Return a positive value if the points pa, pb, and pc occur  */
/*               in counterclockwise order; a negative value if they occur   */
/*               in clockwise order; and zero if they are collinear.  The    */
/*               result is also a rough approximation of twice the signed    */
/*               area of the triangle defined by the three points.           */
/*                                                                           */
/*  Only the first and last routine should be used; the middle two are for   */
/*  timings.                                                                 */
/*                                                                           */
/*  The last three use exact arithmetic to ensure a correct answer.  The     */
/*  result returned is the determinant of a matrix.  In orient2d() only,     */
/*  this determinant is computed adaptively, in the sense that exact         */
/*  arithmetic is used only to the degree it is needed to ensure that the    */
/*  returned value has the correct sign.  Hence, orient2d() is usually quite */
/*  fast, but will run more slowly when the input points are collinear or    */
/*  nearly so.                                                               */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*REAL orient2dfast(pa, pb, pc)
    REAL *pa;
    REAL *pb;
    REAL *pc;*/
REAL orient2dfast(REAL *pa,REAL *pb,REAL *pc)
{
    REAL acx, bcx, acy, bcy;

    acx = pa[0] - pc[0];
    bcx = pb[0] - pc[0];
    acy = pa[1] - pc[1];
    bcy = pb[1] - pc[1];
    return acx * bcy - acy * bcx;
}

// TJH: old style function declaration
/*REAL orient2dexact(pa, pb, pc)
    REAL *pa;
    REAL *pb;
    REAL *pc;*/
REAL orient2dexact(REAL *pa,REAL *pb,REAL *pc)
{
    INEXACT REAL axby1, axcy1, bxcy1, bxay1, cxay1, cxby1;
    REAL axby0, axcy0, bxcy0, bxay0, cxay0, cxby0;
    REAL aterms[4], bterms[4], cterms[4];
    INEXACT REAL aterms3, bterms3, cterms3;
    REAL v[8], w[12];
    int vlength, wlength;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL ahi, alo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j;
    REAL _0;

    Two_Product(pa[0], pb[1], axby1, axby0);
    Two_Product(pa[0], pc[1], axcy1, axcy0);
    Two_Two_Diff(axby1, axby0, axcy1, axcy0,
                 aterms3, aterms[2], aterms[1], aterms[0]);
    aterms[3] = aterms3;

    Two_Product(pb[0], pc[1], bxcy1, bxcy0);
    Two_Product(pb[0], pa[1], bxay1, bxay0);
    Two_Two_Diff(bxcy1, bxcy0, bxay1, bxay0,
                 bterms3, bterms[2], bterms[1], bterms[0]);
    bterms[3] = bterms3;

    Two_Product(pc[0], pa[1], cxay1, cxay0);
    Two_Product(pc[0], pb[1], cxby1, cxby0);
    Two_Two_Diff(cxay1, cxay0, cxby1, cxby0,
                 cterms3, cterms[2], cterms[1], cterms[0]);
    cterms[3] = cterms3;

    vlength = fast_expansion_sum_zeroelim(4, aterms, 4, bterms, v);
    wlength = fast_expansion_sum_zeroelim(vlength, v, 4, cterms, w);

    return w[wlength - 1];
}

// TJH: old style function declaration
/*REAL orient2dslow(pa, pb, pc)
    REAL *pa;
    REAL *pb;
    REAL *pc;*/
REAL orient2dslow(REAL *pa,REAL *pb,REAL *pc)
{
    INEXACT REAL acx, acy, bcx, bcy;
    REAL acxtail, acytail;
    REAL bcxtail, bcytail;
    REAL negate, negatetail;
    REAL axby[8], bxay[8];
    INEXACT REAL axby7, bxay7;
    REAL deter[16];
    int deterlen;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL a0hi, a0lo, a1hi, a1lo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j, _k, _l, _m, _n;
    REAL _0, _1, _2;

    Two_Diff(pa[0], pc[0], acx, acxtail);
    Two_Diff(pa[1], pc[1], acy, acytail);
    Two_Diff(pb[0], pc[0], bcx, bcxtail);
    Two_Diff(pb[1], pc[1], bcy, bcytail);

    Two_Two_Product(acx, acxtail, bcy, bcytail,
                    axby7, axby[6], axby[5], axby[4],
                    axby[3], axby[2], axby[1], axby[0]);
    axby[7] = axby7;
    negate = -acy;
    negatetail = -acytail;
    Two_Two_Product(bcx, bcxtail, negate, negatetail,
                    bxay7, bxay[6], bxay[5], bxay[4],
                    bxay[3], bxay[2], bxay[1], bxay[0]);
    bxay[7] = bxay7;

    deterlen = fast_expansion_sum_zeroelim(8, axby, 8, bxay, deter);

    return deter[deterlen - 1];
}

// TJH: old style function declaration
/*REAL orient2dadapt(pa, pb, pc, detsum)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL detsum;*/
REAL orient2dadapt(REAL *pa,REAL *pb,REAL *pc,REAL detsum)
{
    INEXACT REAL acx, acy, bcx, bcy;
    REAL acxtail, acytail, bcxtail, bcytail;
    INEXACT REAL detleft, detright;
    REAL detlefttail, detrighttail;
    REAL det, errbound;
    REAL B[4], C1[8], C2[12], D[16];
    INEXACT REAL B3;
    int C1length, C2length, Dlength;
    REAL u[4];
    INEXACT REAL u3;
    INEXACT REAL s1, t1;
    REAL s0, t0;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL ahi, alo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j;
    REAL _0;

    acx = (REAL) (pa[0] - pc[0]);
    bcx = (REAL) (pb[0] - pc[0]);
    acy = (REAL) (pa[1] - pc[1]);
    bcy = (REAL) (pb[1] - pc[1]);

    Two_Product(acx, bcy, detleft, detlefttail);
    Two_Product(acy, bcx, detright, detrighttail);

    Two_Two_Diff(detleft, detlefttail, detright, detrighttail,
                 B3, B[2], B[1], B[0]);
    B[3] = B3;

    det = estimate(4, B);
    errbound = ccwerrboundB * detsum;
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }

    Two_Diff_Tail(pa[0], pc[0], acx, acxtail);
    Two_Diff_Tail(pb[0], pc[0], bcx, bcxtail);
    Two_Diff_Tail(pa[1], pc[1], acy, acytail);
    Two_Diff_Tail(pb[1], pc[1], bcy, bcytail);

    if ((acxtail == 0.0) && (acytail == 0.0)
        && (bcxtail == 0.0) && (bcytail == 0.0)) {
        return det;
    }

    errbound = ccwerrboundC * detsum + resulterrbound * Absolute(det);
    det += (acx * bcytail + bcy * acxtail)
        - (acy * bcxtail + bcx * acytail);
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }

    Two_Product(acxtail, bcy, s1, s0);
    Two_Product(acytail, bcx, t1, t0);
    Two_Two_Diff(s1, s0, t1, t0, u3, u[2], u[1], u[0]);
    u[3] = u3;
    C1length = fast_expansion_sum_zeroelim(4, B, 4, u, C1);

    Two_Product(acx, bcytail, s1, s0);
    Two_Product(acy, bcxtail, t1, t0);
    Two_Two_Diff(s1, s0, t1, t0, u3, u[2], u[1], u[0]);
    u[3] = u3;
    C2length = fast_expansion_sum_zeroelim(C1length, C1, 4, u, C2);

    Two_Product(acxtail, bcytail, s1, s0);
    Two_Product(acytail, bcxtail, t1, t0);
    Two_Two_Diff(s1, s0, t1, t0, u3, u[2], u[1], u[0]);
    u[3] = u3;
    Dlength = fast_expansion_sum_zeroelim(C2length, C2, 4, u, D);

    return(D[Dlength - 1]);
}

// TJH: old style function declaration
/*REAL orient2d(pa,pb,pc)
    REAL *pa;
    REAL *pb;
    REAL *pc;*/
REAL orient2d(REAL *pa,REAL *pb,REAL *pc)
{
    REAL detleft, detright, det;
    REAL detsum, errbound;

    detleft = (pa[0] - pc[0]) * (pb[1] - pc[1]);
    detright = (pa[1] - pc[1]) * (pb[0] - pc[0]);
    det = detleft - detright;

    if (detleft > 0.0) {
        if (detright <= 0.0) {
            return det;
        } else {
            detsum = detleft + detright;
        }
    } else if (detleft < 0.0) {
        if (detright >= 0.0) {
            return det;
        } else {
            detsum = -detleft - detright;
        }
    } else {
        return det;
    }

    errbound = ccwerrboundA * detsum;
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }

    return orient2dadapt(pa, pb, pc, detsum);
}

/*****************************************************************************/
/*                                                                           */
/*  orient3dfast()   Approximate 3D orientation test.  Nonrobust.            */
/*  orient3dexact()   Exact 3D orientation test.  Robust.                    */
/*  orient3dslow()   Another exact 3D orientation test.  Robust.             */
/*  orient3d()   Adaptive exact 3D orientation test.  Robust.                */
/*                                                                           */
/*               Return a positive value if the point pd lies below the      */
/*               plane passing through pa, pb, and pc; "below" is defined so */
/*               that pa, pb, and pc appear in counterclockwise order when   */
/*               viewed from above the plane.  Returns a negative value if   */
/*               pd lies above the plane.  Returns zero if the points are    */
/*               coplanar.  The result is also a rough approximation of six  */
/*               times the signed volume of the tetrahedron defined by the   */
/*               four points.                                                */
/*                                                                           */
/*  Only the first and last routine should be used; the middle two are for   */
/*  timings.                                                                 */
/*                                                                           */
/*  The last three use exact arithmetic to ensure a correct answer.  The     */
/*  result returned is the determinant of a matrix.  In orient3d() only,     */
/*  this determinant is computed adaptively, in the sense that exact         */
/*  arithmetic is used only to the degree it is needed to ensure that the    */
/*  returned value has the correct sign.  Hence, orient3d() is usually quite */
/*  fast, but will run more slowly when the input points are coplanar or     */
/*  nearly so.                                                               */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*REAL orient3dfast(pa, pb, pc, pd)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;*/
REAL orient3dfast(REAL *pa,REAL *pb,REAL *pc,REAL *pd)
{
    REAL adx, bdx, cdx;
    REAL ady, bdy, cdy;
    REAL adz, bdz, cdz;

    adx = pa[0] - pd[0];
    bdx = pb[0] - pd[0];
    cdx = pc[0] - pd[0];
    ady = pa[1] - pd[1];
    bdy = pb[1] - pd[1];
    cdy = pc[1] - pd[1];
    adz = pa[2] - pd[2];
    bdz = pb[2] - pd[2];
    cdz = pc[2] - pd[2];

    return adx * (bdy * cdz - bdz * cdy)
        + bdx * (cdy * adz - cdz * ady)
        + cdx * (ady * bdz - adz * bdy);
}

// TJH: old style function declaration
/*REAL orient3dexact(pa, pb, pc, pd)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;*/
REAL orient3dexact(REAL *pa,REAL *pb,REAL *pc,REAL *pd)
{
    INEXACT REAL axby1, bxcy1, cxdy1, dxay1, axcy1, bxdy1;
    INEXACT REAL bxay1, cxby1, dxcy1, axdy1, cxay1, dxby1;
    REAL axby0, bxcy0, cxdy0, dxay0, axcy0, bxdy0;
    REAL bxay0, cxby0, dxcy0, axdy0, cxay0, dxby0;
    REAL ab[4], bc[4], cd[4], da[4], ac[4], bd[4];
    REAL temp8[8];
    int templen;
    REAL abc[12], bcd[12], cda[12], dab[12];
    int abclen, bcdlen, cdalen, dablen;
    REAL adet[24], bdet[24], cdet[24], ddet[24];
    int alen, blen, clen, dlen;
    REAL abdet[48], cddet[48];
    int ablen, cdlen;
    REAL deter[96];
    int deterlen;
    int i;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL ahi, alo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j;
    REAL _0;

    Two_Product(pa[0], pb[1], axby1, axby0);
    Two_Product(pb[0], pa[1], bxay1, bxay0);
    Two_Two_Diff(axby1, axby0, bxay1, bxay0, ab[3], ab[2], ab[1], ab[0]);

    Two_Product(pb[0], pc[1], bxcy1, bxcy0);
    Two_Product(pc[0], pb[1], cxby1, cxby0);
    Two_Two_Diff(bxcy1, bxcy0, cxby1, cxby0, bc[3], bc[2], bc[1], bc[0]);

    Two_Product(pc[0], pd[1], cxdy1, cxdy0);
    Two_Product(pd[0], pc[1], dxcy1, dxcy0);
    Two_Two_Diff(cxdy1, cxdy0, dxcy1, dxcy0, cd[3], cd[2], cd[1], cd[0]);

    Two_Product(pd[0], pa[1], dxay1, dxay0);
    Two_Product(pa[0], pd[1], axdy1, axdy0);
    Two_Two_Diff(dxay1, dxay0, axdy1, axdy0, da[3], da[2], da[1], da[0]);

    Two_Product(pa[0], pc[1], axcy1, axcy0);
    Two_Product(pc[0], pa[1], cxay1, cxay0);
    Two_Two_Diff(axcy1, axcy0, cxay1, cxay0, ac[3], ac[2], ac[1], ac[0]);

    Two_Product(pb[0], pd[1], bxdy1, bxdy0);
    Two_Product(pd[0], pb[1], dxby1, dxby0);
    Two_Two_Diff(bxdy1, bxdy0, dxby1, dxby0, bd[3], bd[2], bd[1], bd[0]);

    templen = fast_expansion_sum_zeroelim(4, cd, 4, da, temp8);
    cdalen = fast_expansion_sum_zeroelim(templen, temp8, 4, ac, cda);
    templen = fast_expansion_sum_zeroelim(4, da, 4, ab, temp8);
    dablen = fast_expansion_sum_zeroelim(templen, temp8, 4, bd, dab);
    for (i = 0; i < 4; i++) {
        bd[i] = -bd[i];
        ac[i] = -ac[i];
    }
    templen = fast_expansion_sum_zeroelim(4, ab, 4, bc, temp8);
    abclen = fast_expansion_sum_zeroelim(templen, temp8, 4, ac, abc);
    templen = fast_expansion_sum_zeroelim(4, bc, 4, cd, temp8);
    bcdlen = fast_expansion_sum_zeroelim(templen, temp8, 4, bd, bcd);

    alen = scale_expansion_zeroelim(bcdlen, bcd, pa[2], adet);
    blen = scale_expansion_zeroelim(cdalen, cda, -pb[2], bdet);
    clen = scale_expansion_zeroelim(dablen, dab, pc[2], cdet);
    dlen = scale_expansion_zeroelim(abclen, abc, -pd[2], ddet);

    ablen = fast_expansion_sum_zeroelim(alen, adet, blen, bdet, abdet);
    cdlen = fast_expansion_sum_zeroelim(clen, cdet, dlen, ddet, cddet);
    deterlen = fast_expansion_sum_zeroelim(ablen, abdet, cdlen, cddet, deter);

    return deter[deterlen - 1];
}

// TJH: old style function declaration
/*REAL orient3dslow(pa, pb, pc, pd)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;*/
REAL orient3dslow(REAL *pa,REAL *pb,REAL *pc,REAL *pd)
{
    INEXACT REAL adx, ady, adz, bdx, bdy, bdz, cdx, cdy, cdz;
    REAL adxtail, adytail, adztail;
    REAL bdxtail, bdytail, bdztail;
    REAL cdxtail, cdytail, cdztail;
    REAL negate, negatetail;
    INEXACT REAL axby7, bxcy7, axcy7, bxay7, cxby7, cxay7;
    REAL axby[8], bxcy[8], axcy[8], bxay[8], cxby[8], cxay[8];
    REAL temp16[16], temp32[32], temp32t[32];
    int temp16len, temp32len, temp32tlen;
    REAL adet[64], bdet[64], cdet[64];
    int alen, blen, clen;
    REAL abdet[128];
    int ablen;
    REAL deter[192];
    int deterlen;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL a0hi, a0lo, a1hi, a1lo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j, _k, _l, _m, _n;
    REAL _0, _1, _2;

    Two_Diff(pa[0], pd[0], adx, adxtail);
    Two_Diff(pa[1], pd[1], ady, adytail);
    Two_Diff(pa[2], pd[2], adz, adztail);
    Two_Diff(pb[0], pd[0], bdx, bdxtail);
    Two_Diff(pb[1], pd[1], bdy, bdytail);
    Two_Diff(pb[2], pd[2], bdz, bdztail);
    Two_Diff(pc[0], pd[0], cdx, cdxtail);
    Two_Diff(pc[1], pd[1], cdy, cdytail);
    Two_Diff(pc[2], pd[2], cdz, cdztail);

    Two_Two_Product(adx, adxtail, bdy, bdytail,
                    axby7, axby[6], axby[5], axby[4],
                    axby[3], axby[2], axby[1], axby[0]);
    axby[7] = axby7;
    negate = -ady;
    negatetail = -adytail;
    Two_Two_Product(bdx, bdxtail, negate, negatetail,
                    bxay7, bxay[6], bxay[5], bxay[4],
                    bxay[3], bxay[2], bxay[1], bxay[0]);
    bxay[7] = bxay7;
    Two_Two_Product(bdx, bdxtail, cdy, cdytail,
                    bxcy7, bxcy[6], bxcy[5], bxcy[4],
                    bxcy[3], bxcy[2], bxcy[1], bxcy[0]);
    bxcy[7] = bxcy7;
    negate = -bdy;
    negatetail = -bdytail;
    Two_Two_Product(cdx, cdxtail, negate, negatetail,
                    cxby7, cxby[6], cxby[5], cxby[4],
                    cxby[3], cxby[2], cxby[1], cxby[0]);
    cxby[7] = cxby7;
    Two_Two_Product(cdx, cdxtail, ady, adytail,
                    cxay7, cxay[6], cxay[5], cxay[4],
                    cxay[3], cxay[2], cxay[1], cxay[0]);
    cxay[7] = cxay7;
    negate = -cdy;
    negatetail = -cdytail;
    Two_Two_Product(adx, adxtail, negate, negatetail,
                    axcy7, axcy[6], axcy[5], axcy[4],
                    axcy[3], axcy[2], axcy[1], axcy[0]);
    axcy[7] = axcy7;

    temp16len = fast_expansion_sum_zeroelim(8, bxcy, 8, cxby, temp16);
    temp32len = scale_expansion_zeroelim(temp16len, temp16, adz, temp32);
    temp32tlen = scale_expansion_zeroelim(temp16len, temp16, adztail, temp32t);
    alen = fast_expansion_sum_zeroelim(temp32len, temp32, temp32tlen, temp32t,
                                       adet);

    temp16len = fast_expansion_sum_zeroelim(8, cxay, 8, axcy, temp16);
    temp32len = scale_expansion_zeroelim(temp16len, temp16, bdz, temp32);
    temp32tlen = scale_expansion_zeroelim(temp16len, temp16, bdztail, temp32t);
    blen = fast_expansion_sum_zeroelim(temp32len, temp32, temp32tlen, temp32t,
                                       bdet);

    temp16len = fast_expansion_sum_zeroelim(8, axby, 8, bxay, temp16);
    temp32len = scale_expansion_zeroelim(temp16len, temp16, cdz, temp32);
    temp32tlen = scale_expansion_zeroelim(temp16len, temp16, cdztail, temp32t);
    clen = fast_expansion_sum_zeroelim(temp32len, temp32, temp32tlen, temp32t,
                                       cdet);

    ablen = fast_expansion_sum_zeroelim(alen, adet, blen, bdet, abdet);
    deterlen = fast_expansion_sum_zeroelim(ablen, abdet, clen, cdet, deter);

    return deter[deterlen - 1];
}

// TJH: old style function declaration
/*REAL orient3dadapt(pa, pb, pc, pd, permanent)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;
    REAL permanent;*/
REAL orient3dadapt(REAL *pa,REAL *pb,REAL *pc,REAL *pd,REAL permanent)
{
    INEXACT REAL adx, bdx, cdx, ady, bdy, cdy, adz, bdz, cdz;
    REAL det, errbound;

    INEXACT REAL bdxcdy1, cdxbdy1, cdxady1, adxcdy1, adxbdy1, bdxady1;
    REAL bdxcdy0, cdxbdy0, cdxady0, adxcdy0, adxbdy0, bdxady0;
    REAL bc[4], ca[4], ab[4];
    INEXACT REAL bc3, ca3, ab3;
    REAL adet[8], bdet[8], cdet[8];
    int alen, blen, clen;
    REAL abdet[16];
    int ablen;
    REAL *finnow, *finother, *finswap;
    REAL fin1[192], fin2[192];
    int finlength;

    REAL adxtail, bdxtail, cdxtail;
    REAL adytail, bdytail, cdytail;
    REAL adztail, bdztail, cdztail;
    INEXACT REAL at_blarge, at_clarge;
    INEXACT REAL bt_clarge, bt_alarge;
    INEXACT REAL ct_alarge, ct_blarge;
    REAL at_b[4], at_c[4], bt_c[4], bt_a[4], ct_a[4], ct_b[4];
    int at_blen, at_clen, bt_clen, bt_alen, ct_alen, ct_blen;
    INEXACT REAL bdxt_cdy1, cdxt_bdy1, cdxt_ady1;
    INEXACT REAL adxt_cdy1, adxt_bdy1, bdxt_ady1;
    REAL bdxt_cdy0, cdxt_bdy0, cdxt_ady0;
    REAL adxt_cdy0, adxt_bdy0, bdxt_ady0;
    INEXACT REAL bdyt_cdx1, cdyt_bdx1, cdyt_adx1;
    INEXACT REAL adyt_cdx1, adyt_bdx1, bdyt_adx1;
    REAL bdyt_cdx0, cdyt_bdx0, cdyt_adx0;
    REAL adyt_cdx0, adyt_bdx0, bdyt_adx0;
    REAL bct[8], cat[8], abt[8];
    int bctlen, catlen, abtlen;
    INEXACT REAL bdxt_cdyt1, cdxt_bdyt1, cdxt_adyt1;
    INEXACT REAL adxt_cdyt1, adxt_bdyt1, bdxt_adyt1;
    REAL bdxt_cdyt0, cdxt_bdyt0, cdxt_adyt0;
    REAL adxt_cdyt0, adxt_bdyt0, bdxt_adyt0;
    REAL u[4], v[12], w[16];
    INEXACT REAL u3;
    int vlength, wlength;
    REAL negate;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL ahi, alo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j, _k;
    REAL _0;

    adx = (REAL) (pa[0] - pd[0]);
    bdx = (REAL) (pb[0] - pd[0]);
    cdx = (REAL) (pc[0] - pd[0]);
    ady = (REAL) (pa[1] - pd[1]);
    bdy = (REAL) (pb[1] - pd[1]);
    cdy = (REAL) (pc[1] - pd[1]);
    adz = (REAL) (pa[2] - pd[2]);
    bdz = (REAL) (pb[2] - pd[2]);
    cdz = (REAL) (pc[2] - pd[2]);

    Two_Product(bdx, cdy, bdxcdy1, bdxcdy0);
    Two_Product(cdx, bdy, cdxbdy1, cdxbdy0);
    Two_Two_Diff(bdxcdy1, bdxcdy0, cdxbdy1, cdxbdy0, bc3, bc[2], bc[1], bc[0]);
    bc[3] = bc3;
    alen = scale_expansion_zeroelim(4, bc, adz, adet);

    Two_Product(cdx, ady, cdxady1, cdxady0);
    Two_Product(adx, cdy, adxcdy1, adxcdy0);
    Two_Two_Diff(cdxady1, cdxady0, adxcdy1, adxcdy0, ca3, ca[2], ca[1], ca[0]);
    ca[3] = ca3;
    blen = scale_expansion_zeroelim(4, ca, bdz, bdet);

    Two_Product(adx, bdy, adxbdy1, adxbdy0);
    Two_Product(bdx, ady, bdxady1, bdxady0);
    Two_Two_Diff(adxbdy1, adxbdy0, bdxady1, bdxady0, ab3, ab[2], ab[1], ab[0]);
    ab[3] = ab3;
    clen = scale_expansion_zeroelim(4, ab, cdz, cdet);

    ablen = fast_expansion_sum_zeroelim(alen, adet, blen, bdet, abdet);
    finlength = fast_expansion_sum_zeroelim(ablen, abdet, clen, cdet, fin1);

    det = estimate(finlength, fin1);
    errbound = o3derrboundB * permanent;
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }

    Two_Diff_Tail(pa[0], pd[0], adx, adxtail);
    Two_Diff_Tail(pb[0], pd[0], bdx, bdxtail);
    Two_Diff_Tail(pc[0], pd[0], cdx, cdxtail);
    Two_Diff_Tail(pa[1], pd[1], ady, adytail);
    Two_Diff_Tail(pb[1], pd[1], bdy, bdytail);
    Two_Diff_Tail(pc[1], pd[1], cdy, cdytail);
    Two_Diff_Tail(pa[2], pd[2], adz, adztail);
    Two_Diff_Tail(pb[2], pd[2], bdz, bdztail);
    Two_Diff_Tail(pc[2], pd[2], cdz, cdztail);

    if ((adxtail == 0.0) && (bdxtail == 0.0) && (cdxtail == 0.0)
        && (adytail == 0.0) && (bdytail == 0.0) && (cdytail == 0.0)
        && (adztail == 0.0) && (bdztail == 0.0) && (cdztail == 0.0)) {
        return det;
    }

    errbound = o3derrboundC * permanent + resulterrbound * Absolute(det);
    det += (adz * ((bdx * cdytail + cdy * bdxtail)
                   - (bdy * cdxtail + cdx * bdytail))
            + adztail * (bdx * cdy - bdy * cdx))
        + (bdz * ((cdx * adytail + ady * cdxtail)
                  - (cdy * adxtail + adx * cdytail))
           + bdztail * (cdx * ady - cdy * adx))
        + (cdz * ((adx * bdytail + bdy * adxtail)
                  - (ady * bdxtail + bdx * adytail))
           + cdztail * (adx * bdy - ady * bdx));
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }

    finnow = fin1;
    finother = fin2;

    if (adxtail == 0.0) {
        if (adytail == 0.0) {
            at_b[0] = 0.0;
            at_blen = 1;
            at_c[0] = 0.0;
            at_clen = 1;
        } else {
            negate = -adytail;
            Two_Product(negate, bdx, at_blarge, at_b[0]);
            at_b[1] = at_blarge;
            at_blen = 2;
            Two_Product(adytail, cdx, at_clarge, at_c[0]);
            at_c[1] = at_clarge;
            at_clen = 2;
        }
    } else {
        if (adytail == 0.0) {
            Two_Product(adxtail, bdy, at_blarge, at_b[0]);
            at_b[1] = at_blarge;
            at_blen = 2;
            negate = -adxtail;
            Two_Product(negate, cdy, at_clarge, at_c[0]);
            at_c[1] = at_clarge;
            at_clen = 2;
        } else {
            Two_Product(adxtail, bdy, adxt_bdy1, adxt_bdy0);
            Two_Product(adytail, bdx, adyt_bdx1, adyt_bdx0);
            Two_Two_Diff(adxt_bdy1, adxt_bdy0, adyt_bdx1, adyt_bdx0,
                         at_blarge, at_b[2], at_b[1], at_b[0]);
            at_b[3] = at_blarge;
            at_blen = 4;
            Two_Product(adytail, cdx, adyt_cdx1, adyt_cdx0);
            Two_Product(adxtail, cdy, adxt_cdy1, adxt_cdy0);
            Two_Two_Diff(adyt_cdx1, adyt_cdx0, adxt_cdy1, adxt_cdy0,
                         at_clarge, at_c[2], at_c[1], at_c[0]);
            at_c[3] = at_clarge;
            at_clen = 4;
        }
    }
    if (bdxtail == 0.0) {
        if (bdytail == 0.0) {
            bt_c[0] = 0.0;
            bt_clen = 1;
            bt_a[0] = 0.0;
            bt_alen = 1;
        } else {
            negate = -bdytail;
            Two_Product(negate, cdx, bt_clarge, bt_c[0]);
            bt_c[1] = bt_clarge;
            bt_clen = 2;
            Two_Product(bdytail, adx, bt_alarge, bt_a[0]);
            bt_a[1] = bt_alarge;
            bt_alen = 2;
        }
    } else {
        if (bdytail == 0.0) {
            Two_Product(bdxtail, cdy, bt_clarge, bt_c[0]);
            bt_c[1] = bt_clarge;
            bt_clen = 2;
            negate = -bdxtail;
            Two_Product(negate, ady, bt_alarge, bt_a[0]);
            bt_a[1] = bt_alarge;
            bt_alen = 2;
        } else {
            Two_Product(bdxtail, cdy, bdxt_cdy1, bdxt_cdy0);
            Two_Product(bdytail, cdx, bdyt_cdx1, bdyt_cdx0);
            Two_Two_Diff(bdxt_cdy1, bdxt_cdy0, bdyt_cdx1, bdyt_cdx0,
                         bt_clarge, bt_c[2], bt_c[1], bt_c[0]);
            bt_c[3] = bt_clarge;
            bt_clen = 4;
            Two_Product(bdytail, adx, bdyt_adx1, bdyt_adx0);
            Two_Product(bdxtail, ady, bdxt_ady1, bdxt_ady0);
            Two_Two_Diff(bdyt_adx1, bdyt_adx0, bdxt_ady1, bdxt_ady0,
                         bt_alarge, bt_a[2], bt_a[1], bt_a[0]);
            bt_a[3] = bt_alarge;
            bt_alen = 4;
        }
    }
    if (cdxtail == 0.0) {
        if (cdytail == 0.0) {
            ct_a[0] = 0.0;
            ct_alen = 1;
            ct_b[0] = 0.0;
            ct_blen = 1;
        } else {
            negate = -cdytail;
            Two_Product(negate, adx, ct_alarge, ct_a[0]);
            ct_a[1] = ct_alarge;
            ct_alen = 2;
            Two_Product(cdytail, bdx, ct_blarge, ct_b[0]);
            ct_b[1] = ct_blarge;
            ct_blen = 2;
        }
    } else {
        if (cdytail == 0.0) {
            Two_Product(cdxtail, ady, ct_alarge, ct_a[0]);
            ct_a[1] = ct_alarge;
            ct_alen = 2;
            negate = -cdxtail;
            Two_Product(negate, bdy, ct_blarge, ct_b[0]);
            ct_b[1] = ct_blarge;
            ct_blen = 2;
        } else {
            Two_Product(cdxtail, ady, cdxt_ady1, cdxt_ady0);
            Two_Product(cdytail, adx, cdyt_adx1, cdyt_adx0);
            Two_Two_Diff(cdxt_ady1, cdxt_ady0, cdyt_adx1, cdyt_adx0,
                         ct_alarge, ct_a[2], ct_a[1], ct_a[0]);
            ct_a[3] = ct_alarge;
            ct_alen = 4;
            Two_Product(cdytail, bdx, cdyt_bdx1, cdyt_bdx0);
            Two_Product(cdxtail, bdy, cdxt_bdy1, cdxt_bdy0);
            Two_Two_Diff(cdyt_bdx1, cdyt_bdx0, cdxt_bdy1, cdxt_bdy0,
                         ct_blarge, ct_b[2], ct_b[1], ct_b[0]);
            ct_b[3] = ct_blarge;
            ct_blen = 4;
        }
    }

    bctlen = fast_expansion_sum_zeroelim(bt_clen, bt_c, ct_blen, ct_b, bct);
    wlength = scale_expansion_zeroelim(bctlen, bct, adz, w);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                            finother);
    finswap = finnow; finnow = finother; finother = finswap;

    catlen = fast_expansion_sum_zeroelim(ct_alen, ct_a, at_clen, at_c, cat);
    wlength = scale_expansion_zeroelim(catlen, cat, bdz, w);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                            finother);
    finswap = finnow; finnow = finother; finother = finswap;

    abtlen = fast_expansion_sum_zeroelim(at_blen, at_b, bt_alen, bt_a, abt);
    wlength = scale_expansion_zeroelim(abtlen, abt, cdz, w);
    finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                            finother);
    finswap = finnow; finnow = finother; finother = finswap;

    if (adztail != 0.0) {
        vlength = scale_expansion_zeroelim(4, bc, adztail, v);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, vlength, v,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }
    if (bdztail != 0.0) {
        vlength = scale_expansion_zeroelim(4, ca, bdztail, v);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, vlength, v,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }
    if (cdztail != 0.0) {
        vlength = scale_expansion_zeroelim(4, ab, cdztail, v);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, vlength, v,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }

    if (adxtail != 0.0) {
        if (bdytail != 0.0) {
            Two_Product(adxtail, bdytail, adxt_bdyt1, adxt_bdyt0);
            Two_One_Product(adxt_bdyt1, adxt_bdyt0, cdz, u3, u[2], u[1], u[0]);
            u[3] = u3;
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                    finother);
            finswap = finnow; finnow = finother; finother = finswap;
            if (cdztail != 0.0) {
                Two_One_Product(adxt_bdyt1, adxt_bdyt0, cdztail, u3, u[2], u[1], u[0]);
                u[3] = u3;
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                        finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }
        }
        if (cdytail != 0.0) {
            negate = -adxtail;
            Two_Product(negate, cdytail, adxt_cdyt1, adxt_cdyt0);
            Two_One_Product(adxt_cdyt1, adxt_cdyt0, bdz, u3, u[2], u[1], u[0]);
            u[3] = u3;
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                    finother);
            finswap = finnow; finnow = finother; finother = finswap;
            if (bdztail != 0.0) {
                Two_One_Product(adxt_cdyt1, adxt_cdyt0, bdztail, u3, u[2], u[1], u[0]);
                u[3] = u3;
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                        finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }
        }
    }
    if (bdxtail != 0.0) {
        if (cdytail != 0.0) {
            Two_Product(bdxtail, cdytail, bdxt_cdyt1, bdxt_cdyt0);
            Two_One_Product(bdxt_cdyt1, bdxt_cdyt0, adz, u3, u[2], u[1], u[0]);
            u[3] = u3;
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                    finother);
            finswap = finnow; finnow = finother; finother = finswap;
            if (adztail != 0.0) {
                Two_One_Product(bdxt_cdyt1, bdxt_cdyt0, adztail, u3, u[2], u[1], u[0]);
                u[3] = u3;
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                        finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }
        }
        if (adytail != 0.0) {
            negate = -bdxtail;
            Two_Product(negate, adytail, bdxt_adyt1, bdxt_adyt0);
            Two_One_Product(bdxt_adyt1, bdxt_adyt0, cdz, u3, u[2], u[1], u[0]);
            u[3] = u3;
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                    finother);
            finswap = finnow; finnow = finother; finother = finswap;
            if (cdztail != 0.0) {
                Two_One_Product(bdxt_adyt1, bdxt_adyt0, cdztail, u3, u[2], u[1], u[0]);
                u[3] = u3;
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                        finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }
        }
    }
    if (cdxtail != 0.0) {
        if (adytail != 0.0) {
            Two_Product(cdxtail, adytail, cdxt_adyt1, cdxt_adyt0);
            Two_One_Product(cdxt_adyt1, cdxt_adyt0, bdz, u3, u[2], u[1], u[0]);
            u[3] = u3;
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                    finother);
            finswap = finnow; finnow = finother; finother = finswap;
            if (bdztail != 0.0) {
                Two_One_Product(cdxt_adyt1, cdxt_adyt0, bdztail, u3, u[2], u[1], u[0]);
                u[3] = u3;
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                        finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }
        }
        if (bdytail != 0.0) {
            negate = -cdxtail;
            Two_Product(negate, bdytail, cdxt_bdyt1, cdxt_bdyt0);
            Two_One_Product(cdxt_bdyt1, cdxt_bdyt0, adz, u3, u[2], u[1], u[0]);
            u[3] = u3;
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                    finother);
            finswap = finnow; finnow = finother; finother = finswap;
            if (adztail != 0.0) {
                Two_One_Product(cdxt_bdyt1, cdxt_bdyt0, adztail, u3, u[2], u[1], u[0]);
                u[3] = u3;
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, 4, u,
                                                        finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }
        }
    }

    if (adztail != 0.0) {
        wlength = scale_expansion_zeroelim(bctlen, bct, adztail, w);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }
    if (bdztail != 0.0) {
        wlength = scale_expansion_zeroelim(catlen, cat, bdztail, w);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }
    if (cdztail != 0.0) {
        wlength = scale_expansion_zeroelim(abtlen, abt, cdztail, w);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, wlength, w,
                                                finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }

    return finnow[finlength - 1];
}

// TJH: old style function declaration
/*REAL orient3d(pa, pb, pc, pd)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;*/
REAL orient3d(REAL *pa,REAL *pb,REAL *pc,REAL *pd)
{
    REAL adx, bdx, cdx, ady, bdy, cdy, adz, bdz, cdz;
    REAL bdxcdy, cdxbdy, cdxady, adxcdy, adxbdy, bdxady;
    REAL det;
    REAL permanent, errbound;

    adx = pa[0] - pd[0];
    bdx = pb[0] - pd[0];
    cdx = pc[0] - pd[0];
    ady = pa[1] - pd[1];
    bdy = pb[1] - pd[1];
    cdy = pc[1] - pd[1];
    adz = pa[2] - pd[2];
    bdz = pb[2] - pd[2];
    cdz = pc[2] - pd[2];

    bdxcdy = bdx * cdy;
    cdxbdy = cdx * bdy;

    cdxady = cdx * ady;
    adxcdy = adx * cdy;

    adxbdy = adx * bdy;
    bdxady = bdx * ady;

    det = adz * (bdxcdy - cdxbdy) 
        + bdz * (cdxady - adxcdy)
        + cdz * (adxbdy - bdxady);

    permanent = (Absolute(bdxcdy) + Absolute(cdxbdy)) * Absolute(adz)
        + (Absolute(cdxady) + Absolute(adxcdy)) * Absolute(bdz)
        + (Absolute(adxbdy) + Absolute(bdxady)) * Absolute(cdz);
    errbound = o3derrboundA * permanent;
    if ((det > errbound) || (-det > errbound)) {
        return det;
    }

    return orient3dadapt(pa, pb, pc, pd, permanent);
}

/*****************************************************************************/
/*                                                                           */
/*  incirclefast()   Approximate 2D incircle test.  Nonrobust.               */
/*  incircleexact()   Exact 2D incircle test.  Robust.                       */
/*  incircleslow()   Another exact 2D incircle test.  Robust.                */
/*  incircle()   Adaptive exact 2D incircle test.  Robust.                   */
/*                                                                           */
/*               Return a positive value if the point pd lies inside the     */
/*               circle passing through pa, pb, and pc; a negative value if  */
/*               it lies outside; and zero if the four points are cocircular.*/
/*               The points pa, pb, and pc must be in counterclockwise       */
/*               order, or the sign of the result will be reversed.          */
/*                                                                           */
/*  Only the first and last routine should be used; the middle two are for   */
/*  timings.                                                                 */
/*                                                                           */
/*  The last three use exact arithmetic to ensure a correct answer.  The     */
/*  result returned is the determinant of a matrix.  In incircle() only,     */
/*  this determinant is computed adaptively, in the sense that exact         */
/*  arithmetic is used only to the degree it is needed to ensure that the    */
/*  returned value has the correct sign.  Hence, incircle() is usually quite */
/*  fast, but will run more slowly when the input points are cocircular or   */
/*  nearly so.                                                               */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*REAL incirclefast(pa, pb, pc, pd)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;*/
REAL incirclefast(REAL *pa,REAL *pb,REAL *pc,REAL *pd)
{
    REAL adx, ady, bdx, bdy, cdx, cdy;
    REAL abdet, bcdet, cadet;
    REAL alift, blift, clift;

    adx = pa[0] - pd[0];
    ady = pa[1] - pd[1];
    bdx = pb[0] - pd[0];
    bdy = pb[1] - pd[1];
    cdx = pc[0] - pd[0];
    cdy = pc[1] - pd[1];

    abdet = adx * bdy - bdx * ady;
    bcdet = bdx * cdy - cdx * bdy;
    cadet = cdx * ady - adx * cdy;
    alift = adx * adx + ady * ady;
    blift = bdx * bdx + bdy * bdy;
    clift = cdx * cdx + cdy * cdy;

    return alift * bcdet + blift * cadet + clift * abdet;
}

// TJH: old style function declaration
/*REAL incircleexact(pa, pb, pc, pd)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;*/
REAL incircleexact(REAL *pa,REAL *pb,REAL *pc,REAL *pd)
{
    INEXACT REAL axby1, bxcy1, cxdy1, dxay1, axcy1, bxdy1;
    INEXACT REAL bxay1, cxby1, dxcy1, axdy1, cxay1, dxby1;
    REAL axby0, bxcy0, cxdy0, dxay0, axcy0, bxdy0;
    REAL bxay0, cxby0, dxcy0, axdy0, cxay0, dxby0;
    REAL ab[4], bc[4], cd[4], da[4], ac[4], bd[4];
    REAL temp8[8];
    int templen;
    REAL abc[12], bcd[12], cda[12], dab[12];
    int abclen, bcdlen, cdalen, dablen;
    REAL det24x[24], det24y[24], det48x[48], det48y[48];
    int xlen, ylen;
    REAL adet[96], bdet[96], cdet[96], ddet[96];
    int alen, blen, clen, dlen;
    REAL abdet[192], cddet[192];
    int ablen, cdlen;
    REAL deter[384];
    int deterlen;
    int i;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL ahi, alo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j;
    REAL _0;

    Two_Product(pa[0], pb[1], axby1, axby0);
    Two_Product(pb[0], pa[1], bxay1, bxay0);
    Two_Two_Diff(axby1, axby0, bxay1, bxay0, ab[3], ab[2], ab[1], ab[0]);

    Two_Product(pb[0], pc[1], bxcy1, bxcy0);
    Two_Product(pc[0], pb[1], cxby1, cxby0);
    Two_Two_Diff(bxcy1, bxcy0, cxby1, cxby0, bc[3], bc[2], bc[1], bc[0]);

    Two_Product(pc[0], pd[1], cxdy1, cxdy0);
    Two_Product(pd[0], pc[1], dxcy1, dxcy0);
    Two_Two_Diff(cxdy1, cxdy0, dxcy1, dxcy0, cd[3], cd[2], cd[1], cd[0]);

    Two_Product(pd[0], pa[1], dxay1, dxay0);
    Two_Product(pa[0], pd[1], axdy1, axdy0);
    Two_Two_Diff(dxay1, dxay0, axdy1, axdy0, da[3], da[2], da[1], da[0]);

    Two_Product(pa[0], pc[1], axcy1, axcy0);
    Two_Product(pc[0], pa[1], cxay1, cxay0);
    Two_Two_Diff(axcy1, axcy0, cxay1, cxay0, ac[3], ac[2], ac[1], ac[0]);

    Two_Product(pb[0], pd[1], bxdy1, bxdy0);
    Two_Product(pd[0], pb[1], dxby1, dxby0);
    Two_Two_Diff(bxdy1, bxdy0, dxby1, dxby0, bd[3], bd[2], bd[1], bd[0]);

    templen = fast_expansion_sum_zeroelim(4, cd, 4, da, temp8);
    cdalen = fast_expansion_sum_zeroelim(templen, temp8, 4, ac, cda);
    templen = fast_expansion_sum_zeroelim(4, da, 4, ab, temp8);
    dablen = fast_expansion_sum_zeroelim(templen, temp8, 4, bd, dab);
    for (i = 0; i < 4; i++) {
        bd[i] = -bd[i];
        ac[i] = -ac[i];
    }
    templen = fast_expansion_sum_zeroelim(4, ab, 4, bc, temp8);
    abclen = fast_expansion_sum_zeroelim(templen, temp8, 4, ac, abc);
    templen = fast_expansion_sum_zeroelim(4, bc, 4, cd, temp8);
    bcdlen = fast_expansion_sum_zeroelim(templen, temp8, 4, bd, bcd);

    xlen = scale_expansion_zeroelim(bcdlen, bcd, pa[0], det24x);
    xlen = scale_expansion_zeroelim(xlen, det24x, pa[0], det48x);
    ylen = scale_expansion_zeroelim(bcdlen, bcd, pa[1], det24y);
    ylen = scale_expansion_zeroelim(ylen, det24y, pa[1], det48y);
    alen = fast_expansion_sum_zeroelim(xlen, det48x, ylen, det48y, adet);

    xlen = scale_expansion_zeroelim(cdalen, cda, pb[0], det24x);
    xlen = scale_expansion_zeroelim(xlen, det24x, -pb[0], det48x);
    ylen = scale_expansion_zeroelim(cdalen, cda, pb[1], det24y);
    ylen = scale_expansion_zeroelim(ylen, det24y, -pb[1], det48y);
    blen = fast_expansion_sum_zeroelim(xlen, det48x, ylen, det48y, bdet);

    xlen = scale_expansion_zeroelim(dablen, dab, pc[0], det24x);
    xlen = scale_expansion_zeroelim(xlen, det24x, pc[0], det48x);
    ylen = scale_expansion_zeroelim(dablen, dab, pc[1], det24y);
    ylen = scale_expansion_zeroelim(ylen, det24y, pc[1], det48y);
    clen = fast_expansion_sum_zeroelim(xlen, det48x, ylen, det48y, cdet);

    xlen = scale_expansion_zeroelim(abclen, abc, pd[0], det24x);
    xlen = scale_expansion_zeroelim(xlen, det24x, -pd[0], det48x);
    ylen = scale_expansion_zeroelim(abclen, abc, pd[1], det24y);
    ylen = scale_expansion_zeroelim(ylen, det24y, -pd[1], det48y);
    dlen = fast_expansion_sum_zeroelim(xlen, det48x, ylen, det48y, ddet);

    ablen = fast_expansion_sum_zeroelim(alen, adet, blen, bdet, abdet);
    cdlen = fast_expansion_sum_zeroelim(clen, cdet, dlen, ddet, cddet);
    deterlen = fast_expansion_sum_zeroelim(ablen, abdet, cdlen, cddet, deter);

    return deter[deterlen - 1];
}

// TJH: old style function declaration
/*REAL incircleslow(pa, pb, pc, pd)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;*/
REAL incircleslow(REAL *pa,REAL *pb,REAL *pc,REAL *pd)
{
    INEXACT REAL adx, bdx, cdx, ady, bdy, cdy;
    REAL adxtail, bdxtail, cdxtail;
    REAL adytail, bdytail, cdytail;
    REAL negate, negatetail;
    INEXACT REAL axby7, bxcy7, axcy7, bxay7, cxby7, cxay7;
    REAL axby[8], bxcy[8], axcy[8], bxay[8], cxby[8], cxay[8];
    REAL temp16[16];
    int temp16len;
    REAL detx[32], detxx[64], detxt[32], detxxt[64], detxtxt[64];
    int xlen, xxlen, xtlen, xxtlen, xtxtlen;
    REAL x1[128], x2[192];
    int x1len, x2len;
    REAL dety[32], detyy[64], detyt[32], detyyt[64], detytyt[64];
    int ylen, yylen, ytlen, yytlen, ytytlen;
    REAL y1[128], y2[192];
    int y1len, y2len;
    REAL adet[384], bdet[384], cdet[384], abdet[768], deter[1152];
    int alen, blen, clen, ablen, deterlen;
    int i;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL a0hi, a0lo, a1hi, a1lo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j, _k, _l, _m, _n;
    REAL _0, _1, _2;

    Two_Diff(pa[0], pd[0], adx, adxtail);
    Two_Diff(pa[1], pd[1], ady, adytail);
    Two_Diff(pb[0], pd[0], bdx, bdxtail);
    Two_Diff(pb[1], pd[1], bdy, bdytail);
    Two_Diff(pc[0], pd[0], cdx, cdxtail);
    Two_Diff(pc[1], pd[1], cdy, cdytail);

    Two_Two_Product(adx, adxtail, bdy, bdytail,
                    axby7, axby[6], axby[5], axby[4],
                    axby[3], axby[2], axby[1], axby[0]);
    axby[7] = axby7;
    negate = -ady;
    negatetail = -adytail;
    Two_Two_Product(bdx, bdxtail, negate, negatetail,
                    bxay7, bxay[6], bxay[5], bxay[4],
                    bxay[3], bxay[2], bxay[1], bxay[0]);
    bxay[7] = bxay7;
    Two_Two_Product(bdx, bdxtail, cdy, cdytail,
                    bxcy7, bxcy[6], bxcy[5], bxcy[4],
                    bxcy[3], bxcy[2], bxcy[1], bxcy[0]);
    bxcy[7] = bxcy7;
    negate = -bdy;
    negatetail = -bdytail;
    Two_Two_Product(cdx, cdxtail, negate, negatetail,
                    cxby7, cxby[6], cxby[5], cxby[4],
                    cxby[3], cxby[2], cxby[1], cxby[0]);
    cxby[7] = cxby7;
    Two_Two_Product(cdx, cdxtail, ady, adytail,
                    cxay7, cxay[6], cxay[5], cxay[4],
                    cxay[3], cxay[2], cxay[1], cxay[0]);
    cxay[7] = cxay7;
    negate = -cdy;
    negatetail = -cdytail;
    Two_Two_Product(adx, adxtail, negate, negatetail,
                    axcy7, axcy[6], axcy[5], axcy[4],
                    axcy[3], axcy[2], axcy[1], axcy[0]);
    axcy[7] = axcy7;


    temp16len = fast_expansion_sum_zeroelim(8, bxcy, 8, cxby, temp16);

    xlen = scale_expansion_zeroelim(temp16len, temp16, adx, detx);
    xxlen = scale_expansion_zeroelim(xlen, detx, adx, detxx);
    xtlen = scale_expansion_zeroelim(temp16len, temp16, adxtail, detxt);
    xxtlen = scale_expansion_zeroelim(xtlen, detxt, adx, detxxt);
    for (i = 0; i < xxtlen; i++) {
        detxxt[i] *= 2.0;
    }
    xtxtlen = scale_expansion_zeroelim(xtlen, detxt, adxtail, detxtxt);
    x1len = fast_expansion_sum_zeroelim(xxlen, detxx, xxtlen, detxxt, x1);
    x2len = fast_expansion_sum_zeroelim(x1len, x1, xtxtlen, detxtxt, x2);

    ylen = scale_expansion_zeroelim(temp16len, temp16, ady, dety);
    yylen = scale_expansion_zeroelim(ylen, dety, ady, detyy);
    ytlen = scale_expansion_zeroelim(temp16len, temp16, adytail, detyt);
    yytlen = scale_expansion_zeroelim(ytlen, detyt, ady, detyyt);
    for (i = 0; i < yytlen; i++) {
        detyyt[i] *= 2.0;
    }
    ytytlen = scale_expansion_zeroelim(ytlen, detyt, adytail, detytyt);
    y1len = fast_expansion_sum_zeroelim(yylen, detyy, yytlen, detyyt, y1);
    y2len = fast_expansion_sum_zeroelim(y1len, y1, ytytlen, detytyt, y2);

    alen = fast_expansion_sum_zeroelim(x2len, x2, y2len, y2, adet);


    temp16len = fast_expansion_sum_zeroelim(8, cxay, 8, axcy, temp16);

    xlen = scale_expansion_zeroelim(temp16len, temp16, bdx, detx);
    xxlen = scale_expansion_zeroelim(xlen, detx, bdx, detxx);
    xtlen = scale_expansion_zeroelim(temp16len, temp16, bdxtail, detxt);
    xxtlen = scale_expansion_zeroelim(xtlen, detxt, bdx, detxxt);
    for (i = 0; i < xxtlen; i++) {
        detxxt[i] *= 2.0;
    }
    xtxtlen = scale_expansion_zeroelim(xtlen, detxt, bdxtail, detxtxt);
    x1len = fast_expansion_sum_zeroelim(xxlen, detxx, xxtlen, detxxt, x1);
    x2len = fast_expansion_sum_zeroelim(x1len, x1, xtxtlen, detxtxt, x2);

    ylen = scale_expansion_zeroelim(temp16len, temp16, bdy, dety);
    yylen = scale_expansion_zeroelim(ylen, dety, bdy, detyy);
    ytlen = scale_expansion_zeroelim(temp16len, temp16, bdytail, detyt);
    yytlen = scale_expansion_zeroelim(ytlen, detyt, bdy, detyyt);
    for (i = 0; i < yytlen; i++) {
        detyyt[i] *= 2.0;
    }
    ytytlen = scale_expansion_zeroelim(ytlen, detyt, bdytail, detytyt);
    y1len = fast_expansion_sum_zeroelim(yylen, detyy, yytlen, detyyt, y1);
    y2len = fast_expansion_sum_zeroelim(y1len, y1, ytytlen, detytyt, y2);

    blen = fast_expansion_sum_zeroelim(x2len, x2, y2len, y2, bdet);


    temp16len = fast_expansion_sum_zeroelim(8, axby, 8, bxay, temp16);

    xlen = scale_expansion_zeroelim(temp16len, temp16, cdx, detx);
    xxlen = scale_expansion_zeroelim(xlen, detx, cdx, detxx);
    xtlen = scale_expansion_zeroelim(temp16len, temp16, cdxtail, detxt);
    xxtlen = scale_expansion_zeroelim(xtlen, detxt, cdx, detxxt);
    for (i = 0; i < xxtlen; i++) {
        detxxt[i] *= 2.0;
    }
    xtxtlen = scale_expansion_zeroelim(xtlen, detxt, cdxtail, detxtxt);
    x1len = fast_expansion_sum_zeroelim(xxlen, detxx, xxtlen, detxxt, x1);
    x2len = fast_expansion_sum_zeroelim(x1len, x1, xtxtlen, detxtxt, x2);

    ylen = scale_expansion_zeroelim(temp16len, temp16, cdy, dety);
    yylen = scale_expansion_zeroelim(ylen, dety, cdy, detyy);
    ytlen = scale_expansion_zeroelim(temp16len, temp16, cdytail, detyt);
    yytlen = scale_expansion_zeroelim(ytlen, detyt, cdy, detyyt);
    for (i = 0; i < yytlen; i++) {
        detyyt[i] *= 2.0;
    }
    ytytlen = scale_expansion_zeroelim(ytlen, detyt, cdytail, detytyt);
    y1len = fast_expansion_sum_zeroelim(yylen, detyy, yytlen, detyyt, y1);
    y2len = fast_expansion_sum_zeroelim(y1len, y1, ytytlen, detytyt, y2);

    clen = fast_expansion_sum_zeroelim(x2len, x2, y2len, y2, cdet);

    ablen = fast_expansion_sum_zeroelim(alen, adet, blen, bdet, abdet);
    deterlen = fast_expansion_sum_zeroelim(ablen, abdet, clen, cdet, deter);

    return deter[deterlen - 1];
}

// TJH: old style function declaration
/*REAL incircleadapt(pa, pb, pc, pd, permanent)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;
    REAL permanent;*/
REAL incircleadapt(REAL *pa,REAL *pb,REAL *pc,REAL *pd,REAL permanent)
{
    INEXACT REAL adx, bdx, cdx, ady, bdy, cdy;
    REAL det, errbound;

    INEXACT REAL bdxcdy1, cdxbdy1, cdxady1, adxcdy1, adxbdy1, bdxady1;
    REAL bdxcdy0, cdxbdy0, cdxady0, adxcdy0, adxbdy0, bdxady0;
    REAL bc[4], ca[4], ab[4];
    INEXACT REAL bc3, ca3, ab3;
    REAL axbc[8], axxbc[16], aybc[8], ayybc[16], adet[32];
    int axbclen, axxbclen, aybclen, ayybclen, alen;
    REAL bxca[8], bxxca[16], byca[8], byyca[16], bdet[32];
    int bxcalen, bxxcalen, bycalen, byycalen, blen;
    REAL cxab[8], cxxab[16], cyab[8], cyyab[16], cdet[32];
    int cxablen, cxxablen, cyablen, cyyablen, clen;
    REAL abdet[64];
    int ablen;
    REAL fin1[1152], fin2[1152];
    REAL *finnow, *finother, *finswap;
    int finlength;

    REAL adxtail, bdxtail, cdxtail, adytail, bdytail, cdytail;
    INEXACT REAL adxadx1, adyady1, bdxbdx1, bdybdy1, cdxcdx1, cdycdy1;
    REAL adxadx0, adyady0, bdxbdx0, bdybdy0, cdxcdx0, cdycdy0;
    REAL aa[4], bb[4], cc[4];
    INEXACT REAL aa3, bb3, cc3;
    INEXACT REAL ti1, tj1;
    REAL ti0, tj0;
    REAL u[4], v[4];
    INEXACT REAL u3, v3;
    REAL temp8[8], temp16a[16], temp16b[16], temp16c[16];
    REAL temp32a[32], temp32b[32], temp48[48], temp64[64];
    int temp8len, temp16alen, temp16blen, temp16clen;
    int temp32alen, temp32blen, temp48len, temp64len;
    REAL axtbb[8], axtcc[8], aytbb[8], aytcc[8];
    int axtbblen, axtcclen, aytbblen, aytcclen;
    REAL bxtaa[8], bxtcc[8], bytaa[8], bytcc[8];
    int bxtaalen, bxtcclen, bytaalen, bytcclen;
    REAL cxtaa[8], cxtbb[8], cytaa[8], cytbb[8];
    int cxtaalen, cxtbblen, cytaalen, cytbblen;
    REAL axtbc[8], aytbc[8], bxtca[8], bytca[8], cxtab[8], cytab[8];
    int axtbclen, aytbclen, bxtcalen, bytcalen, cxtablen, cytablen;
    REAL axtbct[16], aytbct[16], bxtcat[16], bytcat[16], cxtabt[16], cytabt[16];
    int axtbctlen, aytbctlen, bxtcatlen, bytcatlen, cxtabtlen, cytabtlen;
    REAL axtbctt[8], aytbctt[8], bxtcatt[8];
    REAL bytcatt[8], cxtabtt[8], cytabtt[8];
    int axtbcttlen, aytbcttlen, bxtcattlen, bytcattlen, cxtabttlen, cytabttlen;
    REAL abt[8], bct[8], cat[8];
    int abtlen, bctlen, catlen;
    REAL abtt[4], bctt[4], catt[4];
    int abttlen, bcttlen, cattlen;
    INEXACT REAL abtt3, bctt3, catt3;
    REAL negate;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL ahi, alo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j;
    REAL _0;

    adx = (REAL) (pa[0] - pd[0]);
    bdx = (REAL) (pb[0] - pd[0]);
    cdx = (REAL) (pc[0] - pd[0]);
    ady = (REAL) (pa[1] - pd[1]);
    bdy = (REAL) (pb[1] - pd[1]);
    cdy = (REAL) (pc[1] - pd[1]);

    Two_Product(bdx, cdy, bdxcdy1, bdxcdy0);
    Two_Product(cdx, bdy, cdxbdy1, cdxbdy0);
    Two_Two_Diff(bdxcdy1, bdxcdy0, cdxbdy1, cdxbdy0, bc3, bc[2], bc[1], bc[0]);
    bc[3] = bc3;
    axbclen = scale_expansion_zeroelim(4, bc, adx, axbc);
    axxbclen = scale_expansion_zeroelim(axbclen, axbc, adx, axxbc);
    aybclen = scale_expansion_zeroelim(4, bc, ady, aybc);
    ayybclen = scale_expansion_zeroelim(aybclen, aybc, ady, ayybc);
    alen = fast_expansion_sum_zeroelim(axxbclen, axxbc, ayybclen, ayybc, adet);

    Two_Product(cdx, ady, cdxady1, cdxady0);
    Two_Product(adx, cdy, adxcdy1, adxcdy0);
    Two_Two_Diff(cdxady1, cdxady0, adxcdy1, adxcdy0, ca3, ca[2], ca[1], ca[0]);
    ca[3] = ca3;
    bxcalen = scale_expansion_zeroelim(4, ca, bdx, bxca);
    bxxcalen = scale_expansion_zeroelim(bxcalen, bxca, bdx, bxxca);
    bycalen = scale_expansion_zeroelim(4, ca, bdy, byca);
    byycalen = scale_expansion_zeroelim(bycalen, byca, bdy, byyca);
    blen = fast_expansion_sum_zeroelim(bxxcalen, bxxca, byycalen, byyca, bdet);

    Two_Product(adx, bdy, adxbdy1, adxbdy0);
    Two_Product(bdx, ady, bdxady1, bdxady0);
    Two_Two_Diff(adxbdy1, adxbdy0, bdxady1, bdxady0, ab3, ab[2], ab[1], ab[0]);
    ab[3] = ab3;
    cxablen = scale_expansion_zeroelim(4, ab, cdx, cxab);
    cxxablen = scale_expansion_zeroelim(cxablen, cxab, cdx, cxxab);
    cyablen = scale_expansion_zeroelim(4, ab, cdy, cyab);
    cyyablen = scale_expansion_zeroelim(cyablen, cyab, cdy, cyyab);
    clen = fast_expansion_sum_zeroelim(cxxablen, cxxab, cyyablen, cyyab, cdet);

    ablen = fast_expansion_sum_zeroelim(alen, adet, blen, bdet, abdet);
    finlength = fast_expansion_sum_zeroelim(ablen, abdet, clen, cdet, fin1);

    det = estimate(finlength, fin1);
    errbound = iccerrboundB * permanent;
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }

    Two_Diff_Tail(pa[0], pd[0], adx, adxtail);
    Two_Diff_Tail(pa[1], pd[1], ady, adytail);
    Two_Diff_Tail(pb[0], pd[0], bdx, bdxtail);
    Two_Diff_Tail(pb[1], pd[1], bdy, bdytail);
    Two_Diff_Tail(pc[0], pd[0], cdx, cdxtail);
    Two_Diff_Tail(pc[1], pd[1], cdy, cdytail);
    if ((adxtail == 0.0) && (bdxtail == 0.0) && (cdxtail == 0.0)
        && (adytail == 0.0) && (bdytail == 0.0) && (cdytail == 0.0)) {
        return det;
    }

    errbound = iccerrboundC * permanent + resulterrbound * Absolute(det);
    det += ((adx * adx + ady * ady) * ((bdx * cdytail + cdy * bdxtail)
                                       - (bdy * cdxtail + cdx * bdytail))
            + 2.0 * (adx * adxtail + ady * adytail) * (bdx * cdy - bdy * cdx))
        + ((bdx * bdx + bdy * bdy) * ((cdx * adytail + ady * cdxtail)
                                      - (cdy * adxtail + adx * cdytail))
           + 2.0 * (bdx * bdxtail + bdy * bdytail) * (cdx * ady - cdy * adx))
        + ((cdx * cdx + cdy * cdy) * ((adx * bdytail + bdy * adxtail)
                                      - (ady * bdxtail + bdx * adytail))
           + 2.0 * (cdx * cdxtail + cdy * cdytail) * (adx * bdy - ady * bdx));
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }

    finnow = fin1;
    finother = fin2;

    if ((bdxtail != 0.0) || (bdytail != 0.0)
        || (cdxtail != 0.0) || (cdytail != 0.0)) {
        Square(adx, adxadx1, adxadx0);
        Square(ady, adyady1, adyady0);
        Two_Two_Sum(adxadx1, adxadx0, adyady1, adyady0, aa3, aa[2], aa[1], aa[0]);
        aa[3] = aa3;
    }
    if ((cdxtail != 0.0) || (cdytail != 0.0)
        || (adxtail != 0.0) || (adytail != 0.0)) {
        Square(bdx, bdxbdx1, bdxbdx0);
        Square(bdy, bdybdy1, bdybdy0);
        Two_Two_Sum(bdxbdx1, bdxbdx0, bdybdy1, bdybdy0, bb3, bb[2], bb[1], bb[0]);
        bb[3] = bb3;
    }
    if ((adxtail != 0.0) || (adytail != 0.0)
        || (bdxtail != 0.0) || (bdytail != 0.0)) {
        Square(cdx, cdxcdx1, cdxcdx0);
        Square(cdy, cdycdy1, cdycdy0);
        Two_Two_Sum(cdxcdx1, cdxcdx0, cdycdy1, cdycdy0, cc3, cc[2], cc[1], cc[0]);
        cc[3] = cc3;
    }

    if (adxtail != 0.0) {
        axtbclen = scale_expansion_zeroelim(4, bc, adxtail, axtbc);
        temp16alen = scale_expansion_zeroelim(axtbclen, axtbc, 2.0 * adx,
                                              temp16a);

        axtcclen = scale_expansion_zeroelim(4, cc, adxtail, axtcc);
        temp16blen = scale_expansion_zeroelim(axtcclen, axtcc, bdy, temp16b);

        axtbblen = scale_expansion_zeroelim(4, bb, adxtail, axtbb);
        temp16clen = scale_expansion_zeroelim(axtbblen, axtbb, -cdy, temp16c);

        temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                 temp16blen, temp16b, temp32a);
        temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                                temp32alen, temp32a, temp48);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                temp48, finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }
    if (adytail != 0.0) {
        aytbclen = scale_expansion_zeroelim(4, bc, adytail, aytbc);
        temp16alen = scale_expansion_zeroelim(aytbclen, aytbc, 2.0 * ady,
                                              temp16a);

        aytbblen = scale_expansion_zeroelim(4, bb, adytail, aytbb);
        temp16blen = scale_expansion_zeroelim(aytbblen, aytbb, cdx, temp16b);

        aytcclen = scale_expansion_zeroelim(4, cc, adytail, aytcc);
        temp16clen = scale_expansion_zeroelim(aytcclen, aytcc, -bdx, temp16c);

        temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                 temp16blen, temp16b, temp32a);
        temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                                temp32alen, temp32a, temp48);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                temp48, finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }
    if (bdxtail != 0.0) {
        bxtcalen = scale_expansion_zeroelim(4, ca, bdxtail, bxtca);
        temp16alen = scale_expansion_zeroelim(bxtcalen, bxtca, 2.0 * bdx,
                                              temp16a);

        bxtaalen = scale_expansion_zeroelim(4, aa, bdxtail, bxtaa);
        temp16blen = scale_expansion_zeroelim(bxtaalen, bxtaa, cdy, temp16b);

        bxtcclen = scale_expansion_zeroelim(4, cc, bdxtail, bxtcc);
        temp16clen = scale_expansion_zeroelim(bxtcclen, bxtcc, -ady, temp16c);

        temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                 temp16blen, temp16b, temp32a);
        temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                                temp32alen, temp32a, temp48);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                temp48, finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }
    if (bdytail != 0.0) {
        bytcalen = scale_expansion_zeroelim(4, ca, bdytail, bytca);
        temp16alen = scale_expansion_zeroelim(bytcalen, bytca, 2.0 * bdy,
                                              temp16a);

        bytcclen = scale_expansion_zeroelim(4, cc, bdytail, bytcc);
        temp16blen = scale_expansion_zeroelim(bytcclen, bytcc, adx, temp16b);

        bytaalen = scale_expansion_zeroelim(4, aa, bdytail, bytaa);
        temp16clen = scale_expansion_zeroelim(bytaalen, bytaa, -cdx, temp16c);

        temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                 temp16blen, temp16b, temp32a);
        temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                                temp32alen, temp32a, temp48);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                temp48, finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }
    if (cdxtail != 0.0) {
        cxtablen = scale_expansion_zeroelim(4, ab, cdxtail, cxtab);
        temp16alen = scale_expansion_zeroelim(cxtablen, cxtab, 2.0 * cdx,
                                              temp16a);

        cxtbblen = scale_expansion_zeroelim(4, bb, cdxtail, cxtbb);
        temp16blen = scale_expansion_zeroelim(cxtbblen, cxtbb, ady, temp16b);

        cxtaalen = scale_expansion_zeroelim(4, aa, cdxtail, cxtaa);
        temp16clen = scale_expansion_zeroelim(cxtaalen, cxtaa, -bdy, temp16c);

        temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                 temp16blen, temp16b, temp32a);
        temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                                temp32alen, temp32a, temp48);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                temp48, finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }
    if (cdytail != 0.0) {
        cytablen = scale_expansion_zeroelim(4, ab, cdytail, cytab);
        temp16alen = scale_expansion_zeroelim(cytablen, cytab, 2.0 * cdy,
                                              temp16a);

        cytaalen = scale_expansion_zeroelim(4, aa, cdytail, cytaa);
        temp16blen = scale_expansion_zeroelim(cytaalen, cytaa, bdx, temp16b);

        cytbblen = scale_expansion_zeroelim(4, bb, cdytail, cytbb);
        temp16clen = scale_expansion_zeroelim(cytbblen, cytbb, -adx, temp16c);

        temp32alen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                 temp16blen, temp16b, temp32a);
        temp48len = fast_expansion_sum_zeroelim(temp16clen, temp16c,
                                                temp32alen, temp32a, temp48);
        finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                temp48, finother);
        finswap = finnow; finnow = finother; finother = finswap;
    }

    if ((adxtail != 0.0) || (adytail != 0.0)) {
        if ((bdxtail != 0.0) || (bdytail != 0.0)
            || (cdxtail != 0.0) || (cdytail != 0.0)) {
            Two_Product(bdxtail, cdy, ti1, ti0);
            Two_Product(bdx, cdytail, tj1, tj0);
            Two_Two_Sum(ti1, ti0, tj1, tj0, u3, u[2], u[1], u[0]);
            u[3] = u3;
            negate = -bdy;
            Two_Product(cdxtail, negate, ti1, ti0);
            negate = -bdytail;
            Two_Product(cdx, negate, tj1, tj0);
            Two_Two_Sum(ti1, ti0, tj1, tj0, v3, v[2], v[1], v[0]);
            v[3] = v3;
            bctlen = fast_expansion_sum_zeroelim(4, u, 4, v, bct);

            Two_Product(bdxtail, cdytail, ti1, ti0);
            Two_Product(cdxtail, bdytail, tj1, tj0);
            Two_Two_Diff(ti1, ti0, tj1, tj0, bctt3, bctt[2], bctt[1], bctt[0]);
            bctt[3] = bctt3;
            bcttlen = 4;
        } else {
            bct[0] = 0.0;
            bctlen = 1;
            bctt[0] = 0.0;
            bcttlen = 1;
        }

        if (adxtail != 0.0) {
            temp16alen = scale_expansion_zeroelim(axtbclen, axtbc, adxtail, temp16a);
            axtbctlen = scale_expansion_zeroelim(bctlen, bct, adxtail, axtbct);
            temp32alen = scale_expansion_zeroelim(axtbctlen, axtbct, 2.0 * adx,
                                                  temp32a);
            temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                    temp32alen, temp32a, temp48);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                    temp48, finother);
            finswap = finnow; finnow = finother; finother = finswap;
            if (bdytail != 0.0) {
                temp8len = scale_expansion_zeroelim(4, cc, adxtail, temp8);
                temp16alen = scale_expansion_zeroelim(temp8len, temp8, bdytail,
                                                      temp16a);
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                        temp16a, finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }
            if (cdytail != 0.0) {
                temp8len = scale_expansion_zeroelim(4, bb, -adxtail, temp8);
                temp16alen = scale_expansion_zeroelim(temp8len, temp8, cdytail,
                                                      temp16a);
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                        temp16a, finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }

            temp32alen = scale_expansion_zeroelim(axtbctlen, axtbct, adxtail,
                                                  temp32a);
            axtbcttlen = scale_expansion_zeroelim(bcttlen, bctt, adxtail, axtbctt);
            temp16alen = scale_expansion_zeroelim(axtbcttlen, axtbctt, 2.0 * adx,
                                                  temp16a);
            temp16blen = scale_expansion_zeroelim(axtbcttlen, axtbctt, adxtail,
                                                  temp16b);
            temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                     temp16blen, temp16b, temp32b);
            temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                                    temp32blen, temp32b, temp64);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                                    temp64, finother);
            finswap = finnow; finnow = finother; finother = finswap;
        }
        if (adytail != 0.0) {
            temp16alen = scale_expansion_zeroelim(aytbclen, aytbc, adytail, temp16a);
            aytbctlen = scale_expansion_zeroelim(bctlen, bct, adytail, aytbct);
            temp32alen = scale_expansion_zeroelim(aytbctlen, aytbct, 2.0 * ady,
                                                  temp32a);
            temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                    temp32alen, temp32a, temp48);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                    temp48, finother);
            finswap = finnow; finnow = finother; finother = finswap;


            temp32alen = scale_expansion_zeroelim(aytbctlen, aytbct, adytail,
                                                  temp32a);
            aytbcttlen = scale_expansion_zeroelim(bcttlen, bctt, adytail, aytbctt);
            temp16alen = scale_expansion_zeroelim(aytbcttlen, aytbctt, 2.0 * ady,
                                                  temp16a);
            temp16blen = scale_expansion_zeroelim(aytbcttlen, aytbctt, adytail,
                                                  temp16b);
            temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                     temp16blen, temp16b, temp32b);
            temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                                    temp32blen, temp32b, temp64);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                                    temp64, finother);
            finswap = finnow; finnow = finother; finother = finswap;
        }
    }
    if ((bdxtail != 0.0) || (bdytail != 0.0)) {
        if ((cdxtail != 0.0) || (cdytail != 0.0)
            || (adxtail != 0.0) || (adytail != 0.0)) {
            Two_Product(cdxtail, ady, ti1, ti0);
            Two_Product(cdx, adytail, tj1, tj0);
            Two_Two_Sum(ti1, ti0, tj1, tj0, u3, u[2], u[1], u[0]);
            u[3] = u3;
            negate = -cdy;
            Two_Product(adxtail, negate, ti1, ti0);
            negate = -cdytail;
            Two_Product(adx, negate, tj1, tj0);
            Two_Two_Sum(ti1, ti0, tj1, tj0, v3, v[2], v[1], v[0]);
            v[3] = v3;
            catlen = fast_expansion_sum_zeroelim(4, u, 4, v, cat);

            Two_Product(cdxtail, adytail, ti1, ti0);
            Two_Product(adxtail, cdytail, tj1, tj0);
            Two_Two_Diff(ti1, ti0, tj1, tj0, catt3, catt[2], catt[1], catt[0]);
            catt[3] = catt3;
            cattlen = 4;
        } else {
            cat[0] = 0.0;
            catlen = 1;
            catt[0] = 0.0;
            cattlen = 1;
        }

        if (bdxtail != 0.0) {
            temp16alen = scale_expansion_zeroelim(bxtcalen, bxtca, bdxtail, temp16a);
            bxtcatlen = scale_expansion_zeroelim(catlen, cat, bdxtail, bxtcat);
            temp32alen = scale_expansion_zeroelim(bxtcatlen, bxtcat, 2.0 * bdx,
                                                  temp32a);
            temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                    temp32alen, temp32a, temp48);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                    temp48, finother);
            finswap = finnow; finnow = finother; finother = finswap;
            if (cdytail != 0.0) {
                temp8len = scale_expansion_zeroelim(4, aa, bdxtail, temp8);
                temp16alen = scale_expansion_zeroelim(temp8len, temp8, cdytail,
                                                      temp16a);
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                        temp16a, finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }
            if (adytail != 0.0) {
                temp8len = scale_expansion_zeroelim(4, cc, -bdxtail, temp8);
                temp16alen = scale_expansion_zeroelim(temp8len, temp8, adytail,
                                                      temp16a);
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                        temp16a, finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }

            temp32alen = scale_expansion_zeroelim(bxtcatlen, bxtcat, bdxtail,
                                                  temp32a);
            bxtcattlen = scale_expansion_zeroelim(cattlen, catt, bdxtail, bxtcatt);
            temp16alen = scale_expansion_zeroelim(bxtcattlen, bxtcatt, 2.0 * bdx,
                                                  temp16a);
            temp16blen = scale_expansion_zeroelim(bxtcattlen, bxtcatt, bdxtail,
                                                  temp16b);
            temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                     temp16blen, temp16b, temp32b);
            temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                                    temp32blen, temp32b, temp64);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                                    temp64, finother);
            finswap = finnow; finnow = finother; finother = finswap;
        }
        if (bdytail != 0.0) {
            temp16alen = scale_expansion_zeroelim(bytcalen, bytca, bdytail, temp16a);
            bytcatlen = scale_expansion_zeroelim(catlen, cat, bdytail, bytcat);
            temp32alen = scale_expansion_zeroelim(bytcatlen, bytcat, 2.0 * bdy,
                                                  temp32a);
            temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                    temp32alen, temp32a, temp48);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                    temp48, finother);
            finswap = finnow; finnow = finother; finother = finswap;


            temp32alen = scale_expansion_zeroelim(bytcatlen, bytcat, bdytail,
                                                  temp32a);
            bytcattlen = scale_expansion_zeroelim(cattlen, catt, bdytail, bytcatt);
            temp16alen = scale_expansion_zeroelim(bytcattlen, bytcatt, 2.0 * bdy,
                                                  temp16a);
            temp16blen = scale_expansion_zeroelim(bytcattlen, bytcatt, bdytail,
                                                  temp16b);
            temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                     temp16blen, temp16b, temp32b);
            temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                                    temp32blen, temp32b, temp64);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                                    temp64, finother);
            finswap = finnow; finnow = finother; finother = finswap;
        }
    }
    if ((cdxtail != 0.0) || (cdytail != 0.0)) {
        if ((adxtail != 0.0) || (adytail != 0.0)
            || (bdxtail != 0.0) || (bdytail != 0.0)) {
            Two_Product(adxtail, bdy, ti1, ti0);
            Two_Product(adx, bdytail, tj1, tj0);
            Two_Two_Sum(ti1, ti0, tj1, tj0, u3, u[2], u[1], u[0]);
            u[3] = u3;
            negate = -ady;
            Two_Product(bdxtail, negate, ti1, ti0);
            negate = -adytail;
            Two_Product(bdx, negate, tj1, tj0);
            Two_Two_Sum(ti1, ti0, tj1, tj0, v3, v[2], v[1], v[0]);
            v[3] = v3;
            abtlen = fast_expansion_sum_zeroelim(4, u, 4, v, abt);

            Two_Product(adxtail, bdytail, ti1, ti0);
            Two_Product(bdxtail, adytail, tj1, tj0);
            Two_Two_Diff(ti1, ti0, tj1, tj0, abtt3, abtt[2], abtt[1], abtt[0]);
            abtt[3] = abtt3;
            abttlen = 4;
        } else {
            abt[0] = 0.0;
            abtlen = 1;
            abtt[0] = 0.0;
            abttlen = 1;
        }

        if (cdxtail != 0.0) {
            temp16alen = scale_expansion_zeroelim(cxtablen, cxtab, cdxtail, temp16a);
            cxtabtlen = scale_expansion_zeroelim(abtlen, abt, cdxtail, cxtabt);
            temp32alen = scale_expansion_zeroelim(cxtabtlen, cxtabt, 2.0 * cdx,
                                                  temp32a);
            temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                    temp32alen, temp32a, temp48);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                    temp48, finother);
            finswap = finnow; finnow = finother; finother = finswap;
            if (adytail != 0.0) {
                temp8len = scale_expansion_zeroelim(4, bb, cdxtail, temp8);
                temp16alen = scale_expansion_zeroelim(temp8len, temp8, adytail,
                                                      temp16a);
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                        temp16a, finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }
            if (bdytail != 0.0) {
                temp8len = scale_expansion_zeroelim(4, aa, -cdxtail, temp8);
                temp16alen = scale_expansion_zeroelim(temp8len, temp8, bdytail,
                                                      temp16a);
                finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp16alen,
                                                        temp16a, finother);
                finswap = finnow; finnow = finother; finother = finswap;
            }

            temp32alen = scale_expansion_zeroelim(cxtabtlen, cxtabt, cdxtail,
                                                  temp32a);
            cxtabttlen = scale_expansion_zeroelim(abttlen, abtt, cdxtail, cxtabtt);
            temp16alen = scale_expansion_zeroelim(cxtabttlen, cxtabtt, 2.0 * cdx,
                                                  temp16a);
            temp16blen = scale_expansion_zeroelim(cxtabttlen, cxtabtt, cdxtail,
                                                  temp16b);
            temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                     temp16blen, temp16b, temp32b);
            temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                                    temp32blen, temp32b, temp64);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                                    temp64, finother);
            finswap = finnow; finnow = finother; finother = finswap;
        }
        if (cdytail != 0.0) {
            temp16alen = scale_expansion_zeroelim(cytablen, cytab, cdytail, temp16a);
            cytabtlen = scale_expansion_zeroelim(abtlen, abt, cdytail, cytabt);
            temp32alen = scale_expansion_zeroelim(cytabtlen, cytabt, 2.0 * cdy,
                                                  temp32a);
            temp48len = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                    temp32alen, temp32a, temp48);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp48len,
                                                    temp48, finother);
            finswap = finnow; finnow = finother; finother = finswap;


            temp32alen = scale_expansion_zeroelim(cytabtlen, cytabt, cdytail,
                                                  temp32a);
            cytabttlen = scale_expansion_zeroelim(abttlen, abtt, cdytail, cytabtt);
            temp16alen = scale_expansion_zeroelim(cytabttlen, cytabtt, 2.0 * cdy,
                                                  temp16a);
            temp16blen = scale_expansion_zeroelim(cytabttlen, cytabtt, cdytail,
                                                  temp16b);
            temp32blen = fast_expansion_sum_zeroelim(temp16alen, temp16a,
                                                     temp16blen, temp16b, temp32b);
            temp64len = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                                    temp32blen, temp32b, temp64);
            finlength = fast_expansion_sum_zeroelim(finlength, finnow, temp64len,
                                                    temp64, finother);
            finswap = finnow; finnow = finother; finother = finswap;
        }
    }

    return finnow[finlength - 1];
}

// TJH: old style function declaration
/*REAL incircle(pa, pb, pc, pd)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;*/
REAL incircle(REAL *pa,REAL *pb,REAL *pc,REAL *pd)
{
    REAL adx, bdx, cdx, ady, bdy, cdy;
    REAL bdxcdy, cdxbdy, cdxady, adxcdy, adxbdy, bdxady;
    REAL alift, blift, clift;
    REAL det;
    REAL permanent, errbound;

    adx = pa[0] - pd[0];
    bdx = pb[0] - pd[0];
    cdx = pc[0] - pd[0];
    ady = pa[1] - pd[1];
    bdy = pb[1] - pd[1];
    cdy = pc[1] - pd[1];

    bdxcdy = bdx * cdy;
    cdxbdy = cdx * bdy;
    alift = adx * adx + ady * ady;

    cdxady = cdx * ady;
    adxcdy = adx * cdy;
    blift = bdx * bdx + bdy * bdy;

    adxbdy = adx * bdy;
    bdxady = bdx * ady;
    clift = cdx * cdx + cdy * cdy;

    det = alift * (bdxcdy - cdxbdy)
        + blift * (cdxady - adxcdy)
        + clift * (adxbdy - bdxady);

    permanent = (Absolute(bdxcdy) + Absolute(cdxbdy)) * alift
        + (Absolute(cdxady) + Absolute(adxcdy)) * blift
        + (Absolute(adxbdy) + Absolute(bdxady)) * clift;
    errbound = iccerrboundA * permanent;
    if ((det > errbound) || (-det > errbound)) {
        return det;
    }

    return incircleadapt(pa, pb, pc, pd, permanent);
}

/*****************************************************************************/
/*                                                                           */
/*  inspherefast()   Approximate 3D insphere test.  Nonrobust.               */
/*  insphereexact()   Exact 3D insphere test.  Robust.                       */
/*  insphereslow()   Another exact 3D insphere test.  Robust.                */
/*  insphere()   Adaptive exact 3D insphere test.  Robust.                   */
/*                                                                           */
/*               Return a positive value if the point pe lies inside the     */
/*               sphere passing through pa, pb, pc, and pd; a negative value */
/*               if it lies outside; and zero if the five points are         */
/*               cospherical.  The points pa, pb, pc, and pd must be ordered */
/*               so that they have a positive orientation (as defined by     */
/*               orient3d()), or the sign of the result will be reversed.    */
/*                                                                           */
/*  Only the first and last routine should be used; the middle two are for   */
/*  timings.                                                                 */
/*                                                                           */
/*  The last three use exact arithmetic to ensure a correct answer.  The     */
/*  result returned is the determinant of a matrix.  In insphere() only,     */
/*  this determinant is computed adaptively, in the sense that exact         */
/*  arithmetic is used only to the degree it is needed to ensure that the    */
/*  returned value has the correct sign.  Hence, insphere() is usually quite */
/*  fast, but will run more slowly when the input points are cospherical or  */
/*  nearly so.                                                               */
/*                                                                           */
/*****************************************************************************/

// TJH: old style function declaration
/*REAL inspherefast(pa, pb, pc, pd, pe)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;
    REAL *pe;*/
REAL inspherefast(REAL *pa,REAL *pb,REAL *pc,REAL *pd,REAL *pe)
{
    REAL aex, bex, cex, dex;
    REAL aey, bey, cey, dey;
    REAL aez, bez, cez, dez;
    REAL alift, blift, clift, dlift;
    REAL ab, bc, cd, da, ac, bd;
    REAL abc, bcd, cda, dab;

    aex = pa[0] - pe[0];
    bex = pb[0] - pe[0];
    cex = pc[0] - pe[0];
    dex = pd[0] - pe[0];
    aey = pa[1] - pe[1];
    bey = pb[1] - pe[1];
    cey = pc[1] - pe[1];
    dey = pd[1] - pe[1];
    aez = pa[2] - pe[2];
    bez = pb[2] - pe[2];
    cez = pc[2] - pe[2];
    dez = pd[2] - pe[2];

    ab = aex * bey - bex * aey;
    bc = bex * cey - cex * bey;
    cd = cex * dey - dex * cey;
    da = dex * aey - aex * dey;

    ac = aex * cey - cex * aey;
    bd = bex * dey - dex * bey;

    abc = aez * bc - bez * ac + cez * ab;
    bcd = bez * cd - cez * bd + dez * bc;
    cda = cez * da + dez * ac + aez * cd;
    dab = dez * ab + aez * bd + bez * da;

    alift = aex * aex + aey * aey + aez * aez;
    blift = bex * bex + bey * bey + bez * bez;
    clift = cex * cex + cey * cey + cez * cez;
    dlift = dex * dex + dey * dey + dez * dez;

    return (dlift * abc - clift * dab) + (blift * cda - alift * bcd);
}

// TJH: old style function declaration
/*REAL insphereexact(pa, pb, pc, pd, pe)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;
    REAL *pe;*/
REAL insphereexact(REAL *pa,REAL *pb,REAL *pc,REAL *pd,REAL *pe)
{
    INEXACT REAL axby1, bxcy1, cxdy1, dxey1, exay1;
    INEXACT REAL bxay1, cxby1, dxcy1, exdy1, axey1;
    INEXACT REAL axcy1, bxdy1, cxey1, dxay1, exby1;
    INEXACT REAL cxay1, dxby1, excy1, axdy1, bxey1;
    REAL axby0, bxcy0, cxdy0, dxey0, exay0;
    REAL bxay0, cxby0, dxcy0, exdy0, axey0;
    REAL axcy0, bxdy0, cxey0, dxay0, exby0;
    REAL cxay0, dxby0, excy0, axdy0, bxey0;
    REAL ab[4], bc[4], cd[4], de[4], ea[4];
    REAL ac[4], bd[4], ce[4], da[4], eb[4];
    REAL temp8a[8], temp8b[8], temp16[16];
    int temp8alen, temp8blen, temp16len;
    REAL abc[24], bcd[24], cde[24], dea[24], eab[24];
    REAL abd[24], bce[24], cda[24], deb[24], eac[24];
    int abclen, bcdlen, cdelen, dealen, eablen;
    int abdlen, bcelen, cdalen, deblen, eaclen;
    REAL temp48a[48], temp48b[48];
    int temp48alen, temp48blen;
    REAL abcd[96], bcde[96], cdea[96], deab[96], eabc[96];
    int abcdlen, bcdelen, cdealen, deablen, eabclen;
    REAL temp192[192];
    REAL det384x[384], det384y[384], det384z[384];
    int xlen, ylen, zlen;
    REAL detxy[768];
    int xylen;
    REAL adet[1152], bdet[1152], cdet[1152], ddet[1152], edet[1152];
    int alen, blen, clen, dlen, elen;
    REAL abdet[2304], cddet[2304], cdedet[3456];
    int ablen, cdlen;
    REAL deter[5760];
    int deterlen;
    int i;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL ahi, alo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j;
    REAL _0;

    Two_Product(pa[0], pb[1], axby1, axby0);
    Two_Product(pb[0], pa[1], bxay1, bxay0);
    Two_Two_Diff(axby1, axby0, bxay1, bxay0, ab[3], ab[2], ab[1], ab[0]);

    Two_Product(pb[0], pc[1], bxcy1, bxcy0);
    Two_Product(pc[0], pb[1], cxby1, cxby0);
    Two_Two_Diff(bxcy1, bxcy0, cxby1, cxby0, bc[3], bc[2], bc[1], bc[0]);

    Two_Product(pc[0], pd[1], cxdy1, cxdy0);
    Two_Product(pd[0], pc[1], dxcy1, dxcy0);
    Two_Two_Diff(cxdy1, cxdy0, dxcy1, dxcy0, cd[3], cd[2], cd[1], cd[0]);

    Two_Product(pd[0], pe[1], dxey1, dxey0);
    Two_Product(pe[0], pd[1], exdy1, exdy0);
    Two_Two_Diff(dxey1, dxey0, exdy1, exdy0, de[3], de[2], de[1], de[0]);

    Two_Product(pe[0], pa[1], exay1, exay0);
    Two_Product(pa[0], pe[1], axey1, axey0);
    Two_Two_Diff(exay1, exay0, axey1, axey0, ea[3], ea[2], ea[1], ea[0]);

    Two_Product(pa[0], pc[1], axcy1, axcy0);
    Two_Product(pc[0], pa[1], cxay1, cxay0);
    Two_Two_Diff(axcy1, axcy0, cxay1, cxay0, ac[3], ac[2], ac[1], ac[0]);

    Two_Product(pb[0], pd[1], bxdy1, bxdy0);
    Two_Product(pd[0], pb[1], dxby1, dxby0);
    Two_Two_Diff(bxdy1, bxdy0, dxby1, dxby0, bd[3], bd[2], bd[1], bd[0]);

    Two_Product(pc[0], pe[1], cxey1, cxey0);
    Two_Product(pe[0], pc[1], excy1, excy0);
    Two_Two_Diff(cxey1, cxey0, excy1, excy0, ce[3], ce[2], ce[1], ce[0]);

    Two_Product(pd[0], pa[1], dxay1, dxay0);
    Two_Product(pa[0], pd[1], axdy1, axdy0);
    Two_Two_Diff(dxay1, dxay0, axdy1, axdy0, da[3], da[2], da[1], da[0]);

    Two_Product(pe[0], pb[1], exby1, exby0);
    Two_Product(pb[0], pe[1], bxey1, bxey0);
    Two_Two_Diff(exby1, exby0, bxey1, bxey0, eb[3], eb[2], eb[1], eb[0]);

    temp8alen = scale_expansion_zeroelim(4, bc, pa[2], temp8a);
    temp8blen = scale_expansion_zeroelim(4, ac, -pb[2], temp8b);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp8blen, temp8b,
                                            temp16);
    temp8alen = scale_expansion_zeroelim(4, ab, pc[2], temp8a);
    abclen = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp16len, temp16,
                                         abc);

    temp8alen = scale_expansion_zeroelim(4, cd, pb[2], temp8a);
    temp8blen = scale_expansion_zeroelim(4, bd, -pc[2], temp8b);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp8blen, temp8b,
                                            temp16);
    temp8alen = scale_expansion_zeroelim(4, bc, pd[2], temp8a);
    bcdlen = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp16len, temp16,
                                         bcd);

    temp8alen = scale_expansion_zeroelim(4, de, pc[2], temp8a);
    temp8blen = scale_expansion_zeroelim(4, ce, -pd[2], temp8b);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp8blen, temp8b,
                                            temp16);
    temp8alen = scale_expansion_zeroelim(4, cd, pe[2], temp8a);
    cdelen = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp16len, temp16,
                                         cde);

    temp8alen = scale_expansion_zeroelim(4, ea, pd[2], temp8a);
    temp8blen = scale_expansion_zeroelim(4, da, -pe[2], temp8b);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp8blen, temp8b,
                                            temp16);
    temp8alen = scale_expansion_zeroelim(4, de, pa[2], temp8a);
    dealen = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp16len, temp16,
                                         dea);

    temp8alen = scale_expansion_zeroelim(4, ab, pe[2], temp8a);
    temp8blen = scale_expansion_zeroelim(4, eb, -pa[2], temp8b);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp8blen, temp8b,
                                            temp16);
    temp8alen = scale_expansion_zeroelim(4, ea, pb[2], temp8a);
    eablen = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp16len, temp16,
                                         eab);

    temp8alen = scale_expansion_zeroelim(4, bd, pa[2], temp8a);
    temp8blen = scale_expansion_zeroelim(4, da, pb[2], temp8b);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp8blen, temp8b,
                                            temp16);
    temp8alen = scale_expansion_zeroelim(4, ab, pd[2], temp8a);
    abdlen = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp16len, temp16,
                                         abd);

    temp8alen = scale_expansion_zeroelim(4, ce, pb[2], temp8a);
    temp8blen = scale_expansion_zeroelim(4, eb, pc[2], temp8b);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp8blen, temp8b,
                                            temp16);
    temp8alen = scale_expansion_zeroelim(4, bc, pe[2], temp8a);
    bcelen = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp16len, temp16,
                                         bce);

    temp8alen = scale_expansion_zeroelim(4, da, pc[2], temp8a);
    temp8blen = scale_expansion_zeroelim(4, ac, pd[2], temp8b);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp8blen, temp8b,
                                            temp16);
    temp8alen = scale_expansion_zeroelim(4, cd, pa[2], temp8a);
    cdalen = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp16len, temp16,
                                         cda);

    temp8alen = scale_expansion_zeroelim(4, eb, pd[2], temp8a);
    temp8blen = scale_expansion_zeroelim(4, bd, pe[2], temp8b);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp8blen, temp8b,
                                            temp16);
    temp8alen = scale_expansion_zeroelim(4, de, pb[2], temp8a);
    deblen = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp16len, temp16,
                                         deb);

    temp8alen = scale_expansion_zeroelim(4, ac, pe[2], temp8a);
    temp8blen = scale_expansion_zeroelim(4, ce, pa[2], temp8b);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp8blen, temp8b,
                                            temp16);
    temp8alen = scale_expansion_zeroelim(4, ea, pc[2], temp8a);
    eaclen = fast_expansion_sum_zeroelim(temp8alen, temp8a, temp16len, temp16,
                                         eac);

    temp48alen = fast_expansion_sum_zeroelim(cdelen, cde, bcelen, bce, temp48a);
    temp48blen = fast_expansion_sum_zeroelim(deblen, deb, bcdlen, bcd, temp48b);
    for (i = 0; i < temp48blen; i++) {
        temp48b[i] = -temp48b[i];
    }
    bcdelen = fast_expansion_sum_zeroelim(temp48alen, temp48a,
                                          temp48blen, temp48b, bcde);
    xlen = scale_expansion_zeroelim(bcdelen, bcde, pa[0], temp192);
    xlen = scale_expansion_zeroelim(xlen, temp192, pa[0], det384x);
    ylen = scale_expansion_zeroelim(bcdelen, bcde, pa[1], temp192);
    ylen = scale_expansion_zeroelim(ylen, temp192, pa[1], det384y);
    zlen = scale_expansion_zeroelim(bcdelen, bcde, pa[2], temp192);
    zlen = scale_expansion_zeroelim(zlen, temp192, pa[2], det384z);
    xylen = fast_expansion_sum_zeroelim(xlen, det384x, ylen, det384y, detxy);
    alen = fast_expansion_sum_zeroelim(xylen, detxy, zlen, det384z, adet);

    temp48alen = fast_expansion_sum_zeroelim(dealen, dea, cdalen, cda, temp48a);
    temp48blen = fast_expansion_sum_zeroelim(eaclen, eac, cdelen, cde, temp48b);
    for (i = 0; i < temp48blen; i++) {
        temp48b[i] = -temp48b[i];
    }
    cdealen = fast_expansion_sum_zeroelim(temp48alen, temp48a,
                                          temp48blen, temp48b, cdea);
    xlen = scale_expansion_zeroelim(cdealen, cdea, pb[0], temp192);
    xlen = scale_expansion_zeroelim(xlen, temp192, pb[0], det384x);
    ylen = scale_expansion_zeroelim(cdealen, cdea, pb[1], temp192);
    ylen = scale_expansion_zeroelim(ylen, temp192, pb[1], det384y);
    zlen = scale_expansion_zeroelim(cdealen, cdea, pb[2], temp192);
    zlen = scale_expansion_zeroelim(zlen, temp192, pb[2], det384z);
    xylen = fast_expansion_sum_zeroelim(xlen, det384x, ylen, det384y, detxy);
    blen = fast_expansion_sum_zeroelim(xylen, detxy, zlen, det384z, bdet);

    temp48alen = fast_expansion_sum_zeroelim(eablen, eab, deblen, deb, temp48a);
    temp48blen = fast_expansion_sum_zeroelim(abdlen, abd, dealen, dea, temp48b);
    for (i = 0; i < temp48blen; i++) {
        temp48b[i] = -temp48b[i];
    }
    deablen = fast_expansion_sum_zeroelim(temp48alen, temp48a,
                                          temp48blen, temp48b, deab);
    xlen = scale_expansion_zeroelim(deablen, deab, pc[0], temp192);
    xlen = scale_expansion_zeroelim(xlen, temp192, pc[0], det384x);
    ylen = scale_expansion_zeroelim(deablen, deab, pc[1], temp192);
    ylen = scale_expansion_zeroelim(ylen, temp192, pc[1], det384y);
    zlen = scale_expansion_zeroelim(deablen, deab, pc[2], temp192);
    zlen = scale_expansion_zeroelim(zlen, temp192, pc[2], det384z);
    xylen = fast_expansion_sum_zeroelim(xlen, det384x, ylen, det384y, detxy);
    clen = fast_expansion_sum_zeroelim(xylen, detxy, zlen, det384z, cdet);

    temp48alen = fast_expansion_sum_zeroelim(abclen, abc, eaclen, eac, temp48a);
    temp48blen = fast_expansion_sum_zeroelim(bcelen, bce, eablen, eab, temp48b);
    for (i = 0; i < temp48blen; i++) {
        temp48b[i] = -temp48b[i];
    }
    eabclen = fast_expansion_sum_zeroelim(temp48alen, temp48a,
                                          temp48blen, temp48b, eabc);
    xlen = scale_expansion_zeroelim(eabclen, eabc, pd[0], temp192);
    xlen = scale_expansion_zeroelim(xlen, temp192, pd[0], det384x);
    ylen = scale_expansion_zeroelim(eabclen, eabc, pd[1], temp192);
    ylen = scale_expansion_zeroelim(ylen, temp192, pd[1], det384y);
    zlen = scale_expansion_zeroelim(eabclen, eabc, pd[2], temp192);
    zlen = scale_expansion_zeroelim(zlen, temp192, pd[2], det384z);
    xylen = fast_expansion_sum_zeroelim(xlen, det384x, ylen, det384y, detxy);
    dlen = fast_expansion_sum_zeroelim(xylen, detxy, zlen, det384z, ddet);

    temp48alen = fast_expansion_sum_zeroelim(bcdlen, bcd, abdlen, abd, temp48a);
    temp48blen = fast_expansion_sum_zeroelim(cdalen, cda, abclen, abc, temp48b);
    for (i = 0; i < temp48blen; i++) {
        temp48b[i] = -temp48b[i];
    }
    abcdlen = fast_expansion_sum_zeroelim(temp48alen, temp48a,
                                          temp48blen, temp48b, abcd);
    xlen = scale_expansion_zeroelim(abcdlen, abcd, pe[0], temp192);
    xlen = scale_expansion_zeroelim(xlen, temp192, pe[0], det384x);
    ylen = scale_expansion_zeroelim(abcdlen, abcd, pe[1], temp192);
    ylen = scale_expansion_zeroelim(ylen, temp192, pe[1], det384y);
    zlen = scale_expansion_zeroelim(abcdlen, abcd, pe[2], temp192);
    zlen = scale_expansion_zeroelim(zlen, temp192, pe[2], det384z);
    xylen = fast_expansion_sum_zeroelim(xlen, det384x, ylen, det384y, detxy);
    elen = fast_expansion_sum_zeroelim(xylen, detxy, zlen, det384z, edet);

    ablen = fast_expansion_sum_zeroelim(alen, adet, blen, bdet, abdet);
    cdlen = fast_expansion_sum_zeroelim(clen, cdet, dlen, ddet, cddet);
    cdelen = fast_expansion_sum_zeroelim(cdlen, cddet, elen, edet, cdedet);
    deterlen = fast_expansion_sum_zeroelim(ablen, abdet, cdelen, cdedet, deter);

    return deter[deterlen - 1];
}

// TJH: old style function declaration
/*REAL insphereslow(pa, pb, pc, pd, pe)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;
    REAL *pe;*/
REAL insphereslow(REAL *pa,REAL *pb,REAL *pc,REAL *pd,REAL *pe)
{
    INEXACT REAL aex, bex, cex, dex, aey, bey, cey, dey, aez, bez, cez, dez;
    REAL aextail, bextail, cextail, dextail;
    REAL aeytail, beytail, ceytail, deytail;
    REAL aeztail, beztail, ceztail, deztail;
    REAL negate, negatetail;
    INEXACT REAL axby7, bxcy7, cxdy7, dxay7, axcy7, bxdy7;
    INEXACT REAL bxay7, cxby7, dxcy7, axdy7, cxay7, dxby7;
    REAL axby[8], bxcy[8], cxdy[8], dxay[8], axcy[8], bxdy[8];
    REAL bxay[8], cxby[8], dxcy[8], axdy[8], cxay[8], dxby[8];
    REAL ab[16], bc[16], cd[16], da[16], ac[16], bd[16];
    int ablen, bclen, cdlen, dalen, aclen, bdlen;
    REAL temp32a[32], temp32b[32], temp64a[64], temp64b[64], temp64c[64];
    int temp32alen, temp32blen, temp64alen, temp64blen, temp64clen;
    REAL temp128[128], temp192[192];
    int temp128len, temp192len;
    REAL detx[384], detxx[768], detxt[384], detxxt[768], detxtxt[768];
    int xlen, xxlen, xtlen, xxtlen, xtxtlen;
    REAL x1[1536], x2[2304];
    int x1len, x2len;
    REAL dety[384], detyy[768], detyt[384], detyyt[768], detytyt[768];
    int ylen, yylen, ytlen, yytlen, ytytlen;
    REAL y1[1536], y2[2304];
    int y1len, y2len;
    REAL detz[384], detzz[768], detzt[384], detzzt[768], detztzt[768];
    int zlen, zzlen, ztlen, zztlen, ztztlen;
    REAL z1[1536], z2[2304];
    int z1len, z2len;
    REAL detxy[4608];
    int xylen;
    REAL adet[6912], bdet[6912], cdet[6912], ddet[6912];
    int alen, blen, clen, dlen;
    REAL abdet[13824], cddet[13824], deter[27648];
    int deterlen;
    int i;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL a0hi, a0lo, a1hi, a1lo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j, _k, _l, _m, _n;
    REAL _0, _1, _2;

    Two_Diff(pa[0], pe[0], aex, aextail);
    Two_Diff(pa[1], pe[1], aey, aeytail);
    Two_Diff(pa[2], pe[2], aez, aeztail);
    Two_Diff(pb[0], pe[0], bex, bextail);
    Two_Diff(pb[1], pe[1], bey, beytail);
    Two_Diff(pb[2], pe[2], bez, beztail);
    Two_Diff(pc[0], pe[0], cex, cextail);
    Two_Diff(pc[1], pe[1], cey, ceytail);
    Two_Diff(pc[2], pe[2], cez, ceztail);
    Two_Diff(pd[0], pe[0], dex, dextail);
    Two_Diff(pd[1], pe[1], dey, deytail);
    Two_Diff(pd[2], pe[2], dez, deztail);

    Two_Two_Product(aex, aextail, bey, beytail,
                    axby7, axby[6], axby[5], axby[4],
                    axby[3], axby[2], axby[1], axby[0]);
    axby[7] = axby7;
    negate = -aey;
    negatetail = -aeytail;
    Two_Two_Product(bex, bextail, negate, negatetail,
                    bxay7, bxay[6], bxay[5], bxay[4],
                    bxay[3], bxay[2], bxay[1], bxay[0]);
    bxay[7] = bxay7;
    ablen = fast_expansion_sum_zeroelim(8, axby, 8, bxay, ab);
    Two_Two_Product(bex, bextail, cey, ceytail,
                    bxcy7, bxcy[6], bxcy[5], bxcy[4],
                    bxcy[3], bxcy[2], bxcy[1], bxcy[0]);
    bxcy[7] = bxcy7;
    negate = -bey;
    negatetail = -beytail;
    Two_Two_Product(cex, cextail, negate, negatetail,
                    cxby7, cxby[6], cxby[5], cxby[4],
                    cxby[3], cxby[2], cxby[1], cxby[0]);
    cxby[7] = cxby7;
    bclen = fast_expansion_sum_zeroelim(8, bxcy, 8, cxby, bc);
    Two_Two_Product(cex, cextail, dey, deytail,
                    cxdy7, cxdy[6], cxdy[5], cxdy[4],
                    cxdy[3], cxdy[2], cxdy[1], cxdy[0]);
    cxdy[7] = cxdy7;
    negate = -cey;
    negatetail = -ceytail;
    Two_Two_Product(dex, dextail, negate, negatetail,
                    dxcy7, dxcy[6], dxcy[5], dxcy[4],
                    dxcy[3], dxcy[2], dxcy[1], dxcy[0]);
    dxcy[7] = dxcy7;
    cdlen = fast_expansion_sum_zeroelim(8, cxdy, 8, dxcy, cd);
    Two_Two_Product(dex, dextail, aey, aeytail,
                    dxay7, dxay[6], dxay[5], dxay[4],
                    dxay[3], dxay[2], dxay[1], dxay[0]);
    dxay[7] = dxay7;
    negate = -dey;
    negatetail = -deytail;
    Two_Two_Product(aex, aextail, negate, negatetail,
                    axdy7, axdy[6], axdy[5], axdy[4],
                    axdy[3], axdy[2], axdy[1], axdy[0]);
    axdy[7] = axdy7;
    dalen = fast_expansion_sum_zeroelim(8, dxay, 8, axdy, da);
    Two_Two_Product(aex, aextail, cey, ceytail,
                    axcy7, axcy[6], axcy[5], axcy[4],
                    axcy[3], axcy[2], axcy[1], axcy[0]);
    axcy[7] = axcy7;
    negate = -aey;
    negatetail = -aeytail;
    Two_Two_Product(cex, cextail, negate, negatetail,
                    cxay7, cxay[6], cxay[5], cxay[4],
                    cxay[3], cxay[2], cxay[1], cxay[0]);
    cxay[7] = cxay7;
    aclen = fast_expansion_sum_zeroelim(8, axcy, 8, cxay, ac);
    Two_Two_Product(bex, bextail, dey, deytail,
                    bxdy7, bxdy[6], bxdy[5], bxdy[4],
                    bxdy[3], bxdy[2], bxdy[1], bxdy[0]);
    bxdy[7] = bxdy7;
    negate = -bey;
    negatetail = -beytail;
    Two_Two_Product(dex, dextail, negate, negatetail,
                    dxby7, dxby[6], dxby[5], dxby[4],
                    dxby[3], dxby[2], dxby[1], dxby[0]);
    dxby[7] = dxby7;
    bdlen = fast_expansion_sum_zeroelim(8, bxdy, 8, dxby, bd);

    temp32alen = scale_expansion_zeroelim(cdlen, cd, -bez, temp32a);
    temp32blen = scale_expansion_zeroelim(cdlen, cd, -beztail, temp32b);
    temp64alen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64a);
    temp32alen = scale_expansion_zeroelim(bdlen, bd, cez, temp32a);
    temp32blen = scale_expansion_zeroelim(bdlen, bd, ceztail, temp32b);
    temp64blen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64b);
    temp32alen = scale_expansion_zeroelim(bclen, bc, -dez, temp32a);
    temp32blen = scale_expansion_zeroelim(bclen, bc, -deztail, temp32b);
    temp64clen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64c);
    temp128len = fast_expansion_sum_zeroelim(temp64alen, temp64a,
                                             temp64blen, temp64b, temp128);
    temp192len = fast_expansion_sum_zeroelim(temp64clen, temp64c,
                                             temp128len, temp128, temp192);
    xlen = scale_expansion_zeroelim(temp192len, temp192, aex, detx);
    xxlen = scale_expansion_zeroelim(xlen, detx, aex, detxx);
    xtlen = scale_expansion_zeroelim(temp192len, temp192, aextail, detxt);
    xxtlen = scale_expansion_zeroelim(xtlen, detxt, aex, detxxt);
    for (i = 0; i < xxtlen; i++) {
        detxxt[i] *= 2.0;
    }
    xtxtlen = scale_expansion_zeroelim(xtlen, detxt, aextail, detxtxt);
    x1len = fast_expansion_sum_zeroelim(xxlen, detxx, xxtlen, detxxt, x1);
    x2len = fast_expansion_sum_zeroelim(x1len, x1, xtxtlen, detxtxt, x2);
    ylen = scale_expansion_zeroelim(temp192len, temp192, aey, dety);
    yylen = scale_expansion_zeroelim(ylen, dety, aey, detyy);
    ytlen = scale_expansion_zeroelim(temp192len, temp192, aeytail, detyt);
    yytlen = scale_expansion_zeroelim(ytlen, detyt, aey, detyyt);
    for (i = 0; i < yytlen; i++) {
        detyyt[i] *= 2.0;
    }
    ytytlen = scale_expansion_zeroelim(ytlen, detyt, aeytail, detytyt);
    y1len = fast_expansion_sum_zeroelim(yylen, detyy, yytlen, detyyt, y1);
    y2len = fast_expansion_sum_zeroelim(y1len, y1, ytytlen, detytyt, y2);
    zlen = scale_expansion_zeroelim(temp192len, temp192, aez, detz);
    zzlen = scale_expansion_zeroelim(zlen, detz, aez, detzz);
    ztlen = scale_expansion_zeroelim(temp192len, temp192, aeztail, detzt);
    zztlen = scale_expansion_zeroelim(ztlen, detzt, aez, detzzt);
    for (i = 0; i < zztlen; i++) {
        detzzt[i] *= 2.0;
    }
    ztztlen = scale_expansion_zeroelim(ztlen, detzt, aeztail, detztzt);
    z1len = fast_expansion_sum_zeroelim(zzlen, detzz, zztlen, detzzt, z1);
    z2len = fast_expansion_sum_zeroelim(z1len, z1, ztztlen, detztzt, z2);
    xylen = fast_expansion_sum_zeroelim(x2len, x2, y2len, y2, detxy);
    alen = fast_expansion_sum_zeroelim(z2len, z2, xylen, detxy, adet);

    temp32alen = scale_expansion_zeroelim(dalen, da, cez, temp32a);
    temp32blen = scale_expansion_zeroelim(dalen, da, ceztail, temp32b);
    temp64alen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64a);
    temp32alen = scale_expansion_zeroelim(aclen, ac, dez, temp32a);
    temp32blen = scale_expansion_zeroelim(aclen, ac, deztail, temp32b);
    temp64blen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64b);
    temp32alen = scale_expansion_zeroelim(cdlen, cd, aez, temp32a);
    temp32blen = scale_expansion_zeroelim(cdlen, cd, aeztail, temp32b);
    temp64clen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64c);
    temp128len = fast_expansion_sum_zeroelim(temp64alen, temp64a,
                                             temp64blen, temp64b, temp128);
    temp192len = fast_expansion_sum_zeroelim(temp64clen, temp64c,
                                             temp128len, temp128, temp192);
    xlen = scale_expansion_zeroelim(temp192len, temp192, bex, detx);
    xxlen = scale_expansion_zeroelim(xlen, detx, bex, detxx);
    xtlen = scale_expansion_zeroelim(temp192len, temp192, bextail, detxt);
    xxtlen = scale_expansion_zeroelim(xtlen, detxt, bex, detxxt);
    for (i = 0; i < xxtlen; i++) {
        detxxt[i] *= 2.0;
    }
    xtxtlen = scale_expansion_zeroelim(xtlen, detxt, bextail, detxtxt);
    x1len = fast_expansion_sum_zeroelim(xxlen, detxx, xxtlen, detxxt, x1);
    x2len = fast_expansion_sum_zeroelim(x1len, x1, xtxtlen, detxtxt, x2);
    ylen = scale_expansion_zeroelim(temp192len, temp192, bey, dety);
    yylen = scale_expansion_zeroelim(ylen, dety, bey, detyy);
    ytlen = scale_expansion_zeroelim(temp192len, temp192, beytail, detyt);
    yytlen = scale_expansion_zeroelim(ytlen, detyt, bey, detyyt);
    for (i = 0; i < yytlen; i++) {
        detyyt[i] *= 2.0;
    }
    ytytlen = scale_expansion_zeroelim(ytlen, detyt, beytail, detytyt);
    y1len = fast_expansion_sum_zeroelim(yylen, detyy, yytlen, detyyt, y1);
    y2len = fast_expansion_sum_zeroelim(y1len, y1, ytytlen, detytyt, y2);
    zlen = scale_expansion_zeroelim(temp192len, temp192, bez, detz);
    zzlen = scale_expansion_zeroelim(zlen, detz, bez, detzz);
    ztlen = scale_expansion_zeroelim(temp192len, temp192, beztail, detzt);
    zztlen = scale_expansion_zeroelim(ztlen, detzt, bez, detzzt);
    for (i = 0; i < zztlen; i++) {
        detzzt[i] *= 2.0;
    }
    ztztlen = scale_expansion_zeroelim(ztlen, detzt, beztail, detztzt);
    z1len = fast_expansion_sum_zeroelim(zzlen, detzz, zztlen, detzzt, z1);
    z2len = fast_expansion_sum_zeroelim(z1len, z1, ztztlen, detztzt, z2);
    xylen = fast_expansion_sum_zeroelim(x2len, x2, y2len, y2, detxy);
    blen = fast_expansion_sum_zeroelim(z2len, z2, xylen, detxy, bdet);

    temp32alen = scale_expansion_zeroelim(ablen, ab, -dez, temp32a);
    temp32blen = scale_expansion_zeroelim(ablen, ab, -deztail, temp32b);
    temp64alen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64a);
    temp32alen = scale_expansion_zeroelim(bdlen, bd, -aez, temp32a);
    temp32blen = scale_expansion_zeroelim(bdlen, bd, -aeztail, temp32b);
    temp64blen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64b);
    temp32alen = scale_expansion_zeroelim(dalen, da, -bez, temp32a);
    temp32blen = scale_expansion_zeroelim(dalen, da, -beztail, temp32b);
    temp64clen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64c);
    temp128len = fast_expansion_sum_zeroelim(temp64alen, temp64a,
                                             temp64blen, temp64b, temp128);
    temp192len = fast_expansion_sum_zeroelim(temp64clen, temp64c,
                                             temp128len, temp128, temp192);
    xlen = scale_expansion_zeroelim(temp192len, temp192, cex, detx);
    xxlen = scale_expansion_zeroelim(xlen, detx, cex, detxx);
    xtlen = scale_expansion_zeroelim(temp192len, temp192, cextail, detxt);
    xxtlen = scale_expansion_zeroelim(xtlen, detxt, cex, detxxt);
    for (i = 0; i < xxtlen; i++) {
        detxxt[i] *= 2.0;
    }
    xtxtlen = scale_expansion_zeroelim(xtlen, detxt, cextail, detxtxt);
    x1len = fast_expansion_sum_zeroelim(xxlen, detxx, xxtlen, detxxt, x1);
    x2len = fast_expansion_sum_zeroelim(x1len, x1, xtxtlen, detxtxt, x2);
    ylen = scale_expansion_zeroelim(temp192len, temp192, cey, dety);
    yylen = scale_expansion_zeroelim(ylen, dety, cey, detyy);
    ytlen = scale_expansion_zeroelim(temp192len, temp192, ceytail, detyt);
    yytlen = scale_expansion_zeroelim(ytlen, detyt, cey, detyyt);
    for (i = 0; i < yytlen; i++) {
        detyyt[i] *= 2.0;
    }
    ytytlen = scale_expansion_zeroelim(ytlen, detyt, ceytail, detytyt);
    y1len = fast_expansion_sum_zeroelim(yylen, detyy, yytlen, detyyt, y1);
    y2len = fast_expansion_sum_zeroelim(y1len, y1, ytytlen, detytyt, y2);
    zlen = scale_expansion_zeroelim(temp192len, temp192, cez, detz);
    zzlen = scale_expansion_zeroelim(zlen, detz, cez, detzz);
    ztlen = scale_expansion_zeroelim(temp192len, temp192, ceztail, detzt);
    zztlen = scale_expansion_zeroelim(ztlen, detzt, cez, detzzt);
    for (i = 0; i < zztlen; i++) {
        detzzt[i] *= 2.0;
    }
    ztztlen = scale_expansion_zeroelim(ztlen, detzt, ceztail, detztzt);
    z1len = fast_expansion_sum_zeroelim(zzlen, detzz, zztlen, detzzt, z1);
    z2len = fast_expansion_sum_zeroelim(z1len, z1, ztztlen, detztzt, z2);
    xylen = fast_expansion_sum_zeroelim(x2len, x2, y2len, y2, detxy);
    clen = fast_expansion_sum_zeroelim(z2len, z2, xylen, detxy, cdet);

    temp32alen = scale_expansion_zeroelim(bclen, bc, aez, temp32a);
    temp32blen = scale_expansion_zeroelim(bclen, bc, aeztail, temp32b);
    temp64alen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64a);
    temp32alen = scale_expansion_zeroelim(aclen, ac, -bez, temp32a);
    temp32blen = scale_expansion_zeroelim(aclen, ac, -beztail, temp32b);
    temp64blen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64b);
    temp32alen = scale_expansion_zeroelim(ablen, ab, cez, temp32a);
    temp32blen = scale_expansion_zeroelim(ablen, ab, ceztail, temp32b);
    temp64clen = fast_expansion_sum_zeroelim(temp32alen, temp32a,
                                             temp32blen, temp32b, temp64c);
    temp128len = fast_expansion_sum_zeroelim(temp64alen, temp64a,
                                             temp64blen, temp64b, temp128);
    temp192len = fast_expansion_sum_zeroelim(temp64clen, temp64c,
                                             temp128len, temp128, temp192);
    xlen = scale_expansion_zeroelim(temp192len, temp192, dex, detx);
    xxlen = scale_expansion_zeroelim(xlen, detx, dex, detxx);
    xtlen = scale_expansion_zeroelim(temp192len, temp192, dextail, detxt);
    xxtlen = scale_expansion_zeroelim(xtlen, detxt, dex, detxxt);
    for (i = 0; i < xxtlen; i++) {
        detxxt[i] *= 2.0;
    }
    xtxtlen = scale_expansion_zeroelim(xtlen, detxt, dextail, detxtxt);
    x1len = fast_expansion_sum_zeroelim(xxlen, detxx, xxtlen, detxxt, x1);
    x2len = fast_expansion_sum_zeroelim(x1len, x1, xtxtlen, detxtxt, x2);
    ylen = scale_expansion_zeroelim(temp192len, temp192, dey, dety);
    yylen = scale_expansion_zeroelim(ylen, dety, dey, detyy);
    ytlen = scale_expansion_zeroelim(temp192len, temp192, deytail, detyt);
    yytlen = scale_expansion_zeroelim(ytlen, detyt, dey, detyyt);
    for (i = 0; i < yytlen; i++) {
        detyyt[i] *= 2.0;
    }
    ytytlen = scale_expansion_zeroelim(ytlen, detyt, deytail, detytyt);
    y1len = fast_expansion_sum_zeroelim(yylen, detyy, yytlen, detyyt, y1);
    y2len = fast_expansion_sum_zeroelim(y1len, y1, ytytlen, detytyt, y2);
    zlen = scale_expansion_zeroelim(temp192len, temp192, dez, detz);
    zzlen = scale_expansion_zeroelim(zlen, detz, dez, detzz);
    ztlen = scale_expansion_zeroelim(temp192len, temp192, deztail, detzt);
    zztlen = scale_expansion_zeroelim(ztlen, detzt, dez, detzzt);
    for (i = 0; i < zztlen; i++) {
        detzzt[i] *= 2.0;
    }
    ztztlen = scale_expansion_zeroelim(ztlen, detzt, deztail, detztzt);
    z1len = fast_expansion_sum_zeroelim(zzlen, detzz, zztlen, detzzt, z1);
    z2len = fast_expansion_sum_zeroelim(z1len, z1, ztztlen, detztzt, z2);
    xylen = fast_expansion_sum_zeroelim(x2len, x2, y2len, y2, detxy);
    dlen = fast_expansion_sum_zeroelim(z2len, z2, xylen, detxy, ddet);

    ablen = fast_expansion_sum_zeroelim(alen, adet, blen, bdet, abdet);
    cdlen = fast_expansion_sum_zeroelim(clen, cdet, dlen, ddet, cddet);
    deterlen = fast_expansion_sum_zeroelim(ablen, abdet, cdlen, cddet, deter);

    return deter[deterlen - 1];
}

// TJH: old style function declaration
/*REAL insphereadapt(pa, pb, pc, pd, pe, permanent)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;
    REAL *pe;
    REAL permanent;*/
REAL insphereadapt(REAL *pa,REAL *pb,REAL *pc,REAL *pd,REAL *pe,REAL permanent)
{
    INEXACT REAL aex, bex, cex, dex, aey, bey, cey, dey, aez, bez, cez, dez;
    REAL det, errbound;

    INEXACT REAL aexbey1, bexaey1, bexcey1, cexbey1;
    INEXACT REAL cexdey1, dexcey1, dexaey1, aexdey1;
    INEXACT REAL aexcey1, cexaey1, bexdey1, dexbey1;
    REAL aexbey0, bexaey0, bexcey0, cexbey0;
    REAL cexdey0, dexcey0, dexaey0, aexdey0;
    REAL aexcey0, cexaey0, bexdey0, dexbey0;
    REAL ab[4], bc[4], cd[4], da[4], ac[4], bd[4];
    INEXACT REAL ab3, bc3, cd3, da3, ac3, bd3;
    REAL abeps, bceps, cdeps, daeps, aceps, bdeps;
    REAL temp8a[8], temp8b[8], temp8c[8], temp16[16], temp24[24], temp48[48];
    int temp8alen, temp8blen, temp8clen, temp16len, temp24len, temp48len;
    REAL xdet[96], ydet[96], zdet[96], xydet[192];
    int xlen, ylen, zlen, xylen;
    REAL adet[288], bdet[288], cdet[288], ddet[288];
    int alen, blen, clen, dlen;
    REAL abdet[576], cddet[576];
    int ablen, cdlen;
    REAL fin1[1152];
    int finlength;

    REAL aextail, bextail, cextail, dextail;
    REAL aeytail, beytail, ceytail, deytail;
    REAL aeztail, beztail, ceztail, deztail;

    INEXACT REAL bvirt;
    REAL avirt, bround, around;
    INEXACT REAL c;
    INEXACT REAL abig;
    REAL ahi, alo, bhi, blo;
    REAL err1, err2, err3;
    INEXACT REAL _i, _j;
    REAL _0;

    aex = (REAL) (pa[0] - pe[0]);
    bex = (REAL) (pb[0] - pe[0]);
    cex = (REAL) (pc[0] - pe[0]);
    dex = (REAL) (pd[0] - pe[0]);
    aey = (REAL) (pa[1] - pe[1]);
    bey = (REAL) (pb[1] - pe[1]);
    cey = (REAL) (pc[1] - pe[1]);
    dey = (REAL) (pd[1] - pe[1]);
    aez = (REAL) (pa[2] - pe[2]);
    bez = (REAL) (pb[2] - pe[2]);
    cez = (REAL) (pc[2] - pe[2]);
    dez = (REAL) (pd[2] - pe[2]);

    Two_Product(aex, bey, aexbey1, aexbey0);
    Two_Product(bex, aey, bexaey1, bexaey0);
    Two_Two_Diff(aexbey1, aexbey0, bexaey1, bexaey0, ab3, ab[2], ab[1], ab[0]);
    ab[3] = ab3;

    Two_Product(bex, cey, bexcey1, bexcey0);
    Two_Product(cex, bey, cexbey1, cexbey0);
    Two_Two_Diff(bexcey1, bexcey0, cexbey1, cexbey0, bc3, bc[2], bc[1], bc[0]);
    bc[3] = bc3;

    Two_Product(cex, dey, cexdey1, cexdey0);
    Two_Product(dex, cey, dexcey1, dexcey0);
    Two_Two_Diff(cexdey1, cexdey0, dexcey1, dexcey0, cd3, cd[2], cd[1], cd[0]);
    cd[3] = cd3;

    Two_Product(dex, aey, dexaey1, dexaey0);
    Two_Product(aex, dey, aexdey1, aexdey0);
    Two_Two_Diff(dexaey1, dexaey0, aexdey1, aexdey0, da3, da[2], da[1], da[0]);
    da[3] = da3;

    Two_Product(aex, cey, aexcey1, aexcey0);
    Two_Product(cex, aey, cexaey1, cexaey0);
    Two_Two_Diff(aexcey1, aexcey0, cexaey1, cexaey0, ac3, ac[2], ac[1], ac[0]);
    ac[3] = ac3;

    Two_Product(bex, dey, bexdey1, bexdey0);
    Two_Product(dex, bey, dexbey1, dexbey0);
    Two_Two_Diff(bexdey1, bexdey0, dexbey1, dexbey0, bd3, bd[2], bd[1], bd[0]);
    bd[3] = bd3;

    temp8alen = scale_expansion_zeroelim(4, cd, bez, temp8a);
    temp8blen = scale_expansion_zeroelim(4, bd, -cez, temp8b);
    temp8clen = scale_expansion_zeroelim(4, bc, dez, temp8c);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a,
                                            temp8blen, temp8b, temp16);
    temp24len = fast_expansion_sum_zeroelim(temp8clen, temp8c,
                                            temp16len, temp16, temp24);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, aex, temp48);
    xlen = scale_expansion_zeroelim(temp48len, temp48, -aex, xdet);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, aey, temp48);
    ylen = scale_expansion_zeroelim(temp48len, temp48, -aey, ydet);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, aez, temp48);
    zlen = scale_expansion_zeroelim(temp48len, temp48, -aez, zdet);
    xylen = fast_expansion_sum_zeroelim(xlen, xdet, ylen, ydet, xydet);
    alen = fast_expansion_sum_zeroelim(xylen, xydet, zlen, zdet, adet);

    temp8alen = scale_expansion_zeroelim(4, da, cez, temp8a);
    temp8blen = scale_expansion_zeroelim(4, ac, dez, temp8b);
    temp8clen = scale_expansion_zeroelim(4, cd, aez, temp8c);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a,
                                            temp8blen, temp8b, temp16);
    temp24len = fast_expansion_sum_zeroelim(temp8clen, temp8c,
                                            temp16len, temp16, temp24);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, bex, temp48);
    xlen = scale_expansion_zeroelim(temp48len, temp48, bex, xdet);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, bey, temp48);
    ylen = scale_expansion_zeroelim(temp48len, temp48, bey, ydet);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, bez, temp48);
    zlen = scale_expansion_zeroelim(temp48len, temp48, bez, zdet);
    xylen = fast_expansion_sum_zeroelim(xlen, xdet, ylen, ydet, xydet);
    blen = fast_expansion_sum_zeroelim(xylen, xydet, zlen, zdet, bdet);

    temp8alen = scale_expansion_zeroelim(4, ab, dez, temp8a);
    temp8blen = scale_expansion_zeroelim(4, bd, aez, temp8b);
    temp8clen = scale_expansion_zeroelim(4, da, bez, temp8c);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a,
                                            temp8blen, temp8b, temp16);
    temp24len = fast_expansion_sum_zeroelim(temp8clen, temp8c,
                                            temp16len, temp16, temp24);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, cex, temp48);
    xlen = scale_expansion_zeroelim(temp48len, temp48, -cex, xdet);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, cey, temp48);
    ylen = scale_expansion_zeroelim(temp48len, temp48, -cey, ydet);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, cez, temp48);
    zlen = scale_expansion_zeroelim(temp48len, temp48, -cez, zdet);
    xylen = fast_expansion_sum_zeroelim(xlen, xdet, ylen, ydet, xydet);
    clen = fast_expansion_sum_zeroelim(xylen, xydet, zlen, zdet, cdet);

    temp8alen = scale_expansion_zeroelim(4, bc, aez, temp8a);
    temp8blen = scale_expansion_zeroelim(4, ac, -bez, temp8b);
    temp8clen = scale_expansion_zeroelim(4, ab, cez, temp8c);
    temp16len = fast_expansion_sum_zeroelim(temp8alen, temp8a,
                                            temp8blen, temp8b, temp16);
    temp24len = fast_expansion_sum_zeroelim(temp8clen, temp8c,
                                            temp16len, temp16, temp24);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, dex, temp48);
    xlen = scale_expansion_zeroelim(temp48len, temp48, dex, xdet);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, dey, temp48);
    ylen = scale_expansion_zeroelim(temp48len, temp48, dey, ydet);
    temp48len = scale_expansion_zeroelim(temp24len, temp24, dez, temp48);
    zlen = scale_expansion_zeroelim(temp48len, temp48, dez, zdet);
    xylen = fast_expansion_sum_zeroelim(xlen, xdet, ylen, ydet, xydet);
    dlen = fast_expansion_sum_zeroelim(xylen, xydet, zlen, zdet, ddet);

    ablen = fast_expansion_sum_zeroelim(alen, adet, blen, bdet, abdet);
    cdlen = fast_expansion_sum_zeroelim(clen, cdet, dlen, ddet, cddet);
    finlength = fast_expansion_sum_zeroelim(ablen, abdet, cdlen, cddet, fin1);

    det = estimate(finlength, fin1);
    errbound = isperrboundB * permanent;
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }

    Two_Diff_Tail(pa[0], pe[0], aex, aextail);
    Two_Diff_Tail(pa[1], pe[1], aey, aeytail);
    Two_Diff_Tail(pa[2], pe[2], aez, aeztail);
    Two_Diff_Tail(pb[0], pe[0], bex, bextail);
    Two_Diff_Tail(pb[1], pe[1], bey, beytail);
    Two_Diff_Tail(pb[2], pe[2], bez, beztail);
    Two_Diff_Tail(pc[0], pe[0], cex, cextail);
    Two_Diff_Tail(pc[1], pe[1], cey, ceytail);
    Two_Diff_Tail(pc[2], pe[2], cez, ceztail);
    Two_Diff_Tail(pd[0], pe[0], dex, dextail);
    Two_Diff_Tail(pd[1], pe[1], dey, deytail);
    Two_Diff_Tail(pd[2], pe[2], dez, deztail);
    if ((aextail == 0.0) && (aeytail == 0.0) && (aeztail == 0.0)
        && (bextail == 0.0) && (beytail == 0.0) && (beztail == 0.0)
        && (cextail == 0.0) && (ceytail == 0.0) && (ceztail == 0.0)
        && (dextail == 0.0) && (deytail == 0.0) && (deztail == 0.0)) {
        return det;
    }

    errbound = isperrboundC * permanent + resulterrbound * Absolute(det);
    abeps = (aex * beytail + bey * aextail)
        - (aey * bextail + bex * aeytail);
    bceps = (bex * ceytail + cey * bextail)
        - (bey * cextail + cex * beytail);
    cdeps = (cex * deytail + dey * cextail)
        - (cey * dextail + dex * ceytail);
    daeps = (dex * aeytail + aey * dextail)
        - (dey * aextail + aex * deytail);
    aceps = (aex * ceytail + cey * aextail)
        - (aey * cextail + cex * aeytail);
    bdeps = (bex * deytail + dey * bextail)
        - (bey * dextail + dex * beytail);
    det += (((bex * bex + bey * bey + bez * bez)
             * ((cez * daeps + dez * aceps + aez * cdeps)
                + (ceztail * da3 + deztail * ac3 + aeztail * cd3))
             + (dex * dex + dey * dey + dez * dez)
             * ((aez * bceps - bez * aceps + cez * abeps)
                + (aeztail * bc3 - beztail * ac3 + ceztail * ab3)))
            - ((aex * aex + aey * aey + aez * aez)
               * ((bez * cdeps - cez * bdeps + dez * bceps)
                  + (beztail * cd3 - ceztail * bd3 + deztail * bc3))
               + (cex * cex + cey * cey + cez * cez)
               * ((dez * abeps + aez * bdeps + bez * daeps)
                  + (deztail * ab3 + aeztail * bd3 + beztail * da3))))
        + 2.0 * (((bex * bextail + bey * beytail + bez * beztail)
                  * (cez * da3 + dez * ac3 + aez * cd3)
                  + (dex * dextail + dey * deytail + dez * deztail)
                  * (aez * bc3 - bez * ac3 + cez * ab3))
                 - ((aex * aextail + aey * aeytail + aez * aeztail)
                    * (bez * cd3 - cez * bd3 + dez * bc3)
                    + (cex * cextail + cey * ceytail + cez * ceztail)
                    * (dez * ab3 + aez * bd3 + bez * da3)));
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }

    return insphereexact(pa, pb, pc, pd, pe);
}

// TJH: old style function declaration
/*REAL insphere(pa, pb, pc, pd, pe)
    REAL *pa;
    REAL *pb;
    REAL *pc;
    REAL *pd;
    REAL *pe;*/
REAL insphere(REAL *pa,REAL *pb,REAL *pc,REAL *pd,REAL *pe)
{
    REAL aex, bex, cex, dex;
    REAL aey, bey, cey, dey;
    REAL aez, bez, cez, dez;
    REAL aexbey, bexaey, bexcey, cexbey, cexdey, dexcey, dexaey, aexdey;
    REAL aexcey, cexaey, bexdey, dexbey;
    REAL alift, blift, clift, dlift;
    REAL ab, bc, cd, da, ac, bd;
    REAL abc, bcd, cda, dab;
    REAL aezplus, bezplus, cezplus, dezplus;
    REAL aexbeyplus, bexaeyplus, bexceyplus, cexbeyplus;
    REAL cexdeyplus, dexceyplus, dexaeyplus, aexdeyplus;
    REAL aexceyplus, cexaeyplus, bexdeyplus, dexbeyplus;
    REAL det;
    REAL permanent, errbound;

    aex = pa[0] - pe[0];
    bex = pb[0] - pe[0];
    cex = pc[0] - pe[0];
    dex = pd[0] - pe[0];
    aey = pa[1] - pe[1];
    bey = pb[1] - pe[1];
    cey = pc[1] - pe[1];
    dey = pd[1] - pe[1];
    aez = pa[2] - pe[2];
    bez = pb[2] - pe[2];
    cez = pc[2] - pe[2];
    dez = pd[2] - pe[2];

    aexbey = aex * bey;
    bexaey = bex * aey;
    ab = aexbey - bexaey;
    bexcey = bex * cey;
    cexbey = cex * bey;
    bc = bexcey - cexbey;
    cexdey = cex * dey;
    dexcey = dex * cey;
    cd = cexdey - dexcey;
    dexaey = dex * aey;
    aexdey = aex * dey;
    da = dexaey - aexdey;

    aexcey = aex * cey;
    cexaey = cex * aey;
    ac = aexcey - cexaey;
    bexdey = bex * dey;
    dexbey = dex * bey;
    bd = bexdey - dexbey;

    abc = aez * bc - bez * ac + cez * ab;
    bcd = bez * cd - cez * bd + dez * bc;
    cda = cez * da + dez * ac + aez * cd;
    dab = dez * ab + aez * bd + bez * da;

    alift = aex * aex + aey * aey + aez * aez;
    blift = bex * bex + bey * bey + bez * bez;
    clift = cex * cex + cey * cey + cez * cez;
    dlift = dex * dex + dey * dey + dez * dez;

    det = (dlift * abc - clift * dab) + (blift * cda - alift * bcd);

    aezplus = Absolute(aez);
    bezplus = Absolute(bez);
    cezplus = Absolute(cez);
    dezplus = Absolute(dez);
    aexbeyplus = Absolute(aexbey);
    bexaeyplus = Absolute(bexaey);
    bexceyplus = Absolute(bexcey);
    cexbeyplus = Absolute(cexbey);
    cexdeyplus = Absolute(cexdey);
    dexceyplus = Absolute(dexcey);
    dexaeyplus = Absolute(dexaey);
    aexdeyplus = Absolute(aexdey);
    aexceyplus = Absolute(aexcey);
    cexaeyplus = Absolute(cexaey);
    bexdeyplus = Absolute(bexdey);
    dexbeyplus = Absolute(dexbey);
    permanent = ((cexdeyplus + dexceyplus) * bezplus
                 + (dexbeyplus + bexdeyplus) * cezplus
                 + (bexceyplus + cexbeyplus) * dezplus)
        * alift
        + ((dexaeyplus + aexdeyplus) * cezplus
           + (aexceyplus + cexaeyplus) * dezplus
           + (cexdeyplus + dexceyplus) * aezplus)
        * blift
        + ((aexbeyplus + bexaeyplus) * dezplus
           + (bexdeyplus + dexbeyplus) * aezplus
           + (dexaeyplus + aexdeyplus) * bezplus)
        * clift
        + ((bexceyplus + cexbeyplus) * aezplus
           + (cexaeyplus + aexceyplus) * bezplus
           + (aexbeyplus + bexaeyplus) * cezplus)
        * dlift;
    errbound = isperrboundA * permanent;
    if ((det > errbound) || (-det > errbound)) {
        return det;
    }

    return insphereadapt(pa, pb, pc, pd, pe, permanent);
}
//========rand.c=============================================================
/*
 * Ken Clarkson wrote this.  Copyright (c) 1995 by AT&T..
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#include <math.h>
#include <stdio.h>
#include <time.h>

//#include "hull.h"   TJH: this file is now above

double erand48 (unsigned short X[3]);

unsigned short X[3];

double double_rand(void) {return erand48(X);}

void init_rand(long seed) {
  X[1]=(seed==0) ? time(0) : seed;
    // TJH: added this if statement
    if(DFILE)
        fprintf(DFILE,"init_rand: seed = %d\n",X[1]);
}

/* commented out by TJH

#ifdef cray
double logb(double x) {
  if (x<=0) return -1e2460;
  return log(x)/log(2);
}
#endif*/

double logb(double x)
{
  return log(x)/log(2);
}
//========nrand48.c=============================================================
/*
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 */

// #include "rand48.h"  TJH: this file is now above

long
nrand48(unsigned short xseed[3])
{
  _dorand48(xseed);
  return ((long) xseed[2] << 15) + ((long) xseed[1] >> 1);
}
//========drand48.c=============================================================
/*
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 */
 
/*
 * modified to compile in MetroWerks C++, D. Eppstein, April 1997
 * (my changes marked with "DE")
 */

//#include "rand48.h"   TJH: this file is now above

/* declare erand48 - DE */
extern double erand48(unsigned short xseed[3]);

extern unsigned short _rand48_seed[3];

double
drand48(void)
{
  return erand48(_rand48_seed);
}
//========erand48.c=============================================================
/*
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 */

// #include "rand48.h"  TJH: this file is now above

double
erand48(unsigned short xseed[3])
{
  _dorand48(xseed);
  return ldexp((double) xseed[0], -48) +
         ldexp((double) xseed[1], -32) +
         ldexp((double) xseed[2], -16);
}
//========_rand48.c=============================================================
/*
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 */

//#include "rand48.h"   TJH: this file is now above

unsigned short _rand48_seed[3] = {
  RAND48_SEED_0,
  RAND48_SEED_1,
  RAND48_SEED_2
};
unsigned short _rand48_mult[3] = {
  RAND48_MULT_0,
  RAND48_MULT_1,
  RAND48_MULT_2
};
unsigned short _rand48_add = RAND48_ADD;

void
_dorand48(unsigned short xseed[3])
{
  unsigned long accu;
  unsigned short temp[2];

  accu = (unsigned long) _rand48_mult[0] * (unsigned long) xseed[0] +
   (unsigned long) _rand48_add;
  temp[0] = (unsigned short) accu;  /* lower 16 bits */
  accu >>= sizeof(unsigned short) * 8;
  accu += (unsigned long) _rand48_mult[0] * (unsigned long) xseed[1] +
   (unsigned long) _rand48_mult[1] * (unsigned long) xseed[0];
  temp[1] = (unsigned short) accu;  /* middle 16 bits */
  accu >>= sizeof(unsigned short) * 8;
  accu += _rand48_mult[0] * xseed[2] + _rand48_mult[1] * xseed[1] + _rand48_mult[2] * xseed[0];
  xseed[0] = temp[0];
  xseed[1] = temp[1];
  xseed[2] = (unsigned short) accu;
}
//========seed48.c=============================================================
/*
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 */

//#include "rand48.h"   TJH: this file is now above

extern unsigned short _rand48_seed[3];
extern unsigned short _rand48_mult[3];
extern unsigned short _rand48_add;

unsigned short *
seed48(unsigned short xseed[3])
{
  static unsigned short sseed[3];

  sseed[0] = _rand48_seed[0];
  sseed[1] = _rand48_seed[1];
  sseed[2] = _rand48_seed[2];
  _rand48_seed[0] = xseed[0];
  _rand48_seed[1] = xseed[1];
  _rand48_seed[2] = xseed[2];
  _rand48_mult[0] = RAND48_MULT_0;
  _rand48_mult[1] = RAND48_MULT_1;
  _rand48_mult[2] = RAND48_MULT_2;
  _rand48_add = RAND48_ADD;
  return sseed;
}
//========srand48.c=============================================================
/*
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 */

// #include "rand48.h"   TJH: this file is now above

extern unsigned short _rand48_seed[3];
extern unsigned short _rand48_mult[3];
extern unsigned short _rand48_add;

void
srand48(long seed)
{
  _rand48_seed[0] = RAND48_SEED_0;
  _rand48_seed[1] = (unsigned short) seed;
  _rand48_seed[2] = (unsigned short) (seed >> 16);
  _rand48_mult[0] = RAND48_MULT_0;
  _rand48_mult[1] = RAND48_MULT_1;
  _rand48_mult[2] = RAND48_MULT_2;
  _rand48_add = RAND48_ADD;
}
//=====================================================================

void vtkPowerCrustSurfaceReconstruction::Execute()
{
  vtkDataSet *input= this->GetInput();
  vtkIdType numPts=input->GetNumberOfPoints();

  vtkPolyData *output = this->GetOutput();

  // ghost cell stuff
  unsigned char  updateLevel = (unsigned char)(output->GetUpdateGhostLevel());
  unsigned char  *cellGhostLevels = NULL;


  // make sure output is initialized
  // create some points for the output
  {
  vtkPoints *points = vtkPoints::New();
  output->SetPoints(points);
  points->Delete();
  }

  {
  vtkCellArray *polys = vtkCellArray::New();
  output->SetPolys(polys);
  polys->Delete();
  }

  {
  vtkPoints *points = vtkPoints::New();
  this->medial_surface->SetPoints(points);
  points->Delete();
  }

  {
  vtkCellArray *polys = vtkCellArray::New();
  this->medial_surface->SetPolys(polys);
  polys->Delete();
  }

  {
  vtkFloatArray *pole_weights = vtkFloatArray::New();
  pole_weights->SetNumberOfComponents(1);
  this->medial_surface->GetPointData()->SetScalars(pole_weights);
  pole_weights->Delete();
  }

  vtk_input = input;
  vtk_output = output;
  vtk_medial_surface = this->medial_surface;
  our_filter=this;

  // this function is in hullmain.c
  adapted_main();

  this->medial_surface->Modified();

}

void vtkPowerCrustSurfaceReconstruction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

void vtkPowerCrustSurfaceReconstruction::ComputeInputUpdateExtents(vtkDataObject *output)
{
  int piece, numPieces, ghostLevels;
  
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("No Input");
    return;
    }
  piece = output->GetUpdatePiece();
  numPieces = output->GetUpdateNumberOfPieces();
  ghostLevels = output->GetUpdateGhostLevel();
  
  if (numPieces > 1)
    {
    ++ghostLevels;
    }

  this->GetInput()->SetUpdateExtent(piece, numPieces, ghostLevels);

  this->GetInput()->RequestExactExtentOn();
}

void vtkPowerCrustSurfaceReconstruction::ExecuteInformation()
{
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("No Input");
    return;
    }
}


/*=================TimsFun.txt===============================================================

Today I will try to compile the PowerCrust code on my Win2K box.
One day I dream of PowerCrust being a self-contained VTK class, how happy we'd be.

Phase One - Compiling PowerCrust on a Windows box
-------------------------------------------------

Downloaded the source from Nina: powercrust.tar.gz
from the PowerCrust homepage: http://www.cs.utexas.edu/users/amenta/powercrust/welcome.html
Looked in the Makefile for the files needed to compile the main powercrust application.
Built a VC++7 project containing:

crust.c
fg.c
heap.c
hull.c
hullmain.c
io.c
label.c
math.c
pointops.c
power.c
predicates.c
rand.c
ch.c

plus:

stormacs.h
hull.h
points.h
pointsites.h

Hit F7 to build the application. Ah, the naivete!
Of couse there was an endless stream of error messages.

\powercrust\predicates.c(119) : fatal error C1083: Cannot open include file: 'sys/time.h': 
No such file or directory

So I commented out the #include line of predicates.c, and the error went away. (if only they 
were all this easy)

\powercrust\hullmain.c(44) : fatal error C1083: Cannot open include file: 'getopt.h': No such 
file or directory

So I found the files getopt.h and getopt.c somewhere (c:\texmf\doc\graphics\texdraw - part of 
my LaTeX distrib!) and added 

them to the project. Man this is crazy. Guess Unix people get these files with the system.

Lots of errors to do with old-style C function declarations - we can set a flag in the compiler 
to cope with this. In VC++7 it was a matter of removing the /clr on the property pages : 
Configuration Properties : C/C++ : General : Compile As Managed = Not using managed extensions.

It compiles! Glory be.

Lots of link errors though (did you think it would be that easy?). Some saying random() was 
not defined. Hmmm... Surely rand() does what we want. Likewise for srandom().

Created a new file tim_defs.h with the lines:

#include <stdio.h>
int random() { return rand(); }
void srandom(int s) { srand(s); }

And #included it in hullmain.c. Seemed to do the trick.

Next link error:

io.obj : error LNK2019: unresolved external symbol _popen referenced in function _epopen

Hmmm.. what is popen? A net search reveals that some people think it is part of stdio.h - but 
stdio.h is #included in io.c, so something is wrong. Must be another Unix v. Windows issue.

Further search revealed this page:

http://developer.gnome.org/doc/API/glib/glib-windows-compatability-functions.html

Which is all very interesting. I try to copy the port for popen and pclose by adding the lines:

#define popen _popen
#define pclose _pclose

to the file tim_defs.h. This seems to work. If only I knew what I was doing.

Next link error:

rand.obj : error LNK2019: unresolved external symbol _erand48 referenced in function _double_rand

Another function in the Unix libraries it seems, stdlib.h this time. Hmmm...

Eventually found this page:

http://www.ics.uci.edu/~eppstein/projects/pairs/Source/testbed/rand48/

(David Eppstein is one of the guys who worked with Nina, but google didn't know that...)

So I borrowed the files:

_rand48.c
drand48.c
erand48.c
nrand48.c
rand48.3
rand48.h
seed48.h
seed48.c
srand48.c

and added them to the project... and do you know it actually worked! 

Final link error said logb wasn't defined. Looked in rand.c, the code there looked funny, 
so I did this instead:

// commented out by TJH (how many people do you know that work on a cray?)

//#ifdef cray
//double logb(double x) {
//  if (x<=0) return -1e2460;
//  return log(x)/log(2);
//}
//#endif

double logb(double x)
{
  return log(x)/log(2);
}

A fool marches on where wish men fear to tread.

And.. and.. and.. it's linked. powercrust.exe has been produced. You can run it and everything. 
It speaks:

reading from stdin
main output to stdout

If you say (like in the README)

powercrust -m 10000000 -i hotdogs.pts

then it starts doing its stuff, churning away producing data. Some of it goes wrong for various 
reasons (like 'cat' not being a command in windows...)

It produces a whole series of files:

axis
axisface
pole
sp
pc
pnf
axis.off
axisface.off
head
inpball
inpole
outpole
pc.off
poleinfo
tpoleinfo
di
ma
re
rere

<sigh>

Some of these are described in the README - eg. pc.off will contain the final surface. Sadly 
all the .off files are of zero size, because the program didn't run to completion.

In hullmain.c, in the main() function there are the system calls to 'cat'. If we change these 
to 'type' then the DOS command works instead. Likewise we can change 'rm' to 'del'.

Now the program works, only the files mentioned in the README are left over. We can load the 
file pc.off into our favourite viewer and view the surface created. (Should there be a vtkOFFReader?)

So, now all we need to do is modify its file processing to use VTK data structures and we're 
done. vtkPowerCrust here we come!


Phase Two - Porting PowerCrust to VTK
-------------------------------------

We'd like to be able to release PowerCrust so all the VTK users can use it. This means compiling 
it as a class to be released with VTK. This means no system-dependent calls (like 'type' and 'del') 
and ideally no intermediate files saved to disk. Also, ideally all the code would be in one file 
rather than spread over twenty. Too much to ask? We will see.

The first and most obvious thing is to produce a wrapper, a skeleton vtkDataSetToPolyDataFilter 
derivative that calls the powercrust code as is and uses the output. Then gradually we squeeze 
everything into the wrapper.

I used #include "hullmain.c" and lots of similar lines to include the .c files in the file I was
creating, vtkPowerCrustSurfaceReconstruction.cxx. Yes, I know this looks awful but the final goal 
was to, yes, spooge everything into one file for VTK neatness. Got lots of errors, names colliding, 
old style function declarations being choked on, etc.

Working through the errors one by one. Lots of little changes all over, I'm afraid - all marked 
with 'TJH'. Had to be done (or so it seems to me at the moment). The most tedious bit so far was 
replacing the function declarations in predicates.c... The good news is that it compiles and links 
again. 

So, all we've got to do now is make it work in VTK, hook the functionality up so that the input 
vtkPointSet is passed as a set of points to the PowerCrust code, and the output surface is returned 
as a vtkPolyData. At the moment the powercrust code uses lots of temporary files for storage, this 
is really strange to my eyes, maybe it saves on memory but surely at the expense of a lot of speed.

Commented out the main() function in hullmain.c, moved it over to vtkPowerCrustSurfaceReconstruction.cxx 
so we can do evil things with it and bend it to our own ends. Basically our first-pass approach is 
to replace file reading with taking data from a vtkDataSet, leaving everything else as is. Managed 
to lose getopt.c and getopt.h which is nice because they came from elsewhere, did this by commenting 
out the big switch statement that parsed the command-line options.

Made an adapted version of read_next_site() called vtk_read_next_site() that takes data from our 
input vtkDataSet instead of from INFILE. We still need the old version because the file 'sp' is read 
in again (as INFILE) after being written to (as SPFILE). (sigh)

Man the code is hairy, loads of globals and stuff. I mean, lots of respect to all the authors and 
that for a) the algorithms they came up with and b) making their code available but really, 
this is like code from the eighties. (maybe it is...)

Hopefully soon we should be able to use the filter to take a bunch of points and produce its pc.off 
output - which we can check. If this works then we can go ahead and pipe the output not to file but 
to a vtkPolyData. 

Well, it kind of worked, got some output. Some warnings about unlabelled poles which are probably 
because the cactus data has a hole in it. Will try hooking up the output directly. Ok, done that. 
Had to read in the final file instead of taking the data from the point at which it was produced, 
which is odd. But it works now, can use powercrust in a tcl script. 

Sometimes hangs. Memory leaks also. Oh, and have to keep deleting the intermediate output files 
else get all sorts of problems - this file stuff is a real mess and the biggest problem with the 
powercrust code.

Solved the hanging - there was a conversion from char* to double I'd managed to overlook...
(indface[i])

OK, managed to get rid of all of the file handling. In the end it wasn't too hard - the temporary 
file was never actually being read in for example, since the function off_out doesn't get used. 
The file 'sp' contained the poles and their weights, which we now store as vtk_medial_surface and 
its scalars.

Got the code all into one file for neatness in VTK. It looks awful now but I suspect there will not
be a lot of people coming in to clear things up, a reimplementation would probably be quicker.

One twist to the tale, to get good-looking surfaces with normals we need to run 'orient', that
came with the powercrust distrib as setNormals.C and ndefs.h. We will have to now include these in
code above somehow. Oh, and running 'simplify' results in a *much* simpler surface, so we should do
this too, this is more important. BTW, compiling these in windows isn't too hard, you need to start 
from a 'Win32 console application' in MSVC and deal with the errors that come up.

On the command line, the whole run might consist of:

powercrust -m 1000000 -i hotdogs.pts
simplify -i poleinfo -o simp_poles -n 1.0 -r 0.3
powercrust -p -i simp_poles

(with the final output in pc.off)

But we'd just commented out the code that dealt with the -p option, and outputting poleinfo, damn.


To-do list:
- get rid of the code we don't use (expecting a reasonable amount)
- work out some scheme for getting rid of mult_up, based on precision of input points.
- include simplify and orient to make good-looking output

=============================================================================================*/
