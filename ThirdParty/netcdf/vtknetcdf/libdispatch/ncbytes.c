/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ncbytes.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEFAULTALLOC 1024
#define ALLOCINCR 1024

static int ncbytesdebug = 1;

static long
ncbytesfail(void)
{
    fflush(stdout);
    fprintf(stderr,"bytebuffer failure\n");
    fflush(stderr);
    if(ncbytesdebug) abort();
    return FALSE;
}

NCbytes*
ncbytesnew(void)
{
  NCbytes* bb = (NCbytes*)malloc(sizeof(NCbytes));
  if(bb == NULL) return (NCbytes*)ncbytesfail();
  bb->alloc=0;
  bb->length=0;
  bb->content=NULL;
  bb->nonextendible = 0;
  return bb;
}

int
ncbytessetalloc(NCbytes* bb, unsigned int sz)
{
  char* newcontent;
  if(bb == NULL) return ncbytesfail();
  if(sz <= 0) {sz = (bb->alloc?2*bb->alloc:DEFAULTALLOC);}
  if(bb->alloc >= sz) return TRUE;
  if(bb->nonextendible) return ncbytesfail();
  newcontent=(char*)calloc(sz,sizeof(char));
  if(bb->alloc > 0 && bb->length > 0 && bb->content != NULL) {
    memcpy((void*)newcontent,(void*)bb->content,sizeof(char)*bb->length);
  }
  if(bb->content != NULL) free(bb->content);
  bb->content=newcontent;
  bb->alloc=sz;
  return TRUE;
}

void
ncbytesfree(NCbytes* bb)
{
  if(bb == NULL) return;
  if(!bb->nonextendible && bb->content != NULL) free(bb->content);
  free(bb);
}

int
ncbytessetlength(NCbytes* bb, unsigned int sz)
{
  if(bb == NULL) return ncbytesfail();
  if(sz > bb->alloc) {if(!ncbytessetalloc(bb,sz)) return ncbytesfail();}
  bb->length = sz;
  return TRUE;
}

int
ncbytesfill(NCbytes* bb, char fill)
{
  unsigned int i;
  if(bb == NULL) return ncbytesfail();
  for(i=0;i<bb->length;i++) bb->content[i] = fill;
  return TRUE;
}

int
ncbytesget(NCbytes* bb, unsigned int index)
{
  if(bb == NULL) return -1;
  if(index >= bb->length) return -1;
  return bb->content[index];
}

int
ncbytesset(NCbytes* bb, unsigned int index, char elem)
{
  if(bb == NULL) return ncbytesfail();
  if(index >= bb->length) return ncbytesfail();
  bb->content[index] = elem;
  return TRUE;
}

int
ncbytesappend(NCbytes* bb, char elem)
{
  if(bb == NULL) return ncbytesfail();
  if(bb->length >= bb->alloc) if(!ncbytessetalloc(bb,0)) return ncbytesfail();
  bb->content[bb->length] = elem;
  bb->length++;
  return TRUE;
}

/* This assumes s is a null terminated string*/
int
ncbytescat(NCbytes* bb, char* s)
{
    ncbytesappendn(bb,(void*)s,strlen(s)+1); /* include trailing null*/
    /* back up over the trailing null*/
    if(bb->length == 0) return ncbytesfail();
    bb->length--;
    return 1;
}

int
ncbytesappendn(NCbytes* bb, void* elem, unsigned int n)
{
  if(bb == NULL || elem == NULL) return ncbytesfail();
  if(n == 0) {n = strlen((char*)elem);}
  while(!ncbytesavail(bb,n)) {if(!ncbytessetalloc(bb,0)) return ncbytesfail();}
  memcpy((void*)&bb->content[bb->length],(void*)elem,n);
  bb->length += n;
  return TRUE;
}

int
ncbytesprepend(NCbytes* bb, char elem)
{
  int i; /* do not make unsigned */
  if(bb == NULL) return ncbytesfail();
  if(bb->length >= bb->alloc) if(!ncbytessetalloc(bb,0)) return ncbytesfail();
  /* could we trust memcpy? instead */
  for(i=bb->alloc;i>=1;i--) {bb->content[i]=bb->content[i-1];}
  bb->content[0] = elem;
  bb->length++;
  return TRUE;
}

char*
ncbytesdup(NCbytes* bb)
{
    char* result = (char*)malloc(bb->length+1);
    memcpy((void*)result,(const void*)bb->content,bb->length);
    result[bb->length] = '\0'; /* just in case it is a string*/
    return result;
}

char*
ncbytesextract(NCbytes* bb)
{
    char* result = bb->content;
    bb->alloc = 0;
    bb->length = 0;
    bb->content = NULL;
    return result;
}

int
ncbytessetcontents(NCbytes* bb, char* contents, unsigned int alloc)
{
    if(bb == NULL) return ncbytesfail();
    ncbytesclear(bb);
    if(!bb->nonextendible && bb->content != NULL) free(bb->content);
    bb->content = contents;
    bb->length = 0;
    bb->alloc = alloc;
    bb->nonextendible = 1;
    return 1;
}

/* Null terminate the byte string without extending its length */
/* For debugging */
int
ncbytesnull(NCbytes* bb)
{
    ncbytesappend(bb,'\0');
    bb->length--;
    return 1;
}

