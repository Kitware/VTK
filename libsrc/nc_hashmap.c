#include "nc3internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* this should be prime */
#define TABLE_STARTSIZE 1021

#define ACTIVE 1

#define MAX(a,b) ((a) > (b) ? (a) : (b))

extern uint32_t hash_fast(const void *key, size_t length);

static int isPrime(unsigned long val)
{
  int i;

  for (i = 9; i--;)
  {
#ifdef HAVE_RANDOM
    unsigned long a = ((unsigned long)random() % (val-4)) + 2;
#else
	  unsigned long a = ((unsigned long)rand() % (val - 4)) + 2;
#endif
    unsigned long p = 1;
    unsigned long exp = val-1;
    while (exp)
    {
      if (exp & 1)
	p = (p*a)%val;

      a = (a*a)%val;
      exp >>= 1;
    }

    if (p != 1)
      return 0;
  }

  return 1;
}

static unsigned long findPrimeGreaterThan(unsigned long val)
{
  if (val & 1)
    val+=2;
  else
    val++;

  while (!isPrime(val))
    val+=2;

  return val;
}

static void rehashDim(const NC_dimarray* ncap)
{
  NC_hashmap* hm = ncap->hashmap;
  unsigned long size = hm->size;
  unsigned long count = hm->count;

  hEntry* table = hm->table;

  hm->size = findPrimeGreaterThan(size<<1);
  hm->table = (hEntry*)calloc(sizeof(hEntry), hm->size);
  hm->count = 0;

  while(size > 0) {
    --size;
    if (table[size].flags == ACTIVE) {
      NC_dim *elem = ncap->value[table[size].data-1];
      NC_hashmapAddDim(ncap, table[size].data-1, elem->name->cp);
      assert(NC_hashmapGetDim(ncap, elem->name->cp) == table[size].data-1);
    }
  }

  free(table);
  assert(count == hm->count);
}

static void rehashVar(const NC_vararray* ncap)
{
  NC_hashmap* hm = ncap->hashmap;
  unsigned long size = hm->size;
  unsigned long count = hm->count;

  hEntry* table = hm->table;

  hm->size = findPrimeGreaterThan(size<<1);
  hm->table = (hEntry*)calloc(sizeof(hEntry), (size_t)hm->size);
  hm->count = 0;

  while(size > 0) {
    --size;
    if (table[size].flags == ACTIVE) {
      NC_var *elem = ncap->value[table[size].data-1];
      NC_hashmapAddVar(ncap, table[size].data-1, elem->name->cp);
      assert(NC_hashmapGetVar(ncap, elem->name->cp) == table[size].data-1);
    }
  }

  free(table);
  assert(count == hm->count);
}

NC_hashmap* NC_hashmapCreate(unsigned long startsize)
{
  NC_hashmap* hm = (NC_hashmap*)malloc(sizeof(NC_hashmap));

  if (!startsize)
    startsize = TABLE_STARTSIZE;
  else {
    startsize *= 4;
    startsize /= 3;
    startsize = findPrimeGreaterThan(startsize-2);
  }

  hm->table = (hEntry*)calloc(sizeof(hEntry), (size_t)startsize);
  hm->size = startsize;
  hm->count = 0;

  return hm;
}

void NC_hashmapAddDim(const NC_dimarray* ncap, long data, const char *name)
{
  unsigned long key = hash_fast(name, strlen(name));
  NC_hashmap* hash = ncap->hashmap;

  if (hash->size*3/4 <= hash->count) {
    rehashDim(ncap);
  }

  do
  {
    unsigned long i;
    unsigned long index = key % hash->size;
    unsigned long step = (key % MAX(1,(hash->size-2))) + 1;

    for (i = 0; i < hash->size; i++)
    {
      if (hash->table[index].flags & ACTIVE)
      {
	hEntry entry = hash->table[index];
	if (entry.key == key &&
	    strncmp(name, ncap->value[entry.data-1]->name->cp,
		    ncap->value[entry.data-1]->name->nchars) == 0)
	  {
	  hash->table[index].data = data+1;
	  return;
	}
      }
      else
      {
	hash->table[index].flags |= ACTIVE;
	hash->table[index].data = data+1;
	hash->table[index].key = key;
	++hash->count;
	return;
      }

      index = (index + step) % hash->size;
    }

    /* it should not be possible that we EVER come this far, but unfortunately
       not every generated prime number is prime (Carmichael numbers...) */
    rehashDim(ncap);
  }
  while (1);
}

