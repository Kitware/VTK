/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5B_ATTRIBS_H
#define __H5CORE_H5B_ATTRIBS_H

#include "h5core/h5_types.h"
#include <vtk_hdf5.h>

#define H5BLOCK_FIELD_ORIGIN_NAME	"__Origin__"
#define H5BLOCK_FIELD_SPACING_NAME	"__Spacing__"
#define H5BLOCK_FIELD_XCOORD_NAME	"__X_Coordinates__"
#define H5BLOCK_FIELD_YCOORD_NAME	"__Y_Coordinates__"
#define H5BLOCK_FIELD_ZCOORD_NAME	"__Z_Coordinates__"

#ifdef __cplusplus
extern "C" {
#endif

VTKH5HUT_EXPORT
h5_ssize_t
h5b_get_num_field_attribs (
	const h5_file_t,
        const char*);

VTKH5HUT_EXPORT
h5_err_t
h5b_get_field_attrib_info_by_idx (
	const h5_file_t,
	const char*, const h5_size_t, char* const, const h5_size_t,
        h5_int64_t* const, h5_size_t*);

VTKH5HUT_EXPORT
h5_err_t
h5b_has_field_attrib (
	const h5_file_t fh,
	const char* const field_name,
	const char* const attrib_name);

VTKH5HUT_EXPORT
h5_err_t
h5b_get_field_attrib_info_by_name (
	const h5_file_t,
	const char* const,
	const char* const,
	h5_int64_t*,
	h5_size_t*);

VTKH5HUT_EXPORT
h5_err_t
h5b_write_field_attrib (
	const h5_file_t,
        const char*, const char*, const h5_int64_t, const void*, const h5_int64_t);

VTKH5HUT_EXPORT
h5_err_t
h5b_read_field_attrib (
	const h5_file_t,
	const char*, const char*, const h5_int64_t, void* const);

VTKH5HUT_EXPORT
h5_err_t
h5b_set_3d_field_coords (
        const h5_file_t,
        const int, const char* const, const char* const,
        const h5_float64_t* const, const h5_int64_t n_coords);

VTKH5HUT_EXPORT
h5_err_t
h5b_get_3d_field_coords (
        const h5_file_t,
        const int rank, const char* const, const char* const,
	h5_float64_t* const, const h5_int64_t);
#ifdef __cplusplus
}
#endif

#endif
