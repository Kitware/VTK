/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5_FILE_ATTRIBS_H
#define __H5CORE_H5_FILE_ATTRIBS_H

#include "h5core/h5_types.h"
#include <vtk_hdf5.h>

#ifdef __cplusplus
extern "C" {
#endif
	
VTKH5HUT_EXPORT
h5_err_t
h5_has_file_attrib (
	const h5_file_t, const char* const);

VTKH5HUT_EXPORT
h5_err_t
h5_get_file_attrib_info_by_name (
	const h5_file_t, const char* const,
	h5_int64_t* const, h5_size_t* const);

VTKH5HUT_EXPORT
h5_ssize_t
h5_get_num_file_attribs (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_err_t
h5_get_file_attrib_info_by_idx (
	const h5_file_t, const h5_size_t, char* const, const h5_size_t,
	h5_int64_t* const, h5_size_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5_read_file_attrib (
	const h5_file_t, const char* const, const h5_types_t,
	void* const);

VTKH5HUT_EXPORT
h5_err_t
h5_write_file_attrib (
	const h5_file_t, const char* const, const h5_types_t,
	const void* const, const h5_size_t);

#ifdef __cplusplus
}
#endif

#endif
