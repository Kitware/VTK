
#ifndef _H5PART_ATTRIB_H_
#define _H5PART_ATTRIB_H_

#ifdef __cplusplus
extern "C" {
#endif


VTKH5PART_EXPORT
h5part_int64_t
H5PartWriteFileAttribFloat64 (
	H5PartFile *f,
	const char *name,
	const h5part_float64_t data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5PartWriteFileAttribFloat32 (
	H5PartFile *f,
	const char *name,
	const h5part_float32_t data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5PartWriteFileAttribInt64 (
	H5PartFile *f,
	const char *name,
	const h5part_int64_t data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5PartWriteFileAttribInt32 (
	H5PartFile *f,
	const char *name,
	const h5part_int32_t data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5PartWriteStepAttribFloat64 (
	H5PartFile *f,
	const char *name,
	const h5part_float64_t data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5PartWriteStepAttribFloat32 (
	H5PartFile *f,
	const char *name,
	const h5part_float32_t data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5PartWriteStepAttribInt64 (
	H5PartFile *f,
	const char *name,
	const h5part_int64_t data
	);

VTKH5PART_EXPORT
h5part_int64_t
H5PartWriteStepAttribInt32 (
	H5PartFile *f,
	const char *name,
	const h5part_int32_t data
	);


#ifdef __cplusplus
}
#endif

#endif
