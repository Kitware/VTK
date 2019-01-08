/*
Copyright (c) 1998-2017 University Corporation for Atmospheric Research/Unidata
See LICENSE.txt for license information.
*/

#ifndef NCLISTMAP_H
#define NCLISTMAP_H

#include "nclist.h"
#include "nchashmap.h"

/*
This listmap datastructure is an ordered list of objects. It is
used pervasively in libsrc to store metadata relationships.  The
goal is to provide both by-name (via nc_hashmap) and indexed
access (via NClist) to the objects in the listmap.  Using
hashmap might be overkill for some relationships, but we can
sort that out later.
As a rule, we use this to store definitional relationships
such as (in groups) dimension definitions, variable definitions, type defs
and subgroup defs. We do not, as a rule, use this to store reference relationships
such as the list of dimensions for a variable.
*/

/* Generic list + matching hashtable */
typedef struct NC_listmap {
   NClist* list;
   NC_hashmap* map;
} NC_listmap;

/* Locate object by name in an NC_listmap */
extern void* NC_listmap_get(NC_listmap* listmap, const char* name);

/* Locate object by index in an NC_listmap */
extern void* NC_listmap_iget(NC_listmap* listmap, size_t index);

/* Get the index of an object; if not present, return 0
   (=> you have to do your own presence check to avoid ambiguity)
*/
extern size_t NC_listmap_index(NC_listmap* listmap, void* obj);

/* Add object to the end of an index; assume cast (char**)obj is defined */
/* Return 1 if ok, 0 otherwise.*/
extern int NC_listmap_add(NC_listmap* listmap, void* obj);

/* Remove object from listmap; assume cast (char**)target is defined */
/* Return 1 if ok, 0 otherwise.*/
extern int NC_listmap_del(NC_listmap* listmap, void* target);

/* Remove object from listmap by index; assume cast (char**)target is defined */
/* Return 1 if ok, 0 otherwise.*/
extern int NC_listmap_idel(NC_listmap* listmap, size_t index);

/* Rehash object after it has been given a new name */
/* Return 1 if ok, 0 otherwise.*/
extern int NC_listmap_move(NC_listmap* listmap, void* obj, const char* oldname);

/* Change data associated with a key */
/* Return 1 if ok, 0 otherwise.*/
extern int NC_listmap_setdata(NC_listmap* listmap, void* obj, void* data);

/* Pseudo iterator; start index at 0, return 1 if more data, 0 if done.
   Usage:
      size_t iter;
      void* data;
      for(iter=0;NC_listmap_next(listmap,iter,&data);iter++) {f(data);}
*/
extern size_t NC_listmap_next(NC_listmap*, size_t iter, void** datap);

/* Reverse pseudo iterator; start index at 0, return 1 if more data, 0 if done.
   Differs from NC_listmap_next in that it iterates from last to first.
   This means that the iter value cannot be directly used as an index
   for e.g. NC_listmap_iget().
   Usage:
      size_t iter;
      void* data;
      for(iter=0;NC_listmap_prev(listmap,iter,&data);iter++) {f(data);}
*/
extern size_t NC_listmap_prev(NC_listmap* listmap, size_t iter, void** datap);

/* Reset a list map without free'ing the map itself */
/* Return 1 if ok; 0 otherwise */
extern int NC_listmap_clear(NC_listmap* listmap);

/* Initialize a list map without malloc'ing the map itself */
/* Return 1 if ok; 0 otherwise */
extern int NC_listmap_init(NC_listmap* listmap, size_t initsize);

extern int NC_listmap_verify(NC_listmap* lm, int dump);

/* Inline functions */

/* Test if map has been initialized */
#define NC_listmap_initialized(listmap) ((listmap)->list != NULL)

/* Get number of entries in a listmap */
/* size_t NC_listmap_size(NC_listmap* listmap) */
#define NC_listmap_size(listmap) ((listmap)==NULL?0:(nclistlength((listmap)->list)))

#endif /*NCLISTMAP_H*/
