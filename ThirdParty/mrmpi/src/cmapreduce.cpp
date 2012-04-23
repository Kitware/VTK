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

// C interface to MapReduce library
// ditto for Fortran, scripting language, or other hi-level languages

#include "cmapreduce.h"
#include "mapreduce.h"
#include "keyvalue.h"

using namespace MAPREDUCE_NS;

void *MR_create(MPI_Comm comm)
{
  MapReduce *mr = new MapReduce(comm);
  return (void *) mr;
}

void *MR_create_mpi()
{
  MapReduce *mr = new MapReduce();
  return (void *) mr;
}

void *MR_create_mpi_finalize()
{
  MapReduce *mr = new MapReduce(0.0);
  return (void *) mr;
}

void *MR_copy(void *MRptr)
{
  MapReduce *mr = (MapReduce *) MRptr;
  MapReduce *mr2 = new MapReduce(*mr);
  return (void *) mr2;
}

void MR_destroy(void *MRptr)
{
  MapReduce *mr = (MapReduce *) MRptr;
  delete mr;
}

int MR_aggregate(void *MRptr, int (*myhash)(char *, int))
{
  MapReduce *mr = (MapReduce *) MRptr;
  return mr->aggregate(myhash);
}

int MR_clone(void *MRptr)
{
  MapReduce *mr = (MapReduce *) MRptr;
  return mr->clone();
}

int MR_collapse(void *MRptr, char *key, int keybytes)
{
  MapReduce *mr = (MapReduce *) MRptr;
  return mr->collapse(key,keybytes);
}

int MR_collate(void *MRptr, int (*myhash)(char *, int))
{
  MapReduce *mr = (MapReduce *) MRptr;
  return mr->collate(myhash);
}

