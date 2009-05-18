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

#include "mpi.h"
#include "stdlib.h"
#include "stdio.h"
#include "error.h"

using namespace MAPREDUCE_NS;

/* ---------------------------------------------------------------------- */

Error::Error(MPI_Comm caller)
{
  comm = caller;
  MPI_Comm_rank(comm,&me);
}

/* ----------------------------------------------------------------------
   called by all procs
------------------------------------------------------------------------- */

void Error::all(const char *str)
{
  if (me == 0) printf("ERROR: %s\n",str);
  MPI_Finalize();
  exit(1);
}

/* ----------------------------------------------------------------------
   called by one proc
------------------------------------------------------------------------- */

void Error::one(const char *str)
{
  printf("ERROR on proc %d: %s\n",me,str);
  MPI_Abort(comm,1);
}

/* ----------------------------------------------------------------------
   called by one proc
------------------------------------------------------------------------- */

void Error::warning(const char *str)
{
  printf("WARNING: %s\n",str);
}
