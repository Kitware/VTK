/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5BLOCK_IO_H
#define __H5BLOCK_IO_H

#include "h5core/h5_log.h"
#include "h5core/h5b_io.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
   \addtogroup h5block_io
   @{
*/

/*
  !                 _ _                       _            
  !  __      ___ __(_) |_ ___   ___  ___ __ _| | __ _ _ __ 
  !  \ \ /\ / / '__| | __/ _ \ / __|/ __/ _` | |/ _` | '__|
  !   \ V  V /| |  | | ||  __/ \__ \ (_| (_| | | (_| | |   
  !    \_/\_/ |_|  |_|\__\___| |___/\___\__,_|_|\__,_|_|
*/

/**
   \fn h5_err_t H5Block3dWriteScalarFieldFloat64 (
	const h5_file_t f,
	const char* name,
	const h5_float64_t* buffer
	)

   \fn h5_err_t H5Block3dWriteScalarFieldFloat32 (
	const h5_file_t f,
	const char* name,
	const h5_float32_t* buffer
	)

   \fn h5_err_t H5Block3dWriteScalarFieldInt64 (
	const h5_file_t f,
	const char* name,
	const h5_int64_t* buffer
	)

   \fn h5_err_t H5Block3dWriteScalarFieldInt32 (
	const h5_file_t f,
	const char* name,
	const h5_int32_t* buffer
	)

  Write a 3-dimensional field with scalar values to the current
  step/iteration using the previously defined field view.  Ensure
  that the size of the buffer matches the number of elements in
  the view.

  Supported data types are

  - 64-bit floating point (\c h5_float64_t)
  - 32-bit floating point (\c h5_float32_t)
  - 64-bit integer (\c h5_int64_t)
  - 32-bit integer (\c h5_int32_t)

  \note Use the FORTRAN indexing scheme to store data in the buffer.

  \param f	[in]  file handle
  \param name	[in]  name of field to be written
  \param buffer [in]  data to be written

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5Block3dReadScalarFieldFloat64()
  \see H5Block3dReadScalarFieldFloat32()
  \see H5Block3dReadScalarFieldInt64()
  \see H5Block3dReadScalarFieldInt32()
*/
static inline h5_err_t
H5Block3dWriteScalarFieldFloat64 (
	const h5_file_t f,
	const char* name,
	const h5_float64_t* buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', buffer=%p",
                      (h5_file_p)f, name, buffer);
	H5_API_RETURN (
		h5b_write_scalar_data (
			f, name, (void*)buffer,
			H5_FLOAT64_T));
}

static inline h5_err_t
H5Block3dWriteScalarFieldFloat32 (
	const h5_file_t f,
	const char* name,
	const h5_float32_t* buffer
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', buffer=%p",
                      (h5_file_p)f, name, buffer);
	H5_API_RETURN (
		h5b_write_scalar_data (
			f, name, buffer,
			H5_FLOAT32_T));
}

static inline h5_err_t
H5Block3dWriteScalarFieldInt64 (
	const h5_file_t f,
	const char* name,
	const h5_int64_t* buffer
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', buffer=%p",
                      (h5_file_p)f, name, buffer);
	H5_API_RETURN (
		h5b_write_scalar_data (
			f, name, buffer, H5_INT64_T));
}

static inline h5_err_t
H5Block3dWriteScalarFieldInt32 (
	const h5_file_t f,
	const char* name,
	const h5_int32_t* buffer
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', buffer=%p",
                      (h5_file_p)f, name, buffer);
	H5_API_RETURN (
		h5b_write_scalar_data (
			f, name, buffer,
			H5_INT32_T));
}

/*
  !                      _                 _            
  !   _ __ ___  __ _  __| |  ___  ___ __ _| | __ _ _ __ 
  !  | '__/ _ \/ _` |/ _` | / __|/ __/ _` | |/ _` | '__|
  !  | | |  __/ (_| | (_| | \__ \ (_| (_| | | (_| | |   
  !  |_|  \___|\__,_|\__,_| |___/\___\__,_|_|\__,_|_|   
*/

