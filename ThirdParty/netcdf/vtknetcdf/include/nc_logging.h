/* Copyright 2018, University Corporation for Atmospheric Research. See
   COPYRIGHT file for copying and redistribution conditions. */
/** 
 * @file @internal This file is part of netcdf-4, a netCDF-like
 * interface for HDF5, or a HDF5 backend for netCDF, depending on your
 * point of view.
 *
 * This file contains macros and prototypes relating to logging.
 *
 * @author Ed Hartnett
*/

#ifndef _NCLOGGING_
#define _NCLOGGING_

#include <stdlib.h>
#include <assert.h>

#ifdef LOGGING

/* To log something... */
void nc_log(int severity, const char *fmt, ...);
void nc_log_hdf5(void);

#define LOG(e) nc_log e

/* To log based on error code, and set retval. */
#define BAIL2(e) \
   do { \
      retval = e; \
      LOG((0, "file %s, line %d.\n%s", __FILE__, __LINE__, nc_strerror(e))); \
      nc_log_hdf5(); \
   } while (0) 

/* To set retval and jump to exit, without logging error message. */
#define BAIL_QUIET(e) \
   do { \
      retval = e; \
      goto exit; \
   } while (0) 

#else /* LOGGING */

/* These definitions will be used unless LOGGING is defined. */
#define LOG(e)

#define BAIL2(e) \
   do { \
      retval = e; \
   } while (0)

#define BAIL_QUIET BAIL

#ifdef USE_NETCDF_4
#ifndef ENABLE_SET_LOG_LEVEL
/* Define away any calls to nc_set_log_level(), if its not enabled. */
#define nc_set_log_level(e)
#endif /* ENABLE_SET_LOG_LEVEL */
#endif

#endif /* LOGGING */

/* To log an error message (if 'LOGGING' is defined), set retval, and jump to exit. */
#define BAIL(e) \
   do { \
      BAIL2(e); \
      goto exit; \
   } while (0) 

#endif /* _NCLOGGING_ */

