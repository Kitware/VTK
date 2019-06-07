/* This is part of the netCDF package. Copyright 2018 University
 * Corporation for Atmospheric Research/Unidata. See \ref copyright
 * file for conditions of use. */
/** @internal @file
 *
 * Common includes, defines, etc., for test code in the libsrc4 and
 * nc_test4 directories.
 *
 * @author Ed Hartnett, Denis Heimbigner, Ward Fisher
 */

#ifndef _NC_TESTS_H
#define _NC_TESTS_H

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nc_logging.h"
#ifdef USE_PARALLEL
#include "netcdf_par.h"
#endif
#include "netcdf.h"

/** NC_MAX_DIMS for tests.  Allows different NC_MAX_DIMS values
 * without breaking this test with a heap or stack overflow. */
#define NC_TESTS_MAX_DIMS 1024

#define MAX_NUM_FORMATS 5 /**< Max number of available binary formats. */

/** Useful define for tests. */
/** \{ */
#define MEGABYTE 1048576
#define HALF_MEG (MEGABYTE/2)
#define MILLION 1000000
#define SIXTEEN_MEG 16777216
#define FOUR_MEG (SIXTEEN_MEG/4)
#define THIRTY_TWO_MEG (SIXTEEN_MEG * 2)
#define SIXTY_FOUR_MEG (SIXTEEN_MEG * 4)
#define ONE_TWENTY_EIGHT_MEG (SIXTEEN_MEG * 8)
#define TEST_VAL_42 42
#define BAD_NAME "dd//d/  "
#define NUM_CLASSIC_TYPES 6
#define NUM_NETCDF_TYPES 12
/** \} */

#ifdef USE_PNETCDF
#define TEST_PNETCDF 1
#else
#undef TEST_PNETCDF
#endif

#endif /* _NC_TESTS_H */
