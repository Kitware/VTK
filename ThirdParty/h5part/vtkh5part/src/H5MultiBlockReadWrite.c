
#include <stdlib.h>
#include <string.h>

#include <vtk_hdf5.h>
#include "H5Part.h"
#include "H5PartErrors.h"
#include "H5PartPrivate.h"

#include "H5MultiBlockErrors.h"
#include "H5MultiBlockPrivate.h"

#ifdef PARALLEL_IO


/*!
  \ingroup h5multiblock_data

  Write a multiblock field \c name from the buffer starting at \c data
  to the current time-step using the defined block decomposition and dimensions.
  Values are floating points (64-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5MultiBlock3dWriteFieldFloat64 (
	H5PartFile *f,		/*!< IN: file handle */
	const char *name,       /*!< IN: name of dataset to write */
	const h5part_float64_t *data      /*!< IN: data to write */
	) {

	SET_FNAME( "H5MultiBlock3dWriteFieldFloat64" );

	h5part_int64_t herr;

	herr = _H5MultiBlock_write_data ( f, name, data, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_data

  Allocate a buffer to hold a block from a multiblock field and place the
  pointer in \c data, then read the block into the buffer. Uses the block
  decomposition specified in the file and the defined halo radius.
  Values are floating points (64-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5MultiBlock3dReadFieldFloat64 (
	H5PartFile *f,		/*!< IN: file handle */
	const char *name,	/*!< IN: name of dataset to read */
	h5part_float64_t **data	/*!< OUT: ptr to read buffer */
	) {

	SET_FNAME( "H5MultiBlock3dReadFieldFloat64" );

	h5part_int64_t herr;

	herr = _H5MultiBlock_read_data ( f, name, (char**) data, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_data

  Write a multiblock field \c name from the buffer starting at \c data
  to the current time-step using the defined block decomposition and dimensions.
  Values are floating points (32-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5MultiBlock3dWriteFieldFloat32 (
	H5PartFile *f,		/*!< IN: file handle */
	const char *name,       /*!< IN: name of dataset to write */
	const h5part_float32_t *data      /*!< IN: data to write */
	) {

	SET_FNAME( "H5MultiBlock3dWriteFieldFloat32" );

	h5part_int64_t herr;

	herr = _H5MultiBlock_write_data ( f, name, data, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_data

  Allocate a buffer to hold a block from a multiblock field and place the
  pointer in \c data, then read the block into the buffer. Uses the block
  decomposition specified in the file and the defined halo radius.
  Values are floating points (32-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5MultiBlock3dReadFieldFloat32 (
	H5PartFile *f,		/*!< IN: file handle */
	const char *name,	/*!< IN: name of dataset to read */
	h5part_float32_t **data	/*!< OUT: ptr to read buffer */
	) {

	SET_FNAME( "H5MultiBlock3dReadFieldFloat32" );

	h5part_int64_t herr;

	herr = _H5MultiBlock_read_data ( f, name, (char**) data, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_data

  Write a multiblock field \c name from the buffer starting at \c data
  to the current time-step using the defined block decomposition and dimensions.
  Values are integers (64-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5MultiBlock3dWriteFieldInt64 (
	H5PartFile *f,		/*!< IN: file handle */
	const char *name,       /*!< IN: name of dataset to write */
	const h5part_int64_t *data      /*!< IN: data to write */
	) {

	SET_FNAME( "H5MultiBlock3dWriteFieldInt64" );

	h5part_int64_t herr;

	herr = _H5MultiBlock_write_data ( f, name, data, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_data

  Allocate a buffer to hold a block from a multiblock field and place the
  pointer in \c data, then read the block into the buffer. Uses the block
  decomposition specified in the file and the defined halo radius.
  Values are integers (64-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5MultiBlock3dReadFieldInt64 (
	H5PartFile *f,		/*!< IN: file handle */
	const char *name,	/*!< IN: name of dataset to read */
	h5part_int64_t **data	/*!< OUT: ptr to read buffer */
	) {

	SET_FNAME( "H5MultiBlock3dReadFieldInt64" );

	h5part_int64_t herr;

	herr = _H5MultiBlock_read_data ( f, name, (char**) data, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_data

  Write a multiblock field \c name from the buffer starting at \c data
  to the current time-step using the defined block decomposition and dimensions.
  Values are integers (32-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5MultiBlock3dWriteFieldInt32 (
	H5PartFile *f,		/*!< IN: file handle */
	const char *name,       /*!< IN: name of dataset to write */
	const h5part_int32_t *data      /*!< IN: data to write */
	) {

	SET_FNAME( "H5MultiBlock3dWriteFieldInt32" );

	h5part_int64_t herr;

	herr = _H5MultiBlock_write_data ( f, name, data, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5multiblock_data

  Allocate a buffer to hold a block from a multiblock field and place the
  pointer in \c data, then read the block into the buffer. Uses the block
  decomposition specified in the file and the defined halo radius.
  Values are integers (32-bit).

  You must use the Fortran indexing scheme to access items in \c data.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5MultiBlock3dReadFieldInt32 (
	H5PartFile *f,		/*!< IN: file handle */
	const char *name,	/*!< IN: name of dataset to read */
	h5part_int32_t **data	/*!< OUT: ptr to read buffer */
	) {

	SET_FNAME( "H5MultiBlock3dReadFieldInt32" );

	h5part_int64_t herr;

	herr = _H5MultiBlock_read_data ( f, name, (char**) data, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

#endif // PARALLEL_IO
