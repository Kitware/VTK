
#ifndef _H5MULTIBLOCK_READWRITE_H_
#define _H5MULTIBLOCK_READWRITE_H_

#ifdef __cplusplus
extern "C" {
#endif


VTKH5PART_EXPORT
h5part_int64_t
H5MultiBlock3dWriteFieldFloat64 (
	H5PartFile *f,
	const char *name,
	const h5part_float64_t *data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5MultiBlock3dReadFieldFloat64 (
	H5PartFile *f,
	const char *name,
	h5part_float64_t **data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5MultiBlock3dWriteFieldFloat32 (
	H5PartFile *f,
	const char *name,
	const h5part_float32_t *data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5MultiBlock3dReadFieldFloat32 (
	H5PartFile *f,
	const char *name,
	h5part_float32_t **data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5MultiBlock3dWriteFieldInt64 (
	H5PartFile *f,
	const char *name,
	const h5part_int64_t *data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5MultiBlock3dReadFieldInt64 (
	H5PartFile *f,
	const char *name,
	h5part_int64_t **data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5MultiBlock3dWriteFieldInt32 (
	H5PartFile *f,
	const char *name,
	const h5part_int32_t *data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5MultiBlock3dReadFieldInt32 (
	H5PartFile *f,
	const char *name,
	h5part_int32_t **data
	);


#ifdef __cplusplus
}
#endif

#endif
