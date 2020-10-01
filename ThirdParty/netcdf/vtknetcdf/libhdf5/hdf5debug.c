/*********************************************************************
 *   Copyright 2018, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/
#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#if !defined _WIN32 && !defined __CYGWIN__
#include <execinfo.h>
#endif

#include "hdf5debug.h"

#ifdef H5CATCH

#define STSIZE 1000

static void* stacktrace[STSIZE];

int
nch5breakpoint(int err)
{
#if !defined _WIN32 && !defined __CYGWIN__
    int count = 0;
    char** trace = NULL;
    int i;

    count = backtrace(stacktrace,STSIZE);
    trace = backtrace_symbols(stacktrace, STSIZE);
    fprintf(stderr,"backtrace:\n");
    for(i=0;i<count;i++)
        fprintf(stderr,"[%03d] %s\n",i,trace[i]);
#if 0
    if(trace != NULL) free(trace);
#endif
#endif
    return err;
}

int
nch5throw(int err)
{
    if(err == 0) return err;
    return nch5breakpoint(err);
}
#endif

