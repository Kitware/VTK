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
#include "keyvalue.h"
#include "memory.h"
#include "error.h"

using namespace MAPREDUCE_NS;

#define KEYCHUNK 25000000
#define BUFCHUNK 25000000

/* ---------------------------------------------------------------------- */

KeyValue::KeyValue(MPI_Comm caller)
{
  comm = caller;
  memory = new Memory(comm);
  error = new Error(comm);

  nkey = maxkey = 0;
  keysize = maxkeysize = 0;
  valuesize = maxvaluesize = 0;

  keys = NULL;
  values = NULL;
  keydata = NULL;
  valuedata = NULL;
}

/* ----------------------------------------------------------------------
   copy constructor
------------------------------------------------------------------------- */

KeyValue::KeyValue(KeyValue &kv)
{
  memory = new Memory(kv.comm);
  error = new Error(kv.comm);

  nkey = kv.nkey;
  maxkey = kv.maxkey;
  keysize = kv.keysize;
  maxkeysize = kv.maxkeysize;
  valuesize = kv.valuesize;
  maxvaluesize = kv.maxvaluesize;

  keys = (int *) memory->smalloc(maxkey*sizeof(int),"KV:keys");
  values = (int *) memory->smalloc(maxkey*sizeof(int),"KV:values");
  keydata = (char *) memory->smalloc(maxkeysize,"KV:keydata");
  valuedata = (char *) memory->smalloc(maxvaluesize,"KV:valuedata");

  memcpy(keys,kv.keys,(nkey+1)*sizeof(int));
  memcpy(values,kv.values,(nkey+1)*sizeof(int));
  memcpy(keydata,kv.keydata,keysize);
  memcpy(valuedata,kv.valuedata,valuesize);
}

/* ---------------------------------------------------------------------- */

KeyValue::~KeyValue()
{
  delete memory;
  delete error;
  memory->sfree(keys);
  memory->sfree(values);
  memory->sfree(keydata);
  memory->sfree(valuedata);
}

/* ----------------------------------------------------------------------
   add a single key/value pair to list
   grow memory as needed
------------------------------------------------------------------------- */

void KeyValue::add(char *key, int keybytes, char *value, int valuebytes)
{
  if (nkey == maxkey) {
    maxkey += KEYCHUNK;
    keys = (int *) memory->srealloc(keys,maxkey*sizeof(int),"KV:keys");
    values = (int *) memory->srealloc(values,maxkey*sizeof(int),"KV:values");
  }
  if (keysize+keybytes >= maxkeysize) {
    maxkeysize += BUFCHUNK;
    keydata = (char *) memory->srealloc(keydata,maxkeysize,"KV:keydata");
  }
  if (valuesize+valuebytes >= maxvaluesize) {
    maxvaluesize += BUFCHUNK;
    valuedata = (char *) memory->srealloc(valuedata,maxvaluesize,
            "KV:valuedata");
  }

  keys[nkey] = keysize;
  memcpy(&keydata[keys[nkey]],key,keybytes);
  keysize += keybytes;

  values[nkey] = valuesize;
  memcpy(&valuedata[values[nkey]],value,valuebytes);
  valuesize += valuebytes;

  nkey++;
}

/* ----------------------------------------------------------------------
   add N fixed-length key/value pairs to list
   grow memory as needed
------------------------------------------------------------------------- */

void KeyValue::add(int n, char *key, int keybytes,
       char *value, int valuebytes)
{
  if (nkey+n >= maxkey) {
    while (nkey+n >= maxkey) maxkey += KEYCHUNK;
    keys = (int *) memory->srealloc(keys,maxkey*sizeof(int),"KV:keys");
    values = (int *) memory->srealloc(values,maxkey*sizeof(int),"KV:values");
  }

  for (int i = 0; i < n; i++) {
    keys[nkey+i] = keysize;
    keysize += keybytes;
    values[nkey+i] = valuesize;
    valuesize += valuebytes;
  }

  if (keysize > maxkeysize) {
    while (keysize >= maxkeysize) maxkeysize += BUFCHUNK;
    keydata = (char *) memory->srealloc(keydata,maxkeysize,"KV:keydata");
  }
  if (valuesize > maxvaluesize) {
    while (valuesize >= maxvaluesize) maxvaluesize += BUFCHUNK;
    valuedata = (char *) memory->srealloc(valuedata,maxvaluesize,
            "KV:valuedata");
  }

  memcpy(&keydata[keys[nkey]],key,n*keybytes);
  memcpy(&valuedata[values[nkey]],value,n*valuebytes);
  nkey += n;
}

