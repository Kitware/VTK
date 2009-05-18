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

#ifndef KEY_VALUE_H
#define KEY_VALUE_H

#include "mpi.h"
#include "mrmpi_config.h"

namespace MAPREDUCE_NS {

class MRMPI_EXPORT KeyValue {
 public:
  int nkey;                     // # of KV pairs
  int keysize;                  // size of keydata array
  int valuesize;                // size of valuedata array
  int *keys;                    // keys[i] = Ith key offset in keydata
  int *values;                  // values[i] = Ith value offset in valuedata
  char *keydata;                // keys, one after another
  char *valuedata;              // values, one after another

  int maxkey;                   // max size of keys,values arrays
  int maxkeysize;               // max size of keydata
  int maxvaluesize;             // max size of valuedata

  KeyValue(MPI_Comm);
  KeyValue(KeyValue &);
  ~KeyValue();

  void add(char *, int, char *, int);
  void add(int, char *, int, char *, int);
  void add(int, char *, int *, char *, int *);
  void add(KeyValue *);

  int pack(char **);
  void unpack(char *);
  void complete();

 private:
  MPI_Comm comm;
  class Memory *memory;
  class Error *error;
};

}

#endif
