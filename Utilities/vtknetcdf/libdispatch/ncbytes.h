/* Copyright 2009, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

#ifndef NCBYTES_H
#define NCBYTES_H 1

typedef struct NCbytes {
  int nonextendible; /* 1 => fail if an attempt is made to extend this buffer*/
  unsigned int alloc;
  unsigned int length;
  char* content;
} NCbytes;

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__) || defined(__CPLUSPLUS)
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

EXTERNC NCbytes* ncbytesnew(void);
EXTERNC void ncbytesfree(NCbytes*);
EXTERNC int ncbytessetalloc(NCbytes*,unsigned int);
EXTERNC int ncbytessetlength(NCbytes*,unsigned int);
EXTERNC int ncbytesfill(NCbytes*, char fill);

/* Produce a duplicate of the contents*/
EXTERNC char* ncbytesdup(NCbytes*);
/* Extract the contents and leave buffer empty */
EXTERNC char* ncbytesextract(NCbytes*);

/* Return the ith byte; -1 if no such index */
EXTERNC int ncbytesget(NCbytes*,unsigned int);
/* Set the ith byte */
EXTERNC int ncbytesset(NCbytes*,unsigned int,char);

/* Append one byte */
EXTERNC int ncbytesappend(NCbytes*,char); /* Add at Tail */
/* Append n bytes */
EXTERNC int ncbytesappendn(NCbytes*,void*,unsigned int); /* Add at Tail */

/* Null terminate the byte string without extending its length (for debugging) */
EXTERNC int ncbytesnull(NCbytes*);

/* Concatenate a null-terminated string to the end of the buffer */
EXTERNC int ncbytescat(NCbytes*,char*);

/* Set the contents of the buffer; mark the buffer as non-extendible */
EXTERNC int ncbytessetcontents(NCbytes*, char*, unsigned int);

/* Following are always "in-lined"*/
#define ncbyteslength(bb) ((bb)?(bb)->length:0U)
#define ncbytesalloc(bb) ((bb)?(bb)->alloc:0U)
#define ncbytescontents(bb) ((bb && bb->content)?(bb)->content:(char*)"")
#define ncbytesextend(bb,len) ncbytessetalloc((bb),(len)+(bb->alloc))
#define ncbytesclear(bb) ((bb)?(bb)->length=0:0U)
#define ncbytesavail(bb,n) ((bb)?((bb)->alloc - (bb)->length) >= (n):0U)

#endif /*NCBYTES_H*/
