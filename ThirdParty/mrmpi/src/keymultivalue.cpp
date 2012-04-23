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
#include "string.h"
#include "vtkType.h"
#include "keymultivalue.h"
#include "keyvalue.h"
#include "hash.h"
#include "memory.h"

using namespace MAPREDUCE_NS;

#ifdef MIN
#  undef MIN
#endif
#define MIN(A,B) ((A) < (B)) ? (A) : (B)

#ifdef MAX
#  undef MAX
#endif 
#define MAX(A,B) ((A) > (B)) ? (A) : (B)

#define KEYCHUNK 25000000
#define BUCKETDEPTH 2

/* ---------------------------------------------------------------------- */

KeyMultiValue::KeyMultiValue(MPI_Comm caller)
{
  comm = caller;
  memory = new Memory(comm);

  nkey = 0;
  keysize = 0;
  multivaluesize = 0;

  keys = NULL;
  multivalues = NULL;
  nvalues = NULL;
  valuesizes = NULL;
  keydata = NULL;
  multivaluedata = NULL;

  maxdepth = 0;
}

/* ----------------------------------------------------------------------
   copy constructor
------------------------------------------------------------------------- */

KeyMultiValue::KeyMultiValue(KeyMultiValue &kmv)
{
  comm = kmv.comm;
  memory = new Memory(comm);

  nkey = kmv.nkey;
  keysize = kmv.keysize;
  multivaluesize = kmv.multivaluesize;

  keys = (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:keys");
  multivalues = 
    (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:multivalues");
  nvalues = (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:nvalues");
  valuesizes = (int *) memory->smalloc(nkey*sizeof(int),"KMV:valuesizes");
  keydata = (char *) memory->smalloc(keysize,"KMV:keydata");
  multivaluedata = 
    (char *) memory->smalloc(multivaluesize,"KMV:multivaluedata");

  memcpy(keys,kmv.keys,(nkey+1)*sizeof(int));
  memcpy(multivalues,kmv.multivalues,(nkey+1)*sizeof(int));
  memcpy(nvalues,kmv.nvalues,(nkey+1)*sizeof(int));
  memcpy(valuesizes,kmv.valuesizes,nkey*sizeof(int));
  memcpy(keydata,kmv.keydata,keysize);
  memcpy(multivaluedata,kmv.multivaluedata,multivaluesize);

  maxdepth = kmv.maxdepth;
}

/* ---------------------------------------------------------------------- */

KeyMultiValue::~KeyMultiValue()
{
  delete memory;

  memory->sfree(keys);
  memory->sfree(multivalues);
  memory->sfree(nvalues);
  memory->sfree(valuesizes);
  memory->sfree(keydata);
  memory->sfree(multivaluedata);
}

/* ----------------------------------------------------------------------
   convert a KV with non-unique keys into a KMV with unique keys
   each processor works on only its data
------------------------------------------------------------------------- */

void KeyMultiValue::convert(KeyValue *kv)
{
  int kv_nkey = kv->nkey;
  int *kv_keys = kv->keys;
  int *kv_values = kv->values;
  char *kv_keydata = kv->keydata;
  char *kv_valuedata = kv->valuedata;

  // allocate empty hash

  nbuckets = 1;
  hashmask = nbuckets-1;
  buckets = (int *) memory->smalloc(nbuckets*sizeof(int),"KMV:buckets");
  buckets[0] = -1;
  maxdepth = 0;

  nunique = maxunique = 0;
  uniques = NULL;

  // use hash to identify unique keys
  // for each key, either create new unique or increment nvalue count

  for (int i = 0; i < kv_nkey; i++) {
    char *key = &kv_keydata[kv_keys[i]];
    int keybytes = kv_keys[i+1] - kv_keys[i];
    int ibucket = hash(key,keybytes);
    int ikey = find(ibucket,key,keybytes,kv);

    if (ikey < 0) {
      if (nunique == maxunique) {
  maxunique += KEYCHUNK;
  uniques = (Unique *)
    memory->srealloc(uniques,maxunique*sizeof(Unique),"KMV:uniques");
      }
      uniques[nunique].keyindex = i;
      uniques[nunique].mvsize = kv_values[i+1] - kv_values[i];
      uniques[nunique].nvalue = 1;
      uniques[nunique].next = -1;
      nunique++;
      if (nunique > BUCKETDEPTH*nbuckets) grow_buckets(kv);

    } else {
      uniques[ikey].mvsize += kv_values[i+1] - kv_values[i];
      uniques[ikey].nvalue++;
    }
  }

  // setup keys and keydata for KMV from each unique key in KV

  nkey = nunique;
  keys = (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:keys");

  keys[0] = 0;
  for (int i = 1; i <= nunique; i++)
    keys[i] = keys[i-1] + 
      kv_keys[uniques[i-1].keyindex+1] - kv_keys[uniques[i-1].keyindex];
  keysize = keys[nkey];

  keydata = (char *) memory->smalloc(keysize,"KMV:keydata");

  for (int i = 0; i < nunique; i++)
    memcpy(&keydata[keys[i]],&kv_keydata[kv_keys[uniques[i].keyindex]],
     keys[i+1]-keys[i]);

  // setup multivalues & nvalues for KMV from unique mvsize & nvalue

  multivalues = 
    (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:multivalues");
  nvalues = (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:nvalues");

  multivalues[0] = 0;
  nvalues[0] = 0;
  for (int i = 1; i <= nunique; i++) {
    multivalues[i] = multivalues[i-1] + uniques[i-1].mvsize;
    nvalues[i] = nvalues[i-1] + uniques[i-1].nvalue;
  }

  // setup valuesizes and multivaluedata for KMV
  // rehash KV key to find corresponding unique entry
  // copy KV values one at a time into KMV multivalue at offset
  // zero unique mvsize & nvalue so can use to compute offsets

  multivaluesize = kv->valuesize;
  valuesizes = (int *) memory->smalloc(kv_nkey*sizeof(int),"KMV:valuesizes");
  multivaluedata = 
    (char *) memory->smalloc(multivaluesize,"KMV:multivaluedata");

  for (int i = 0; i < nunique; i++) {
    uniques[i].mvsize = 0;
    uniques[i].nvalue = 0;
  }

  for (int i = 0; i < kv_nkey; i++) {
    char *key = &kv_keydata[kv_keys[i]];
    int keybytes = kv_keys[i+1] - kv_keys[i];
    int ibucket = hash(key,keybytes);
    int ikey = find(ibucket,key,keybytes,kv);

    char *value = &kv_valuedata[kv_values[i]];
    int valuebytes = kv_values[i+1] - kv_values[i];
    int offset = multivalues[ikey] + uniques[ikey].mvsize;
    memcpy(&multivaluedata[offset],value,valuebytes);
    uniques[ikey].mvsize += valuebytes;
    offset = nvalues[ikey] + uniques[ikey].nvalue;
    valuesizes[offset] = valuebytes;
    uniques[ikey].nvalue++;
  }

  // clean up

  memory->sfree(buckets);
  memory->sfree(uniques);
}

/* ----------------------------------------------------------------------
   clone a KV directly into a KMV, one KV pair -> one KMV pair
   each processor works on only its data
------------------------------------------------------------------------- */

void KeyMultiValue::clone(KeyValue *kv)
{
  nkey = kv->nkey;
  keysize = kv->keysize;
  multivaluesize = kv->valuesize;

  keys = (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:keys");
  multivalues = 
    (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:multivalues");
  nvalues = (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:nvalues");
  valuesizes = (int *) memory->smalloc(nkey*sizeof(int),"KMV:valuesizes");
  keydata = (char *) memory->smalloc(keysize,"KMV:keydata");
  multivaluedata = 
    (char *) memory->smalloc(multivaluesize,"KMV:multivaluedata");

  memcpy(keys,kv->keys,(nkey+1)*sizeof(int));
  memcpy(multivalues,kv->values,(nkey+1)*sizeof(int));
  for (int i = 0; i <= nkey; i++) nvalues[i] = i;
  for (int i = 0; i < nkey; i++)
    valuesizes[i] = multivalues[i+1] - multivalues[i];
  memcpy(keydata,kv->keydata,keysize);
  memcpy(multivaluedata,kv->valuedata,multivaluesize);
}

/* ----------------------------------------------------------------------
   collapse a KV into a KMV with one KMV pair
   new KMV key = key, new KMV multivalue = KV key,value,key,value,...
   each processor works on only its data
------------------------------------------------------------------------- */

void KeyMultiValue::collapse(char *key, int keybytes, KeyValue *kv)
{
  nkey = 1;
  keysize = keybytes;
  multivaluesize = kv->keysize + kv->valuesize;

  keys = (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:keys");
  multivalues = 
    (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:multivalues");
  nvalues = (int *) memory->smalloc((nkey+1)*sizeof(int),"KMV:nvalues");
  valuesizes =
    (int *) memory->smalloc(2*kv->nkey*sizeof(int),"KMV:valuesizes");
  keydata = (char *) memory->smalloc(keysize,"KMV:keydata");
  multivaluedata = 
    (char *) memory->smalloc(multivaluesize,"KMV:multivaluedata");

  keys[0] = 0;
  keys[1] = keybytes;
  multivalues[0] = 0;
  multivalues[1] = multivaluesize;
  nvalues[0] = 0;
  nvalues[1] = 2 * kv->nkey;

  memcpy(keydata,key,keybytes);

  // copy KV keys and values into single KMV multivalue

  int kv_nkey = kv->nkey;
  int *kv_keys = kv->keys;
  int *kv_values = kv->values;
  char *kv_keydata = kv->keydata;
  char *kv_valuedata = kv->valuedata;

  int size;
  int offsetsize = 0;
  int offsetdata = 0;
  for (int i = 0; i < kv_nkey; i++) {
    size = kv_keys[i+1] - kv_keys[i];
    valuesizes[offsetsize++] = size;
    memcpy(&multivaluedata[offsetdata],&kv_keydata[kv_keys[i]],size);
    offsetdata += size;
    size = kv_values[i+1] - kv_values[i];
    valuesizes[offsetsize++] = size;
    memcpy(&multivaluedata[offsetdata],&kv_valuedata[kv_values[i]],size);
    offsetdata += size;
  }
}

/* ----------------------------------------------------------------------
   double the number of hash buckets
   requires re-hashing all keys to put them in new buckets
------------------------------------------------------------------------- */

void KeyMultiValue::grow_buckets(KeyValue *kv)
{
  nbuckets *= 2;
  hashmask = nbuckets-1;
  maxdepth = 0;
  memory->sfree(buckets);
  buckets = (int *) memory->smalloc(nbuckets*sizeof(int),"KMV:buckets");
  for (int i = 0; i < nbuckets; i++) buckets[i] = -1;

  // rehash current unique keys

  int *kv_keys = kv->keys;
  char *kv_keydata = kv->keydata;

  for (int i = 0; i < nunique; i++) {
    uniques[i].next = -1;
    int ikey = uniques[i].keyindex;
    char *key = &kv_keydata[kv_keys[ikey]];
    int keybytes = kv_keys[ikey+1] - kv_keys[ikey];
    int ibucket = hash(key,keybytes);

    int depth = 1;
    if (buckets[ibucket] < 0) buckets[ibucket] = i;
    else {
      int iprevious = -1;
      int ikey2 = buckets[ibucket];
      while (ikey2 >= 0) {
        iprevious = ikey2;
        ikey2 = uniques[ikey2].next;
        depth++;
      }
      if (iprevious >= 0)
        uniques[iprevious].next = i;
    }
    maxdepth = MAX(maxdepth,depth);
  }
}

/* ----------------------------------------------------------------------
   find a Unique in ibucket that matches key
   return index of Unique
   if cannot find key, return -1
   if not found, also update pointers for Unique that will be added
------------------------------------------------------------------------- */

int KeyMultiValue::find(int ibucket, char *key, int keybytes, KeyValue *kv)
{
  int ikey = buckets[ibucket];

  if (ikey < 0) {
    buckets[ibucket] = nunique;
    maxdepth = MAX(maxdepth,1);
    return -1;
  }
  
  int depth = 1;
  while (ikey >= 0) {
    int index = uniques[ikey].keyindex;
    char *key2 = &kv->keydata[kv->keys[index]];
    int key2len = kv->keys[index+1] - kv->keys[index];
    if (keybytes == key2len && match(key,key2,keybytes)) return ikey;
    int next = uniques[ikey].next;
    if (next < 0) uniques[ikey].next = nunique;
    ikey = next;
    depth++;
  }

  maxdepth = MAX(maxdepth,depth);
  return -1;
}

/* ----------------------------------------------------------------------
   check match of 2 keys
   return 1 if a match, else 0
------------------------------------------------------------------------- */

int KeyMultiValue::match(char *key1, char *key2, int keybytes)
{
  for (int i = 0; i < keybytes; i++)
    if (key1[i] != key2[i]) return 0;
  return 1;
}

/* ----------------------------------------------------------------------
   hash a key to a bucket
------------------------------------------------------------------------- */

int KeyMultiValue::hash(char *key, int keybytes)
{
  vtkTypeUInt32 ubucket = hashlittle(key,keybytes,0);
  int ibucket = ubucket & hashmask;
  return ibucket;
}
