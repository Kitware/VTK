/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_FCMP_H
#define __PRIVATE_H5_FCMP_H

#include "h5core/h5_types.h"

h5_int64_t
h5priv_fcmp (
        h5_float64_t A,
        h5_float64_t B,
        h5_int32_t maxUlps );

#endif