/**
   \fn h5_err_t H5Block3dReadScalarFieldFloat64 (
	const h5_file_t f,
	const char* name,
	h5_float64_t* buffer
	)

   \fn h5_err_t H5Block3dReadScalarFieldFloat32 (
	const h5_file_t f,
	const char* name,
	h5_float32_t* buffer
	)

   \fn h5_err_t H5Block3dReadScalarFieldInt64 (
	const h5_file_t f,
	const char* name,
	h5_int64_t* buffer
	)

   \fn h5_err_t H5Block3dReadScalarFieldInt32 (
	const h5_file_t f,
	const char* name,
	h5_int32_t* buffer
	)

  Read a 3-dimensional field with scalar values from the current
  step/iteration using the previously defined field layout.

  Ensure that the size of the buffer matches the number of elements in
  the view.

  \note Use the FORTRAN indexing scheme to store data in the buffer.

  \param f	[in]  file handle
  \param name	[in]  name of field to be read
  \param buffer [out] buffer for data to be read

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5Block3dReadScalarFieldFloat64 (
	const h5_file_t f,
	const char* name,
	h5_float64_t* buffer
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', buffer=%p",
                      (h5_file_p)f, name, buffer);
	H5_API_RETURN (
		h5b_read_scalar_data (
			f, name, (void*)buffer,
			H5_FLOAT64_T));
}

static inline h5_err_t
H5Block3dReadScalarFieldFloat32 (
	const h5_file_t f,
	const char* name,
	h5_float32_t* const buffer
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', buffer=%p",
                      (h5_file_p)f, name, buffer);
	H5_API_RETURN (
		h5b_read_scalar_data (
			f, name, buffer,
			H5_FLOAT32_T));
}

static inline h5_err_t
H5Block3dReadScalarFieldInt64 (
	const h5_file_t f,
	const char* name,
	h5_int64_t* const buffer
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', buffer=%p",
                      (h5_file_p)f, name, buffer);
	H5_API_RETURN (
		h5b_read_scalar_data (
			f, name, buffer, H5_INT64_T));
}

static inline h5_err_t
H5Block3dReadScalarFieldInt32 (
	const h5_file_t f,
	const char* name,
	h5_int32_t* const buffer
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', buffer=%p",
                      (h5_file_p)f, name, buffer);
	H5_API_RETURN (
		h5b_read_scalar_data (
			f, name, buffer,
			H5_INT32_T));
}

/*
  !                 _ _         _____     _                  _             
  !  __      ___ __(_) |_ ___  |___ /  __| | __   _____  ___| |_ ___  _ __ 
  !  \ \ /\ / / '__| | __/ _ \   |_ \ / _` | \ \ / / _ \/ __| __/ _ \| '__|
  !   \ V  V /| |  | | ||  __/  ___) | (_| |  \ V /  __/ (__| || (_) | |   
  !    \_/\_/ |_|  |_|\__\___| |____/ \__,_|   \_/ \___|\___|\__\___/|_|
 */

