/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5_FILE_ATTRIBS_H
#define __H5_FILE_ATTRIBS_H

#include <string.h>

#include "h5core/h5_log.h"
#include "h5core/h5_file_attribs.h"

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
   \addtogroup h5_file_attribs
   @{
*/

/**
  Query the number of attributes attached to the file's root ("/").

  \return   Number of attributes
  \return   \c H5_FAILURE on error

  \see H5GetFileAttribInfo()
*/
static inline h5_int64_t
H5GetNumFileAttribs (
	const h5_file_t f              ///< [in]  file handle
	) {
	H5_API_ENTER (h5_int64_t,
                      "f=%p",
                      (h5_file_p)f);
	H5_API_RETURN (h5_get_num_file_attribs (f));
}

/**
  Gets the name, type and number of elements of the file attribute
  given by its index.

  This function can be used to retrieve all attributes bound to the
  file \c f by looping from \c 0 to the number of attribute minus one.
  The number of attributes attached to the file \c f can be queried by
  calling \ref H5GetNumFileAttribs().

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5GetFileAttribInfo (
	const h5_file_t f,		///< [in]  file handle
	const h5_size_t idx,    	///< [in]  index of attribute to query
	char* const attrib_name,	///< [out] name of attribute
	const h5_size_t len_attrib_name,///< [in]  size of buffer \c attrib_name
	h5_int64_t* const attrib_type,  ///< [out] type of value
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
		h5_get_file_attrib_info_by_idx (
			f,
			idx,
			attrib_name, len_attrib_name,
			attrib_type,
			nelems));
}

static inline h5_err_t
H5GetFileAttribName (
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
		h5_get_file_attrib_info_by_idx (
			f,
			idx,
			attrib_name, len_attrib_name,
			NULL,
			NULL));
}

/**
   Determines whether a file attribute with a given name exists.

   \return      true (value \c >0) if atrribute exists
   \return      false (\c 0) if attribute does not exist
   \return      \c H5_FAILURE on error
 */
static inline h5_err_t
H5HasFileAttrib (
	const h5_file_t f,		///< [in]  file handle
	const char* const attrib_name	///< [in]  name of attribute to query
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, "
		      "attrib_name=%p",
		      (h5_file_p)f,
		      attrib_name);
	H5_API_RETURN (
		h5_has_file_attrib (
			f,
			attrib_name));
}

/**
  Get the type and number of elements of the file attribute
  given by its name.

  \return   \c H5_SUCCESS on success
  \return   \c H5_FAILURE on error
*/
static inline h5_err_t
H5GetFileAttribInfoByName (
	const h5_file_t f,		///< [in]  file handle.
	const char* const attrib_name,  ///< [in]  name of attribute.
	h5_int64_t* const attrib_type,  ///< [out] type of value..
	h5_size_t* const nelems         ///< [out] number of elements.
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, "
		      "attrib_name=%s, "
		      "attrib_type=%p, nelems=%p",
                      (h5_file_p)f,
                      attrib_name,
                      attrib_type, nelems);
	H5_API_RETURN (
		h5_get_file_attrib_info_by_name (
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
   \fn h5_err_t H5WriteFileAttribString (
	const h5_file_t f,
	const char* attrib_name,
	const char* buffer
	)

   \fn  h5_err_t H5WriteFileAttribFloat64 (
	const h5_file_t f,
	const char* attrib_name,
	const h5_float64_t* buffer,
	const h5_size_t nelems
	)

   \fn h5_err_t H5WriteFileAttribFloat32 (
	const h5_file_t f,
	const char* attrib_name,
	const h5_float32_t* buffer,
	const h5_size_t nelems
	)

   \fn h5_err_t H5WriteFileAttribInt64 (
	const h5_file_t f,
	const char* attrib_name,
	const h5_int64_t* buffer,
	const h5_size_t nelems
	)

   \fn h5_err_t H5WriteFileAttribInt32 (
	const h5_file_t f,
	const char* attrib_name,
	const h5_int32_t* buffer,
	const h5_size_t nelems
	)

  Attach an attribute to a file given by a handle.

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

  \see H5ReadFileAttribString()
  \see H5ReadFileAttribFloat64()
  \see H5ReadFileAttribFloat32()
  \see H5ReadFileAttribInt64()
  \see H5ReadFileAttribInt32()
*/
static inline h5_err_t
H5WriteFileAttribString (
	const h5_file_t f,
	const char* const attrib_name,
	const char* const buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer='%s'",
                      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_write_file_attrib (
			f,
			attrib_name,
			H5_STRING_T,
			buffer,
			strlen(buffer) + 1 ));
}

static inline h5_err_t
H5WriteFileAttribFloat64 (
	const h5_file_t f,
	const char* const attrib_name,
	const h5_float64_t* const buffers,
	const h5_size_t nelems
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffers=%p, nelems=%llu",
		      (h5_file_p)f, attrib_name, buffers, (long long unsigned)nelems);
	H5_API_RETURN (
		h5_write_file_attrib (
			f,
			attrib_name,
			H5_FLOAT64_T,
			buffers,
			nelems));
}

