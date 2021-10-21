/*
 * Copyright 2018 University Corporation for Atmospheric
 * Research/Unidata. See COPYRIGHT file for more info.
 *
 * This header file is for the parallel I/O functions of netCDF.
 *
 */
/* "$Id: netcdf_par.h,v 1.1 2010/06/01 15:46:49 ed Exp $" */

#ifndef NCCONFIGURE_H
#define NCCONFIGURE_H 1

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/*
This is included in bottom
of config.h. It is where,
typically, alternatives to
missing functions should be
defined and missing types defined.
*/

#ifdef _WIN32
#ifndef HAVE_SSIZE_T
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#define HAVE_SSIZE_T 1
#endif
#endif

#include "config.h"
#include "vtk_netcdf_mangle.h"

/*Warning: Cygwin with -ansi does not define these functions
  in its headers.*/
#ifndef _WIN32
#if __STDC__ == 1 /*supposed to be same as -ansi flag */

/* WARNING: in some systems, these functions may be defined as macros, so check */
#ifndef strdup
extern char* strdup(const char*);
#endif
#ifndef HAVE_STRLCAT
extern size_t strlcat(char*,const char*,size_t);
#endif
#ifndef snprintf
extern int snprintf(char*, size_t, const char*, ...);
#endif
#ifndef strcasecmp
extern int strcasecmp(const char*, const char*);
#endif
#ifndef strtoll
extern long long int strtoll(const char*, char**, int);
#endif
#ifndef strtoull
extern unsigned long long int strtoull(const char*, char**, int);
#endif
#ifndef fileno
extern int fileno(FILE*);
#endif

#endif /*STDC*/
#endif /*!_WIN32*/

#ifdef _WIN32
#ifndef HAVE_STRLCAT
#define strlcat(d,s,n) strcat_s((d),(n),(s))
#endif
#endif

/* handle null arguments */
#ifndef nulldup
#ifdef HAVE_STRDUP
#define nulldup(s) ((s)==NULL?NULL:strdup(s))
#else
extern char *nulldup(const char* s);
#endif
#endif

#ifndef nulllen
#define nulllen(s) ((s)==NULL?0:strlen(s))
#endif

#ifndef nullfree
#define nullfree(s) {if((s)!=NULL) {free(s);} else {}}
#endif

#ifndef HAVE_UCHAR
typedef unsigned char uchar;
#endif

#ifndef HAVE_LONGLONG
typedef long long longlong;
typedef unsigned long long ulonglong;
#endif

#ifndef HAVE_USHORT
typedef unsigned short ushort;
#endif

#ifndef HAVE_UINT
typedef unsigned int uint;
#endif

#ifndef HAVE_UINT64
typedef unsigned long long uint64;
#endif

#ifndef HAVE_UINT64_T
typedef unsigned long long uint64_t;
#endif

#ifndef _WIN32
#ifndef HAVE_UINTPTR_T
#if SIZEOF_VOIDP == 8
#define uintptr_t unsigned long
#else
#define uintptr_t unsigned int
#endif
#endif
#endif

#ifndef HAVE_SIZE64_T
typedef unsigned long long size64_t;
#endif

/* Provide a fixed size alternative to off_t or off64_t */
typedef long long fileoffset_t;

#ifndef NC_UNUSED
#define NC_UNUSED(var) (void)var
#endif


#endif /* NCCONFIGURE_H */