/**
   \fn h5_err_t H5Block3dWriteVector3dFieldFloat64 (
	const h5_file_t f,
	const char* name,
	const h5_float64_t* x_buf,
	const h5_float64_t* y_buf,
	const h5_float64_t* z_buf
	)

   \fn h5_err_t H5Block3dWriteVector3dFieldFloat32 (
	const h5_file_t f,
	const char* name,
	const h5_float32_t* x_buf,
	const h5_float32_t* y_buf,
	const h5_float32_t* z_buf
	)

   \fn h5_err_t H5Block3dWriteVector3dFieldInt64 (
	const h5_file_t f,
	const char* name,
	const h5_int64_t* x_buf,
	const h5_int64_t* y_buf,
	const h5_int64_t* z_buf
	)
   \fn h5_err_t H5Block3dWriteVector3dFieldInt32 (
	const h5_file_t f,
	const char* name,
	const h5_int32_t* x_buf,
	const h5_int32_t* y_buf,
	const h5_int32_t* z_buf
	)

  Write a 3-dimensional field with 3-dimensional vectors as values to
  the current step/iteration using the previously defined field view.
  Ensure that the size of the buffer matches the number of elements in
  the view.

  Supported data types are

  - 64-bit floating point (\c h5_float64_t)
  - 32-bit floating point (\c h5_float32_t)
  - 64-bit integer (\c h5_int64_t)
  - 32-bit integer (\c h5_int32_t)

  \note Use the FORTRAN indexing scheme to store data in the buffer.

  \param f	[in]  file handle.
  \param name   [in]  name of field to be written
  \param x_buf	[in]  pointer to buffer with X axis data
  \param y_buf  [in]  pointer to buffer with Y axis data
  \param z_buf	[in]  pointer to buffer with Y axis data

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5Block3dReadVector3dFieldFloat64()
  \see H5Block3dReadVector3dFieldFloat32()
  \see H5Block3dReadVector3dFieldInt64()
  \see H5Block3dReadVector3dFieldInt32()
*/
static inline h5_err_t
H5Block3dWriteVector3dFieldFloat64 (
	const h5_file_t f,
	const char* name,
	const h5_float64_t* x_buf,
	const h5_float64_t* y_buf,
	const h5_float64_t* z_buf
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', x_buf=%p, y_buf=%p, z_buf=%p",
		      (h5_file_p)f, name, x_buf, y_buf, z_buf);
	H5_API_RETURN (
		h5b_write_vector3d_data(
			f, name,
			(void*)x_buf, (void*)y_buf, (void*)z_buf,
			H5_FLOAT64_T));
}

static inline h5_err_t
H5Block3dWriteVector3dFieldFloat32 (
	const h5_file_t f,
	const char* name,
	const h5_float32_t* x_buf,
	const h5_float32_t* y_buf,
	const h5_float32_t* z_buf
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', x_buf=%p, y_buf=%p, z_buf=%p",
		      (h5_file_p)f, name, x_buf, y_buf, z_buf);
	H5_API_RETURN(
		h5b_write_vector3d_data (
			f, name,
			x_buf, y_buf, z_buf,
			H5_FLOAT32_T));
}

static inline h5_err_t
H5Block3dWriteVector3dFieldInt64 (
	const h5_file_t f,
	const char* name,
	const h5_int64_t* x_buf,
	const h5_int64_t* y_buf,
	const h5_int64_t* z_buf
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', x_buf=%p, y_buf=%p, z_buf=%p",
		      (h5_file_p)f, name, x_buf, y_buf, z_buf);
	H5_API_RETURN (
		h5b_write_vector3d_data (
			f, name,
			x_buf, y_buf, z_buf,
			H5_INT64_T));
}

static inline h5_err_t
H5Block3dWriteVector3dFieldInt32 (
	const h5_file_t f,
	const char* name,
	const h5_int32_t* x_buf,
	const h5_int32_t* y_buf,
	const h5_int32_t* z_buf
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', x_buf=%p, y_buf=%p, z_buf=%p",
		      (h5_file_p)f, name, x_buf, y_buf, z_buf);
	H5_API_RETURN (
		h5b_write_vector3d_data(
			f, name,
			x_buf, y_buf, z_buf,
			H5_INT32_T));
}

/*
  !                      _   _____     _                  _             
  !   _ __ ___  __ _  __| | |___ /  __| | __   _____  ___| |_ ___  _ __ 
  !  | '__/ _ \/ _` |/ _` |   |_ \ / _` | \ \ / / _ \/ __| __/ _ \| '__|
  !  | | |  __/ (_| | (_| |  ___) | (_| |  \ V /  __/ (__| || (_) | |   
  !  |_|  \___|\__,_|\__,_| |____/ \__,_|   \_/ \___|\___|\__\___/|_|
 */
