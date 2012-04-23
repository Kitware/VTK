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

#ifndef MAP_REDUCE_H
#define MAP_REDUCE_H

#include "mpi.h"
#include "vtkType.h"
#include "mrmpi_config.h"

namespace MAPREDUCE_NS {

class MRMPI_EXPORT MapReduce {
 public:
  int mapstyle;     // 0 = chunks, 1 = strided, 2 = master/slave
  int verbosity;    // 0 = none, 1 = totals, 2 = proc histograms
  int timer;        // 0 = none, 1 = summary, 2 = proc histograms

  class KeyValue *kv;              // single KV stored by MR
  class KeyMultiValue *kmv;        // single KMV stored by MR

  static MapReduce *mrptr;         // holds a ptr to MR currently being used
  static int instance_count;       // count # of instantiated MRs
  static int mpi_finalize_flag;    // 1 if MR library should finalize MPI

  // library API

  MapReduce(MPI_Comm);
  MapReduce();
  MapReduce(double);
  MapReduce(MapReduce &);
  ~MapReduce();

  int aggregate(int (*)(char *, int));
  int clone();
  int collapse(char *, int);
  int collate(int (*)(char *, int));
  int compress(void (*)(char *, int, char *,
      int, int *, class KeyValue *, void *),
         void *);
  int convert();
  int gather(int);

  int map(int, void (*)(int, class KeyValue *, void *),
    void *, int addflag = 0);
  int map(char *, void (*)(int, char *, class KeyValue *, void *),
    void *, int addflag = 0);
  int map(int, int, char **, char, int, 
    void (*)(int, char *, int, class KeyValue *, void *),
    void *, int addflag = 0);
  int map(int, int, char **, char *, int, 
    void (*)(int, char *, int, class KeyValue *, void *),
    void *, int addflag = 0);
  int map(class KeyValue *, void (*)(int, char *, int, char *, int, 
             class KeyValue *, void *),
    void *, int addflag = 0);

  int reduce(void (*)(char *, int, char *,
          int, int *, class KeyValue *, void *),
       void *);
  int scrunch(int, char *, int);
  int sort_keys(int (*)(char *, int, char *, int));
  int sort_values(int (*)(char *, int, char *, int));
  int sort_multivalues(int (*)(char *, int, char *, int));

  void kv_stats(int);
  void kmv_stats(int);

  // query functions

  MPI_Comm communicator() {return comm;};
  int num_procs() {return nprocs;};
  int my_proc() {return me;};

  // functions accessed thru non-class wrapper functions

  void map_file_wrapper(int, class KeyValue *);
  int compare_keys_wrapper(int, int);
  int compare_values_wrapper(int, int);
  int compare_multivalues_wrapper(int, int);

 private:
  MPI_Comm comm;
  int me,nprocs;
  double time_start,time_stop;
  class Memory *memory;
  class Error *error;

  typedef int (CompareFunc)(char *, int, char *, int);  // used by sorts
  CompareFunc *compare;

  char **mv_values;      // used by sort_multivalues()
  int *mv_valuesizes;

  struct FileMap {       // used by file map()
    int sepwhich;
    char sepchar;
    char *sepstr;
    int delta;
    char **filename;          // names of files to read
    vtkTypeUInt64 *filesize;       // size in bytes of each file
    int *tasksperfile;        // # of map tasks for each file
    int *whichfile;           // which file each map task reads
    int *whichtask;           // which sub-task in file each map task is
    typedef void (MapFileFunc)(int, char *, int, class KeyValue *, void *);
    MapFileFunc *appmapfile;  // user map function
    void *ptr;                // user data ptr
  };
  FileMap filemap;

  int map_file(int, int, char **,
         void (*)(int, char *, int, class KeyValue *, void *),
         void *, int addflag);
  void sort_kv(int);
  void stats(const char *, int, int);
  void histogram(int, double *, double &, double &, double &,
     int, int *, int *);
  void start_timer();
};

}

#endif
