/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef H5PART_IO
#define H5PART_IO

#include "h5core/h5_log.h"
#include "h5core/h5u_io.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
   \addtogroup h5part_io
   @{
*/

/*
  !                 _ _       
  !  __      ___ __(_) |_ ___ 
  !  \ \ /\ / / '__| | __/ _ \
  !   \ V  V /| |  | | ||  __/
  !    \_/\_/ |_|  |_|\__\___|
*/

/**
   \fn h5_err_t H5PartWriteDataFloat64 (
	const h5_file_t f,
	const char* name,
	const h5_float64_t* data
	)

   \fn h5_err_t H5PartWriteDataFloat32 (
	const h5_file_t f,
	const char* name,
	const h5_float32_t* data
	)

   \fn h5_err_t H5PartWriteDataInt64 (
	const h5_file_t f,
	const char* name,
	const h5_int64_t* data
	)

   \fn h5_err_t H5PartWriteDataInt32 (
	const h5_file_t f,
	const char* name,
	const h5_int32_t* data
	)

  Write a dataset to the current step/iteration.

  After setting the current (time-)step/iteration and view, you can start
  writing datasets into the file. Each dataset has a name associated with
  it (chosen by the user) in order to facilitate later retrieval. The name
  of the dataset is specified in the parameter \c name, which must be a
  null-terminated string.

  There are no restrictions on naming of datasets, but it is useful to arrive
  at some common naming convention when sharing data with other groups.

  The writing routines also implicitly store the datatype of the array so that
  the array can be reconstructed properly on other systems with incompatible
  type representations.

  All data that is written after setting the (time-)step/iteration is
  associated with that (time-)step/iteratiion. While the number of
  elements can change for each timestep, you cannot change the number
  of elements in the middle of a given (time-)step/iteration.

  The data is committed to disk before the routine returns.
  \param f	[in]  file handle.
  \param name   [in]  name to associate array with
  \param data	[in]  array to commit to disk.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5PartReadDataFloat64()
  \see H5PartReadDataFloat32()
  \see H5PartReadDataInt64()
  \see H5PartReadDataInt32()
 */
static inline h5_err_t
H5PartWriteDataFloat64 (
	const h5_file_t f,
	const char* name,
	const h5_float64_t* data
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', date=%p",
                      (h5_file_p)f, name, data);
	H5_API_RETURN (
		h5u_write_dataset (
			f, name, (void*)data,
			H5_FLOAT64_T));
}

static inline h5_err_t
H5PartWriteDataFloat32 (
	const h5_file_t f,
	const char* name,
	const h5_float32_t* data
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', date=%p",
                      (h5_file_p)f, name, data);
	H5_API_RETURN (
		h5u_write_dataset (
			f, name, (void*)data,
			H5_FLOAT32_T));
}

static inline h5_err_t
H5PartWriteDataInt64 (
	const h5_file_t f,
	const char* name,
	const h5_int64_t* data
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', date=%p",
                      (h5_file_p)f, name, data);
	H5_API_RETURN (
		h5u_write_dataset (
			f, name, (void*)data,
			H5_INT64_T));
}

static inline h5_err_t
H5PartWriteDataInt32 (
	const h5_file_t f,
	const char* name,
	const h5_int32_t* data
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', date=%p",
                      (h5_file_p)f, name, data);
	H5_API_RETURN (
		h5u_write_dataset (
			f, name, (void*)data,
			H5_INT32_T));
}

/**
   \fn h5_err_t H5PartReadDataFloat64 (
	const h5_file_t f,
	const char* name,
	h5_float64_t* data
	)

   \fn h5_err_t H5PartReadDataFloat32 (
	const h5_file_t f,
	const char* name,
	h5_float32_t* data
	)

   \fn h5_err_t H5PartReadDataInt64 (
	const h5_file_t f,
	const char* name,
	h5_int64_t* data
	)

   \fn h5_err_t H5PartReadDataInt32 (
	const h5_file_t f,
	const char* name,
	h5_int32_t* data
	)

  Read dataset from file.

  See \ref H5PartWriteDataFloat64() etc. for more details.

  \param f	[in]  file handle
  \param name   [in]  name of dataset to be read
  \param data	[out] buffer for data to be read

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5PartWriteDataFloat64()
  \see H5PartWriteDataFloat32()
  \see H5PartWriteDataInt64()
  \see H5PartWriteDataInt32()
*/
static inline h5_err_t
H5PartReadDataFloat64 (
	const h5_file_t f,
	const char* name,
	h5_float64_t* data
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', date=%p",
                      (h5_file_p)f, name, data);
	H5_API_RETURN (
		h5u_read_dataset (
			f, name, data, H5_FLOAT64_T));
}

static inline h5_err_t
H5PartReadDataFloat32 (
	const h5_file_t f,
	const char* name,
	h5_float32_t* data
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', date=%p",
                      (h5_file_p)f, name, data);
	H5_API_RETURN (
		h5u_read_dataset (
			f, name, data, H5_FLOAT32_T));
}

static inline h5_err_t
H5PartReadDataInt64 (
	const h5_file_t f,
	const char* name,
	h5_int64_t* data
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', date=%p",
                      (h5_file_p)f, name, data);
	H5_API_RETURN (
		h5u_read_dataset (
			f, name, data,
			H5_INT64_T));
}

static inline h5_err_t
H5PartReadDataInt32 (
	const h5_file_t f,
	const char* name,
	h5_int32_t* data
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', date=%p",
                      (h5_file_p)f, name, data);
	H5_API_RETURN (
		h5u_read_dataset (
			f, name, data,
			H5_INT32_T));
}

#ifdef __cplusplus
}
#endif

///< @}
#endif