/**
   \fn h5_err_t H5Block3dReadVector3dFieldFloat64 (
	const h5_file_t f,
	const char* name,
	h5_float64_t* const x_buf,
	h5_float64_t* const y_buf,
	h5_float64_t* const z_buf
	)

   \fn h5_err_t H5Block3dReadVector3dFieldFloat32 (
	const h5_file_t f,
	const char* name,
	h5_float32_t* const x_buf,
	h5_float32_t* const y_buf,
	h5_float32_t* const z_buf
	)

   \fn h5_err_t H5Block3dReadVector3dFieldInt64 (
	const h5_file_t f,
	const char* name,
	h5_int64_t* const x_buf,
	h5_int64_t* const y_buf,
	h5_int64_t* const z_buf
	)

   \fn h5_err_t H5Block3dReadVector3dFieldInt32 (
	const h5_file_t f,
	const char* name,
	h5_int32_t* const x_buf,
	h5_int32_t* const y_buf,
	h5_int32_t* const z_buf
	)

  Read a 3-dimensional field with 3-dimensional vectors as values from
  the current step/iteration using the previously defined field layout.

  Ensure that the size of the buffer matches the number of elements in
  the view.

  \note Use the FORTRAN indexing scheme to store data in the buffer.

  \param f	    [in]  file handle
  \param name	    [in]  name of field to be read
  \param x_buf	    [in]  buffer for X axis data to be read
  \param y_buf	    [in]  buffer for Y axis data to be read
  \param z_buf	    [in]  buffer for Z axis data to be read

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5Block3dWriteVector3dFieldFloat64()
  \see H5Block3dWriteVector3dFieldFloat32()
  \see H5Block3dWriteVector3dFieldInt64()
  \see H5Block3dWriteVector3dFieldInt32()
*/
static inline h5_err_t
H5Block3dReadVector3dFieldFloat64 (
	const h5_file_t f,
	const char* name,
	h5_float64_t* const x_buf,
	h5_float64_t* const y_buf,
	h5_float64_t* const z_buf
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', x_buf=%p, y_buf=%p, z_buf=%p",
		      (h5_file_p)f, name, x_buf, y_buf, z_buf);
	H5_API_RETURN (
		h5b_read_vector3d_data (
			f, name,
			x_buf, y_buf, z_buf,
			H5_FLOAT64_T));
}

static inline h5_err_t
H5Block3dReadVector3dFieldFloat32 (
	const h5_file_t f,
	const char* name,
	h5_float32_t* const x_buf,
	h5_float32_t* const y_buf,
	h5_float32_t* const z_buf
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', x_buf=%p, y_buf=%p, z_buf=%p",
		      (h5_file_p)f, name, x_buf, y_buf, z_buf);
	H5_API_RETURN (
		h5b_read_vector3d_data (
			f, name,
			x_buf, y_buf, z_buf,
			H5_FLOAT32_T));
}

static inline h5_err_t
H5Block3dReadVector3dFieldInt64 (
	const h5_file_t f,
	const char* name,
	h5_int64_t* const x_buf,
	h5_int64_t* const y_buf,
	h5_int64_t* const z_buf
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', x_buf=%p, y_buf=%p, z_buf=%p",
		      (h5_file_p)f, name, x_buf, y_buf, z_buf);
	H5_API_RETURN (
		h5b_read_vector3d_data (
			f, name,
			x_buf, y_buf, z_buf,
			H5_INT64_T));
}

static inline h5_err_t
H5Block3dReadVector3dFieldInt32 (
	const h5_file_t f,
	const char* name,
	h5_int32_t* const x_buf,
	h5_int32_t* const y_buf,
	h5_int32_t* const z_buf
	) {

	H5_API_ENTER (h5_err_t,
                      "f=%p, name='%s', x_buf=%p, y_buf=%p, z_buf=%p",
		      (h5_file_p)f, name, x_buf, y_buf, z_buf);
	H5_API_RETURN (
		h5b_read_vector3d_data (
			f, name,
			x_buf, y_buf, z_buf,
			H5_INT32_T));
}
///< @}

#ifdef __cplusplus
}
#endif

#endif