/* ----------------------------------------------------------------------
   add N variable-length key/value pairs to list
   grow memory as needed
------------------------------------------------------------------------- */

void KeyValue::add(int n, char *key, int *keybytes,
       char *value, int *valuebytes)
{
  if (nkey+n >= maxkey) {
    while (nkey+n >= maxkey) maxkey += KEYCHUNK;
    keys = (int *) memory->srealloc(keys,maxkey*sizeof(int),"KV:keys");
    values = (int *) memory->srealloc(values,maxkey*sizeof(int),"KV:values");
  }

  int keystart = keysize;
  int valuestart = valuesize;
  for (int i = 0; i < n; i++) {
    keys[nkey+i] = keysize;
    keysize += keybytes[i];
    values[nkey+i] = valuesize;
    valuesize += valuebytes[i];
  }

  if (keysize > maxkeysize) {
    while (keysize >= maxkeysize) maxkeysize += BUFCHUNK;
    keydata = (char *) memory->srealloc(keydata,maxkeysize,"KV:keydata");
  }
  if (valuesize > maxvaluesize) {
    while (valuesize >= maxvaluesize) maxvaluesize += BUFCHUNK;
    valuedata = (char *) memory->srealloc(valuedata,maxvaluesize,
            "KV:valuedata");
  }

  memcpy(&keydata[keys[nkey]],key,keysize-keystart);
  memcpy(&valuedata[values[nkey]],value,valuesize-valuestart);
  nkey += n;
}

/* ----------------------------------------------------------------------
   add key/value pairs from another KV to list
   grow memory as needed
------------------------------------------------------------------------- */

void KeyValue::add(KeyValue *kv)
{
  if (kv == NULL) error->all("Cannot add a KeyValue that does not exist");

  // grow memory as needed
  
  if (nkey + kv->nkey + 1 >= maxkey) {
    while (nkey+kv->nkey+1 >= maxkey) maxkey += KEYCHUNK;
    keys = (int *) memory->srealloc(keys,maxkey*sizeof(int),"KV:keys");
    values = (int *) memory->srealloc(values,maxkey*sizeof(int),"KV:values");
  }
  if (keysize + kv->keysize > maxkeysize) {
    while (keysize+kv->keysize >= maxkeysize) maxkeysize += BUFCHUNK;
    keydata = (char *) memory->srealloc(keydata,maxkeysize,"KV:keydata");
  }
  if (valuesize + kv->valuesize > maxvaluesize) {
    while (valuesize+kv->valuesize >= maxvaluesize) maxvaluesize += BUFCHUNK;
    valuedata = (char *) memory->srealloc(valuedata,maxvaluesize,
            "KV:valuedata");
  }

  // add other KV data to existing KV data

  memcpy(&keys[nkey],kv->keys,(kv->nkey+1)*sizeof(int));
  memcpy(&values[nkey],kv->values,(kv->nkey+1)*sizeof(int));
  memcpy(&keydata[keysize],kv->keydata,kv->keysize);
  memcpy(&valuedata[valuesize],kv->valuedata,kv->valuesize);

  // add offset to new key/value indices

  int nkey_new = nkey + kv->nkey;
  for (int i = nkey; i <= nkey_new; i++) {
    keys[i] += keysize;
    values[i] += valuesize;
  }

  keysize += kv->keysize;
  valuesize += kv->valuesize;
  nkey += kv->nkey;
}

