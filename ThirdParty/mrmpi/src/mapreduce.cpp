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
#include "ctype.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "vtkType.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "mapreduce.h"
#include "keyvalue.h"
#include "keymultivalue.h"
#include "irregular.h"
#include "hash.h"
#include "memory.h"
#include "error.h"

using namespace MAPREDUCE_NS;

// allocate space for and initialize static class variables

MapReduce *MapReduce::mrptr;
int MapReduce::instance_count = 0;
int MapReduce::mpi_finalize_flag = 0;

// prototypes for non-class functions

void map_file_standalone(int nmap, KeyValue *ptr, void *data);

extern "C" {
  int compare_keys_standalone(const void *, const void *);
  int compare_values_standalone(const void *, const void *);
  int compare_multivalues_standalone(const void *, const void *);
};

#ifdef MIN
#  undef MIN
#endif
#define MIN(A,B) ((A) < (B)) ? (A) : (B)

#ifdef MAX
#  undef MAX
#endif
#define MAX(A,B) ((A) > (B)) ? (A) : (B)

#define FILECHUNK 1
#define MAXLINE 1024

/* ----------------------------------------------------------------------
   construct using caller's MPI communicator
   perform no MPI_init() and no MPI_Finalize()
------------------------------------------------------------------------- */

MapReduce::MapReduce(MPI_Comm caller)
{
  instance_count++;

  comm = caller;
  MPI_Comm_rank(comm,&me);
  MPI_Comm_size(comm,&nprocs);

  memory = new Memory(comm);
  error = new Error(comm);

  kv = NULL;
  kmv = NULL;

  mapstyle = 0;
  verbosity = 0;
  timer = 0;
}

/* ----------------------------------------------------------------------
   construct without MPI communicator, use MPI_COMM_WORLD
   perform MPI_Init() if not already initialized
   perform no MPI_Finalize()
------------------------------------------------------------------------- */

MapReduce::MapReduce()
{
  instance_count++;

  int flag;
  MPI_Initialized(&flag);

  if (!flag) {
    int argc = 0;
    char **argv = NULL;
    MPI_Init(&argc,&argv);
  }

  comm = MPI_COMM_WORLD;
  MPI_Comm_rank(comm,&me);
  MPI_Comm_size(comm,&nprocs);

  memory = new Memory(comm);
  error = new Error(comm);

  kv = NULL;
  kmv = NULL;

  mapstyle = 0;
  verbosity = 0;
  timer = 0;
}

/* ----------------------------------------------------------------------
   construct without MPI communicator, use MPI_COMM_WORLD
   perform MPI_Init() if not already initialized
   perform MPI_Finalize() if final instance is destructed
------------------------------------------------------------------------- */

MapReduce::MapReduce(double dummy)
{
  (void)dummy;
  instance_count++;
  mpi_finalize_flag = 1;

  int flag;
  MPI_Initialized(&flag);

  if (!flag) {
    int argc = 0;
    char **argv = NULL;
    MPI_Init(&argc,&argv);
  }

  comm = MPI_COMM_WORLD;
  MPI_Comm_rank(comm,&me);
  MPI_Comm_size(comm,&nprocs);

  memory = new Memory(comm);
  error = new Error(comm);

  kv = NULL;
  kmv = NULL;

  mapstyle = 0;
  verbosity = 0;
  timer = 0;
}

/* ----------------------------------------------------------------------
   copy constructor, perform a deep copy
------------------------------------------------------------------------- */

MapReduce::MapReduce(MapReduce &mr)
{
  instance_count++;

  comm = mr.comm;
  MPI_Comm_rank(comm,&me);
  MPI_Comm_size(comm,&nprocs);

  memory = new Memory(comm);
  error = new Error(comm);

  kv = NULL;
  kmv = NULL;

  if (mr.kv) kv = new KeyValue(*mr.kv);
  if (mr.kmv) kmv = new KeyMultiValue(*mr.kmv);

  mapstyle = mr.mapstyle;
  verbosity = mr.verbosity;
  timer = mr.timer;
}

/* ----------------------------------------------------------------------
   free all memory
   if finalize_flag is set and this is last instance, then finalize MPI
------------------------------------------------------------------------- */

MapReduce::~MapReduce()
{
  delete memory;
  delete error;
  delete kv;
  delete kmv;

  instance_count--;
  if (mpi_finalize_flag && instance_count == 0) MPI_Finalize();
}

/* ----------------------------------------------------------------------
   aggregate a KV across procs to create a new KV
   initially, key copies can exist on many procs
   after aggregation, all copies of key are on same proc
   performed via parallel distributed hashing
   hash = user hash function (NULL if not provided)
   requires irregular all2all communication
------------------------------------------------------------------------- */

