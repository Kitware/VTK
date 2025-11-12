/* Copyright 2018, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

#ifndef UTILS_H
#define UTILS_H 1

/* Define a header-only simple version of a dynamically expandable list and byte buffer */
/* To be used in code that should be independent of libnetcdf */

typedef struct VList {
  unsigned alloc;
  unsigned length;
  void** content;
} VList;

typedef struct VString {
  int nonextendible; /* 1 => fail if an attempt is made to extend this string*/
  unsigned int alloc;
  unsigned int length;
  char* content;
} VString;

/* VString has a fixed expansion size */
#define VSTRALLOC 64

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__)
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

static int util_initialized = 0;

static void util_initialize(void);

static VList*
vlistnew(void)
{
  VList* l;
  if(!util_initialized) util_initialize();
  l = (VList*)calloc(1,sizeof(VList));
  assert(l != NULL);
  return l;
}

static void
vlistfree(VList* l)
{
  if(l == NULL) return;
  if(l->content != NULL) {free(l->content); l->content = NULL;}
  free(l);
}

static void
vlistexpand(VList* l)
{
  void** newcontent = NULL;
  size_t newsz;

  if(l == NULL) return;
  newsz = (l->length * 2) + 1; /* basically double allocated space */
  if(l->alloc >= newsz) return; /* space already allocated */
  newcontent=(void**)calloc(newsz,sizeof(void*));
  assert(newcontent != NULL);
  if(l->alloc > 0 && l->length > 0 && l->content != NULL) { /* something to copy */
    memcpy((void*)newcontent,(void*)l->content,sizeof(void*)*l->length);
  }
  if(l->content != NULL) free(l->content);
  l->content=newcontent;
  l->alloc=newsz;
  /* size is the same */  
}

static void*
vlistget(VList* l, unsigned index) /* Return the ith element of l */
{
  if(l == NULL || l->length == 0) return NULL;
  assert(index < l->length);
  return l->content[index];
}

static void
vlistpush(VList* l, void* elem)
{
  if(l == NULL) return;
  while(l->length >= l->alloc) vlistexpand(l);
  l->content[l->length] = elem;
  l->length++;
}

static void*
vlistfirst(VList* l) /* remove first element */
{
  unsigned i,len;
  void* elem;
  if(l == NULL || l->length == 0) return NULL;
  elem = l->content[0];
  len = l->length;
  for(i=1;i<len;i++) l->content[i-1] = l->content[i];
  l->length--;
  return elem;  
}

static void
vlistfreeall(VList* l) /* call free() on each list element*/
{
  unsigned i;
  if(l == NULL || l->length == 0) return;
  for(i=0;i<l->length;i++) if(l->content[i] != NULL) {free(l->content[i]);}
  vlistfree(l);
}

static VString*
vsnew(void)
{
  VString* vs = NULL;
  if(!util_initialized) util_initialize();
  vs = (VString*)calloc(1,sizeof(VString));
  assert(vs != NULL);
  return vs;
}

static void
vsfree(VString* vs)
{
  if(vs == NULL) return;
  if(vs->content != NULL) free(vs->content);
  free(vs);
}

static void
vsexpand(VString* vs)
{
  char* newcontent = NULL;
  size_t newsz;

  if(vs == NULL) return;
  assert(vs->nonextendible == 0);
  newsz = (vs->alloc + VSTRALLOC); /* basically double allocated space */
  if(vs->alloc >= newsz) return; /* space already allocated */
  newcontent=(char*)calloc(1,newsz+1);/* always room for nul term */
  assert(newcontent != NULL);
  if(vs->alloc > 0 && vs->length > 0 && vs->content != NULL) /* something to copy */
    memcpy((void*)newcontent,(void*)vs->content,vs->length);
  newcontent[vs->length] = '\0'; /* ensure null terminated */
  if(vs->content != NULL) free(vs->content);
  vs->content=newcontent;
  vs->alloc=newsz;
  /* length is the same */  
}

static void
vsappendn(VString* vs, const char* elem, unsigned n)
{
  size_t need;
  assert(vs != NULL && elem != NULL);
  if(n == 0) {n = strlen(elem);}
  need = vs->length + n;
  if(vs->nonextendible) {
     /* Space must already be available */
      assert(vs->alloc >= need);
  } else {
      while(vs->alloc < need)
          vsexpand(vs);
  }
  memcpy(&vs->content[vs->length],elem,n);
  vs->length += n;
  if(!vs->nonextendible)
      vs->content[vs->length] = '\0';
}

static void
vsappend(VString* vs, char elem)
{
  char s[2];
  s[0] = elem;
  s[1] = '\0';
  vsappendn(vs,s,1);
}

/* Set unexpandible contents */
static void
vssetcontents(VString* vs, char* contents, unsigned alloc)
{
    assert(vs != NULL && contents != NULL);
    vs->length = 0;
    if(!vs->nonextendible && vs->content != NULL) free(vs->content);
    vs->content = contents;
    vs->length = alloc;
    vs->alloc = alloc;
    vs->nonextendible = 1;
}

/* Extract the content and leave content null */
static char*
vsextract(VString* vs)
{
    char* x = NULL;
    if(vs == NULL || vs->content == NULL) return NULL;
    x = vs->content;
    vs->content = NULL;
    vs->length = 0;
    vs->alloc = 0;
    return x;
}

static void
util_initialize(void)
{
    /* quiet compiler */
    void* f = NULL;
    f = f;
    f = (void*)vlistnew;
    f = (void*)vlistfree;
    f = (void*)vlistexpand;
    f = (void*)vlistget;
    f = (void*)vlistpush;
    f = (void*)vlistfirst;
    f = (void*)vlistfreeall;
    f = (void*)vsnew;
    f = (void*)vsfree;
    f = (void*)vsexpand;
    f = (void*)vssetcontents;
    f = (void*)vsappendn;
    f = (void*)vsappend;
    f = (void*)vsextract;
    util_initialized = 1;
}

/* Following are always "in-lined"*/
#define vlistcontents(l)  ((l)==NULL?NULL:(l)->content)
#define vlistlength(l)  ((l)==NULL?0:(int)(l)->length)
#define vlistclear(l)  vlistsetlength(l,0)
#define vlistsetlength(l,len)  do{if((l)!=NULL) (l)->length=len;} while(0)

#define vscontents(vs)  ((vs)==NULL?NULL:(vs)->content)
#define vslength(vs)  ((vs)==NULL?0:(int)(vs)->length)
#define vscat(vs,s)  vsappendn(vs,s,0)
#define vsclear(vs)  vssetlength(vs,0)
#define vssetlength(vs,len)  do{if((vs)!=NULL) (vs)->length=len;} while(0)

#endif /*UTILS_H*/
