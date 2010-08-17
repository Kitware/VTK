/*
 *  Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: libvers.c,v 2.16 2009/04/07 17:42:13 ed Exp $ */

#include <ncconfig.h>

#include "rename.h"

/*
 * A version string. This whole function is not needed in netCDF-4,
 * which has it's own version of this function.
 */
#ifndef USE_NETCDF4
#define SKIP_LEADING_GARBAGE 33  /* # of chars prior to the actual version */
#define XSTRING(x)  #x
#define STRING(x)  XSTRING(x)
static const char nc_libvers[] =
  "\044Id: \100(#) netcdf library version " VERSION " of "__DATE__" "__TIME__" $";

const char *
nc_inq_libvers(void)
{
  return &nc_libvers[SKIP_LEADING_GARBAGE];
}

#else
typedef int ignore; /* define at least one symbol */
#endif /* USE_NETCDF4*/
