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

#ifndef ERROR_H
#define ERROR_H

#include "mpi.h"
#include "mrmpi_config.h"

namespace MAPREDUCE_NS {

class MRMPI_EXPORT Error {
 public:
  Error(MPI_Comm);

  void all(const char *);
  void one(const char *);
  void warning(const char *);

 private:
  MPI_Comm comm;
  int me;
};

}

#endif
