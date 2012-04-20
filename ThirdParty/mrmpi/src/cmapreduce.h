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

/* C or Fortran style interface to MapReduce library */
/* ifdefs allow this file to be included in a C program */

#include "mpi.h"
#include "mrmpi_config.h"

#ifdef __cplusplus
extern "C" {
#endif

MRMPI_EXPORT void *MR_create(MPI_Comm comm);
MRMPI_EXPORT void *MR_create_mpi();
MRMPI_EXPORT void *MR_create_mpi_finalize();
MRMPI_EXPORT void *MR_copy(void *MRptr);
MRMPI_EXPORT void MR_destroy(void *MRptr);

MRMPI_EXPORT int MR_aggregate(void *MRptr, int (*myhash)(char *, int));
MRMPI_EXPORT int MR_clone(void *MRptr);
MRMPI_EXPORT int MR_collapse(void *MRptr, char *key, int keybytes);
MRMPI_EXPORT int MR_collate(void *MRptr, int (*myhash)(char *, int));
MRMPI_EXPORT int MR_compress(void *MRptr, 
    void (*mycompress)(char *, int, char *,
           int, int *, void *KVptr, void *APPptr),
    void *APPptr);
MRMPI_EXPORT int MR_convert(void *MRptr);
MRMPI_EXPORT int MR_gather(void *MRptr, int numprocs);

MRMPI_EXPORT int MR_map(void *MRptr, int nmap,
     void (*mymap)(int, void *KVptr, void *APPptr),
     void *APPptr);
MRMPI_EXPORT int MR_map_add(void *MRptr, int nmap,
         void (*mymap)(int, void *KVptr, void *APPptr),
         void *APPptr, int addflag);
MRMPI_EXPORT int MR_map_file_list(void *MRptr, char *file,
         void (*mymap)(int, char *, void *KVptr, void *APPptr),
         void *APPptr);
MRMPI_EXPORT int MR_map_file_list_add(void *MRptr, char *file,
       void (*mymap)(int, char *, void *KVptr, void *APPptr),
       void *APPptr, int addflag);
MRMPI_EXPORT int MR_map_file_char(void *MRptr, int nmap, int nfiles, char **files,
         char sepchar, int delta,
         void (*mymap)(int, char *, int, 
           void *KVptr, void *APPptr),
         void *APPptr);
MRMPI_EXPORT int MR_map_file_char_add(void *MRptr, int nmap, int nfiles, char **files,
       char sepchar, int delta,
       void (*mymap)(int, char *, int, 
               void *KVptr, void *APPptr),
       void *APPptr, int addflag);
MRMPI_EXPORT int MR_map_file_str(void *MRptr, int nmap, int nfiles, char **files,
        char *sepstr, int delta,
        void (*mymap)(int, char *, int, 
          void *KVptr, void *APPptr),
        void *APPptr);
MRMPI_EXPORT int MR_map_file_str_add(void *MRptr, int nmap, int nfiles, char **files,
      char *sepstr, int delta,
      void (*mymap)(int, char *, int, 
              void *KVptr, void *APPptr),
      void *APPptr, int addflag);
MRMPI_EXPORT int MR_map_kv(void *MRptr, void *MRptr2,
        void (*mymap)(int, char *, int, char *, int, 
          void *KVptr, void *APPptr),
        void *APPptr);
MRMPI_EXPORT int MR_map_kv_add(void *MRptr, void *MRptr2,
      void (*mymap)(int, char *, int, char *, int, 
        void *KVptr, void *APPptr),
      void *APPptr, int addflag);

MRMPI_EXPORT int MR_reduce(void *MRptr,
        void (*myreduce)(char *, int, char *,
             int, int *, void *KVptr, void *APPptr),
        void *APPptr);
MRMPI_EXPORT int MR_scrunch(void *MRptr, int numprocs, char *key, int keybytes);
MRMPI_EXPORT int MR_sort_keys(void *MRptr, 
     int (*mycompare)(char *, int, char *, int));
MRMPI_EXPORT int MR_sort_values(void *MRptr,
       int (*mycompare)(char *, int, char *, int));
MRMPI_EXPORT int MR_sort_multivalues(void *MRptr,
      int (*mycompare)(char *, int, char *, int));

MRMPI_EXPORT void MR_kv_stats(void *MRptr, int level);
MRMPI_EXPORT void MR_kmv_stats(void *MRptr, int level);

MRMPI_EXPORT void MR_set_mapstyle(void *MRptr, int value);
MRMPI_EXPORT void MR_set_verbosity(void *MRptr, int value);
MRMPI_EXPORT void MR_set_timer(void *MRptr, int value);

MRMPI_EXPORT void MR_kv_add(void *KVptr, char *key, int keybytes, 
         char *value, int valuebytes);
MRMPI_EXPORT void MR_kv_add_multi_static(void *KVptr, int n,
          char *key, int keybytes,
          char *value, int valuebytes);
MRMPI_EXPORT void MR_kv_add_multi_dynamic(void *KVptr, int n,
           char *key, int *keybytes,
           char *value, int *valuebytes);
MRMPI_EXPORT void MR_kv_add_kv(void *MRptr, void *MRptr2);

#ifdef __cplusplus
}
#endif