int MapReduce::aggregate(int (*hash)(char *, int))
{
  if (kv == NULL) error->all("Cannot aggregate without KeyValue");
  if (timer) start_timer();

  if (nprocs == 1) {
    stats("Aggregate",0,verbosity);
    return kv->nkey;
  }

  KeyValue *kvnew = new KeyValue(comm);
  Irregular *irregular = new Irregular(comm);

  // hash each key to a proc ID
  // either use user-provided hash function or hashlittle()

  int nkey = kv->nkey;
  int *keys = kv->keys;
  int *values = kv->values;
  char *keydata = kv->keydata;

  int *proclist = new int[kv->nkey];

  for (int i = 0; i < nkey; i++) {
    char *key = &keydata[keys[i]];
    int keybytes = keys[i+1] - keys[i];
    if (hash) proclist[i] = hash(key,keybytes) % nprocs;
    else proclist[i] = hashlittle(key,keybytes,nprocs) % nprocs;
  }

  // redistribute key sizes, key data, value sizes, value data
  // convert key/value offsets into sizes

  irregular->pattern(nkey,proclist);

  int *slength = proclist;
  for (int i = 0; i < nkey; i++) slength[i] = keys[i+1] - keys[i];

  int nbytes = irregular->size(sizeof(int));
  kvnew->nkey = kvnew->maxkey = nbytes / sizeof(int);
  kvnew->keys = (int *) memory->smalloc(nbytes,"MR:keys");
  irregular->exchange((char *) slength,(char *) kvnew->keys);

  nbytes = irregular->size(slength,kv->keys,kvnew->keys);
  kvnew->keysize = kvnew->maxkeysize = nbytes;
  kvnew->keydata = (char *) memory->smalloc(nbytes,"MR:keydata");
  irregular->exchange(kv->keydata,kvnew->keydata);

  for (int i = 0; i < nkey; i++) slength[i] = values[i+1] - values[i];

  nbytes = irregular->size(sizeof(int));
  kvnew->values = (int *) memory->smalloc(nbytes,"MR:values");
  irregular->exchange((char *) slength,(char *) kvnew->values);

  nbytes = irregular->size(slength,kv->values,kvnew->values);
  kvnew->valuesize = kvnew->maxvaluesize = nbytes;
  kvnew->valuedata = (char *) memory->smalloc(nbytes,"MR:valuedata");
  irregular->exchange(kv->valuedata,kvnew->valuedata);

  delete [] slength;
  delete irregular;

  // convert key/value sizes back into offsets

  nkey = kvnew->nkey;
  keys = kvnew->keys;
  values = kvnew->values;

  int keysize = 0;
  int valuesize = 0;
  int tmp;

  for (int i = 0; i < nkey; i++) {
    tmp = keys[i];
    keys[i] = keysize;
    keysize += tmp;
    tmp = values[i];
    values[i] = valuesize;
    valuesize += tmp;
  }

  delete kv;
  kv = kvnew;
  kv->complete();

  stats("Aggregate",0,verbosity);

  int nkeyall;
  MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   clone KV to KMV so that KMV pairs are one-to-one copies of KV pairs
   each proc clones only its data
   assume each KV key is unique, but is not required
------------------------------------------------------------------------- */

int MapReduce::clone()
{
  if (kv == NULL) error->all("Cannot clone without KeyValue");
  if (timer) start_timer();

  kmv = new KeyMultiValue(comm);
  kmv->clone(kv);
  delete kv;
  kv = NULL;

  stats("Clone",1,verbosity);

  int nkeyall;
  MPI_Allreduce(&kmv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   collapse KV into a KMV with a single key/value
   each proc collapses only its data
   new key = provided key name (same on every proc)
   new value = list of old key,value,key,value,etc
------------------------------------------------------------------------- */

int MapReduce::collapse(char *key, int keybytes)
{
  if (kv == NULL) error->all("Cannot collapse without KeyValue");
  if (timer) start_timer();

  kmv = new KeyMultiValue(comm);
  kmv->collapse(key,keybytes,kv);
  delete kv;
  kv = NULL;

  stats("Collapse",1,verbosity);

  int nkeyall;
  MPI_Allreduce(&kmv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   collate KV to create a KMV
   aggregate followed by a convert
   hash = user hash function (NULL if not provided)
------------------------------------------------------------------------- */

int MapReduce::collate(int (*hash)(char *, int))
{
  if (kv == NULL) error->all("Cannot collate without KeyValue");
  if (timer) start_timer();

  int verbosity_hold = verbosity;
  int timer_hold = timer;
  verbosity = timer = 0;

  aggregate(hash);
  convert();

  verbosity = verbosity_hold;
  timer = timer_hold;
  stats("Collate",1,verbosity);

  int nkeyall;
  MPI_Allreduce(&kmv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   compress KV to create a smaller KV
   duplicate keys are replaced with a single key/value
   each proc compresses only its data
   create a temporary KMV
   call appcompress() with each key/multivalue in KMV
   appcompress() returns single key/value to new KV
------------------------------------------------------------------------- */

int MapReduce::compress(void (*appcompress)(char *, int, char *,
              int, int *, KeyValue *, void *),
      void *ptr)
{
  if (kv == NULL) error->all("Cannot compress without KeyValue");
  if (timer) start_timer();

  KeyMultiValue *kmvtmp = new KeyMultiValue(comm);
  kmvtmp->convert(kv);
  kv = new KeyValue(comm);

  int ncompress = kmvtmp->nkey;
  int *keys = kmvtmp->keys;
  int *multivalues = kmvtmp->multivalues;
  int *nvalues = kmvtmp->nvalues;
  int *valuesizes = kmvtmp->valuesizes;
  char *keydata = kmvtmp->keydata;
  char *multivaluedata = kmvtmp->multivaluedata;

  for (int i = 0; i < ncompress; i++)
    appcompress(&keydata[keys[i]],keys[i+1]-keys[i],
    &multivaluedata[multivalues[i]],
    nvalues[i+1]-nvalues[i],&valuesizes[nvalues[i]],
    kv,ptr);

  delete kmvtmp;
  kv->complete();

  stats("Compress",0,verbosity);

  int nkeyall;
  MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   convert KV to KMV
   duplicate keys are replaced with a single key/multivalue
   each proc converts only its data
   new key = old unique key
   new multivalue = concatenated list of all values for that key in KV
------------------------------------------------------------------------- */

int MapReduce::convert()
{
  if (kv == NULL) error->all("Cannot convert without KeyValue");
  if (timer) start_timer();

  kmv = new KeyMultiValue(comm);
  kmv->convert(kv);
  delete kv;
  kv = NULL;

  stats("Convert",1,verbosity);

  int nkeyall;
  MPI_Allreduce(&kmv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   gather a distributed KV to a new KV on fewer procs
   numprocs = # of procs new KV resides on (0 to numprocs-1)
------------------------------------------------------------------------- */

int MapReduce::gather(int numprocs)
{
  if (kv == NULL) error->all("Cannot gather without KeyValue");
  if (numprocs < 1 || numprocs > nprocs) 
    error->all("Invalid proc count for gather");
  if (timer) start_timer();

  if (nprocs == 1 || numprocs == nprocs) {
    stats("Gather",0,verbosity);
    int nkeyall;
    MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
    return nkeyall;
  }

  // lo procs collect key/value pairs from hi procs
  // lo procs are those with ID < numprocs
  // lo procs recv from set of hi procs with same (ID % numprocs)

  int flag,size;
  MPI_Status status;

  if (me < numprocs) {
    char *buf = NULL;
    int maxsize = 0;

    for (int iproc = me+numprocs; iproc < nprocs; iproc += numprocs) {
      MPI_Send(&flag,0,MPI_INT,iproc,0,comm);
      MPI_Recv(&size,1,MPI_INT,iproc,0,comm,&status);
      if (size > maxsize) {
  delete [] buf;
  buf = new char[size];
      }
      MPI_Recv(buf,size,MPI_BYTE,iproc,0,comm,&status);
      kv->unpack(buf);
    }
    
    delete [] buf;

  } else {
    char *buf;
    size = kv->pack(&buf);

    int iproc = me % numprocs;
    MPI_Recv(&flag,0,MPI_INT,iproc,0,comm,&status);
    MPI_Send(&size,1,MPI_INT,iproc,0,comm);
    MPI_Send(buf,size,MPI_BYTE,iproc,0,comm);

    delete [] buf;
    delete kv;
    kv = new KeyValue(comm);
  }

  kv->complete();
  stats("Gather",0,verbosity);

  int nkeyall;
  MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   create a KV via a parallel map operation for nmap tasks
   make one call to appmap() for each task
   mapstyle determines how tasks are partitioned to processors
------------------------------------------------------------------------- */

int MapReduce::map(int nmap, void (*appmap)(int, KeyValue *, void *),
       void *ptr, int addflag)
{
  MPI_Status status;

  if (timer) start_timer();

  delete kmv;
  kmv = NULL;

  if (addflag == 0) {
    delete kv;
    kv = new KeyValue(comm);
  } else if (kv == NULL) 
    kv = new KeyValue(comm);

  // nprocs = 1 = all tasks to single processor
  // mapstyle 0 = chunk of tasks to each proc
  // mapstyle 1 = strided tasks to each proc
  // mapstyle 2 = master/slave assignment of tasks

  if (nprocs == 1) {
    for (int itask = 0; itask < nmap; itask++)
      appmap(itask,kv,ptr);

  } else if (mapstyle == 0) {
    vtkTypeUInt64 nmap64 = nmap;
    int lo = me * (int)(nmap64 / nprocs);
    int hi = (me+1) * (int)(nmap64 / nprocs);
    for (int itask = lo; itask < hi; itask++)
      appmap(itask,kv,ptr);

  } else if (mapstyle == 1) {
    for (int itask = me; itask < nmap; itask += nprocs)
      appmap(itask,kv,ptr);

  } else if (mapstyle == 2) {
    if (me == 0) {
      int doneflag = -1;
      int ndone = 0;
      int itask = 0;
      for (int iproc = 1; iproc < nprocs; iproc++) {
  if (itask < nmap) {
    MPI_Send(&itask,1,MPI_INT,iproc,0,comm);
    itask++;
  } else {
    MPI_Send(&doneflag,1,MPI_INT,iproc,0,comm);
    ndone++;
  }
      }
      while (ndone < nprocs-1) {
  int iproc,tmp;
  MPI_Recv(&tmp,1,MPI_INT,MPI_ANY_SOURCE,0,comm,&status);
  iproc = status.MPI_SOURCE;

  if (itask < nmap) {
    MPI_Send(&itask,1,MPI_INT,iproc,0,comm);
    itask++;
  } else {
    MPI_Send(&doneflag,1,MPI_INT,iproc,0,comm);
    ndone++;
  }
      }

    } else {
      while (1) {
        int itask;
        MPI_Recv(&itask,1,MPI_INT,0,0,comm,&status);
        if (itask < 0) break;
        appmap(itask,kv,ptr);
        MPI_Send(&itask,1,MPI_INT,0,0,comm);
      }
    }

  } else error->all("Invalid mapstyle setting");

  kv->complete();
  stats("Map",0,verbosity);

  int nkeyall;
  MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   create a KV via a parallel map operation for list of files in file
   make one call to appmap() for each file in file
   mapstyle determines how tasks are partitioned to processors
------------------------------------------------------------------------- */

int MapReduce::map(char *file, void (*appmap)(int, char *, KeyValue *, void *),
       void *ptr, int addflag)
{
  int n;
  char line[MAXLINE];
  MPI_Status status;

  if (timer) start_timer();

  delete kmv;
  kmv = NULL;

  if (addflag == 0) {
    delete kv;
    kv = new KeyValue(comm);
  } else if (kv == NULL) 
    kv = new KeyValue(comm);

  // open file and extract filenames
  // bcast each filename to all procs
  // trim whitespace from beginning and end of filename

  int nmap = 0;
  int maxfiles = 0;
  char **files = NULL;
  FILE *fp = 0;

  if (me == 0) {
    fp = fopen(file,"r");
    if (fp == NULL) error->one("Could not open file of file names");
  }

  while (1) {
    if (me == 0) {
      if (fgets(line,MAXLINE,fp) == NULL) n = 0;
      else n = static_cast<int>(strlen(line)) + 1;
    }
    MPI_Bcast(&n,1,MPI_INT,0,comm);
    if (n == 0) {
      if (me == 0) fclose(fp);
      break;
    }

    MPI_Bcast(line,n,MPI_CHAR,0,comm);

    char *ptr1 = line;
    while (isspace(*ptr1)) ptr1++;
    if (strlen(ptr1) == 0) error->all("Blank line in file of file names");
    char *ptr2 = ptr1 + strlen(ptr1) - 1;
    while (isspace(*ptr2)) ptr2--;
    ptr2++;
    *ptr2 = '\0';

    if (nmap == maxfiles) {
      maxfiles += FILECHUNK;
      files = (char **)
      memory->srealloc(files,maxfiles*sizeof(char *),"MR:files");
    }
    n = static_cast<int>(strlen(ptr1)) + 1;
    files[nmap] = new char[n];
    strcpy(files[nmap],ptr1);
    nmap++;
  }
  
  // nprocs = 1 = all tasks to single processor
  // mapstyle 0 = chunk of tasks to each proc
  // mapstyle 1 = strided tasks to each proc
  // mapstyle 2 = master/slave assignment of tasks

  if (nprocs == 1) {
    for (int itask = 0; itask < nmap; itask++)
      appmap(itask,files[itask],kv,ptr);

  } else if (mapstyle == 0) {
    vtkTypeUInt64 nmap64 = nmap;
    int lo = me * (int)(nmap64 / nprocs);
    int hi = (me+1) * (int)(nmap64 / nprocs);
    for (int itask = lo; itask < hi; itask++)
      appmap(itask,files[itask],kv,ptr);

  } else if (mapstyle == 1) {
    for (int itask = me; itask < nmap; itask += nprocs)
      appmap(itask,files[itask],kv,ptr);

  } else if (mapstyle == 2) {
    if (me == 0) {
      int doneflag = -1;
      int ndone = 0;
      int itask = 0;
      for (int iproc = 1; iproc < nprocs; iproc++) {
        if (itask < nmap) {
          MPI_Send(&itask,1,MPI_INT,iproc,0,comm);
          itask++;
        } else {
          MPI_Send(&doneflag,1,MPI_INT,iproc,0,comm);
          ndone++;
        }
      }
      while (ndone < nprocs-1) {
        int iproc,tmp;
        MPI_Recv(&tmp,1,MPI_INT,MPI_ANY_SOURCE,0,comm,&status);
        iproc = status.MPI_SOURCE;
      
        if (itask < nmap) {
          MPI_Send(&itask,1,MPI_INT,iproc,0,comm);
          itask++;
        } else {
          MPI_Send(&doneflag,1,MPI_INT,iproc,0,comm);
          ndone++;
        }
      }

    } else {
      while (1) {
        int itask;
        MPI_Recv(&itask,1,MPI_INT,0,0,comm,&status);
        if (itask < 0) break;
        appmap(itask,files[itask],kv,ptr);
        MPI_Send(&itask,1,MPI_INT,0,0,comm);
      }
    }

  } else error->all("Invalid mapstyle setting");

  // clean up file list

  for (int i = 0; i < nmap; i++) delete files[i];
  memory->sfree(files);

  kv->complete();
  stats("Map",0,verbosity);

  int nkeyall;
  MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   create a KV via a parallel map operation for nmap tasks
   nfiles filenames are split into nmap pieces based on separator char
------------------------------------------------------------------------- */

int MapReduce::map(int nmap, int nfiles, char **files,
       char sepchar, int delta,
       void (*appmap)(int, char *, int, KeyValue *, void *),
       void *ptr, int addflag)
{
  filemap.sepwhich = 1;
  filemap.sepchar = sepchar;
  filemap.delta = delta;

  return map_file(nmap,nfiles,files,appmap,ptr,addflag);
}

/* ----------------------------------------------------------------------
   create a KV via a parallel map operation for nmap tasks
   nfiles filenames are split into nmap pieces based on separator string
------------------------------------------------------------------------- */

int MapReduce::map(int nmap, int nfiles, char **files,
       char *sepstr, int delta,
       void (*appmap)(int, char *, int, KeyValue *, void *),
       void *ptr, int addflag)
{
  filemap.sepwhich = 0;
  int n = static_cast<int>(strlen(sepstr)) + 1;
  filemap.sepstr = new char[n];
  strcpy(filemap.sepstr,sepstr);
  filemap.delta = delta;

  return map_file(nmap,nfiles,files,appmap,ptr,addflag);
}

/* ----------------------------------------------------------------------
   called by 2 map methods that take files and a separator
   create a KV via a parallel map operation for nmap tasks
   nfiles filenames are split into nmap pieces based on separator
   FileMap struct stores info on how to split files
   calls non-file map() to partition tasks to processors
     with callback to non-class map_file_standalone()
   map_file_standalone() reads chunk of file and passes it to user appmap()
------------------------------------------------------------------------- */

int MapReduce::map_file(int nmap, int nfiles, char **files,
      void (*appmap)(int, char *, int, KeyValue *, void *),
      void *ptr, int addflag)
{
  if (nfiles > nmap) error->all("Cannot map with more files than tasks");
  if (timer) start_timer();

  delete kmv;
  kmv = NULL;

  // copy filenames into FileMap

  filemap.filename = new char*[nfiles];
  for (int i = 0; i < nfiles; i++) {
    int n = static_cast<int>(strlen(files[i])) + 1;
    filemap.filename[i] = new char[n];
    strcpy(filemap.filename[i],files[i]);
  }

  // get filesize of each file via stat()
  // proc 0 queries files, bcasts results to all procs

  filemap.filesize = new vtkTypeUInt64[nfiles];
  struct stat stbuf;

  if (me == 0) {
    for (int i = 0; i < nfiles; i++) {
      int flag = stat(files[i],&stbuf);
      if (flag < 0) error->one("Could not query file size");
      filemap.filesize[i] = stbuf.st_size;
    }
  }

  MPI_Bcast(filemap.filesize,nfiles*sizeof(vtkTypeUInt64),MPI_BYTE,0,comm);

  // ntotal = total size of all files
  // nideal = ideal # of bytes per task

  vtkTypeUInt64 ntotal = 0;
  for (int i = 0; i < nfiles; i++) ntotal += filemap.filesize[i];
  vtkTypeUInt64 nideal = MAX(1,ntotal/nmap);

  // tasksperfile[i] = # of tasks for Ith file
  // initial assignment based on ideal chunk size
  // increment/decrement tasksperfile until reach target # of tasks
  // even small files must have 1 task

  filemap.tasksperfile = new int[nfiles];

  int ntasks = 0;
  for (int i = 0; i < nfiles; i++) {
    filemap.tasksperfile[i] = MAX(1,(int)(filemap.filesize[i]/nideal));
    ntasks += filemap.tasksperfile[i];
  }

  while (ntasks < nmap)
    for (int i = 0; i < nfiles; i++)
      if (filemap.filesize[i] > nideal) {
  filemap.tasksperfile[i]++;
  ntasks++;
  if (ntasks == nmap) break;
      }
  while (ntasks > nmap)
    for (int i = 0; i < nfiles; i++)
      if (filemap.tasksperfile[i] > 1) {
  filemap.tasksperfile[i]--;
  ntasks--;
  if (ntasks == nmap) break;
      }

  // check if any tasks are so small they will cause overlapping reads w/ delta
  // if so, reduce number of tasks for that file and issue warning

  int flag = 0;
  for (int i = 0; i < nfiles; i++) {
    if ((vtkTypeInt64)(filemap.filesize[i] / filemap.tasksperfile[i]) > filemap.delta)
      continue;
    flag = 1;
    while (filemap.tasksperfile[i] > 1) {
      filemap.tasksperfile[i]--;
      nmap--;
      if ((vtkTypeInt64)(filemap.filesize[i] / filemap.tasksperfile[i]) > filemap.delta) break;
    }
  }

  if (flag && me == 0) {
    char str[128];
    sprintf(str,"File(s) too small for file delta - decreased map tasks to %d",
      nmap);
    error->warning(str);
  }

  // whichfile[i] = which file is associated with the Ith task
  // whichtask[i] = which task in that file the Ith task is

  filemap.whichfile = new int[nmap];
  filemap.whichtask = new int[nmap];

  int itask = 0;
  for (int i = 0; i < nfiles; i++)
    for (int j = 0; j < filemap.tasksperfile[i]; j++) {
      filemap.whichfile[itask] = i;
      filemap.whichtask[itask++] = j;
    }

  // use non-file map() partition tasks to procs
  // it calls map_file_standalone once for each task

  int verbosity_hold = verbosity;
  int timer_hold = timer;
  verbosity = timer = 0;

  filemap.appmapfile = appmap;
  filemap.ptr = ptr;
  map(nmap,&map_file_standalone,this,addflag);

  verbosity = verbosity_hold;
  timer = timer_hold;
  stats("Map",0,verbosity);

  // destroy FileMap

  if (filemap.sepwhich == 0) delete [] filemap.sepstr;
  for (int i = 0; i < nfiles; i++) delete [] filemap.filename[i];
  delete [] filemap.filename;
  delete [] filemap.filesize;
  delete [] filemap.tasksperfile;
  delete [] filemap.whichfile;
  delete [] filemap.whichtask;

  int nkeyall;
  MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   wrappers on user-provided appmapfile function
   2-level wrapper needed b/c file map() calls non-file map()
     and cannot pass it a class method unless it were static,
     but then it couldn't access MR class data
   so non-file map() is passed standalone non-class method
   standalone calls back into class wrapper which calls user appmapfile()
------------------------------------------------------------------------- */

void map_file_standalone(int imap, KeyValue *kv, void *ptr)
{
  MapReduce *mr = (MapReduce *) ptr;
  mr->map_file_wrapper(imap,kv);
}

void MapReduce::map_file_wrapper(int imap, KeyValue *pkv)
{
  // readstart = position in file to start reading for this task
  // readsize = # of bytes to read including delta

  vtkTypeUInt64 filesize = filemap.filesize[filemap.whichfile[imap]];
  int itask = filemap.whichtask[imap];
  int ntask = filemap.tasksperfile[filemap.whichfile[imap]];

  vtkTypeUInt64 readstart = itask*filesize/ntask;
  vtkTypeUInt64 readnext = (vtkTypeUInt64)(itask+1)*filesize/ntask;
  int readsize = (int)(readnext - readstart) + filemap.delta;
  readsize = MIN(readsize,(int)(filesize-readstart));

  // read from appropriate file
  // terminate string with NULL

  char *str = new char[readsize+1];
  FILE *fp = fopen(filemap.filename[filemap.whichfile[imap]],"rb");
  fseek(fp,(long)readstart,SEEK_SET);
  fread(str,1,readsize,fp);
  str[readsize] = '\0';
  fclose(fp);

  // if not first task in file, trim start of string
  // separator can be single char or a string
  // str[strstart] = 1st char in string
  // if separator = char, strstart is char after separator
  // if separator = string, strstart is 1st char of separator

  int strstart = 0;
  if (itask > 0) {
    char *ptr;
    if (filemap.sepwhich) ptr = strchr(str,filemap.sepchar);
    else ptr = strstr(str,filemap.sepstr);
    if (ptr == NULL || ptr-str > filemap.delta)
      error->one("Could not find separator within delta");
    strstart = ptr-str + filemap.sepwhich;
  }

  // if not last task in file, trim end of string
  // separator can be single char or a string
  // str[strstop] = last char in string = inserted NULL
  // if separator = char, NULL is char after separator
  // if separator = string, NULL is 1st char of separator

  int strstop = readsize;
  if (itask < ntask-1) {
    char *ptr;
    if (filemap.sepwhich) 
      ptr = strchr(&str[(int)(readnext-readstart)],filemap.sepchar);
    else 
      ptr = strstr(&str[(int)(readnext-readstart)],filemap.sepstr);
    if (ptr == NULL) error->one("Could not find separator within delta");
    if (filemap.sepwhich) ptr++;
    *ptr = '\0';
    strstop = ptr-str;
  }

  // call user appmapfile() function with user data ptr

  int strsize = strstop - strstart + 1;
  filemap.appmapfile(imap,&str[strstart],strsize,pkv,filemap.ptr);
  delete [] str;
}

/* ----------------------------------------------------------------------
   create a KV via a parallel map operation from an existing kv_src
   make one call to appmap() for each key/value pair in kv_src
   each proc operates on key/value pairs it owns
------------------------------------------------------------------------- */

int MapReduce::map(KeyValue *kv_src, 
       void (*appmap)(int, char *, int, char *, int, 
          KeyValue *, void *), void *ptr, int addflag)
{
  if (kv_src == NULL) error->all("Cannot map a KeyValue that does not exist");
  if (timer) start_timer();

  delete kmv;
  kmv = NULL;

  // kv_dest = KeyValue object where new KV will go
  // be careful if kv and kv_src are the same

  KeyValue *kv_dest;

  if (kv == kv_src) {
    if (addflag) kv_dest = kv;
    else kv_dest = new KeyValue(comm);
  } else {
    if (addflag) {
      if (kv == NULL) kv_dest = new KeyValue(comm);
      else kv_dest = kv;
    } else {
      delete kv;
      kv_dest = new KeyValue(comm);
    }
  }

  int nkey = kv_src->nkey;
  int *keys = kv_src->keys;
  int *values = kv_src->values;
  char *keydata = kv_src->keydata;
  char *valuedata = kv_src->valuedata;

  for (int i = 0; i < nkey; i++) {
    char *key = &keydata[keys[i]];
    int keybytes = keys[i+1] - keys[i];
    char *value = &valuedata[values[i]];
    int valuebytes = values[i+1] - values[i];
    appmap(i,key,keybytes,value,valuebytes,kv_dest,ptr);
  }
  
  if (kv == kv_src && addflag == 0) delete kv;
  kv = kv_dest;
  kv->complete();
  stats("Map",0,verbosity);

  int nkeyall;
  MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   create a KV from a KMV via a parallel reduce operation for nmap tasks
   make one call to appreduce() for each KMV pair
   each proc processes its owned KMV pairs
------------------------------------------------------------------------- */

int MapReduce::reduce(void (*appreduce)(char *, int, char *,
          int, int *, KeyValue *, void *),
          void *ptr)
{
  if (kmv == NULL) error->all("Cannot reduce without KeyMultiValue");
  if (timer) start_timer();

  kv = new KeyValue(comm);

  int nreduce = kmv->nkey;
  int *keys = kmv->keys;
  int *multivalues = kmv->multivalues;
  int *nvalues = kmv->nvalues;
  int *valuesizes = kmv->valuesizes;
  char *keydata = kmv->keydata;
  char *multivaluedata = kmv->multivaluedata;

  for (int i = 0; i < nreduce; i++)
    appreduce(&keydata[keys[i]],keys[i+1]-keys[i],
        &multivaluedata[multivalues[i]],
        nvalues[i+1]-nvalues[i],&valuesizes[nvalues[i]],
        kv,ptr);

  kv->complete();
  delete kmv;
  kmv = NULL;

  stats("Reduce",0,verbosity);

  int nkeyall;
  MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   scrunch KV to create a KMV on fewer processors, each with a single pair
   gather followed by a collapse
   numprocs = # of procs new KMV resides on (0 to numprocs-1)
   new key = provided key name (same on every proc)
   new value = list of old key,value,key,value,etc
------------------------------------------------------------------------- */

int MapReduce::scrunch(int numprocs, char *key, int keybytes)
{
  if (kv == NULL) error->all("Cannot scrunch without KeyValue");
  if (timer) start_timer();

  int verbosity_hold = verbosity;
  int timer_hold = timer;
  verbosity = timer = 0;

  gather(numprocs);
  collapse(key,keybytes);

  verbosity = verbosity_hold;
  timer = timer_hold;
  stats("Scrunch",1,verbosity);

  int nkeyall;
  MPI_Allreduce(&kmv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   sort keys in a KV to create a new KV
   use appcompare() to compare 2 keys
   each proc sorts only its data
------------------------------------------------------------------------- */

int MapReduce::sort_keys(int (*appcompare)(char *, int, char *, int))
{
  if (kv == NULL) error->all("Cannot sort_keys without KeyValue");
  if (timer) start_timer();

  compare = appcompare;
  sort_kv(0);

  stats("Sort_keys",0,verbosity);

  int nkeyall;
  MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   sort values in a KV to create a new KV
   use appcompare() to compare 2 values
   each proc sorts only its data
------------------------------------------------------------------------- */

int MapReduce::sort_values(int (*appcompare)(char *, int, char *, int))
{
  if (kv == NULL) error->all("Cannot sort_values without KeyValue");
  if (timer) start_timer();

  compare = appcompare;
  sort_kv(1);

  stats("Sort_values",0,verbosity);

  int nkeyall;
  MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   sort values within each multivalue in a KMV, does not create a new KMV
   use appcompare() to compare 2 values within a multivalue
   each proc sorts only its data
------------------------------------------------------------------------- */

int MapReduce::sort_multivalues(int (*appcompare)(char *, int, char *, int))
{
  if (kmv == NULL) error->all("Cannot sort_multivalues without KeyMultiValue");
  if (timer) start_timer();

  int nkey = kmv->nkey;
  int *multivalues = kmv->multivalues;
  int *nvalues = kmv->nvalues;
  int *valuesizes = kmv->valuesizes;
  char *multivaluedata = kmv->multivaluedata;

  // order = ordering of values in one multivalue in KMV, initially 0 to N-1
  // will get reordered by qsort

  int *order = NULL;
  mv_values = NULL;
  int maxn = 0;
  char *multivalue_new = NULL;
  int maxmv = 0;

  for (int i = 0; i < nkey; i++) {
    int n = nvalues[i+1] - nvalues[i];
    if (n > maxn) {
      maxn = n;
      delete [] order;
      order = new int[n];
      delete [] mv_values;
      mv_values = new char*[n];
    }
    
    for (int j = 0; j < n; j++) order[j] = j;
    mv_valuesizes = &valuesizes[nvalues[i]];
    mv_values[0] = &multivaluedata[multivalues[i]];
    for (int j = 1; j < n; j++)
      mv_values[j] = mv_values[j-1] + mv_valuesizes[j-1];

    compare = appcompare;
    mrptr = this;
    qsort(order,n,sizeof(int),compare_multivalues_standalone);

    // create a new ordered multivalue, overwrite old one

    int mvsize = multivalues[i+1] - multivalues[i];
    if (mvsize > maxmv) {
      maxmv = mvsize;
      delete [] multivalue_new;
      multivalue_new = new char[mvsize];
    }

    int offset = 0;
    for (int j = 0; j < n; j++) {
      int size = mv_valuesizes[order[j]];
      memcpy(&multivalue_new[offset],mv_values[order[j]],size);
      offset += size;
    }

    memcpy(&multivaluedata[multivalues[i]],multivalue_new,mvsize);
  }

  delete [] order;
  delete [] mv_values;
  delete [] multivalue_new;

  stats("Sort_multivalues",0,verbosity);

  int nkeyall;
  MPI_Allreduce(&kmv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  return nkeyall;
}

/* ----------------------------------------------------------------------
   print stats for KV
------------------------------------------------------------------------- */

void MapReduce::kv_stats(int level)
{
  if (kv == NULL) error->all("Cannot print stats without KeyValue");

  int nkeyall;

  MPI_Allreduce(&kv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  double keysize = kv->keysize;
  double keysizeall;
  MPI_Allreduce(&keysize,&keysizeall,1,MPI_DOUBLE,MPI_SUM,comm);
  double valuesize = kv->valuesize;
  double valuesizeall;
  MPI_Allreduce(&valuesize,&valuesizeall,1,MPI_DOUBLE,MPI_SUM,comm);

  if (me == 0)
    printf("%d key/value pairs, %.1g Mb of key data, %.1g Mb of value data\n",
     nkeyall,keysizeall/1024.0/1024.0,valuesizeall/1024.0/1024.0);

  if (level == 2) {
    int histo[10],histotmp[10];
    double ave,max,min;
    double tmp = kv->nkey;
    histogram(1,&tmp,ave,max,min,10,histo,histotmp);
    if (me == 0) {
      printf("  KV pairs:   %g ave %g max %g min\n",ave,max,min);
      printf("  Histogram: ");
      for (int i = 0; i < 10; i++) printf(" %d",histo[i]);
      printf("\n");
    }
    tmp = kv->keysize/1024.0/1024.0;
    histogram(1,&tmp,ave,max,min,10,histo,histotmp);
    if (me == 0) {
      printf("  Kdata (Mb): %g ave %g max %g min\n",ave,max,min);
      printf("  Histogram: ");
      for (int i = 0; i < 10; i++) printf(" %d",histo[i]);
      printf("\n");
    }
    tmp = kv->valuesize/1024.0/1024.0;
    histogram(1,&tmp,ave,max,min,10,histo,histotmp);
    if (me == 0) {
      printf("  Vdata (Mb): %g ave %g max %g min\n",ave,max,min);
      printf("  Histogram: ");
      for (int i = 0; i < 10; i++) printf(" %d",histo[i]);
      printf("\n");
    }
  }
}

/* ----------------------------------------------------------------------
   print stats for KMV
------------------------------------------------------------------------- */

void MapReduce::kmv_stats(int level)
{
  if (kmv == NULL) error->all("Cannot print stats without KeyMultiValue");

  int nkeyall;
  MPI_Allreduce(&kmv->nkey,&nkeyall,1,MPI_INT,MPI_SUM,comm);
  double keysize = kmv->keysize;
  double keysizeall;
  MPI_Allreduce(&keysize,&keysizeall,1,MPI_DOUBLE,MPI_SUM,comm);
  double multivaluesize = kmv->multivaluesize;
  double multivaluesizeall;
  MPI_Allreduce(&multivaluesize,&multivaluesizeall,1,MPI_DOUBLE,MPI_SUM,comm);

  if (me == 0)
    printf("%d key/multi-value pairs, "
     "%.1g Mb of key data, %.1g Mb of value data\n",
     nkeyall,keysizeall/1024.0/1024.0,multivaluesizeall/1024.0/1024.0);

  if (level == 2) {
    int histo[10],histotmp[10];
    double ave,max,min;
    double tmp = kmv->nkey;
    histogram(1,&tmp,ave,max,min,10,histo,histotmp);
    if (me == 0) {
      printf("  KMV pairs:  %g ave %g max %g min\n",ave,max,min);
      printf("  Histogram: ");
      for (int i = 0; i < 10; i++) printf(" %d",histo[i]);
      printf("\n");
    }
    tmp = kmv->keysize/1024.0/1024.0;
    histogram(1,&tmp,ave,max,min,10,histo,histotmp);
    if (me == 0) {
      printf("  Kdata (Mb): %g ave %g max %g min\n",ave,max,min);
      printf("  Histogram: ");
      for (int i = 0; i < 10; i++) printf(" %d",histo[i]);
      printf("\n");
    }
    tmp = kmv->multivaluesize/1024.0/1024.0;
    histogram(1,&tmp,ave,max,min,10,histo,histotmp);
    if (me == 0) {
      printf("  Vdata (Mb): %g ave %g max %g min\n",ave,max,min);
      printf("  Histogram: ");
      for (int i = 0; i < 10; i++) printf(" %d",histo[i]);
      printf("\n");
    }
    tmp = kmv->maxdepth;
    histogram(1,&tmp,ave,max,min,10,histo,histotmp);
    if (me == 0) {
      printf("  Max bucket: %g ave %g max %g min\n",ave,max,min);
      printf("  Histogram: ");
      for (int i = 0; i < 10; i++) printf(" %d",histo[i]);
      printf("\n");
    }
  }
}

/* ----------------------------------------------------------------------
   sort keys or values in a KV to create a new KV
   flag = 0 = sort keys, flag = 1 = sort values
------------------------------------------------------------------------- */

void MapReduce::sort_kv(int flag)
{
  int nkey = kv->nkey;
  int *keys = kv->keys;
  int *values = kv->values;
  char *keydata = kv->keydata;
  char *valuedata = kv->valuedata;

  // order = ordering of keys or values in KV, initially 0 to N-1
  // will be reordered by qsort
  // use reordered order array to build a new KV, one key/value at a time

  int *order = new int[nkey];
  for (int i = 0; i < nkey; i++) order[i] = i;

  mrptr = this;
  if (flag == 0) qsort(order,nkey,sizeof(int),compare_keys_standalone);
  else qsort(order,nkey,sizeof(int),compare_values_standalone);

  KeyValue *kvnew = new KeyValue(comm);
  for (int i = 0; i < nkey; i++) {
    char *key = &keydata[keys[order[i]]];
    int keybytes = keys[order[i]+1] - keys[order[i]];
    char *value = &valuedata[values[order[i]]];
    int valuebytes = values[order[i]+1] - values[order[i]];
    kvnew->add(key,keybytes,value,valuebytes);
  }

  delete [] order;
  delete kv;
  kv = kvnew;
  kv->complete();
}

/* ----------------------------------------------------------------------
   wrappers on user-provided key and value comparison functions
   necessary so can extract 2 keys or values to pass back to application
   2-level wrapper needed b/c qsort() cannot be passed a class method
     unless it were static, but then it couldn't access MR class data
   so qsort() is passed standalone non-class method
   it accesses static class member mrptr, set before call to qsort()
   standalone calls back into class wrapper which calls user compare()
------------------------------------------------------------------------- */

extern "C" {
  int compare_keys_standalone(const void *iptr, const void *jptr)
  {
    return MapReduce::mrptr->compare_keys_wrapper(*(int *) iptr,*(int *) jptr);
  }
}

int MapReduce::compare_keys_wrapper(int i, int j)
{
  return compare(&kv->keydata[kv->keys[i]],kv->keys[i+1]-kv->keys[i],
     &kv->keydata[kv->keys[j]],kv->keys[j+1]-kv->keys[j]);
}

extern "C" {
  int compare_values_standalone(const void *iptr, const void *jptr)
  {
    return MapReduce::mrptr->compare_values_wrapper(*(int *) iptr,*(int *) jptr);
  }
}

int MapReduce::compare_values_wrapper(int i, int j)
{
  return compare(&kv->valuedata[kv->values[i]],kv->values[i+1]-kv->values[i],
     &kv->valuedata[kv->values[j]],kv->values[j+1]-kv->values[j]);
}

extern "C" {
  int compare_multivalues_standalone(const void *iptr, const void *jptr)
  {
    return MapReduce::mrptr->
      compare_multivalues_wrapper(*(int *) iptr,*(int *) jptr);
  }
}
int MapReduce::compare_multivalues_wrapper(int i, int j)
{
  return compare(mv_values[i],mv_valuesizes[i],mv_values[j],mv_valuesizes[j]);
}

/* ----------------------------------------------------------------------
   stats for either KV or KMV
------------------------------------------------------------------------- */

void MapReduce::stats(const char *heading, int which, int level)
{
  if (timer) {
    if (timer == 1) {
      MPI_Barrier(comm);
      time_stop = MPI_Wtime();
      if (me == 0) printf("%s time (secs) = %g\n",heading,time_stop-time_start);
    } else if (timer == 2) {
      time_stop = MPI_Wtime();
      int histo[10],histotmp[10];
      double ave,max,min;
      double tmp = time_stop-time_start;
      histogram(1,&tmp,ave,max,min,10,histo,histotmp);
      if (me == 0) {
  printf("%s time (secs) = %g ave %g max %g min\n",heading,ave,max,min);
  printf("  Histogram: ");
  for (int i = 0; i < 10; i++) printf(" %d",histo[i]);
  printf("\n");
      }
    }
  }

  if (level == 0) return;
  if (me == 0) printf("%s: ",heading);
  if (which == 0) kv_stats(level);
  else kmv_stats(level);
}

/* ---------------------------------------------------------------------- */

void MapReduce::histogram(int n, double *data, 
        double &ave, double &max, double &min,
        int nhisto, int *histo, int *histotmp)
{
  min = 1.0e20;
  max = -1.0e20;
  ave = 0.0;
  for (int i = 0; i < n; i++) {
    ave += data[i];
    if (data[i] < min) min = data[i];
    if (data[i] > max) max = data[i];
  }

  int ntotal;
  MPI_Allreduce(&n,&ntotal,1,MPI_INT,MPI_SUM,comm);
  double tmp;
  MPI_Allreduce(&ave,&tmp,1,MPI_DOUBLE,MPI_SUM,comm);
  ave = tmp/ntotal;
  MPI_Allreduce(&min,&tmp,1,MPI_DOUBLE,MPI_MIN,comm);
  min = tmp;
  MPI_Allreduce(&max,&tmp,1,MPI_DOUBLE,MPI_MAX,comm);
  max = tmp;

  for (int i = 0; i < nhisto; i++) histo[i] = 0;

  int m;
  double del = max - min;
  for (int i = 0; i < n; i++) {
    if (del == 0.0) m = 0;
    else m = static_cast<int> ((data[i]-min)/del * nhisto);
    if (m > nhisto-1) m = nhisto-1;
    histo[m]++;
  }

  MPI_Allreduce(histo,histotmp,nhisto,MPI_INT,MPI_SUM,comm);
  for (int i = 0; i < nhisto; i++) histo[i] = histotmp[i];
}

/* ---------------------------------------------------------------------- */

void MapReduce::start_timer()
{
  if (timer == 1) MPI_Barrier(comm);
  time_start = MPI_Wtime();
}
