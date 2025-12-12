/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5U_TYPES_H
#define __PRIVATE_H5U_TYPES_H

#include "h5core/h5_types.h"

struct h5u_fdata {
	hsize_t nparticles;             /* -> u.nparticles */

	h5_int64_t viewstart; /* -1 if no view is available: A "view" looks */
	h5_int64_t viewend;   /* at a subset of the data. */
	char viewindexed; /* flag whether this view is a list of indices */

	hid_t shape;
	hid_t diskshape;
	hid_t memshape;

	hid_t dcreate_prop;
};
typedef struct h5u_fdata h5u_fdata_t;
#endif
