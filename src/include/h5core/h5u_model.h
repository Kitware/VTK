/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5U_MODEL_H
#define __H5CORE_H5U_MODEL_H

#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

VTKH5HUT_EXPORT
h5_ssize_t
h5u_get_num_datasets (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_err_t
h5u_get_dataset_info_by_idx (
	const h5_file_t,
	const h5_id_t, char* const, const h5_size_t, h5_int64_t* const,
        h5_size_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5u_has_dataset (
	const h5_file_t fh,
	const char* const name
	);

VTKH5HUT_EXPORT
h5_err_t
h5u_get_dataset_info_by_name (
        const h5_file_t f,
        const char* const dataset_name,
        h5_int64_t* const type,
        h5_size_t* const nelem
        );

VTKH5HUT_EXPORT
h5_ssize_t
h5u_get_num_items (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_ssize_t
h5u_get_num_items_in_view (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_ssize_t
h5u_get_totalnum_particles_by_name (
	const h5_file_t,
        const char* const);

VTKH5HUT_EXPORT
h5_ssize_t
h5u_get_totalnum_particles_by_idx (
	const h5_file_t,
        h5_id_t);

VTKH5HUT_EXPORT
h5_err_t
h5u_set_num_items (
	const h5_file_t,
	const h5_size_t, const h5_size_t);

VTKH5HUT_EXPORT
h5_err_t
h5u_has_view (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_err_t
h5u_reset_view (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_err_t
h5u_set_view (
	const h5_file_t, const h5_int64_t, const h5_int64_t);

VTKH5HUT_EXPORT
h5_err_t
h5u_set_view_indices (
	const h5_file_t,
	const h5_size_t* const, const h5_size_t);

VTKH5HUT_EXPORT
h5_err_t
h5u_get_view (
	const h5_file_t,
	h5_int64_t* const, h5_int64_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5u_set_canonical_view (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_err_t
h5u_set_chunk (
        const h5_file_t,
        const h5_size_t);

VTKH5HUT_EXPORT
h5_err_t
h5u_get_chunk (
	const h5_file_t,
	const char*, h5_size_t*);

#ifdef __cplusplus
}
#endif

#endif

