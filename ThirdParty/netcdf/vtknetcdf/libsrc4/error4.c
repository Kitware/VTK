/*

This file is part of netcdf-4, a netCDF-like interface for HDF5, or a
HDF5 backend for netCDF, depending on your point of view.

This file contains functions relating to logging errors. Also it
contains the functions nc_malloc, nc_calloc, and nc_free.

Copyright 2003, University Corporation for Atmospheric Research. See
netcdf-4/docs/COPYRIGHT file for copying and redistribution
conditions.

$Id: error4.c,v 1.4 2010/06/01 17:48:55 ed Exp $
*/

#include <config.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "assert.h"
#include <hdf5.h>

/* This contents of this file get skipped if LOGGING is not defined
 * during compile. */
#ifdef LOGGING

extern int nc_log_level;

/* This function prints out a message, if the severity of the message
   is lower than the global nc_log_level. To use it, do something like
   this:
   
   nc_log(0, "this computer will explode in %d seconds", i);

   After the first arg (the severity), use the rest like a normal
   printf statement. Output will appear on stdout.

   This function is heavily based on the function in section 15.5 of
   the C FAQ. */
void 
nc_log(int severity, const char *fmt, ...)
{
   va_list argp;
   int t;

   /* If the severity is greater than the log level, we don' care to
      print this message. */
   if (severity > nc_log_level)
      return;

   /* If the severity is zero, this is an error. Otherwise insert that
      many tabs before the message. */
   if (!severity)
      fprintf(stdout, "ERROR: ");
   for (t=0; t<severity; t++)
      fprintf(stdout, "\t");

   /* Print out the variable list of args with vprintf. */
   va_start(argp, fmt);
   vfprintf(stdout, fmt, argp);
   va_end(argp);
   
   /* Put on a final linefeed. */
   fprintf(stdout, "\n");
   fflush(stdout);
}

void 
nc_log_hdf5(void)
{
    H5Eprint(NULL);
}

#endif /* ifdef LOGGING */

