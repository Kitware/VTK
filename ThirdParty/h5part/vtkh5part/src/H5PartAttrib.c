
#include <stdlib.h>
#include <string.h>

#include <vtk_hdf5.h>
#include "H5Part.h"
#include "H5PartErrors.h"
#include "H5PartPrivate.h"


/*!
  \ingroup h5part_attrib

  Writes a \c value of type floating points (64-bit)
  to the root ("/") of the file
  as attribute \c name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartWriteFileAttribFloat64 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *name,		        /*!< IN: attribute name */
	const h5part_float64_t value	        /*!< IN: attribute value */
	) {

	SET_FNAME ( "H5PartWriteFileAttribFloat64" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );

	h5part_int64_t herr = _H5Part_write_file_attrib (
		f,
		name,
                H5T_NATIVE_DOUBLE,
                (void*)&value,
		1 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes a \c value of type floating points (32-bit)
  to the root ("/") of the file
  as attribute \c name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartWriteFileAttribFloat32 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *name,		        /*!< IN: attribute name */
	const h5part_float32_t value	        /*!< IN: attribute value */
	) {

	SET_FNAME ( "H5PartWriteFileAttribFloat32" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );

	h5part_int64_t herr = _H5Part_write_file_attrib (
		f,
		name,
                H5T_NATIVE_FLOAT,
                (void*)&value,
		1 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes a \c value of type integers (64-bit)
  to the root ("/") of the file
  as attribute \c name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartWriteFileAttribInt64 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *name,		        /*!< IN: attribute name */
	const h5part_int64_t value	        /*!< IN: attribute value */
	) {

	SET_FNAME ( "H5PartWriteFileAttribInt64" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );

	h5part_int64_t herr = _H5Part_write_file_attrib (
		f,
		name,
                H5T_NATIVE_INT64,
                (void*)&value,
		1 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes a \c value of type integers (32-bit)
  to the root ("/") of the file
  as attribute \c name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartWriteFileAttribInt32 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *name,		        /*!< IN: attribute name */
	const h5part_int32_t value	        /*!< IN: attribute value */
	) {

	SET_FNAME ( "H5PartWriteFileAttribInt32" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );

	h5part_int64_t herr = _H5Part_write_file_attrib (
		f,
		name,
                H5T_NATIVE_INT32,
                (void*)&value,
		1 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes a \c value of type floating points (64-bit)
  to the root ("/") of the file
  as attribute \c name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartWriteStepAttribFloat64 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *name,		        /*!< IN: attribute name */
	const h5part_float64_t value	        /*!< IN: attribute value */
	) {

	SET_FNAME ( "H5PartWriteStepAttribFloat64" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );

	h5part_int64_t herr = _H5Part_write_step_attrib (
		f,
		name,
                H5T_NATIVE_DOUBLE,
                (void*)&value,
		1 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes a \c value of type floating points (32-bit)
  to the root ("/") of the file
  as attribute \c name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartWriteStepAttribFloat32 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *name,		        /*!< IN: attribute name */
	const h5part_float32_t value	        /*!< IN: attribute value */
	) {

	SET_FNAME ( "H5PartWriteStepAttribFloat32" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );

	h5part_int64_t herr = _H5Part_write_step_attrib (
		f,
		name,
                H5T_NATIVE_FLOAT,
                (void*)&value,
		1 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes a \c value of type integers (64-bit)
  to the root ("/") of the file
  as attribute \c name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartWriteStepAttribInt64 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *name,		        /*!< IN: attribute name */
	const h5part_int64_t value	        /*!< IN: attribute value */
	) {

	SET_FNAME ( "H5PartWriteStepAttribInt64" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );

	h5part_int64_t herr = _H5Part_write_step_attrib (
		f,
		name,
                H5T_NATIVE_INT64,
                (void*)&value,
		1 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes a \c value of type integers (32-bit)
  to the root ("/") of the file
  as attribute \c name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartWriteStepAttribInt32 (
	H5PartFile *f,				/*!< IN: file handle */
	const char *name,		        /*!< IN: attribute name */
	const h5part_int32_t value	        /*!< IN: attribute value */
	) {

	SET_FNAME ( "H5PartWriteStepAttribInt32" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );

	h5part_int64_t herr = _H5Part_write_step_attrib (
		f,
		name,
                H5T_NATIVE_INT32,
                (void*)&value,
		1 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}
