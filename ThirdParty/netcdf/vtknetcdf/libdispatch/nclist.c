/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nclist.h"

static ncelem ncDATANULL = (ncelem)0;
/*static int ncinitialized=0;*/

int nclistnull(ncelem e) {return e == ncDATANULL;}

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEFAULTALLOC 16
#define ALLOCINCR 16

NClist* nclistnew(void)
{
  NClist* l;
/*
  if(!ncinitialized) {
    memset((void*)&ncDATANULL,0,sizeof(ncelem));
    ncinitialized = 1;
  }
*/
  l = (NClist*)malloc(sizeof(NClist));
  if(l) {
    l->alloc=0;
    l->length=0;
    l->content=NULL;
  }
  return l;
}

int
nclistfree(NClist* l)
{
  if(l) {
    l->alloc = 0;
    if(l->content != NULL) {free(l->content); l->content = NULL;}
    free(l);
  }
  return TRUE;
}

int
nclistsetalloc(NClist* l, unsigned int sz)
{
  ncelem* newcontent;
  if(l == NULL) return FALSE;
  if(sz <= 0) {sz = (l->length?2*l->length:DEFAULTALLOC);}
  if(l->alloc >= sz) {return TRUE;}
  newcontent=(ncelem*)calloc(sz,sizeof(ncelem));
  if(l->alloc > 0 && l->length > 0 && l->content != NULL) {
    memcpy((void*)newcontent,(void*)l->content,sizeof(ncelem)*l->length);
  }
  if(l->content != NULL) free(l->content);
  l->content=newcontent;
  l->alloc=sz;
  return TRUE;
}

int
nclistsetlength(NClist* l, unsigned int sz)
{
  if(l == NULL) return FALSE;
  if(sz > l->alloc && !nclistsetalloc(l,sz)) return FALSE;
  l->length = sz;
  return TRUE;
}

ncelem
nclistget(NClist* l, unsigned int index)
{
  if(l == NULL || l->length == 0) return ncDATANULL;
  if(index >= l->length) return ncDATANULL;
  return l->content[index];
}

int
nclistset(NClist* l, unsigned int index, ncelem elem)
{
  if(l == NULL) return FALSE;
  if(index >= l->length) return FALSE;
  l->content[index] = elem;
  return TRUE;
}

/* Insert at position i of l; will push up elements i..|seq|. */
int
nclistinsert(NClist* l, unsigned int index, ncelem elem)
{
  int i; /* do not make unsigned */
  if(l == NULL) return FALSE;
  if(index > l->length) return FALSE;
  nclistsetalloc(l,0);
  for(i=(int)l->length;i>index;i--) l->content[i] = l->content[i-1];
  l->content[index] = elem;
  l->length++;
  return TRUE;
}

int
nclistpush(NClist* l, ncelem elem)
{
  if(l == NULL) return FALSE;
  if(l->length >= l->alloc) nclistsetalloc(l,0);
  l->content[l->length] = elem;
  l->length++;
  return TRUE;
}

ncelem
nclistpop(NClist* l)
{
  if(l == NULL || l->length == 0) return ncDATANULL;
  l->length--;  
  return l->content[l->length];
}

ncelem
nclisttop(NClist* l)
{
  if(l == NULL || l->length == 0) return ncDATANULL;
  return l->content[l->length - 1];
}

ncelem
nclistremove(NClist* l, unsigned int i)
{
  unsigned int len;
  ncelem elem;
  if(l == NULL || (len=l->length) == 0) return ncDATANULL;
  if(i >= len) return ncDATANULL;
  elem = l->content[i];
  for(i+=1;i<len;i++) l->content[i-1] = l->content[i];
  l->length--;
  return elem;  
}

/* Duplicate and return the content (null terminate) */
ncelem*
nclistdup(NClist* l)
{
    ncelem* result = (ncelem*)malloc(sizeof(ncelem)*(l->length+1));
    memcpy((void*)result,(void*)l->content,sizeof(ncelem)*l->length);
    result[l->length] = (ncelem)0;
    return result;
}

int
nclistcontains(NClist* list, ncelem elem)
{
    unsigned int i;
    for(i=0;i<nclistlength(list);i++) {
	if(elem == nclistget(list,i)) return 1;
    }
    return 0;
}

/* Extends nclist to include a unique operator 
   which remove duplicate values; NULL values removed
   return value is always 1.
*/

int
nclistunique(NClist* list)
{
    unsigned int i,j,k,len;
    ncelem* content;
    if(list == NULL || list->length == 0) return 1;
    len = list->length;
    content = list->content;
    for(i=0;i<len;i++) {
        for(j=i+1;j<len;j++) {
	    if(content[i] == content[j]) {
		/* compress out jth element */
                for(k=j+1;k<len;k++) content[k-1] = content[k];	
		len--;
	    }
	}
    }
    list->length = len;
    return 1;
}

NClist*
nclistclone(NClist* list)
{
    NClist* clone = nclistnew();
    *clone = *list;
    clone->content = nclistdup(list);
    return clone;
}
