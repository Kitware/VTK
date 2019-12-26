/*********************************************************************
 *   Copyright 2018, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header$
 *********************************************************************/

#ifndef NCLOG_H
#define NCLOG_H

#include <stdarg.h>
#include "ncexternl.h"

#define NCENVFLAG "NCLOGFILE"

/* Suggested tag values */
#define NCLOGNOTE 0
#define NCLOGWARN 1
#define NCLOGERR 2
#define NCLOGDBG 3

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__)
extern "C" {
#endif

EXTERNL void ncloginit(void);
EXTERNL int ncsetlogging(int tf);
EXTERNL int nclogopen(const char* file);
EXTERNL void nclogclose(void);

/* The tag value is an arbitrary integer */
EXTERNL void nclog(int tag, const char* fmt, ...);
EXTERNL void ncvlog(int tag, const char* fmt, va_list ap);
EXTERNL void nclogtext(int tag, const char* text);
EXTERNL void nclogtextn(int tag, const char* text, size_t count);

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__)
}
#endif

#endif /*NCLOG_H*/
