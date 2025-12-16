/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5_MODEL_H
#define __H5CORE_H5_MODEL_H

#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define H5_MAX_NAME_LEN          64

VTKH5HUT_EXPORT
h5_err_t
h5_set_iteration_name_fmt (
	const h5_file_t, const char*, const int);

VTKH5HUT_EXPORT
h5_err_t
h5_get_iteration_name_fmt (
	const h5_file_t, char* const, const int, int* const);


VTKH5HUT_EXPORT
h5_int64_t
h5_set_iteration (
	const h5_file_t, const h5_int64_t);


VTKH5HUT_EXPORT
h5_int64_t
h5_get_iteration (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_ssize_t
h5_get_num_iterations (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_int64_t
h5_has_iteration (
	const h5_file_t, const h5_int64_t);

VTKH5HUT_EXPORT
int
h5_get_num_procs (
	const h5_file_t);

#ifdef __cplusplus
}
#endif
extern int dont_use_parmetis; // Warning bad style! used for switching without makro and recompiling...
extern int max_num_elems_p_chunk; // used for switching chunksize without recompiling
extern int preferred_direction; // DITO used for choosing direction without recompiling

#endif
