
#include "H5Part.h"
#include "H5PartPrivate.h"
#include "Underscore.h"

#if defined(F77_SINGLE_UNDERSCORE)
#define F77NAME(a,b) a
#elif defined(F77_CRAY_UNDERSCORE)
#define F77NAME(a,b) b
#elif defined(F77_NO_UNDERSCORE)
#else
#error Error, no way to determine how to construct fortran bindings
#endif

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_writefileattrib_r8 F77NAME ( \
	h5pt_writefileattrib_r8_, \
	H5PT_WRITEFILEATTRIB_R8 )
#endif

h5part_int64_t
h5pt_writefileattrib_r8 (
	h5part_int64_t *f,
	const char *name,
	const h5part_float64_t *data,
	const h5part_int64_t *nelem,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartWriteFileAttrib (
		filehandle, name2, H5PART_FLOAT64, data, *nelem);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_readfileattrib_r8 F77NAME ( \
	h5pt_readfileattrib_r8_, \
	H5PT_READFILEATTRIB_R8 )
#endif

h5part_int64_t
h5pt_readfileattrib_r8 (
	h5part_int64_t *f,
	const char *name,
	const h5part_float64_t *data,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartReadFileAttrib (
		filehandle, name2, (void*)data);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_writefileattrib_r4 F77NAME ( \
	h5pt_writefileattrib_r4_, \
	H5PT_WRITEFILEATTRIB_R4 )
#endif

h5part_int64_t
h5pt_writefileattrib_r4 (
	h5part_int64_t *f,
	const char *name,
	const h5part_float32_t *data,
	const h5part_int64_t *nelem,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartWriteFileAttrib (
		filehandle, name2, H5PART_FLOAT32, data, *nelem);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_readfileattrib_r4 F77NAME ( \
	h5pt_readfileattrib_r4_, \
	H5PT_READFILEATTRIB_R4 )
#endif

h5part_int64_t
h5pt_readfileattrib_r4 (
	h5part_int64_t *f,
	const char *name,
	const h5part_float32_t *data,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartReadFileAttrib (
		filehandle, name2, (void*)data);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_writefileattrib_i8 F77NAME ( \
	h5pt_writefileattrib_i8_, \
	H5PT_WRITEFILEATTRIB_I8 )
#endif

h5part_int64_t
h5pt_writefileattrib_i8 (
	h5part_int64_t *f,
	const char *name,
	const h5part_int64_t *data,
	const h5part_int64_t *nelem,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartWriteFileAttrib (
		filehandle, name2, H5PART_INT64, data, *nelem);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_readfileattrib_i8 F77NAME ( \
	h5pt_readfileattrib_i8_, \
	H5PT_READFILEATTRIB_I8 )
#endif

h5part_int64_t
h5pt_readfileattrib_i8 (
	h5part_int64_t *f,
	const char *name,
	const h5part_int64_t *data,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartReadFileAttrib (
		filehandle, name2, (void*)data);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_writefileattrib_i4 F77NAME ( \
	h5pt_writefileattrib_i4_, \
	H5PT_WRITEFILEATTRIB_I4 )
#endif

h5part_int64_t
h5pt_writefileattrib_i4 (
	h5part_int64_t *f,
	const char *name,
	const h5part_int32_t *data,
	const h5part_int64_t *nelem,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartWriteFileAttrib (
		filehandle, name2, H5PART_INT32, data, *nelem);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_readfileattrib_i4 F77NAME ( \
	h5pt_readfileattrib_i4_, \
	H5PT_READFILEATTRIB_I4 )
#endif

h5part_int64_t
h5pt_readfileattrib_i4 (
	h5part_int64_t *f,
	const char *name,
	const h5part_int32_t *data,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartReadFileAttrib (
		filehandle, name2, (void*)data);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_writestepattrib_r8 F77NAME ( \
	h5pt_writestepattrib_r8_, \
	H5PT_WRITESTEPATTRIB_R8 )
#endif

h5part_int64_t
h5pt_writestepattrib_r8 (
	h5part_int64_t *f,
	const char *name,
	const h5part_float64_t *data,
	const h5part_int64_t *nelem,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartWriteStepAttrib (
		filehandle, name2, H5PART_FLOAT64, data, *nelem);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_readstepattrib_r8 F77NAME ( \
	h5pt_readstepattrib_r8_, \
	H5PT_READSTEPATTRIB_R8 )
#endif

h5part_int64_t
h5pt_readstepattrib_r8 (
	h5part_int64_t *f,
	const char *name,
	const h5part_float64_t *data,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartReadStepAttrib (
		filehandle, name2, (void*)data);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_writestepattrib_r4 F77NAME ( \
	h5pt_writestepattrib_r4_, \
	H5PT_WRITESTEPATTRIB_R4 )
#endif

h5part_int64_t
h5pt_writestepattrib_r4 (
	h5part_int64_t *f,
	const char *name,
	const h5part_float32_t *data,
	const h5part_int64_t *nelem,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartWriteStepAttrib (
		filehandle, name2, H5PART_FLOAT32, data, *nelem);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_readstepattrib_r4 F77NAME ( \
	h5pt_readstepattrib_r4_, \
	H5PT_READSTEPATTRIB_R4 )
#endif

h5part_int64_t
h5pt_readstepattrib_r4 (
	h5part_int64_t *f,
	const char *name,
	const h5part_float32_t *data,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartReadStepAttrib (
		filehandle, name2, (void*)data);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_writestepattrib_i8 F77NAME ( \
	h5pt_writestepattrib_i8_, \
	H5PT_WRITESTEPATTRIB_I8 )
#endif

h5part_int64_t
h5pt_writestepattrib_i8 (
	h5part_int64_t *f,
	const char *name,
	const h5part_int64_t *data,
	const h5part_int64_t *nelem,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartWriteStepAttrib (
		filehandle, name2, H5PART_INT64, data, *nelem);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_readstepattrib_i8 F77NAME ( \
	h5pt_readstepattrib_i8_, \
	H5PT_READSTEPATTRIB_I8 )
#endif

h5part_int64_t
h5pt_readstepattrib_i8 (
	h5part_int64_t *f,
	const char *name,
	const h5part_int64_t *data,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartReadStepAttrib (
		filehandle, name2, (void*)data);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_writestepattrib_i4 F77NAME ( \
	h5pt_writestepattrib_i4_, \
	H5PT_WRITESTEPATTRIB_I4 )
#endif

h5part_int64_t
h5pt_writestepattrib_i4 (
	h5part_int64_t *f,
	const char *name,
	const h5part_int32_t *data,
	const h5part_int64_t *nelem,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartWriteStepAttrib (
		filehandle, name2, H5PART_INT32, data, *nelem);

	free ( name2 );
	return herr;
}

#if ! defined(F77_NO_UNDERSCORE)
#define h5pt_readstepattrib_i4 F77NAME ( \
	h5pt_readstepattrib_i4_, \
	H5PT_READSTEPATTRIB_I4 )
#endif

h5part_int64_t
h5pt_readstepattrib_i4 (
	h5part_int64_t *f,
	const char *name,
	const h5part_int32_t *data,
	const int l_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *name2 =_H5Part_strdupfor2c ( name, l_name );

	h5part_int64_t herr = H5PartReadStepAttrib (
		filehandle, name2, (void*)data);

	free ( name2 );
	return herr;
}
