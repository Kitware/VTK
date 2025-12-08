/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __H5BLOCK_ATTRIB
#define __H5BLOCK_ATTRIB

#include <string.h>

#include "h5core/h5_log.h"
#include "h5core/h5b_attribs.h"

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
   \addtogroup h5block_attrib
    @{
*/

/**
  Query the number of attributes attached to a given field.

  \return number of attributes
  \return \c H5_FAILURE on error

  \see H5BlockGetFieldAttribInfo()
*/
static inline h5_ssize_t
H5BlockGetNumFieldAttribs (
	const h5_file_t f,		///< [in]  file handle
	const char* field_name		///< [in]  field name
	) {
	H5_API_ENTER (h5_ssize_t,
                      "f=%p, field_name='%s'",
                      (h5_file_p)f, field_name);
	H5_API_RETURN (
		h5b_get_num_field_attribs (
			f, field_name));
}

/**
  Get the name, type and number of elements of a field attribute
  given by its index.

  This function can be used to retrieve all attributes attached to the
  specified field by looping from \c 0 to the number of attributes
  minus one.  The number of attributes attached to the field can be
  queried by calling \ref H5BlockGetNumFieldAttribs().

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5BlockGetNumFieldAttribs()
*/
static inline h5_err_t
H5BlockGetFieldAttribInfo (
	const h5_file_t f,		///< [in]  file handle.
	const char* field_name,		///< [in]  field name.
	const h5_size_t attrib_idx,	///< [in]  index of attribute to query
	char* attrib_name,		///< [out] name of attribute.
	const h5_size_t len_attrib_name,///< [in]  size of buffer \c attrib_name.
	h5_int64_t* attrib_type,	///< [out] type of value.
	h5_size_t* attrib_nelem         ///< [out] number of elements.
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p field_name='%s', "
		      "attrib_idx=%llu, "
		      "attrib_name=%p, len_attrib_name=%llu, "
		      "attrib_type=%p, "
		      "attrib_nelem=%p",
		      (h5_file_p)f,
		      field_name,
		      (long long unsigned)attrib_idx,
		      attrib_name, (long long unsigned)len_attrib_name,
		      attrib_type,
		      attrib_nelem);
	H5_API_RETURN (
		h5b_get_field_attrib_info_by_idx (
			f,
			field_name,
			attrib_idx,
			attrib_name,
			len_attrib_name,
			attrib_type,
			attrib_nelem));
}

/**
   Determines whether a field attribute with a given name exists.

   \return      true (value \c >0) if atrribute exists
   \return      false (\c 0) if attribute does not exist
   \return      \c H5_FAILURE on error
 */
static inline h5_err_t
H5BlockHasFieldAttrib (
	const h5_file_t f,		///< [in]  file handle
	const char* const field_name,	///< [in]  field name
	const char* const attrib_name	///< [in]  name of attribute to query
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p field_name='%s', "
		      "attrib_name=%p, ",
		      (h5_file_p)f,
		      field_name,
		      attrib_name);
	H5_API_RETURN (
		h5b_has_field_attrib (
			f,
			field_name,
			attrib_name));
}

/**
  Get the type and number of elements of a given field attribute.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5BlockGetFieldAttribInfoByName (
	const h5_file_t f,		///< [in]  file handle
	const char* const field_name,	///< [in]  field name
	const char* const attrib_name,	///< [in]  name of attribute to query
	h5_int64_t* attrib_type,	///< [out] type of value
	h5_size_t* attrib_nelem         ///< [out] number of elements
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p field_name='%s', "
		      "attrib_name=%p, "
		      "attrib_type=%p, "
		      "attrib_nelem=%p",
		      (h5_file_p)f,
		      field_name,
		      attrib_name,
		      attrib_type,
		      attrib_nelem);
	H5_API_RETURN (
		h5b_get_field_attrib_info_by_name (
			f,
			field_name,
			attrib_name,
			attrib_type,
			attrib_nelem));
}

/*
  !                       _       _         _   _        
  !   ___ _ __   ___  ___(_) __ _| |   __ _| |_| |_ _ __ 
  !  / __| '_ \ / _ \/ __| |/ _` | |  / _` | __| __| '__|
  !  \__ \ |_) |  __/ (__| | (_| | | | (_| | |_| |_| |   
  !  |___/ .__/ \___|\___|_|\__,_|_|  \__,_|\__|\__|_|   
  !      |_|                                          
 */

