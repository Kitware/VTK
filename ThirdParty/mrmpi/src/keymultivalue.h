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

#ifndef KEY_MULTIVALUE_H
#define KEY_MULTIVALUE_H

#include "mpi.h"
#include "mrmpi_config.h"

namespace MAPREDUCE_NS {

class MRMPI_EXPORT KeyMultiValue {
 public:
  int nkey;               // # of KMV pairs
  int keysize;            // size of keydata array
  int multivaluesize;     // size of multivaluedata array
  int *keys;              // keys[i] = where Ith key starts in keydata
  int *multivalues;       // multivalues[i] = where Ith mv starts in mvdata
  int *nvalues;           // nvalues[i] = where values for Ith multivalue
                          //   start in valuesizes
  int *valuesizes;        // size of each value in all multivalues
                          // valuesizes[nvalues[i]] = size of 1st value
                          //   in Ith multivalue
  char *keydata;          // unique keys, one after another
  char *multivaluedata;   // multivalues, one after another
                          // values in a multivalue are one after another

  int maxdepth;           // max depth of any one hash bucket

  KeyMultiValue(MPI_Comm);
  KeyMultiValue(KeyMultiValue &);
  ~KeyMultiValue();

  void convert(class KeyValue *);
  void clone(class KeyValue *);
  void collapse(char *, int, class KeyValue *);
  void grow_buckets(KeyValue *);
  int hash(char *, int);
  int find(int, char *, int, class KeyValue *);
  int match(char *, char *, int);

 private:
  MPI_Comm comm;
  class Memory *memory;

  struct Unique {       // a unique key
    int keyindex;       // index in KV of this key
    int mvsize;         // total size of values of this key
    int nvalue;         // # of values of this key
    int next;           // index in uniques of next key in this hash bucket
  };

  int nunique;          // # of unique keys in KMV
  int maxunique;        // max # of unique keys in uniques array
  Unique *uniques;      // list of unique keys

  int *buckets;         // bucket[i] = index of 1st key entry in this bucket
  int nbuckets;         // # of hash buckets
  int hashmask;         // bit mask for mapping into hash buckets
};

}

#endif
