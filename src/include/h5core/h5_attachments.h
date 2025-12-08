/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5_ATTACHMENTS
#define __H5CORE_H5_ATTACHMENTS

#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

VTKH5HUT_EXPORT
h5_err_t
h5_add_attachment (
	const h5_file_t, const char* const);

VTKH5HUT_EXPORT
h5_err_t
h5_has_attachments (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_err_t
h5_has_attachment (
	const h5_file_t, const char* const);

VTKH5HUT_EXPORT
h5_ssize_t
h5_get_num_attachments (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_err_t
h5_get_attachment_info_by_idx (
	const h5_file_t, const h5_size_t, char* const, h5_size_t, h5_size_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5_get_attachment_info_by_name (
	const h5_file_t, const char* const, h5_size_t* const);

VTKH5HUT_EXPORT
h5_err_t
h5_get_attachment (
	const h5_file_t, const char* const);

VTKH5HUT_EXPORT
h5_err_t
h5_delete_attachment (
	const h5_file_t, const char* const);


#ifdef __cplusplus
}
#endif

#endif
