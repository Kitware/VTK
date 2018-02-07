/*
 *	Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
#ifndef _NCWINIO_H_
#define _NCWINIO_H_

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include "ncexternl.h"

#ifndef WINPATH
#ifdef _MSC_VER
#define WINPATH 1
#endif
#ifdef __MINGW32__
#define WINPATH 1
#endif
#endif

/* Path Converter */
EXTERNL char* NCpathcvt(const char* path);

#ifdef WINPATH
/* path converter wrappers*/
EXTERNL FILE* NCfopen(const char* path, const char* flags);
EXTERNL int NCopen3(const char* path, int flags, int mode);
EXTERNL int NCopen2(const char* path, int flags);
#else /*!WINPATH*/
#define NCfopen(path,flags) fopen((path),(flags))
#define NCopen3(path,flags,mode) open((path),(flags),(mode))
#define NCopen2(path,flags) open((path),(flags))
#endif /*WINPATH*/

#endif /* _NCWINIO_H_ */
