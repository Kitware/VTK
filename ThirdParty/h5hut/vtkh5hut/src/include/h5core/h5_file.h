/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5CORE_H5_FILE_H
#define __H5CORE_H5_FILE_H


#include "h5core/h5_types.h"

#ifdef __cplusplus
extern "C" {
#endif

VTKH5HUT_EXPORT
h5_prop_t
h5_create_prop (
        const h5_int64_t);

VTKH5HUT_EXPORT
h5_err_t
h5_set_prop_file_throttle (
        h5_prop_t, const h5_int64_t);

VTKH5HUT_EXPORT
h5_err_t
h5_set_prop_file_align (
        h5_prop_t, const h5_int64_t);

VTKH5HUT_EXPORT
h5_err_t
h5_set_prop_file_mpio_collective (
        h5_prop_t, MPI_Comm* const);

VTKH5HUT_EXPORT
h5_err_t
h5_set_prop_file_mpio_independent (
        h5_prop_t, MPI_Comm* const);

VTKH5HUT_EXPORT
h5_err_t
h5_set_prop_file_mpio_posix (
        h5_prop_t, MPI_Comm* const);

VTKH5HUT_EXPORT
h5_err_t
h5_set_prop_file_core_vfd (
        h5_prop_t, h5_int64_t);

VTKH5HUT_EXPORT
h5_err_t
h5_set_prop_file_flush_after_write (
        h5_prop_t _props);

VTKH5HUT_EXPORT
h5_err_t
h5_close_prop (
        h5_prop_t);

VTKH5HUT_EXPORT
h5_file_p
h5_open_file1 (
	const char*, const h5_int32_t, 	MPI_Comm, const h5_size_t);

VTKH5HUT_EXPORT
h5_file_t
h5_open_file2 (
	const char*, const h5_int32_t, h5_prop_t prop);

VTKH5HUT_EXPORT
h5_err_t
h5_check_filehandle (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_err_t
h5_close_file (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_err_t
h5_close_h5hut (
	void);

VTKH5HUT_EXPORT
h5_err_t
h5_flush_iteration (
	const h5_file_t);

VTKH5HUT_EXPORT
h5_err_t
h5_flush_file (
	const h5_file_t);

VTKH5HUT_EXPORT
hid_t
h5_get_hdf5_file(
	const h5_file_t);

#ifdef __cplusplus
}
#endif

#endif
