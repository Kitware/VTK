/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */
#ifndef NCLIST_H
#define NCLIST_H 1

/* Define the type of the elements in the list*/

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__)
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

typedef unsigned long ncelem;

EXTERNC int nclistnull(ncelem);

typedef struct NClist {
  unsigned int alloc;
  unsigned int length;
  ncelem* content;
} NClist;

EXTERNC NClist* nclistnew(void);
EXTERNC int nclistfree(NClist*);
EXTERNC int nclistsetalloc(NClist*,unsigned int);
EXTERNC int nclistsetlength(NClist*,unsigned int);

/* Set the ith element */
EXTERNC int nclistset(NClist*,unsigned int,ncelem);
/* Get value at position i */
EXTERNC ncelem nclistget(NClist*,unsigned int);/* Return the ith element of l */
/* Insert at position i; will push up elements i..|seq|. */
EXTERNC int nclistinsert(NClist*,unsigned int,ncelem);
/* Remove element at position i; will move higher elements down */
EXTERNC ncelem nclistremove(NClist* l, unsigned int i);

/* Tail operations */
EXTERNC int nclistpush(NClist*,ncelem); /* Add at Tail */
EXTERNC ncelem nclistpop(NClist*);
EXTERNC ncelem nclisttop(NClist*);

/* Duplicate and return the content (null terminate) */
EXTERNC ncelem* nclistdup(NClist*);

/* Look for value match */
EXTERNC int nclistcontains(NClist*, ncelem);

/* remove duplicates */
EXTERNC int nclistunique(NClist*);

/* Create a clone of a list */
EXTERNC NClist* nclistclone(NClist*);

/* Following are always "in-lined"*/
#define nclistclear(l) nclistsetlength((l),0U)
#define nclistextend(l,len) nclistsetalloc((l),(len)+(l->alloc))
#define nclistcontents(l) ((l)->content)
#define nclistlength(l)  ((l)?(l)->length:0U)

#endif /*NCLIST_H*/
