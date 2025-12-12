/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5_STEP_ATTRIBS_H
#define __H5_STEP_ATTRIBS_H

#include <string.h>

#include "h5core/h5_log.h"
#include "h5core/h5_step_attribs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
   !   _                   _          
   !  (_)_ __   __ _ _   _(_)_ __ ___ 
   !  | | '_ \ / _` | | | | | '__/ _ \
   !  | | | | | (_| | |_| | | | |  __/
   !  |_|_| |_|\__, |\__,_|_|_|  \___|
   !              |_|
   !
*/
/**
   \addtogroup h5_iteration_attribs
   @{
*/

/**
  Query the number of attributes attached to the current
  step/iteration.

  \return   number of attributes
  \return   \c H5_FAILURE on error
*/
static inline h5_int64_t
H5GetNumStepAttribs (
	const h5_file_t f              ///< [in]  file handle
	) {
	H5_API_ENTER (h5_int64_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5_get_num_iteration_attribs (f));
}

/**
  Gets the name, type and number of elements of the iteration attribute
  given by its index.

  This function can be used to retrieve all attributes attached to the
  current step/iteration by looping from \c 0 to the number of attribute
  minus one.  The number of attributes attached to the current
  step/iteration can be queried by calling \ref H5GetNumStepAttribs().

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5GetStepAttribInfo (
	const h5_file_t f,		///< [in]  file handle
	const h5_size_t idx,    	///< [in]  index of attribute to query
	char* const attrib_name,     	///< [out] name of attribute
	const h5_size_t len_attrib_name,///< [in]  size of buffer \c name
	h5_int64_t* const attrib_type,  ///< [out] type of attribute
	h5_size_t* const nelems         ///< [out] number of elements
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, "
		      "idx=%llu, attrib_name=%p, len_attrib_name=%llu, "
		      "attrib_type=%p, nelems=%p",
                      (h5_file_p)f,
                      (long long unsigned)idx,
                      attrib_name,
                      (long long unsigned)len_attrib_name,
                      attrib_type,
                      nelems);
	H5_API_RETURN (
		h5_get_iteration_attrib_info_by_idx (
			f,
			idx,
			attrib_name,
			len_attrib_name,
			attrib_type,
			nelems));
}

static inline h5_err_t
H5GetStepAttribName (
	const h5_file_t f,		///< [in]  file handle
	const h5_size_t idx,    	///< [in]  index of attribute to query
	char* const attrib_name,	///< [out] name of attribute
	const h5_size_t len_attrib_name ///< [in]  size of buffer \c attrib_name
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, "
		      "idx=%llu, attrib_name=%p, len_attrib_name=%llu, ",
                      (h5_file_p)f,
                      (long long unsigned)idx,
                      attrib_name,
                      (long long unsigned)len_attrib_name);
	H5_API_RETURN (
		h5_get_iteration_attrib_info_by_idx (
			f,
			idx,
			attrib_name, len_attrib_name,
			NULL,
			NULL));
}

/**
   Determines whether a step attribute with a given name exists in
   current step/iteration.

   \return      true (value \c >0) if atrribute exists
   \return      false (\c 0) if attribute does not exist
   \return      \c H5_FAILURE on error
 */
static inline h5_err_t
H5HasStepAttrib (
	const h5_file_t f,	        ///< [in]  file handle
	const char* const attrib_name	///< [in]  name of attribute to query
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, "
		      "attrib_name=%p",
		      (h5_file_p)f,
		      attrib_name);
	H5_API_RETURN (
		h5_has_iteration_attrib (
			f,
			attrib_name));
}

/**
  Gets the type and number of elements of a given step/iteration
  attribute.

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5GetStepAttribInfoByName (
	const h5_file_t f,		///< [in]  file handle
	const char* const attrib_name,  ///< [in]  name of attribute to query
	h5_int64_t* const attrib_type,        ///< [out] type of attribute
	h5_size_t* const nelems               ///< [out] number of elements
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, "
		      "attrib_name=%s, "
		      "attrib_type=%p, nelems=%p",
                      (h5_file_p)f,
                      attrib_name,
                      attrib_type, nelems);
	H5_API_RETURN (
		h5_get_iteration_attrib_info_by_name (
			f,
			attrib_name,
			attrib_type, nelems));
}

/*
  !                 _ _       
  !  __      ___ __(_) |_ ___ 
  !  \ \ /\ / / '__| | __/ _ \
  !   \ V  V /| |  | | ||  __/
  !    \_/\_/ |_|  |_|\__\___|
*/

/**
   \fn h5_err_t H5WriteStepAttribString (
	const h5_file_t f,
	const char* attrib_name,
	const char* buffer
	)

   \fn  h5_err_t H5WriteStepAttribFloat64 (
	const h5_file_t f,
	const char* attrib_name,
	const h5_float64_t* buffer,
	const h5_size_t nelems
	)

   \fn h5_err_t H5WriteStepAttribFloat32 (
	const h5_file_t f,
	const char* attrib_name,
	const h5_float32_t* buffer,
	const h5_size_t nelems
	)

   \fn h5_err_t H5WriteStepAttribInt64 (
	const h5_file_t f,
	const char* attrib_name,
	const h5_int64_t* buffer,
	const h5_size_t nelems
	)

   \fn h5_err_t H5WriteStepAttribInt32 (
	const h5_file_t f,
	const char* attrib_name,
	const h5_int32_t* buffer,
	const h5_size_t nelems
	)

  Attach an attribute to current step.

  The type of the attribute can be

  - a C string (\c char*)
  - an array of 64bit floating point numbers (\c h5_float64_t)
  - an array of 32bit floating point numbers (\c h5_float32_t)
  - an array of 64bit integers (\c h5_int64_t)
  - an array of 32bit integers (\c h5_int32_t)

  \param f		[in]  file handle
  \param attrib_name	[in]  the attribute name
  \param buffer		[in]  data to be written
  \param nelems		[in]  number of elements to be written

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5ReadStepAttribString()
  \see H5ReadStepAttribFloat64()
  \see H5ReadStepAttribFloat32()
  \see H5ReadStepAttribInt64()
  \see H5ReadStepAttribInt32()
*/
static inline h5_err_t
H5WriteStepAttribString (
	const h5_file_t f,
	const char* attrib_name,
	const char* buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer='%s'",
                      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_write_iteration_attrib (
			f, 
			attrib_name,
			H5_STRING_T,
			buffer,
			strlen(buffer) + 1 ));
}

