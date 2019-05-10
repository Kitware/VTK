#ifndef _H5BLOCK_H_
#define _H5BLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

/*!
  Include read/write call variants for different data types and
  field dimensions.
*/
#include "H5BlockReadWrite.h"

#define H5BLOCK_FIELD_ORIGIN_NAME	"__Origin__"
#define H5BLOCK_FIELD_SPACING_NAME	"__Spacing__"

/*! 
  Interface for block structured field data.
*/
VTKH5PART_EXPORT
h5part_int64_t
H5BlockDefine3DFieldLayout (
	H5PartFile *f,
	const h5part_int64_t i_start,
	const h5part_int64_t i_end,
	const h5part_int64_t j_start,
	const h5part_int64_t j_end,
	const h5part_int64_t k_start,
	const h5part_int64_t k_end
	);

VTKH5PART_EXPORT
h5part_int64_t
H5BlockDefine3DChunkDims (
	H5PartFile *f,
	const h5part_int64_t i,
	const h5part_int64_t j,
	const h5part_int64_t k
	);

VTKH5PART_EXPORT
h5part_int64_t
H5BlockGet3DChunkDims(
	H5PartFile *f,
        const char *field_name,
	h5part_int64_t *dims
	);

VTKH5PART_EXPORT
h5part_int64_t
H5Block3dGetPartitionOfProc (
	H5PartFile *f,
	const h5part_int64_t proc,
	h5part_int64_t *i_start,
	h5part_int64_t *i_end,
	h5part_int64_t *j_start,
	h5part_int64_t *j_end,
	h5part_int64_t *k_start,
	h5part_int64_t *k_end
	);

VTKH5PART_EXPORT
h5part_int64_t
H5Block3dGetReducedPartitionOfProc (
	H5PartFile *f,
	h5part_int64_t proc,
	h5part_int64_t *i_start, 
	h5part_int64_t *i_end,
	h5part_int64_t *j_start,
	h5part_int64_t *j_end,
	h5part_int64_t *k_start,
	h5part_int64_t *k_end
	);

VTKH5PART_EXPORT
h5part_int64_t
H5Block3dGetProcOf (
	H5PartFile *f,
	h5part_int64_t i,
	h5part_int64_t j,
	h5part_int64_t k
	);

VTKH5PART_EXPORT
h5part_int64_t
H5BlockGetNumFields (
	H5PartFile *f
	);

VTKH5PART_EXPORT
h5part_int64_t
H5BlockGetFieldInfo (
	H5PartFile *f,
	const h5part_int64_t idx,
	char *name,
	const h5part_int64_t len_name,
	h5part_int64_t *grid_rank,
	h5part_int64_t *grid_dims,
	h5part_int64_t *field_rank,
	h5part_int64_t *type
	);

VTKH5PART_EXPORT
h5part_int64_t
H5BlockGetFieldInfoByName (
	H5PartFile *f,
	const char *field_name,
	h5part_int64_t *grid_rank,
	h5part_int64_t *grid_dims,
	h5part_int64_t *field_rank,
	h5part_int64_t *type
	);

VTKH5PART_EXPORT
h5part_int64_t
H5Block3dGetFieldOrigin (
	H5PartFile *f,
	const char *field_name,
	h5part_float64_t *x_origin,
	h5part_float64_t *y_origin,
	h5part_float64_t *z_origin
	);

VTKH5PART_EXPORT
h5part_int64_t
H5Block3dSetFieldOrigin (
	H5PartFile *f,
	const char *field_name,
	const h5part_float64_t x_origin,
	const h5part_float64_t y_origin,
	const h5part_float64_t z_origin
	);

VTKH5PART_EXPORT
h5part_int64_t
H5Block3dGetFieldSpacing (
	H5PartFile *f,
	const char *field_name,
	h5part_float64_t *x_spacing,
	h5part_float64_t *y_spacing,
	h5part_float64_t *z_spacing
	);

VTKH5PART_EXPORT
h5part_int64_t
H5Block3dSetFieldSpacing (
	H5PartFile *f,
	const char *field_name,
	const h5part_float64_t x_spacing,
	const h5part_float64_t y_spacing,
	const h5part_float64_t z_spacing
	);

VTKH5PART_EXPORT
h5part_int64_t
H5BlockWriteFieldAttrib (
	H5PartFile *f,
	const char *field_name,
	const char *attrib_name,
	const h5part_int64_t attrib_type,
	const void *attrib_value,
	const h5part_int64_t attrib_nelem
	);

VTKH5PART_EXPORT
h5part_int64_t
H5BlockWriteFieldAttribString (
	H5PartFile *f,
	const char *field_name,
	const char *attrib_name,
	const char *attrib_value
	);

VTKH5PART_EXPORT
h5part_int64_t
H5BlockGetNumFieldAttribs (
	H5PartFile *f,
	const char *field_name
	);

VTKH5PART_EXPORT
h5part_int64_t
H5BlockGetFieldAttribInfo (
	H5PartFile *f,
	const char *field_name,
	const h5part_int64_t attrib_idx,
	char *attrib_name,
	const h5part_int64_t len_of_attrib_name,
	h5part_int64_t *attrib_type,
	h5part_int64_t *attrib_nelem
	);

VTKH5PART_EXPORT
h5part_int64_t
H5BlockReadFieldAttrib (
	H5PartFile *f,
	const char *field_name,
	const char *attrib_name,
	void *attrib_value
	);

VTKH5PART_EXPORT
h5part_int64_t
H5BlockHasFieldData (
	H5PartFile *f
	);


#ifdef __cplusplus
}
#endif

#endif