/**
  Set field origin.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5Block3dSetFieldOrigin (
	const h5_file_t f,		///< [in]  file handle.
	const char* field_name,		///< [in]  field name.
	const h5_float64_t x_origin,	///< [in]  X origin.
	const h5_float64_t y_origin,	///< [in]  Y origin.
	const h5_float64_t z_origin	///< [in]  Z origin.
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, field_name='%s', "
		      "x_origin=%g, y_origin=%g, z_origin=%g",
		      (h5_file_p)f, field_name,
		      x_origin, y_origin, z_origin);
	h5_float64_t origin[3] = { x_origin, y_origin, z_origin };
	H5_API_RETURN (
		h5b_write_field_attrib (
			f,
			field_name,
			H5BLOCK_FIELD_ORIGIN_NAME,
			H5_FLOAT64_T, 
			origin,
			3));
}

/**
  Get field origin.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5Block3dGetFieldOrigin (
	const h5_file_t f,		///< [in]  file handle.
	const char* field_name,		///< [in]  field name.
	h5_float64_t* x_origin,		///< [out] X origin.
	h5_float64_t* y_origin,		///< [out] Y origin.
	h5_float64_t* z_origin		///< [out] Z origin.
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, field_name='%s', x_origin=%p, y_origin=%p, z_origin=%p",
		      (h5_file_p)f, field_name, x_origin, y_origin, z_origin);
	h5_float64_t origin[3];

	TRY (h5b_read_field_attrib (
		     f,
		     field_name,
		     H5BLOCK_FIELD_ORIGIN_NAME,
		     H5_FLOAT64_T,
		     origin));

	*x_origin = origin[0];
	*y_origin = origin[1];
	*z_origin = origin[2];

	H5_API_RETURN (H5_SUCCESS);
}

/**
  Set field spacing for field \c field_name in the current
  step/iteration.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5Block3dSetFieldSpacing (
	const h5_file_t f,		///< [in]  file handle.
	const char* field_name,		///< [in]  field name.
	const h5_float64_t x_spacing,	///< [in]  X spacing.
	const h5_float64_t y_spacing,	///< [in]  Y spacing.
	const h5_float64_t z_spacing	///< [in]  Z spacing.
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, field_name='%s', "
		      "x_spacing=%g, y_spacing=%g, z_spacing=%g",
		      (h5_file_p)f, field_name,
		      x_spacing, y_spacing, z_spacing);
	h5_float64_t spacing[3] = { x_spacing, y_spacing, z_spacing };
	H5_API_RETURN (
		h5b_write_field_attrib (
			f,
			field_name,
			H5BLOCK_FIELD_SPACING_NAME,
			H5_FLOAT64_T, 
			spacing,
			3));
}

/**
  Get field spacing for field \c field_name in the current
  step/iteration.

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5Block3dGetFieldSpacing (
	const h5_file_t f,		///< [in]  file handle.
	const char* field_name,		///< [in]  field name.
	h5_float64_t* x_spacing,	///< [out] X spacing.
	h5_float64_t* y_spacing,	///< [out] Y spacing.
	h5_float64_t* z_spacing		///< [out] Z spacing.
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, field_name='%s', "
                      "x_spacing=%p, y_spacing=%p, z_spacing=%p",
		      (h5_file_p)f, field_name, x_spacing, y_spacing, z_spacing);
	h5_float64_t spacing[3];
	TRY (h5b_read_field_attrib (
		     f,
		     field_name,
		     H5BLOCK_FIELD_SPACING_NAME,
		     H5_FLOAT64_T,
		     spacing));
	*x_spacing = spacing[0];
	*y_spacing = spacing[1];
	*z_spacing = spacing[2];
	H5_API_RETURN (H5_SUCCESS);
}

/**
   \fn h5_err_t H5Block3dSetFieldXCoords (
	const h5_file_t f,
	const char* field_name,
	const h5_float64_t* const coords,
	const h5_int64_t n_coords
	)

   \fn h5_err_t H5Block3dSetFieldYCoords (
	const h5_file_t f,
	const char* field_name,
	const h5_float64_t* const coords,
	const h5_int64_t n_coords
	)

   \fn h5_err_t H5Block3dSetFieldZCoords (
	const h5_file_t f,
	const char* field_name,
	const h5_float64_t* const coords,
	const h5_int64_t n_coords
	)

  Set an explicit list of X,Y respective Z coordinates for field \c
  field_name in the current step/iteration. The coordinates are a 1D array
  of floating point values with dimension \c n_coords.

  By convention, the \c coords array should have the same length as
  the X, Y respective Z dimension of the field. A warning will be
  printed if not.

  \param f	    [in] file handle 
  \param field_name [in] field name
  \param coords	    [in] X, Y or Z coordinates
  \param n_coords   [in] number of coordinates

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5Block3dSetFieldXCoords (
	const h5_file_t f,
	const char* field_name,
	const h5_float64_t* const coords,
	const h5_int64_t n_coords
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, "
                      "field_name='%s', "
                      "coords=%p, n_coords=%llu",
                      (h5_file_p)f,
                      field_name,
                      coords, (long long unsigned)n_coords);
        H5_API_RETURN (
		h5b_set_3d_field_coords (
			f, 0, field_name, H5BLOCK_FIELD_XCOORD_NAME,
			coords, n_coords));
}

static inline h5_err_t
H5Block3dSetFieldYCoords (
	const h5_file_t f,
	const char* field_name,
	const h5_float64_t* const coords,
	const h5_int64_t n_coords
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, "
                      "field_name='%s', "
                      "coords=%p, n_coords=%llu",
                      (h5_file_p)f,
                      field_name,
                      coords, (long long unsigned)n_coords);
        H5_API_RETURN (
		h5b_set_3d_field_coords (
			f, 1, field_name, H5BLOCK_FIELD_YCOORD_NAME,
			coords, n_coords));
}
	
static inline h5_err_t
H5Block3dSetFieldZCoords (
	const h5_file_t f,
	const char* field_name,
	const h5_float64_t* const coords,
	const h5_int64_t n_coords
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, "
                      "field_name='%s', "
                      "coords=%p, n_coords=%llu",
                      (h5_file_p)f,
                      field_name,
                      coords, (long long unsigned)n_coords);
        H5_API_RETURN (
		h5b_set_3d_field_coords (
			f, 2, field_name, H5BLOCK_FIELD_ZCOORD_NAME,
			coords, n_coords));
}

/**
   \fn h5_err_t H5Block3dGetFieldXCoords (
	const h5_file_t f,
	const char* field_name,
	h5_float64_t* const coords,
	const h5_int64_t n_coords
	)

   \fn h5_err_t H5Block3dGetFieldYCoords (
	const h5_file_t f,
	const char* field_name,
	h5_float64_t* const coords,
	const h5_int64_t n_coords
	)

   \fn h5_err_t H5Block3dGetFieldZCoords (
	const h5_file_t f,
	const char* field_name,
	h5_float64_t* const coords,
	const h5_int64_t n_coords
	)

  Get the explicit list of X, Y respective Z coordinates for field \c
  field_name in the current step/iteration. The coordinates are read
  into the 1D array \c coords which has length \c n_coords.

  By convention, the \c coords array should have the same length as
  the X, Y respective Z dimension of the field. A warning will be
  printed if they differ.

  \param f	    [in] file handle 
  \param field_name [in] field name
  \param coords	    [in] X, Y or Z coordinates
  \param n_coords   [in] number of coordinates

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error
*/
static inline h5_err_t
H5Block3dGetFieldXCoords (
	const h5_file_t f,
	const char* field_name,
	h5_float64_t* const coords,
	const h5_int64_t n_coords
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, "
                      "field_name='%s', "
                      "coords=%p, n_coords=%llu",
                      (h5_file_p)f,
                      field_name,
                      coords, (long long unsigned)n_coords);
        H5_API_RETURN (
		h5b_get_3d_field_coords (
			f, 0, field_name, H5BLOCK_FIELD_XCOORD_NAME,
			coords, n_coords));
}