static inline h5_err_t
H5WriteFileAttribFloat32 (
	const h5_file_t f,
	const char* const attrib_name,
	const h5_float32_t* const buffers,
	const h5_size_t nelems
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffers=%p, nelems=%llu",
		      (h5_file_p)f, attrib_name, buffers, (long long unsigned)nelems);
	H5_API_RETURN (
		h5_write_file_attrib (
			f,
			attrib_name,
			H5_FLOAT32_T,
			buffers,
			nelems ));
}

static inline h5_err_t
H5WriteFileAttribInt64 (
	const h5_file_t f,
	const char* const attrib_name,
	const h5_int64_t* const buffers,
	const h5_size_t nelems	
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffers=%p, nelems=%llu",
		      (h5_file_p)f, attrib_name, buffers, (long long unsigned)nelems);
	H5_API_RETURN (
		h5_write_file_attrib (
			f,
			attrib_name,
			H5_INT64_T,
			buffers,
			nelems));
}

static inline h5_err_t
H5WriteFileAttribInt32 (
	const h5_file_t f,
	const char* const attrib_name,
	const h5_int32_t* const buffers,
	const h5_size_t nelems
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffers=%p, nelems=%llu",
		      (h5_file_p)f, attrib_name, buffers, (long long unsigned)nelems);
	H5_API_RETURN (
		h5_write_file_attrib (
			f,
			attrib_name,
			H5_INT32_T,
			buffers,
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
   \fn h5_err_t H5ReadFileAttribString (
	const h5_file_t f,
	const char* attrib_name,
	char* buffer
	)

   \fn h5_err_t H5ReadFileAttribFloat64 (
	const h5_file_t f,
	const char* attrib_name,
	h5_float64_t* buffer
	)

   \fn h5_err_t H5ReadFileAttribFloat32 (
	const h5_file_t f,
	const char* attrib_name,
	h5_float32_t* buffer
	)

   \fn h5_err_t H5ReadFileAttribInt64 (
	const h5_file_t f,
	const char* attrib_name,
	h5_int64_t* buffer
	)

   \fn h5_err_t H5ReadFileAttribInt32 (
	const h5_file_t f,
	const char* attrib_name,
	h5_int32_t* buffer
	)

  Read attribute attached to a file given by a handle.

  \note Make sure that the size of the buffer is large enough!

  \param f		[in]  file handle
  \param attrib_name	[in]  attribute name
  \param buffer		[out] buffer for data to be read

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5GetFileAttribInfo()
  \see H5GetFileAttribInfoByName()
  \see H5WriteFileAttribString()
  \see H5WriteFileAttribFloat64()
  \see H5WriteFileAttribFloat32()
  \see H5WriteFileAttribInt64()
  \see H5WriteFileAttribInt32()

 */
static inline h5_err_t
H5ReadFileAttribString (
	const h5_file_t f,
	const char* const attrib_name,
	char* const buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer='%p'",
                      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_read_file_attrib (
			f,
			attrib_name,
			H5_STRING_T,
			(void*)buffer));
}

static inline h5_err_t
H5ReadFileAttribFloat64 (
	const h5_file_t f,
	const char* const attrib_name,
	h5_float64_t* const buffer
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, attrib_name='%s', buffer=%p",
		      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_read_file_attrib (
			f,
			attrib_name,
			H5_FLOAT64_T,
			(void*)buffer));
}

static inline h5_err_t
H5ReadFileAttribFloat32 (
	const h5_file_t f,
	const char* const attrib_name,
	h5_float32_t* const buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p",
		      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_read_file_attrib (
			f,
			attrib_name,
			H5_FLOAT32_T,
			(void*)buffer));
}

static inline h5_err_t
H5ReadFileAttribInt64 (
	const h5_file_t f,
	const char* const attrib_name,
	h5_int64_t* const buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p",
		      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_read_file_attrib (
			f,
			attrib_name,
			H5_INT64_T,
			(void*)buffer));
}

static inline h5_err_t
H5ReadFileAttribInt32 (
	const h5_file_t f,
	const char* const attrib_name,
	h5_int32_t* const buffer
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, attrib_name='%s', buffer=%p",
                      (h5_file_p)f, attrib_name, buffer);
	H5_API_RETURN (
		h5_read_file_attrib (
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