void NC_hashmapAddVar(const NC_vararray* ncap, long data, const char *name)
{
  unsigned long key = hash_fast(name, strlen(name));
  NC_hashmap* hash = ncap->hashmap;

  if (hash->size*3/4 <= hash->count) {
    rehashVar(ncap);
  }

  do
  {
    unsigned long i;
    unsigned long index = key % hash->size;
    unsigned long step = (key % MAX(1,(hash->size-2))) + 1;

    for (i = 0; i < hash->size; i++)
    {
      if (hash->table[index].flags & ACTIVE)
      {
	hEntry entry = hash->table[index];
	if (entry.key == key &&
	    strncmp(name, ncap->value[entry.data-1]->name->cp,
		    ncap->value[entry.data-1]->name->nchars) == 0)
	{
	  hash->table[index].data = data+1;
	  return;
	}
      }
      else
      {
	hash->table[index].flags |= ACTIVE;
	hash->table[index].data = data+1;
	hash->table[index].key = key;
	++hash->count;
	return;
      }

      index = (index + step) % hash->size;
    }

    /* it should not be possible that we EVER come this far, but unfortunately
       not every generated prime number is prime (Carmichael numbers...) */
    rehashVar(ncap);
  }
  while (1);
}

long NC_hashmapRemoveDim(const NC_dimarray* ncap, const char *name)
{
  unsigned long i;
  unsigned long key = hash_fast(name, strlen(name));
  NC_hashmap* hash = ncap->hashmap;

  unsigned long index = key % hash->size;
  unsigned long step = (key % (hash->size-2)) + 1;

  for (i = 0; i < hash->size; i++)
  {
    if (hash->table[index].data > 0)
    {
      hEntry entry = hash->table[index];
      if (entry.key == key &&
	  strncmp(name, ncap->value[entry.data-1]->name->cp,
		  ncap->value[entry.data-1]->name->nchars) == 0)
      {
	if (hash->table[index].flags & ACTIVE)
	{
	  hash->table[index].flags &= ~ACTIVE;
	  --hash->count;
	  return hash->table[index].data-1;
	}
	else /* in, but not active (i.e. deleted) */
	  return -1;
      }
    }
    else /* found an empty place (can't be in) */
      return -1;

    index = (index + step) % hash->size;
  }
  /* everything searched through, but not in */
  return -1;
}

long NC_hashmapRemoveVar(const NC_vararray* ncap, const char *name)
{
  unsigned long i;
  unsigned long key = hash_fast(name, strlen(name));
  NC_hashmap* hash = ncap->hashmap;

  unsigned long index = key % hash->size;
  unsigned long step = (key % (hash->size-2)) + 1;

  for (i = 0; i < hash->size; i++)
  {
    if (hash->table[index].data > 0)
    {
      hEntry entry = hash->table[index];
      if (entry.key == key &&
	  strncmp(name, ncap->value[entry.data-1]->name->cp,
		  ncap->value[entry.data-1]->name->nchars) == 0)
      {
	if (hash->table[index].flags & ACTIVE)
	{
	  hash->table[index].flags &= ~ACTIVE;
	  --hash->count;
	  return hash->table[index].data-1;
	}
	else /* in, but not active (i.e. deleted) */
	  return -1;
      }
    }
    else /* found an empty place (can't be in) */
      return -1;

    index = (index + step) % hash->size;
  }
  /* everything searched through, but not in */
  return -1;
}

long NC_hashmapGetDim(const NC_dimarray* ncap, const char *name)
{
  NC_hashmap* hash = ncap->hashmap;
  if (hash->count)
  {
    unsigned long key = hash_fast(name, strlen(name));
    NC_hashmap* hash = ncap->hashmap;

    unsigned long i;
    unsigned long index = key % hash->size;
    unsigned long step = (key % (hash->size-2)) + 1;

    for (i = 0; i < hash->size; i++)
    {
      hEntry entry = hash->table[index];
      if (entry.key == key &&
	  strncmp(name, ncap->value[entry.data-1]->name->cp,
		  ncap->value[entry.data-1]->name->nchars) == 0)
      {
	if (entry.flags & ACTIVE)
	  return entry.data-1;
	break;
      }
      else
	if (!(entry.flags & ACTIVE))
	  break;

      index = (index + step) % hash->size;
    }
  }

  return -1;
}

long NC_hashmapGetVar(const NC_vararray* ncap, const char *name)
{
  NC_hashmap* hash = ncap->hashmap;
  if (hash->count)
  {
    unsigned long key = hash_fast(name, strlen(name));
    NC_hashmap* hash = ncap->hashmap;

    unsigned long i;
    unsigned long index = key % hash->size;
    unsigned long step = (key % (hash->size-2)) + 1;

    for (i = 0; i < hash->size; i++)
    {
      hEntry entry = hash->table[index];
      if (entry.key == key &&
	  strncmp(name, ncap->value[entry.data-1]->name->cp,
		  ncap->value[entry.data-1]->name->nchars) == 0)
      {
	if (entry.flags & ACTIVE)
	  return entry.data-1;
	break;
      }
      else
	if (!(entry.flags & ACTIVE))
	  break;

      index = (index + step) % hash->size;
    }
  }

  return -1;
}

unsigned long NC_hashmapCount(NC_hashmap* hash)
{
  return hash->count;
}

void NC_hashmapDelete(NC_hashmap* hash)
{
  if (hash) {
    free(hash->table);
    free(hash);
  }
}