static inline h5_err_t
H5Block3dGetFieldYCoords (
	const h5_file_t f,
	const char* field_name,
	h5_float64_t* const coords,
	const h5_int64_t n_coords
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, "
                      "field_name='%s', "
                      "coords=%p, n_coords=%llu",
                      (h5_file_p)f,
                      field_name,
                      coords, (long long unsigned)n_coords);
        H5_API_RETURN (
		h5b_get_3d_field_coords (
			f, 1, field_name, H5BLOCK_FIELD_YCOORD_NAME,
			coords, n_coords));
}

static inline h5_err_t
H5Block3dGetFieldZCoords (
	const h5_file_t f,
	const char* field_name,
	h5_float64_t* const coords,
	const h5_int64_t n_coords
	) {
	H5_API_ENTER (h5_err_t,
                      "f=%p, "
                      "field_name='%s', "
                      "coords=%p, n_coords=%llu",
                      (h5_file_p)f,
                      field_name,
                      coords, (long long unsigned)n_coords);
        H5_API_RETURN (
		h5b_get_3d_field_coords (
			f, 2, field_name, H5BLOCK_FIELD_ZCOORD_NAME,
			coords, n_coords));
}

/*
  !                 _ _       
  !  __      ___ __(_) |_ ___ 
  !  \ \ /\ / / '__| | __/ _ \
  !   \ V  V /| |  | | ||  __/
  !    \_/\_/ |_|  |_|\__\___|
*/

