/* ----------------------------------------------------------------------
   MR-MPI = MapReduce-MPI library
   http://www.cs.sandia.gov/~sjplimp/mapreduce.html
   Steve Plimpton, sjplimp@sandia.gov, Sandia National Laboratories

   Copyright (2009) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the modified Berkeley Software Distribution (BSD) License.

   See the README file in the top-level MapReduce directory.
------------------------------------------------------------------------- */

/*
MapReduce random RMAT matrix generation example in C++
Syntax: rmat N Nz a b c d frac seed {outfile}
  2^N = # of rows in RMAT matrix
  Nz = non-zeroes per row
  a,b,c,d = RMAT params (must sum to 1.0)
  frac = RMAT randomization param (frac < 1, 0 = no randomization)
  seed = RNG seed (positive int)
  outfile = output RMAT matrix to this filename (optional)
*/

#include "mpi.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "cmapreduce.h"

void generate(int, void *, void *);
void cull(char *, int, char *, int, int *, void *, void *);
void output(char *, int, char *, int, int *, void *, void *);
void nonzero(char *, int, char *, int, int *, void *, void *);
void degree(char *, int, char *, int, int *, void *, void *);
void histo(char *, int, char *, int, int *, void *, void *);
int ncompare(char *, int, char *, int);
void stats(int, char *, int, char *, int, void *, void *);

typedef struct {            // RMAT params
  int nlevels,order;
  int nnonzero;
  int ngenerate;
  double a,b,c,d,fraction;
  char *outfile;
  FILE *fp;
} RMAT;

typedef int VERTEX;      // vertex ID

typedef struct {         // edge = 2 vertices
  VERTEX vi,vj;
} EDGE;

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  int me,nprocs;

  MPI_Init(&narg,&args);
  MPI_Comm_rank(MPI_COMM_WORLD,&me);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);

  // parse command-line args

  if (narg != 9 && narg != 10) {
    if (me == 0) printf("Syntax: rmat N Nz a b c d frac seed {outfile}\n");
    MPI_Abort(MPI_COMM_WORLD,1);
  }

  RMAT rmat;
  rmat.nlevels = atoi(args[1]); 
  rmat.nnonzero = atoi(args[2]); 
  rmat.a = atof(args[3]); 
  rmat.b = atof(args[4]); 
  rmat.c = atof(args[5]); 
  rmat.d = atof(args[6]); 
  rmat.fraction = atof(args[7]); 
  int seed = atoi(args[8]);
  if (narg == 10) {
    int n = strlen(args[9]) + 1;
    rmat.outfile = (char *) malloc(n*sizeof(char));
    strcpy(rmat.outfile,args[9]);
  } else rmat.outfile = NULL;

  if (rmat.a + rmat.b + rmat.c + rmat.d != 1.0) {
    if (me == 0) printf("ERROR: a,b,c,d must sum to 1\n");
    MPI_Abort(MPI_COMM_WORLD,1);
  }

  if (rmat.fraction >= 1.0) {
    if (me == 0) printf("ERROR: fraction must be < 1\n");
    MPI_Abort(MPI_COMM_WORLD,1);
  }

  srand48(seed+me);
  rmat.order = 1 << rmat.nlevels;

  void *mr = MR_create(MPI_COMM_WORLD);

  // loop until desired number of unique nonzero entries

  MPI_Barrier(MPI_COMM_WORLD);
  double tstart = MPI_Wtime();

  int niterate = 0;
  int ntotal = (1 << rmat.nlevels) * rmat.nnonzero;
  int nremain = ntotal;
  while (nremain) {
    niterate++;
    rmat.ngenerate = nremain/nprocs;
    if (me < nremain % nprocs) rmat.ngenerate++;
    MR_map_add(mr,nprocs,&generate,&rmat,1);
    int nunique = MR_collate(mr,NULL);
    if (nunique == ntotal) break;
    MR_reduce(mr,&cull,&rmat);
    nremain = ntotal - nunique;
  }

  MPI_Barrier(MPI_COMM_WORLD);
  double tstop = MPI_Wtime();

  // output matrix if requested

  if (rmat.outfile) {
    char fname[128];
    sprintf(fname,"%s.%d",rmat.outfile,me);
    rmat.fp = fopen(fname,"w");
    if (rmat.fp == NULL) {
      printf("ERROR: Could not open output file\n");
      MPI_Abort(MPI_COMM_WORLD,1);
    }
    void *mr2 = MR_copy(mr);
    MR_reduce(mr2,&output,&rmat);
    fclose(rmat.fp);
    MR_destroy(mr2);
  }

  // stats to screen
  // include stats on number of nonzeroes per row

  if (me == 0) {
    printf("%d rows in matrix\n",rmat.order);
    printf("%d nonzeroes in matrix\n",ntotal);
  }

  MR_reduce(mr,&nonzero,NULL);
  MR_collate(mr,NULL);
  MR_reduce(mr,&degree,NULL);
  MR_collate(mr,NULL);
  MR_reduce(mr,&histo,NULL);
  MR_gather(mr,1);
  MR_sort_keys(mr,&ncompare);
  int total = 0;
  MR_map_kv(mr,mr,&stats,&total);
  if (me == 0) printf("%d rows with 0 nonzeroes\n",rmat.order-total);

  if (me == 0)
    printf("%g secs to generate matrix on %d procs in %d iterations\n",
     tstop-tstart,nprocs,niterate);

  // clean up

  MR_destroy(mr);
  free(rmat.outfile);
  MPI_Finalize();
}

