/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5B_MODEL_H
#define __H5CORE_H5B_MODEL_H

#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

VTKH5HUT_EXPORT
h5_err_t
h5b_has_field_data (
        const h5_file_t);

VTKH5HUT_EXPORT
h5_ssize_t
h5b_get_num_fields (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_err_t
h5b_get_field_info (
	const h5_file_t,
	const h5_size_t,
	char* const, const h5_size_t,
	h5_size_t* const, h5_size_t* const, h5_size_t* const, h5_int64_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5b_has_field (
	const h5_file_t,
	const char*);

VTKH5HUT_EXPORT
h5_err_t
h5b_get_field_info_by_name (
	const h5_file_t,
	const char* name,
	h5_size_t* const, h5_size_t* const, h5_size_t* const, h5_int64_t* const);

VTKH5HUT_EXPORT
h5_int64_t
h5b_3d_has_view (
        const h5_file_t f);

VTKH5HUT_EXPORT
h5_err_t
h5b_3d_set_view (
        const h5_file_t,
        const h5_size_t, const h5_size_t,
        const h5_size_t, const h5_size_t,
        const h5_size_t, const h5_size_t,
        const h5_int64_t);

VTKH5HUT_EXPORT
h5_err_t
h5b_3d_get_view (
        const h5_file_t,
        h5_size_t* const, h5_size_t* const,
        h5_size_t* const, h5_size_t* const,
        h5_size_t* const, h5_size_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5b_3d_get_reduced_view (
        const h5_file_t,
        h5_size_t* const, h5_size_t* const,
        h5_size_t* const, h5_size_t* const,
        h5_size_t* const, h5_size_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5b_3d_set_chunk (
        const h5_file_t,
        const h5_size_t, const h5_size_t, const h5_size_t k);

VTKH5HUT_EXPORT
h5_err_t
h5b_3d_get_chunk (
	const h5_file_t,
	const char*,
	h5_size_t* const, h5_size_t* const, h5_size_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5b_3d_set_grid (
	const h5_file_t,
	const h5_size_t, const h5_size_t, const h5_size_t);

VTKH5HUT_EXPORT
h5_err_t
h5b_3d_get_grid_coords (
	const h5_file_t,
	const int,
	h5_int64_t* const, h5_int64_t* const, h5_int64_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5b_3d_set_dims (
	const h5_file_t,
	const h5_size_t, const h5_size_t, const h5_size_t);

VTKH5HUT_EXPORT
h5_err_t
h5b_3d_set_halo (
	const h5_file_t, const h5_size_t, const h5_size_t, const h5_size_t);

#ifdef __cplusplus
}
#endif

#endif