/**
   \fn h5_err_t H5BlockWriteFieldAttribString (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	const char* buffer
	)

   \fn  h5_err_t H5BlockWriteFieldAttribFloat64 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	const h5_float64_t* buffer,
	const h5_size_t nelems
	)

   \fn h5_err_t H5BlockWriteFieldAttribFloat32 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	const h5_float32_t* buffer,
	const h5_size_t nelems
	)

   \fn h5_err_t H5BlockWriteFieldAttribInt64 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	const h5_int64_t* buffer,
	const h5_size_t nelems
	)

   \fn h5_err_t H5BlockWriteFieldAttribInt32 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	const h5_int32_t* buffer,
	const h5_size_t nelems
	)

  Attach an attribute to a given field.

  The type of the attribute can be
  - a C string (\c char*)
  - an array of 64bit floating point numbers (\c h5_float64_t)
  - an array of 32bit floating point numbers (\c h5_float32_t)
  - an array of 64bit integers (\c h5_int64_t)
  - an array of 32bit integers (\c h5_int32_t)

  \param f		file handle
  \param field_name	name of field the attribute is attached to
  \param attrib_name	the attribute name
  \param buffer		data to be written
  \param nelems		number of elements to be written

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5BlockReadFieldAttribString()
  \see H5BlockReadFieldAttribFloat64()
  \see H5BlockReadFieldAttribFloat32()
  \see H5BlockReadFieldAttribInt64()
  \see H5BlockReadFieldAttribInt32()
*/
static inline h5_err_t
H5BlockWriteFieldAttribString (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	const char* buffer
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, "
		      "field_name='%s', "
		      "attrib_name='%s', "
		      "buffer='%s'",
		      (h5_file_p)f,
		      field_name,
		      attrib_name,
		      buffer);
	H5_API_RETURN (
		h5b_write_field_attrib (
			f,
			field_name,
			attrib_name,
			H5_STRING_T,
			buffer,
			strlen(buffer) + 1));
}

static inline h5_err_t
H5BlockWriteFieldAttribFloat64 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	const h5_float64_t* buffer,
	const h5_size_t nelems
	) {

	H5_API_ENTER (h5_err_t,
		      "f=%p, field_name='%s', attrib_name='%s', "
		      "buffer=%p, nelems=%lld",
		      (h5_file_p)f, field_name, attrib_name, buffer, (long long)nelems);
	H5_API_RETURN (
		h5b_write_field_attrib (
			f,
			field_name,
			attrib_name,
			H5_FLOAT64_T,
			buffer,
			nelems));
}

static inline h5_err_t
H5BlockWriteFieldAttribFloat32 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	const h5_float32_t* buffer,
	const h5_size_t nelems
	) {

	H5_API_ENTER (h5_err_t,
		      "f=%p, field_name='%s', attrib_name='%s', "
		      "buffer=%p, nelems=%lld",
		      (h5_file_p)f, field_name, attrib_name, buffer, (long long)nelems);
	H5_API_RETURN (
		h5b_write_field_attrib (
			f,
			field_name,
			attrib_name,
			H5_FLOAT32_T,
			buffer,
			nelems));
}

static inline h5_err_t
H5BlockWriteFieldAttribInt64 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	const h5_int64_t* buffer,
	const h5_size_t nelems
	) {

	H5_API_ENTER (h5_err_t,
		      "f=%p, field_name='%s', attrib_name='%s', buffer=%p, nelems=%lld",
		      (h5_file_p)f, field_name, attrib_name, buffer, (long long)nelems);
	H5_API_RETURN (
		h5b_write_field_attrib (
			f,
			field_name,
			attrib_name,
			H5_INT64_T,
			buffer,
			nelems));
}

