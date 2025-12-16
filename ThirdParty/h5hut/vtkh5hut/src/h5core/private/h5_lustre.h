/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_LUSTRE_H
#define __PRIVATE_H5_LUSTRE_H

#include "h5core/h5_types.h"

h5_err_t
h5_optimize_for_lustre (
	const h5_file_t, const char*);

#endif
