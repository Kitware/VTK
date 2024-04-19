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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#if defined(__cplusplus)
extern "C" {
#endif

/* WARNING: in some systems, these functions may be defined as macros, so check */
#ifndef strdup
#ifndef HAVE_STRDUP
char* strdup(const char*);
#define HAVE_STRDUP
#endif
#endif

#ifndef HAVE_STRLCAT
#ifndef strlcat
size_t vtknetcdf_strlcat(char*,const char*,size_t);
#define strlcat vtknetcdf_strlcat
#endif
#endif

#ifndef HAVE_SNPRINTF
#ifndef snprintf
int snprintf(char*, size_t, const char*, ...);
#endif
#endif

#ifndef HAVE_STRCASECMP
#ifndef strcasecmp
extern int strcasecmp(const char*, const char*);
#endif
#endif

#ifndef HAVE_STRTOLL
#ifndef strtoll
long long int strtoll(const char*, char**, int);
#endif
#endif

#ifndef HAVE_STRTOULL
#ifndef strtoull
unsigned long long int strtoull(const char*, char**, int);
#endif
#endif

#if defined(__cplusplus)
}
#endif

#endif /*STDC*/

#else /*_WIN32*/

#ifndef HAVE_STRLCAT
#define strlcat(d,s,n) strcat_s((d),(n),(s))
#endif


#ifndef __MINGW32__
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#ifndef snprintf
#if _MSC_VER<1900
#define snprintf _snprintf
#endif
#endif
#ifndef fileno
#define fileno(f) _fileno(f)
#endif
#endif /*__MINGW32__*/

#endif /*_WIN32*/

/* handle null arguments */
#ifndef nulldup
#ifndef HAVE_STRDUP
/** Copy s if not NULL.
 *
 * Implementation in terms of strdup in
 *
 * @li include/ncconfigure.h
 * @li include/netcdf_json.h
 * @li libdap4/ncd4.h
 * @li libdispatch/dfile.c
 * @li libdispatch/dinfermodel.c
 * @li libdispatch/drc.c
 * @li libdispatch/dutil.c
 * @li libdispatch/nc.c
 * @li libdispatch/ncjson.c
 * @li libdispatch/ncurl.c
 * @li libncxml/ncxml_ezxml.c
 * @li ncxml_tinyxml2.cpp
 * @li libncxml/ncxml_xml2.c
 * @li libnczarr/zsync.c
 * @li ncdump/ocprint.c
 * @li ncgen/cvt.c
 * @li ncgen/ncgen.h
 * @li ncgen3/ncgen.h
 * @li nczarr_test/test_nczarr_utils.h
 * @li oc2/ocinternal.h
 *
 * Declarations as extern:
 *
 * @li include/ncconfigure.h
 *
 * I'd like it to be
 * static inline const char *nulldup(const char *const s);
 * but that's not what's in ncconfigure.h
 *
 * @param s the string to duplicate
 * @pre s is either NULL or a NULL-terminated string
 *
 * @returns NULL or the duplicated string (caller owns the new
 *     pointer)
 *
 * @throws ENOMEM if out of memory

 *
 * @post returns NULL if s is NULL, or a new pointer to a
 *     freshly-allocated copy of s
 */
static char *nulldup(const char* s) {
    if (s != NULL) {
        ssize_t result_length = strlen(s) + 1;
        char *result = malloc(result_length);
        if (result == NULL) {
#ifdef ENOMEM
          /* C++11, POSIX? */
          errno = ENOMEM;
#else /* ENOMEM */
          errno = 1;
#endif /* ENOMEM */
          return NULL;
        }
        strncpy(result, s, result_length);
        return result;
    } else {
        return NULL;
    }
}
#else /* HAVE_STRDUP */
#define nulldup(s) ((s)==NULL?NULL:strdup(s))
#endif /* HAVE_STRDUP */
#endif /* nulldup */

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

#ifndef HAVE_PTRDIFF_T
typedef long ptrdiff_t;
#endif

/* Provide a fixed size alternative to off_t or off64_t */
typedef long long fileoffset_t;

#ifndef NC_UNUSED
#define NC_UNUSED(var) (void)var
#endif

#endif /* NCCONFIGURE_H */
