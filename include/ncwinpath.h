/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
#ifndef _NCWINIO_H_
#define _NCWINIO_H_

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "ncexternl.h"

#ifndef WINPATH
#ifdef _MSC_VER
#define WINPATH 1
#endif
#ifdef __MINGW32__
#define WINPATH 1
#endif
#endif

/* Define wrapper constants for use with NCaccess */
#ifdef _MSC_VER
#define ACCESS_MODE_EXISTS 0
#define ACCESS_MODE_R 4
#define ACCESS_MODE_W 2
#define ACCESS_MODE_RW 6
#else
#define ACCESS_MODE_EXISTS (F_OK)
#define ACCESS_MODE_R (R_OK)
#define ACCESS_MODE_W (W_OK)
#define ACCESS_MODE_RW (R_OK|W_OK)
#endif

/* Path Converter */
EXTERNL char* NCpathcvt(const char* path);

#ifdef WINPATH
/* path converter wrappers*/
EXTERNL FILE* NCfopen(const char* path, const char* flags);
EXTERNL int NCopen3(const char* path, int flags, int mode);
EXTERNL int NCopen2(const char* path, int flags);
EXTERNL int NCaccess(const char* path, int mode);
EXTERNL int NCremove(const char* path);
#else /*!WINPATH*/
#define NCfopen(path,flags) fopen((path),(flags))
#define NCopen3(path,flags,mode) open((path),(flags),(mode))
#define NCopen2(path,flags) open((path),(flags))
#define NCremove(path) remove(path)
#ifdef _MSC_VER
#define NCaccess(path,mode) _access(path,mode)
#else
#define NCaccess(path,mode) access(path,mode)
#endif
#endif /*WINPATH*/

#endif /* _NCWINIO_H_ */