/* ----------------------------------------------------------------------
   generate RMAT matrix entries
   emit one KV per edge: key = edge, value = NULL
------------------------------------------------------------------------- */

void generate(int itask, void *kv, void *ptr)
{
  RMAT *rmat = (RMAT *) ptr;

  int nlevels = rmat->nlevels;
  int order = rmat->order;
  int ngenerate = rmat->ngenerate;
  double a = rmat->a;
  double b = rmat->b;
  double c = rmat->c;
  double d = rmat->d;
  double fraction = rmat->fraction;

  int i,j,ilevel,delta,m;
  double a1,b1,c1,d1,total,rn;
  EDGE edge;

  for (m = 0; m < ngenerate; m++) {
    delta = order >> 1;
    a1 = a; b1 = b; c1 = c; d1 = d;
    i = j = 0;
    
    for (ilevel = 0; ilevel < nlevels; ilevel++) {
      rn = drand48();
      if (rn < a1) {
      } else if (rn < a1+b1) {
  j += delta;
      } else if (rn < a1+b1+c1) {
  i += delta;
      } else {
  i += delta;
  j += delta;
      }
      
      delta /= 2;
      if (fraction > 0.0) {
  a1 += a1*fraction * (drand48() - 0.5);
  b1 += b1*fraction * (drand48() - 0.5);
  c1 += c1*fraction * (drand48() - 0.5);
  d1 += d1*fraction * (drand48() - 0.5);
  total = a1+b1+c1+d1;
  a1 /= total;
  b1 /= total;
  c1 /= total;
  d1 /= total;
      }
    }

    edge.vi = i;
    edge.vj = j;
    MR_kv_add(kv,(char *) &edge,sizeof(EDGE),NULL,0);
  }
}

/* ----------------------------------------------------------------------
   eliminate duplicate edges
   input: one KMV per edge, MV has multiple entries if duplicates exist
   output: one KV per edge: key = edge, value = NULL
------------------------------------------------------------------------- */

void cull(char *key, int keybytes, char *multivalue,
    int nvalues, int *valuebytes, void *kv, void *ptr) 
{
  MR_kv_add(kv,key,keybytes,NULL,0);
}

/* ----------------------------------------------------------------------
   write edges to a file unique to this processor
------------------------------------------------------------------------- */

void output(char *key, int keybytes, char *multivalue,
      int nvalues, int *valuebytes, void *kv, void *ptr) 
{
  RMAT *rmat = (RMAT *) ptr;
  EDGE *edge = (EDGE *) key;
  fprintf(rmat->fp,"%d %d 1\n",edge->vi+1,edge->vj+1);
}

/* ----------------------------------------------------------------------
   enumerate nonzeroes in each row
   input: one KMV per edge
   output: one KV per edge: key = row I, value = NULL
------------------------------------------------------------------------- */

void nonzero(char *key, int keybytes, char *multivalue,
       int nvalues, int *valuebytes, void *kv, void *ptr) 
{
  EDGE *edge = (EDGE *) key;
  MR_kv_add(kv,(char *) &edge->vi,sizeof(VERTEX),NULL,0);
}

/* ----------------------------------------------------------------------
   count nonzeroes in each row
   input: one KMV per row, MV has entry for each nonzero
   output: one KV: key = # of nonzeroes, value = NULL
------------------------------------------------------------------------- */

void degree(char *key, int keybytes, char *multivalue,
   int nvalues, int *valuebytes, void *kv, void *ptr) 
{
  MR_kv_add(kv,(char *) &nvalues,sizeof(int),NULL,0);
}

/* ----------------------------------------------------------------------
   count rows with same # of nonzeroes
   input: one KMV per nonzero count, MV has entry for each row
   output: one KV: key = # of nonzeroes, value = # of rows
------------------------------------------------------------------------- */

void histo(char *key, int keybytes, char *multivalue,
     int nvalues, int *valuebytes, void *kv, void *ptr) 
{
  MR_kv_add(kv,key,keybytes,(char *) &nvalues,sizeof(int));
}

/* ----------------------------------------------------------------------
   compare two counts
   order values by count, largest first
------------------------------------------------------------------------- */

int ncompare(char *p1, int len1, char *p2, int len2)
{
  int i1 = *(int *) p1;
  int i2 = *(int *) p2;
  if (i1 > i2) return -1;
  else if (i1 < i2) return 1;
  else return 0;
}

/* ----------------------------------------------------------------------
   print # of rows with a specific # of nonzeroes
------------------------------------------------------------------------- */

void stats(int itask, char *key, int keybytes, char *value,
     int valuebytes, void *kv, void *ptr)
{
  int *total = (int *) ptr;
  int nnz = *(int *) key;
  int ncount = *(int *) value;
  *total += ncount;
  printf("%d rows with %d nonzeroes\n",ncount,nnz);
}
