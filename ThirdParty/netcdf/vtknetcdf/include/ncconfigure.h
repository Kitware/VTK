/*
 * Copyright 2010 University Corporation for Atmospheric
 * Research/Unidata. See COPYRIGHT file for more info.
 *
 * This header file is for the parallel I/O functions of netCDF.
 *
 */
/* "$Id: netcdf_par.h,v 1.1 2010/06/01 15:46:49 ed Exp $" */

#ifndef NCCONFIGURE_H
#define NCCONFIGURE_H 1

/*
This is included in bottom
of config.h. It is where,
typically, alternatives to
missing functions should be
defined.
*/

#ifndef HAVE_STRDUP
extern char* strdup(const char*);
#endif

/* #if HAVE_BASETSD_H */
/* #ifndef ssize_t */
/* #ifndef SSIZE_T */
/* #include <BaseTsd.h> */
/* #endif */
/* #define ssize_t SSIZE_T */
/* #endif */
/* #endif */



/* handle null arguments */
#ifndef nulldup
#ifdef HAVE_STRDUP
#define nulldup(s) ((s)==NULL?NULL:strdup(s))
#else
char *nulldup(const char* s);
#endif
#endif


#ifndef nulldup
#define nulldup(s) ((s)==NULL?NULL:strdup(s))
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

#endif /* NCCONFIGURE_H */
