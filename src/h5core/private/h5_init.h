/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_INIT_H
#define __PRIVATE_H5_INIT_H

#include "h5core/h5_types.h"
#include "private/h5t_types.h"

#define UNUSED_ARGUMENT(x) (void)x

#ifdef __cplusplus
extern "C" {
#endif

h5_err_t
h5_initialize (void);

h5_err_t
h5_finalize (void);

extern int h5_initialized;
extern h5_dta_types_t h5_dta_types;
extern int h5_myproc;

#ifdef __cplusplus
}
#endif


#endif