static inline h5_err_t
H5BlockWriteFieldAttribInt32 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	const h5_int32_t* buffer,
	const h5_size_t nelems
	) {

	H5_API_ENTER (h5_err_t,
		      "f=%p, field_name='%s', attrib_name='%s', "
		      "buffer=%p, nelems=%lld",
		      (h5_file_p)f, field_name, attrib_name, buffer, (long long)nelems);
	H5_API_RETURN (
		h5b_write_field_attrib (
			f,
			field_name,
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
   \fn h5_err_t H5BlockReadFieldAttribString (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	char* buffer
	)

   \fn h5_err_t H5BlockReadFieldAttribFloat64 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	h5_float64_t* buffer
	)

   \fn h5_err_t H5BlockReadFieldAttribFloat32 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	h5_float32_t* buffer
	)

   \fn h5_err_t H5BlockReadFieldAttribInt64 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	h5_int64_t* buffer
	)

   \fn h5_err_t H5BlockReadFieldAttribInt32 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	h5_int32_t* buffer
	)

  Read attribute attached to a given field.

  \note Make sure that the size of the buffer is large enough!

  \param f		[in]  file handle
  \param field_name	[in]  name of field the attribute is attached to
  \param attrib_name	[in]  attribute name
  \param buffer		[out] buffer for data to be read

  \return \c H5_SUCCESS on success
  \return \c H5_FAILURE on error

  \see H5BlockGetFieldAttribInfo()
  \see H5BlockGetFieldAttribInfoByName()
  \see H5BlockWriteFieldAttribString()
  \see H5BlockReadFieldAttribFloat64()
  \see H5BlockReadFieldAttribFloat32()
  \see H5BlockReadFieldAttribInt64()
  \see H5BlockReadFieldAttribInt32()
 */
static inline h5_err_t
H5BlockReadFieldAttribString (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	char* buffer
	) {
	H5_API_ENTER (h5_err_t,
		      "f=%p, "
		      "field_name='%s', "
		      "attrib_name='%s', "
		      "buffer=%p",
		      (h5_file_p)f,
		      field_name,
		      attrib_name,
		      buffer);
	H5_API_RETURN (
		h5b_read_field_attrib (
			f,
			field_name,
			attrib_name,
			H5_STRING_T,
			(void*)buffer));
}

static inline h5_err_t
H5BlockReadFieldAttribFloat64 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	h5_float64_t* buffer
	) {

        H5_API_ENTER (h5_err_t,
                      "f=%p, field_name='%s', attrib_name='%s', buffer=%p",
		      (h5_file_p)f, field_name, attrib_name, buffer);
	H5_API_RETURN (
		h5b_read_field_attrib (
			f,
			field_name,
			attrib_name,
			H5_FLOAT64_T,
			(void*)buffer));
}

static inline h5_err_t
H5BlockReadFieldAttribFloat32 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	h5_float32_t* const buffer
	) {

        H5_API_ENTER (h5_err_t,
                      "f=%p, field_name='%s', attrib_name='%s', buffer=%p",
		      (h5_file_p)f, field_name, attrib_name, buffer);
	H5_API_RETURN (
		h5b_read_field_attrib (
			f,
			field_name,
			attrib_name,
			H5_FLOAT32_T,
			buffer));
}

static inline h5_err_t
H5BlockReadFieldAttribInt64 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	h5_int64_t* const buffer
	) {

        H5_API_ENTER (h5_err_t,
                      "f=%p, field_name='%s', attrib_name='%s', buffer=%p",
		      (h5_file_p)f, field_name, attrib_name, buffer);
	H5_API_RETURN (
		h5b_read_field_attrib (
			f,
			field_name,
			attrib_name,
			H5_INT64_T,
			buffer));
}

static inline h5_err_t
H5BlockReadFieldAttribInt32 (
	const h5_file_t f,
	const char* field_name,
	const char* attrib_name,
	h5_int32_t* buffer
	) {
        H5_API_ENTER (h5_err_t,
                      "f=%p, field_name='%s', attrib_name='%s', buffer=%p",
		      (h5_file_p)f, field_name, attrib_name, buffer);
	H5_API_RETURN (
		h5b_read_field_attrib (
			f,
			field_name,
			attrib_name,
			H5_INT32_T,
			(void*)buffer));
}
///<   @}

#ifdef __cplusplus
}
#endif

#endif
