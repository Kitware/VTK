/*********************************************************************
 *   Copyright 1993, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header$
 *********************************************************************/
#ifndef NCHASHMAP_H
#define NCHASHMAP_H

/*
Data is presumed to be an index into some other table
   Assume it can be compared using simple ==
The key is some hash of some null terminated string.
*/

/*! Hashmap-related structs.
  NOTES:
  1. 'data' is the dimid or varid which is non-negative.
  2. 'key' is a copy of the name (char*) of the corresponding object
     (e.g. dim or var)
  3. hashkey is a hash of key.
*/
typedef struct NC_hentry {
    int flags;
    void* data;
    size_t hashkey; /* Hash id */
    char* key; /* actual key; do not free */
} NC_hentry;

/*
The hashmap object must give us the hash table (table),
the |table| size, and the # of defined entries in the table
*/
typedef struct NC_hashmap {
  size_t size; /* allocated */
  size_t count;
  NC_hentry* table;
} NC_hashmap;

/* defined in nc_hashmap.c */

/** Creates a new hashmap near the given size. */
extern NC_hashmap* NC_hashmapnew(size_t startsize);

/** Inserts a new element into the hashmap. */
/* Note we pass the NC_hobjecty struct by value */
extern int NC_hashmapadd(NC_hashmap*, void* data, const char* name);

/** Removes the storage for the element of the key.
    Return 1 if found, 0 otherwise; returns the data in datap if !null
*/
extern int NC_hashmapremove(NC_hashmap*, const char* name, void** datap);

/** Returns the data for the key.
    Return 1 if found, 0 otherwise; returns the data in datap if !null
*/
extern int NC_hashmapget(NC_hashmap*, const char*, void** datap);

/** Change the data for the specified key
    Return 1 if found, 0 otherwise
*/
extern int NC_hashmapsetdata(NC_hashmap*, const char*, void* newdata);

/** Returns the number of saved elements. */
extern size_t NC_hashmapcount(NC_hashmap*);

/** Reclaims the hashmap structure. */
extern int NC_hashmapfree(NC_hashmap*);

#endif /*NCHASHMAP_H*/

