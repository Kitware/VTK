
#include <stdlib.h>
#include <string.h>

#include <vtk_hdf5.h>
#include "H5Part.h"
#include "H5PartErrors.h"
#include "H5PartPrivate.h"

#include "H5BlockTypes.h"
#include "H5Block.h"
#include "H5BlockPrivate.h"
#include "H5BlockErrors.h"

/*!
  \ingroup h5block_data

  Write a 3-dimensional field \c name from the buffer starting at \c data
  to the current time-step using the defined field layout. Values are
  floating points (64-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dWriteScalarFieldFloat64 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	const h5part_float64_t *data      /*!< IN: scalar data to write */
	) {

	SET_FNAME ( "H5Block3dWriteScalarFieldFloat64" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_create_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_write_data ( f, "0", data, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data

  Read a 3-dimensional field \c name into the buffer starting at \c data from
  the current time-step using the defined field layout. Values are
  floating points (64-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dReadScalarFieldFloat64 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to read */
	h5part_float64_t *data	  /*!< OUT: ptr to read buffer */
	) {

	SET_FNAME ( "H5Block3dReadScalarFieldFloat64" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_open_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_read_data ( f, "0", data, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data
*/
/*!
  Write a 3-dimensional field \c name with 3-dimensional vectors as values
  from the buffers starting at \c x_data, \c y_data and \c z_data to the
  current time-step using the defined field layout. Values are 3-dimensional
  vectors with floating points (64-bit) values.

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dWrite3dVectorFieldFloat64 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	const h5part_float64_t *x_data, /*!< IN: X axis data */
	const h5part_float64_t *y_data, /*!< IN: Y axis data */
	const h5part_float64_t *z_data  /*!< IN: Z axis data */
	) {

	SET_FNAME ( "H5Block3dWrite3dVectorFieldFloat64" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_create_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_write_data ( f, "0", x_data, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;
	herr = _H5Block_write_data ( f, "1", y_data, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;
	herr = _H5Block_write_data ( f, "2", z_data, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data
*/
/*!
  Read a 3-dimensional field \c name with 3-dimensional vectors as values
  from the buffers starting at \c x_data, \c y_data and \c z_data to the
  current time-step using the defined field layout. Values are 3-dimensional
  vectors with floating points (64-bit) values.

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dRead3dVectorFieldFloat64 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	h5part_float64_t *x_data, /*!< OUT: X axis data */
	h5part_float64_t *y_data, /*!< OUT: Y axis data */
	h5part_float64_t *z_data  /*!< OUT: Z axis data */
	) {

	SET_FNAME ( "H5Block3dRead3dVectorFieldFloat64" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_open_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_read_data ( f, "0", x_data, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;
	herr = _H5Block_read_data ( f, "1", y_data, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;
	herr = _H5Block_read_data ( f, "2", z_data, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data

  Write a 3-dimensional field \c name from the buffer starting at \c data
  to the current time-step using the defined field layout. Values are
  floating points (32-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dWriteScalarFieldFloat32 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	const h5part_float32_t *data      /*!< IN: scalar data to write */
	) {

	SET_FNAME ( "H5Block3dWriteScalarFieldFloat32" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_create_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_write_data ( f, "0", data, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data

  Read a 3-dimensional field \c name into the buffer starting at \c data from
  the current time-step using the defined field layout. Values are
  floating points (32-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dReadScalarFieldFloat32 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to read */
	h5part_float32_t *data	  /*!< OUT: ptr to read buffer */
	) {

	SET_FNAME ( "H5Block3dReadScalarFieldFloat32" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_open_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_read_data ( f, "0", data, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data
*/
/*!
  Write a 3-dimensional field \c name with 3-dimensional vectors as values
  from the buffers starting at \c x_data, \c y_data and \c z_data to the
  current time-step using the defined field layout. Values are 3-dimensional
  vectors with floating points (32-bit) values.

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dWrite3dVectorFieldFloat32 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	const h5part_float32_t *x_data, /*!< IN: X axis data */
	const h5part_float32_t *y_data, /*!< IN: Y axis data */
	const h5part_float32_t *z_data  /*!< IN: Z axis data */
	) {

	SET_FNAME ( "H5Block3dWrite3dVectorFieldFloat32" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_create_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_write_data ( f, "0", x_data, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;
	herr = _H5Block_write_data ( f, "1", y_data, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;
	herr = _H5Block_write_data ( f, "2", z_data, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data
*/
/*!
  Read a 3-dimensional field \c name with 3-dimensional vectors as values
  from the buffers starting at \c x_data, \c y_data and \c z_data to the
  current time-step using the defined field layout. Values are 3-dimensional
  vectors with floating points (32-bit) values.

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dRead3dVectorFieldFloat32 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	h5part_float32_t *x_data, /*!< OUT: X axis data */
	h5part_float32_t *y_data, /*!< OUT: Y axis data */
	h5part_float32_t *z_data  /*!< OUT: Z axis data */
	) {

	SET_FNAME ( "H5Block3dRead3dVectorFieldFloat32" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_open_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_read_data ( f, "0", x_data, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;
	herr = _H5Block_read_data ( f, "1", y_data, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;
	herr = _H5Block_read_data ( f, "2", z_data, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data

  Write a 3-dimensional field \c name from the buffer starting at \c data
  to the current time-step using the defined field layout. Values are
  integers (64-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dWriteScalarFieldInt64 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	const h5part_int64_t *data      /*!< IN: scalar data to write */
	) {

	SET_FNAME ( "H5Block3dWriteScalarFieldInt64" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_create_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_write_data ( f, "0", data, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data

  Read a 3-dimensional field \c name into the buffer starting at \c data from
  the current time-step using the defined field layout. Values are
  integers (64-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dReadScalarFieldInt64 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to read */
	h5part_int64_t *data	  /*!< OUT: ptr to read buffer */
	) {

	SET_FNAME ( "H5Block3dReadScalarFieldInt64" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_open_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_read_data ( f, "0", data, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data
*/
/*!
  Write a 3-dimensional field \c name with 3-dimensional vectors as values
  from the buffers starting at \c x_data, \c y_data and \c z_data to the
  current time-step using the defined field layout. Values are 3-dimensional
  vectors with integers (64-bit) values.

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dWrite3dVectorFieldInt64 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	const h5part_int64_t *x_data, /*!< IN: X axis data */
	const h5part_int64_t *y_data, /*!< IN: Y axis data */
	const h5part_int64_t *z_data  /*!< IN: Z axis data */
	) {

	SET_FNAME ( "H5Block3dWrite3dVectorFieldInt64" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_create_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_write_data ( f, "0", x_data, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;
	herr = _H5Block_write_data ( f, "1", y_data, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;
	herr = _H5Block_write_data ( f, "2", z_data, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data
*/
/*!
  Read a 3-dimensional field \c name with 3-dimensional vectors as values
  from the buffers starting at \c x_data, \c y_data and \c z_data to the
  current time-step using the defined field layout. Values are 3-dimensional
  vectors with integers (64-bit) values.

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dRead3dVectorFieldInt64 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	h5part_int64_t *x_data, /*!< OUT: X axis data */
	h5part_int64_t *y_data, /*!< OUT: Y axis data */
	h5part_int64_t *z_data  /*!< OUT: Z axis data */
	) {

	SET_FNAME ( "H5Block3dRead3dVectorFieldInt64" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_open_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_read_data ( f, "0", x_data, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;
	herr = _H5Block_read_data ( f, "1", y_data, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;
	herr = _H5Block_read_data ( f, "2", z_data, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data

  Write a 3-dimensional field \c name from the buffer starting at \c data
  to the current time-step using the defined field layout. Values are
  integers (32-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dWriteScalarFieldInt32 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	const h5part_int32_t *data      /*!< IN: scalar data to write */
	) {

	SET_FNAME ( "H5Block3dWriteScalarFieldInt32" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_create_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_write_data ( f, "0", data, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data

  Read a 3-dimensional field \c name into the buffer starting at \c data from
  the current time-step using the defined field layout. Values are
  integers (32-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dReadScalarFieldInt32 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to read */
	h5part_int32_t *data	  /*!< OUT: ptr to read buffer */
	) {

	SET_FNAME ( "H5Block3dReadScalarFieldInt32" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_open_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_read_data ( f, "0", data, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data
*/
/*!
  Write a 3-dimensional field \c name with 3-dimensional vectors as values
  from the buffers starting at \c x_data, \c y_data and \c z_data to the
  current time-step using the defined field layout. Values are 3-dimensional
  vectors with integers (32-bit) values.

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dWrite3dVectorFieldInt32 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	const h5part_int32_t *x_data, /*!< IN: X axis data */
	const h5part_int32_t *y_data, /*!< IN: Y axis data */
	const h5part_int32_t *z_data  /*!< IN: Z axis data */
	) {

	SET_FNAME ( "H5Block3dWrite3dVectorFieldInt32" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_create_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_write_data ( f, "0", x_data, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;
	herr = _H5Block_write_data ( f, "1", y_data, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;
	herr = _H5Block_write_data ( f, "2", z_data, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_data
*/
/*!
  Read a 3-dimensional field \c name with 3-dimensional vectors as values
  from the buffers starting at \c x_data, \c y_data and \c z_data to the
  current time-step using the defined field layout. Values are 3-dimensional
  vectors with integers (32-bit) values.

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dRead3dVectorFieldInt32 (
	H5PartFile *f,		  /*!< IN: file handle */
	const char *name,	       /*!< IN: name of dataset to write */
	h5part_int32_t *x_data, /*!< OUT: X axis data */
	h5part_int32_t *y_data, /*!< OUT: Y axis data */
	h5part_int32_t *z_data  /*!< OUT: Z axis data */
	) {

	SET_FNAME ( "H5Block3dRead3dVectorFieldInt32" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP ( f );
	CHECK_LAYOUT ( f );

	h5part_int64_t herr = _H5Block_open_field_group ( f, name );
	if ( herr < 0 ) return herr;

	herr = _H5Block_read_data ( f, "0", x_data, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;
	herr = _H5Block_read_data ( f, "1", y_data, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;
	herr = _H5Block_read_data ( f, "2", z_data, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_attrib

  Write \c attrib_value with type floating points (64-bit) as attribute \c attrib_name
  to field \c field_name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5BlockWriteFieldAttribFloat64 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const char *attrib_name,		/*!< IN: attribute name */
	const h5part_float64_t *attrib_value,		/*!< IN: attribute value */
	const h5part_int64_t attrib_nelem	/*!< IN: number of elements */
	) {

	SET_FNAME ( "H5BlockWriteFieldAttribFloat64" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	return _write_field_attrib (
		f,
		field_name,
		attrib_name,
                H5T_NATIVE_DOUBLE,
                attrib_value,
		attrib_nelem );
}

/*!
  \ingroup h5block_attrib

  Write \c attrib_value with type floating points (32-bit) as attribute \c attrib_name
  to field \c field_name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5BlockWriteFieldAttribFloat32 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const char *attrib_name,		/*!< IN: attribute name */
	const h5part_float32_t *attrib_value,		/*!< IN: attribute value */
	const h5part_int64_t attrib_nelem	/*!< IN: number of elements */
	) {

	SET_FNAME ( "H5BlockWriteFieldAttribFloat32" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	return _write_field_attrib (
		f,
		field_name,
		attrib_name,
                H5T_NATIVE_FLOAT,
                attrib_value,
		attrib_nelem );
}

/*!
  \ingroup h5block_attrib

  Write \c attrib_value with type integers (64-bit) as attribute \c attrib_name
  to field \c field_name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5BlockWriteFieldAttribInt64 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const char *attrib_name,		/*!< IN: attribute name */
	const h5part_int64_t *attrib_value,		/*!< IN: attribute value */
	const h5part_int64_t attrib_nelem	/*!< IN: number of elements */
	) {

	SET_FNAME ( "H5BlockWriteFieldAttribInt64" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	return _write_field_attrib (
		f,
		field_name,
		attrib_name,
                H5T_NATIVE_INT64,
                attrib_value,
		attrib_nelem );
}

/*!
  \ingroup h5block_attrib

  Write \c attrib_value with type integers (32-bit) as attribute \c attrib_name
  to field \c field_name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5BlockWriteFieldAttribInt32 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const char *attrib_name,		/*!< IN: attribute name */
	const h5part_int32_t *attrib_value,		/*!< IN: attribute value */
	const h5part_int64_t attrib_nelem	/*!< IN: number of elements */
	) {

	SET_FNAME ( "H5BlockWriteFieldAttribInt32" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	return _write_field_attrib (
		f,
		field_name,
		attrib_name,
                H5T_NATIVE_INT32,
                attrib_value,
		attrib_nelem );
}
