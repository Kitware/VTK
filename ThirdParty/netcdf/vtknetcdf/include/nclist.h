/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */
#ifndef NCLIST_H
#define NCLIST_H 1

#include "vtk_netcdf_mangle.h"
/* Define the type of the elements in the list*/

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__)
extern "C" {
#endif

extern int nclistnull(void*);

typedef struct NClist {
  unsigned long alloc;
  unsigned long length;
  void** content;
} NClist;

extern NClist* nclistnew(void);
extern int nclistfree(NClist*);
extern int nclistfreeall(NClist*);
extern int nclistsetalloc(NClist*,unsigned long);
extern int nclistsetlength(NClist*,unsigned long);

/* Set the ith element */
extern int nclistset(NClist*,unsigned long,void*);
/* Get value at position i */
extern void* nclistget(NClist*,unsigned long);/* Return the ith element of l */
/* Insert at position i; will push up elements i..|seq|. */
extern int nclistinsert(NClist*,unsigned long,void*);
/* Remove element at position i; will move higher elements down */
extern void* nclistremove(NClist* l, unsigned long i);

/* Tail operations */
extern int nclistpush(NClist*,void*); /* Add at Tail */
extern void* nclistpop(NClist*);
extern void* nclisttop(NClist*);

/* Duplicate and return the content (null terminate) */
extern void** nclistdup(NClist*);

/* Look for value match */
extern int nclistcontains(NClist*, void*);

/* Remove element by value; only removes first encountered */
extern int nclistelemremove(NClist* l, void* elem);

/* remove duplicates */
extern int nclistunique(NClist*);

/* Create a clone of a list */
extern NClist* nclistclone(NClist*);

extern void* nclistextract(NClist*);

/* Following are always "in-lined"*/
#define nclistclear(l) nclistsetlength((l),0)
#define nclistextend(l,len) nclistsetalloc((l),(len)+(l->alloc))
#define nclistcontents(l)  ((l)==NULL?NULL:(l)->content)
#define nclistlength(l)  ((l)==NULL?0:(l)->length)

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__)
}
#endif

#endif /*NCLIST_H*/