/* ----------------------------------------------------------------------
   pack my KV data into buf to ship to another proc
   return size of reallocated buf in bytes
------------------------------------------------------------------------- */

int KeyValue::pack(char **pbuf)
{
  int n = 3*sizeof(int);
  n += 2*(nkey+1) * sizeof(int);
  n += keysize + valuesize;

  char *buf = new char[n];

  n = 0;
  memcpy(&buf[n],&nkey,sizeof(int));
  n += sizeof(int);
  memcpy(&buf[n],&keysize,sizeof(int));
  n += sizeof(int);
  memcpy(&buf[n],&valuesize,sizeof(int));
  n += sizeof(int);

  memcpy(&buf[n],keys,(nkey+1)*sizeof(int));
  n += (nkey+1)*sizeof(int);
  memcpy(&buf[n],values,(nkey+1)*sizeof(int));
  n += (nkey+1)*sizeof(int);

  memcpy(&buf[n],keydata,keysize);
  n += keysize;
  memcpy(&buf[n],valuedata,valuesize);
  n += valuesize;

  *pbuf = buf;
  return n;
}

/* ----------------------------------------------------------------------
   unpack a buffer of KV data received from another proc
   add key/value pairs to my KV
------------------------------------------------------------------------- */

void KeyValue::unpack(char *buf)
{
  int *nkey_new,*keysize_new,*valuesize_new;
  int *keys_new,*values_new;
  char *keydata_new,*valuedata_new;

  int n = 0;
  nkey_new = (int *) &buf[n];
  n += sizeof(int);
  keysize_new = (int *) &buf[n];
  n += sizeof(int);
  valuesize_new = (int *) &buf[n];
  n += sizeof(int);

  keys_new = (int *) &buf[n];
  n += (*nkey_new+1)*sizeof(int);
  values_new = (int *) &buf[n];
  n += (*nkey_new+1)*sizeof(int);

  keydata_new = &buf[n];
  n += *keysize_new;
  valuedata_new = &buf[n];
  // n += *valuesize_new;

  // grow memory as needed

  if (nkey + *nkey_new >= maxkey) {
    maxkey = nkey + *nkey_new;
    keys = (int *) memory->srealloc(keys,maxkey*sizeof(int),"KV:keys");
    values = (int *) memory->srealloc(values,maxkey*sizeof(int),"KV:values");
  }
  if (keysize + *keysize_new > maxkeysize) {
    maxkeysize = keysize + *keysize_new;
    keydata = (char *) memory->srealloc(keydata,maxkeysize,"KV:keydata");
  }
  if (valuesize + *valuesize_new > maxvaluesize) {
    maxvaluesize = valuesize + *valuesize_new;
    valuedata = (char *) memory->srealloc(valuedata,maxvaluesize,
            "KV:valuedata");
  }

  // add offset to new key/value indices

  for (int i = 0; i <= *nkey_new; i++) {
    keys_new[i] += keysize;
    values_new[i] += valuesize;
  }
  
  // add new KV data to existing KV data

  memcpy(&keys[nkey],keys_new,(*nkey_new+1)*sizeof(int));
  memcpy(&values[nkey],values_new,(*nkey_new+1)*sizeof(int));
  memcpy(&keydata[keysize],keydata_new,*keysize_new);
  memcpy(&valuedata[valuesize],valuedata_new,*valuesize_new);

  keysize += *keysize_new;
  valuesize += *valuesize_new;
  nkey += *nkey_new;
}

/* ----------------------------------------------------------------------
   set key/value ptrs for N+1 location
   so can infer length of each key and value
   do NOT change nkey
------------------------------------------------------------------------- */

void KeyValue::complete()
{
  if (nkey == maxkey) {
    maxkey += KEYCHUNK;
    keys = (int *) memory->srealloc(keys,maxkey*sizeof(int),"KV:keys");
    values = (int *) memory->srealloc(values,maxkey*sizeof(int),"KV:values");
  }

  keys[nkey] = keysize;
  values[nkey] = valuesize;
}