static inline h5_err_t
H5WriteStepAttribFloat64 (
	const h5_file_t f,
	const char* const attrib_name,
	const h5_float64_t* const buffer,
	const h5_size_t nelems
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p, nelems=%llu",
		      (h5_file_p)f, attrib_name, buffer, (long long unsigned)nelems);
	H5_API_RETURN (
		h5_write_iteration_attrib (
			f,
			attrib_name,
			H5_FLOAT64_T,
			buffer,
			nelems));
}

static inline h5_err_t
H5WriteStepAttribFloat32 (
	const h5_file_t f,
	const char* const attrib_name,
	const h5_float32_t* const buffer,
	const h5_size_t nelems
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p, nelems=%llu",
		      (h5_file_p)f, attrib_name, buffer, (long long unsigned)nelems);
	H5_API_RETURN (
		h5_write_iteration_attrib (
			f,
			attrib_name,
			H5_FLOAT32_T,
			buffer,
			nelems ));
}

static inline h5_err_t
H5WriteStepAttribInt64 (
	const h5_file_t f,
	const char* const attrib_name,
	const h5_int64_t* const buffer,
	const h5_size_t nelems
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p, nelems=%llu",
		      (h5_file_p)f, attrib_name, buffer, (long long unsigned)nelems);
	H5_API_RETURN (
		h5_write_iteration_attrib (
			f,
			attrib_name,
			H5_INT64_T,
			buffer,
			nelems));
}

static inline h5_err_t
H5WriteStepAttribInt32 (
	const h5_file_t f,
	const char* const attrib_name,
	const h5_int32_t* const buffer,
	const h5_size_t nelems
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p, nelems=%llu",
		      (h5_file_p)f, attrib_name, buffer, (long long unsigned)nelems);
	H5_API_RETURN (
		h5_write_iteration_attrib (
			f,
			attrib_name,
			H5_INT32_T,
			buffer,
			nelems));
}

/*
  !                      _ 
  !   _ __ ___  __ _  __| |
  !  | '__/ _ \/ _` |/ _` |
  !  | | |  __/ (_| | (_| |
  !  |_|  \___|\__,_|\__,_|
 */

/**
   \fn h5_err_t H5ReadStepAttribString (
	const h5_file_t f,
	const char* attrib_name,
	char* buffer
	)

   \fn h5_err_t H5ReadStepAttribFloat64 (
	const h5_file_t f,
	const char* attrib_name,
	h5_float64_t* buffer
	)

   \fn h5_err_t H5ReadStepAttribFloat32 (
	const h5_file_t f,
	const char* attrib_name,
	h5_float32_t* buffer
	)

   \fn h5_err_t H5ReadStepAttribInt64 (
	const h5_file_t f,
	const char* attrib_name,
	h5_int64_t* buffer
	)

   \fn h5_err_t H5ReadStepAttribInt32 (
	const h5_file_t f,
	const char* attrib_name,
	h5_int32_t* buffer
	)

  Read attribute attached to current step.

  \note Make sure that the size of the buffer is large enough!

  \param f		[in]  file handle
  \param attrib_name	[in]  attribute name
  \param buffer		[out] buffer for data to be read

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5GetStepAttribInfo()
  \see H5GetStepAttribInfoByName()
  \see H5WriteStepAttribString()
  \see H5WriteStepAttribFloat64()
  \see H5WriteStepAttribFloat32()
  \see H5WriteStepAttribInt64()
  \see H5WriteStepAttribInt32()
 */

static inline h5_err_t
H5ReadStepAttribString (
	const h5_file_t f,
	const char* const attrib_name,
	char* const buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p",
		      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_read_iteration_attrib (
			f, 
			attrib_name,
			H5_STRING_T,
			(void*)buffer));
}

static inline h5_err_t
H5ReadStepAttribFloat64 (
	const h5_file_t f,
	const char* const attrib_name,
	h5_float64_t* const buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p",
		      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_read_iteration_attrib (
			f,
			attrib_name,
			H5_FLOAT64_T,
			(void*)buffer));
}

static inline h5_err_t
H5ReadStepAttribFloat32 (
	const h5_file_t f,
	const char* const attrib_name,
	h5_float32_t* const buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p",
		      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_read_iteration_attrib (
			f,
			attrib_name,
			H5_FLOAT32_T,
			(void*)buffer));
}

static inline h5_err_t
H5ReadStepAttribInt64 (
	const h5_file_t f,
	const char* const attrib_name,
	h5_int64_t* const buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p",
		      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_read_iteration_attrib (
			f,
			attrib_name,
			H5_INT64_T,
			(void*)buffer));
}

static inline h5_err_t
H5ReadStepAttribInt32 (
	const h5_file_t f,
	const char* const attrib_name,
	h5_int32_t* const buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p",
		      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_read_iteration_attrib (
			f,
			attrib_name,
			H5_INT32_T,
			(void*)buffer));
}
///< @}

#ifdef __cplusplus
}
#endif

#endif
