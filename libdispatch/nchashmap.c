/*********************************************************************
 *   Copyright 2010, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header$
 *********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nchashmap.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEFAULTALLOC 31

NChashmap* nchashnew(void) {return nchashnew0(DEFAULTALLOC);}

NChashmap* nchashnew0(size_t alloc)
{
  NChashmap* hm;
  if(sizeof(nchashid) != sizeof(void*)){
	fprintf(stderr,"nchashmap: sizeof(nchashid) != sizeof(void*)");
	abort();
  }
  hm = (NChashmap*)malloc(sizeof(NChashmap));
  if(!hm) return NULL;
  hm->alloc = alloc;
  hm->table = (NClist**)malloc(hm->alloc*sizeof(NClist*));
  if(!hm->table) {free(hm); return NULL;}
  memset((void*)hm->table,0,hm->alloc*sizeof(NClist*));
  return hm;
}

int
nchashfree(NChashmap* hm)
{
  if(hm) {
    int i;
    for(i=0;i<hm->alloc;i++) {
	if(hm->table[i] != NULL) nclistfree(hm->table[i]);
    }
    free(hm->table);
    free(hm);
  }
  return TRUE;
}

/* Insert a <nchashid,void*> pair into the table*/
/* Fail if already there*/
int
nchashinsert(NChashmap* hm, nchashid hash, void* value)
{
    int i;
    size_t offset,len;
    NClist* seq;
    void** list;

    offset = (hash % hm->alloc);
    seq = hm->table[offset];
    if(seq == NULL) {seq = nclistnew(); hm->table[offset] = seq;}
    len = nclistlength(seq);
    list = nclistcontents(seq);
    for(i=0;i<len;i+=2,list+=2) {
	if(hash==(nchashid)(*list)) return FALSE;
    }
    nclistpush(seq,(void*)hash);
    nclistpush(seq,value);
    hm->size++;
    return TRUE;
}

/* Insert a <nchashid,void*> pair into the table*/
/* Overwrite if already there*/
int
nchashreplace(NChashmap* hm, nchashid hash, void* value)
{
    int i;
    size_t offset,len;
    NClist* seq;
    void** list;

    offset = (hash % hm->alloc);
    seq = hm->table[offset];
    if(seq == NULL) {seq = nclistnew(); hm->table[offset] = seq;}
    len = nclistlength(seq);
    list = nclistcontents(seq);
    for(i=0;i<len;i+=2,list+=2) {
	if(hash==(nchashid)(*list)) {list[1] = value; return TRUE;}
    }
    nclistpush(seq,(void*)hash);
    nclistpush(seq,value);
    hm->size++;
    return TRUE;
}

/* remove a nchashid*/
/* return TRUE if found, false otherwise*/
int
nchashremove(NChashmap* hm, nchashid hash)
{
    size_t i;
    size_t offset,len;
    NClist* seq;
    void** list;

    offset = (hash % hm->alloc);
    seq = hm->table[offset];
    if(seq == NULL) return TRUE;
    len = nclistlength(seq);
    list = nclistcontents(seq);
    for(i=0;i<len;i+=2,list+=2) {
	if(hash==(nchashid)(*list)) {
	    nclistremove(seq,(i+1));
	    nclistremove(seq,i);
	    hm->size--;
	    if(nclistlength(seq) == 0) {nclistfree(seq); hm->table[offset] = NULL;}
	    return TRUE;
	}
    }
    return FALSE;
}

/* lookup a nchashid; return DATANULL if not found*/
/* (use hashlookup if the possible values include 0)*/
void*
nchashget(NChashmap* hm, nchashid hash)
{
    void* value = NULL;
    if(!nchashlookup(hm,hash,&value)) return NULL;
    return value;
}

int
nchashlookup(NChashmap* hm, nchashid hash, void** valuep)
{
    int i;
    size_t offset,len;
    NClist* seq;
    void** list;

    offset = (hash % hm->alloc);
    seq = hm->table[offset];
    if(seq == NULL) return TRUE;
    len = nclistlength(seq);
    list = nclistcontents(seq);
    for(i=0;i<len;i+=2,list+=2) {
	if(hash==(nchashid)(*list)) {if(valuep) {*valuep = list[1]; return TRUE;}}
    }
    return FALSE;
}

/* Return the ith pair; order is completely arbitrary*/
/* Can be expensive*/
int
nchashith(NChashmap* hm, int index, nchashid* hashp, void** elemp)
{
    int i;
    if(hm == NULL) return FALSE;
    for(i=0;i<hm->alloc;i++) {
	NClist* seq = hm->table[i];
	int len = nclistlength(seq) / 2;
	if(len == 0) continue;
	if((index - len) < 0) {
	    if(hashp) *hashp = (nchashid)nclistget(seq,index*2);
	    if(elemp) *elemp = nclistget(seq,(index*2)+1);
	    return TRUE;
	}
	index -= len;
    }
    return FALSE;
}

/* Return all the keys; order is completely arbitrary*/
/* Can be expensive*/
int
nchashkeys(NChashmap* hm, nchashid** keylist)
{
    int i,j,index;
    nchashid* keys;
    if(hm == NULL) return FALSE;
    if(hm->size == 0) {
	keys = NULL;
    } else {
        keys = (nchashid*)malloc(sizeof(nchashid)*hm->size);
        for(index=0,i=0;i<hm->alloc;i++) {
 	    NClist* seq = hm->table[i];
	    for(j=0;j<nclistlength(seq);j+=2) {
	        keys[index++] = (nchashid)nclistget(seq,j);
	    }
	}
    }
    if(keylist) {*keylist = keys;}
    else {free(keys);}

    return TRUE;
}