int MR_compress(void *MRptr,
    void (*mycompress)(char *, int, char *,
           int, int *, void *, void *),
    void *APPptr)
{
  typedef void (CompressFunc)(char *, int, char *,
            int, int *, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  CompressFunc *appcompress = (CompressFunc *) mycompress;
  return mr->compress(appcompress,APPptr);
}

int MR_convert(void *MRptr)
{
  MapReduce *mr = (MapReduce *) MRptr;
  return mr->convert();
}

int MR_gather(void *MRptr, int numprocs)
{
  MapReduce *mr = (MapReduce *) MRptr;
  return mr->gather(numprocs);
}

int MR_map(void *MRptr, int nmap,
     void (*mymap)(int, void *, void *),
     void *APPptr)
{
  typedef void (MapFunc)(int, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  MapFunc *appmap = (MapFunc *) mymap;
  return mr->map(nmap,appmap,APPptr);
}

int MR_map_add(void *MRptr, int nmap,
     void (*mymap)(int, void *, void *),
     void *APPptr, int addflag)
{
  typedef void (MapFunc)(int, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  MapFunc *appmap = (MapFunc *) mymap;
  return mr->map(nmap,appmap,APPptr,addflag);
}

int MR_map_file_list(void *MRptr, char *file,
         void (*mymap)(int, char *, void *, void *),
         void *APPptr)
{
  typedef void (MapFunc)(int, char *, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  MapFunc *appmap = (MapFunc *) mymap;
  return mr->map(file,appmap,APPptr);
}

int MR_map_file_list_add(void *MRptr, char *file,
       void (*mymap)(int, char *, void *, void *),
       void *APPptr, int addflag)
{
  typedef void (MapFunc)(int, char *, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  MapFunc *appmap = (MapFunc *) mymap;
  return mr->map(file,appmap,APPptr,addflag);
}

int MR_map_file_char(void *MRptr, int nmap, int nfiles, char **files,
         char sepchar, int delta,
         void (*mymap)(int, char *, int, void *, void *),
         void *APPptr)
{
  typedef void (MapFunc)(int, char *, int, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  MapFunc *appmap = (MapFunc *) mymap;
  return mr->map(nmap,nfiles,files,sepchar,delta,appmap,APPptr);
}

int MR_map_file_char_add(void *MRptr, int nmap, int nfiles, char **files,
       char sepchar, int delta,
       void (*mymap)(int, char *, int, void *, void *),
       void *APPptr, int addflag)
{
  typedef void (MapFunc)(int, char *, int, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  MapFunc *appmap = (MapFunc *) mymap;
  return mr->map(nmap,nfiles,files,sepchar,delta,appmap,APPptr,addflag);
}

int MR_map_file_str(void *MRptr, int nmap, int nfiles, char **files,
        char *sepstr, int delta,
        void (*mymap)(int, char *, int, void *, void *),
        void *APPptr)
{
  typedef void (MapFunc)(int, char *, int, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  MapFunc *appmap = (MapFunc *) mymap;
  return mr->map(nmap,nfiles,files,sepstr,delta,appmap,APPptr);
}

int MR_map_file_str_add(void *MRptr, int nmap, int nfiles, char **files,
      char *sepstr, int delta,
      void (*mymap)(int, char *, int, void *, void *),
      void *APPptr, int addflag)
{
  typedef void (MapFunc)(int, char *, int, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  MapFunc *appmap = (MapFunc *) mymap;
  return mr->map(nmap,nfiles,files,sepstr,delta,appmap,APPptr,addflag);
}

int MR_map_kv(void *MRptr, void *MRptr2,
        void (*mymap)(int, char *, int, char *, int, void *, void *),
        void *APPptr)
{
  typedef void (MapFunc)(int, char *, int, char *, int, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  KeyValue *kv = ((MapReduce *) MRptr2)->kv;
  MapFunc *appmap = (MapFunc *) mymap;
  return mr->map(kv,appmap,APPptr);
}

int MR_map_kv_add(void *MRptr, void *MRptr2,
        void (*mymap)(int, char *, int, char *, int, void *, void *),
        void *APPptr, int addflag)
{
  typedef void (MapFunc)(int, char *, int, char *, int, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  KeyValue *kv = ((MapReduce *) MRptr2)->kv;
  MapFunc *appmap = (MapFunc *) mymap;
  return mr->map(kv,appmap,APPptr,addflag);
}

int MR_reduce(void *MRptr,
        void (*myreduce)(char *, int, char *,
             int, int *, void *, void *),
        void *APPptr)
{
  typedef void (ReduceFunc)(char *, int, char *,
          int, int *, KeyValue *, void *);
  MapReduce *mr = (MapReduce *) MRptr;
  ReduceFunc *appreduce = (ReduceFunc *) myreduce;
  return mr->reduce(appreduce,APPptr);
}

int MR_scrunch(void *MRptr, int numprocs, char *key, int keybytes)
{
  MapReduce *mr = (MapReduce *) MRptr;
  return mr->scrunch(numprocs,key,keybytes);
}

int MR_sort_keys(void *MRptr, int (*mycompare)(char *, int, char *, int))
{
  MapReduce *mr = (MapReduce *) MRptr;
  return mr->sort_keys(mycompare);
}

int MR_sort_values(void *MRptr, int (*mycompare)(char *, int, char *, int))
{
  MapReduce *mr = (MapReduce *) MRptr;
  return mr->sort_values(mycompare);
}

int MR_sort_multivalues(void *MRptr, int (*mycompare)(char *, int, 
                  char *, int))
{
  MapReduce *mr = (MapReduce *) MRptr;
  return mr->sort_multivalues(mycompare);
}

void MR_kv_stats(void *MRptr, int level)
{
  MapReduce *mr = (MapReduce *) MRptr;
  mr->kv_stats(level);
}

void MR_kmv_stats(void *MRptr, int level)
{
  MapReduce *mr = (MapReduce *) MRptr;
  mr->kmv_stats(level);
}

void MR_set_mapstyle(void *MRptr, int value)
{
  MapReduce *mr = (MapReduce *) MRptr;
  mr->mapstyle = value;
}

void MR_set_verbosity(void *MRptr, int value)
{
  MapReduce *mr = (MapReduce *) MRptr;
  mr->verbosity = value;
}

void MR_set_timer(void *MRptr, int value)
{
  MapReduce *mr = (MapReduce *) MRptr;
  mr->timer = value;
}

void MR_kv_add(void *KVptr, char *key, int keybytes,
         char *value, int valuebytes)
{
  KeyValue *kv = (KeyValue *) KVptr;
  kv->add(key,keybytes,value,valuebytes);
}

void MR_kv_add_multi_static(void *KVptr, int n, 
          char *key, int keybytes,
          char *value, int valuebytes)
{
  KeyValue *kv = (KeyValue *) KVptr;
  kv->add(n,key,keybytes,value,valuebytes);
}

void MR_kv_add_multi_dynamic(void *KVptr, int n, 
           char *key, int *keybytes,
           char *value, int *valuebytes)
{
  KeyValue *kv = (KeyValue *) KVptr;
  kv->add(n,key,keybytes,value,valuebytes);
}

void MR_kv_add_kv(void *MRptr, void *MRptr2)
{
  KeyValue *kv = ((MapReduce *) MRptr)->kv;
  KeyValue *kv2 = ((MapReduce *) MRptr2)->kv;
  kv->add(kv2);
}
