#include "H5Part.h"
#include "H5PartPrivate.h"
#include "H5Block.h"
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

#define h5bl_define3dlayout F77NAME (					\
					h5bl_define3dlayout_,		\
					H5BL_DEFINE3DLAYOUT )
#define h5bl_define3dchunkdims F77NAME (				\
					h5bl_define3dchunkdims_,	\
					H5BL_DEFINE3DCHUNKDIMS )
#define h5bl_get_partition_of_proc F77NAME (				\
					h5bl_get_partition_of_proc_,	\
					H5BL_GET_PARTITION_OF_PROC )
#define h5bl_get_reduced_partition_of_proc F77NAME (			\
					h5bl_get_reduced_partition_of_proc_,\
					H5BL_GET_REDUCED_PARTITION_OF_PROC )
#define h5bl_get_proc_of F77NAME (					\
					h5bl_get_proc_of_,		\
					H5BL_GET_PROC_OF )
#define h5bl_getnumfields F77NAME (					\
					h5bl_getnumfields_,		\
					H5BL_GETNUMFIELDS )
#define h5bl_getfieldinfo F77NAME (					\
					h5bl_getfieldinfo_,		\
					H5BL_GETFIELDINFO )
#define h5bl_writefieldattrib_string F77NAME (			\
					h5bl_writefieldattrib_string_,	\
					H5BL_WRITEFIELDATTRIB_STRING )
#define h5bl_getnfieldattribs F77NAME (				\
					h5bl_getnfieldattribs_,		\
					H5BL_GETNFIELDATTRIBS )
#define h5bl_getfieldattribinfo F77NAME (				\
					h5bl_getfieldattribinfo_,	\
					h5bl_getfieldattribinfo )
#define h5bl_readfieldattrib_i8 F77NAME (				\
					h5bl_readfieldattrib_i8_,	\
					H5BL_READFIELDATTRIB_I8 )
#define h5bl_readfieldattrib_r8 F77NAME (				\
					h5bl_readfieldattrib_r8_,	\
					H5BL_READFIELDATTRIB_R8 )
#define h5bl_readfieldattrib_string F77NAME (				\
					h5bl_readfieldattrib_string_,	\
					H5BL_READFIELDATTRIB_STRING )
#define h5bl_has_fielddata F77NAME (					\
					h5bl_has_fielddata_,		\
					H5BL_HAS_FIELDDATA )
#define h5bl_3d_set_field_spacing F77NAME (				\
					h5bl_3d_set_field_spacing_,	\
					H5BL_3D_SET_FIELD_SPACING )
#define h5bl_3d_get_field_spacing F77NAME (				\
					h5bl_3d_get_field_spacing_,	\
					H5BL_3D_GET_FIELD_SPACING )
#define h5bl_3d_set_field_origin F77NAME (				\
					h5bl_3d_set_field_origin_,	\
					H5BL_3D_SET_FIELD_ORIGIN )
#define h5bl_3d_get_field_origin F77NAME (				\
					h5bl_3d_get_field_origin_,	\
					H5BL_3D_GET_FIELD_origin )
#endif

h5part_int64_t
h5bl_define3dlayout (
	h5part_int64_t *f,
	const h5part_int64_t *i_start,	/*!< start index of i */
	const h5part_int64_t *i_end,	/*!< end index of i */
	const h5part_int64_t *j_start,	/*!< start index of j */
	const h5part_int64_t *j_end,	/*!< end index of j */
	const h5part_int64_t *k_start,	/*!< start index of k */
	const h5part_int64_t *k_end	/*!< end index of k */
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	return H5BlockDefine3DFieldLayout (
		filehandle,
		*i_start-1, *i_end-1,
		*j_start-1, *j_end-1,
		*k_start-1, *k_end-1 );
}

h5part_int64_t
h5bl_define3dchunkdims (
	h5part_int64_t *f,
	const h5part_int64_t *i,
	const h5part_int64_t *j,
	const h5part_int64_t *k
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	return H5BlockDefine3DChunkDims ( filehandle, *i, *j, *k );
}

h5part_int64_t
h5bl_get_partition_of_proc (
	h5part_int64_t *f,		/*!< file handle */
	const h5part_int64_t *proc,
	h5part_int64_t *i_start,	/*!< start index of i */
	h5part_int64_t *i_end,		/*!< end index of i */
	h5part_int64_t *j_start,	/*!< start index of j */
	h5part_int64_t *j_end,		/*!< end index of j */
	h5part_int64_t *k_start,	/*!< start index of k */
	h5part_int64_t *k_end		/*!< end index of k */
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	h5part_int64_t herr = H5Block3dGetPartitionOfProc (
		filehandle,
		*proc,
		i_start, i_end, j_start, j_end, k_start, k_end );
	if ( herr < 0 ) return herr;

	(*i_start)++;
	(*i_end)++;
	(*j_start)++;
	(*j_end)++;
	(*k_start)++;
	(*k_end)++;

	return H5PART_SUCCESS;
}

h5part_int64_t
h5bl_get_reduced_partition_of_proc (
	h5part_int64_t *f,
	const h5part_int64_t *proc,
	h5part_int64_t *i_start, 
	h5part_int64_t *i_end,
	h5part_int64_t *j_start,
	h5part_int64_t *j_end,
	h5part_int64_t *k_start,
	h5part_int64_t *k_end
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	h5part_int64_t herr = H5Block3dGetReducedPartitionOfProc (
		filehandle,
		*proc,
		i_start, i_end, j_start, j_end, k_start, k_end );
	if ( herr < 0 ) return herr;

	(*i_start)++;
	(*i_end)++;
	(*j_start)++;
	(*j_end)++;
	(*k_start)++;
	(*k_end)++;

	return H5PART_SUCCESS;
}

h5part_int64_t
h5bl_get_proc_of (
	h5part_int64_t *f,
	const h5part_int64_t *i,
	const h5part_int64_t *j,
	const h5part_int64_t *k
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	return H5Block3dGetProcOf ( filehandle, (*i)-1, (*j)-1, (*k)-1 );
}

h5part_int64_t
h5bl_getnumfields (
	h5part_int64_t *f			/*!< file handle */
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	return H5BlockGetNumFields ( filehandle );
}

h5part_int64_t
h5bl_getfieldinfo (
	h5part_int64_t *f,
	const h5part_int64_t *idx,
	char *field_name,
	h5part_int64_t *grid_rank,
	h5part_int64_t *grid_dims,
	h5part_int64_t *field_dims,
	h5part_int64_t *type,
	const int l_field_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	h5part_int64_t herr = H5BlockGetFieldInfo (
		filehandle, *idx, field_name, l_field_name,
		grid_rank, grid_dims, field_dims, type );
	_H5Part_strc2for ( field_name, l_field_name );
	return herr;
}

h5part_int64_t
h5bl_writefieldattrib_string (
	h5part_int64_t *f,
	const char *field_name,
	const char *attrib_name,
	const char *attrib_value,
	const int l_field_name,
	const int l_attrib_name,
	const int l_attrib_value
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *field_name2 =_H5Part_strdupfor2c ( field_name,  l_field_name );
	char *attrib_name2=_H5Part_strdupfor2c ( attrib_name, l_attrib_name );
	char *attrib_value2=_H5Part_strdupfor2c( attrib_value,l_attrib_value );

	h5part_int64_t herr = H5BlockWriteFieldAttribString (
		filehandle, field_name2, attrib_name2, attrib_value2 );

	free ( field_name2 );
	free ( attrib_name2 );
	free ( attrib_value2 );
	return herr;
}


h5part_int64_t
h5bl_getnfieldattribs (
	h5part_int64_t *f,
	const char *field_name,
	const int l_field_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *field_name2 = _H5Part_strdupfor2c ( field_name,   l_field_name );

	h5part_int64_t herr = H5BlockGetNumFieldAttribs (
		filehandle, field_name2 );
	
	free ( field_name2 );
	return herr;
}

h5part_int64_t
h5bl_getfieldattribinfo (
	h5part_int64_t *f,
	const char *field_name,
	const h5part_int64_t *attrib_idx,
	char *attrib_name,
	h5part_int64_t *attrib_nelem,
	const int l_field_name,
	const int l_attrib_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	h5part_int64_t	attrib_type;

	char *field_name2 = _H5Part_strdupfor2c ( field_name,   l_field_name );

	h5part_int64_t herr = H5BlockGetFieldAttribInfo (
		filehandle, field_name2, *attrib_idx,
		attrib_name, l_attrib_name,
		&attrib_type,
		attrib_nelem );

	_H5Part_strc2for ( attrib_name, l_attrib_name );

	free ( field_name2 );
	return herr;
}


h5part_int64_t
h5bl_readfieldattrib_i8 (
	h5part_int64_t *f,
	const char *field_name,
	const char *attrib_name,
	h5part_int64_t *attrib_value,
	const int l_field_name,
	const int l_attrib_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *field_name2 =_H5Part_strdupfor2c ( field_name,   l_field_name );
	char *attrib_name2=_H5Part_strdupfor2c ( attrib_name,  l_attrib_name );

	h5part_int64_t herr = H5BlockReadFieldAttrib (
		filehandle, field_name2, attrib_name2, attrib_value );

	free ( field_name2 );
	free ( attrib_name2 );
	return herr;
}

h5part_int64_t
h5bl_readfieldattrib_r8 (
	h5part_int64_t *f,
	const char *field_name,
	const char *attrib_name,
	h5part_float64_t *attrib_value,
	const int l_field_name,
	const int l_attrib_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *field_name2 =_H5Part_strdupfor2c ( field_name,   l_field_name );
	char *attrib_name2=_H5Part_strdupfor2c ( attrib_name,  l_attrib_name );

	h5part_int64_t herr = H5BlockReadFieldAttrib (
		filehandle, field_name2, attrib_name2, attrib_value );

	free ( field_name2 );
	free ( attrib_name2 );
	return herr;
}

h5part_int64_t
h5bl_readfieldattrib_string (
	h5part_int64_t *f,
	const char *field_name,
	const char *attrib_name,
	char *attrib_value,
	const int l_field_name,
	const int l_attrib_name,
	const int l_attrib_value
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *field_name2 =_H5Part_strdupfor2c ( field_name,   l_field_name );
	char *attrib_name2=_H5Part_strdupfor2c ( attrib_name,  l_attrib_name );

	h5part_int64_t herr = H5BlockReadFieldAttrib (
		filehandle, field_name2, attrib_name2, attrib_value );

	_H5Part_strc2for ( attrib_value, l_attrib_value );

	free ( field_name2 );
	free ( attrib_name2 );
	return herr;
}

h5part_int64_t
h5bl_has_fielddata (
	h5part_int64_t *f
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	return H5BlockHasFieldData ( filehandle );
}

h5part_int64_t
h5bl_3d_get_field_spacing (
	h5part_int64_t *f,
	const char *field_name,
	h5part_float64_t *x,
	h5part_float64_t *y,
	h5part_float64_t *z,
	const int l_field_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *field_name2 =  _H5Part_strdupfor2c ( field_name, l_field_name );

	h5part_int64_t herr = H5Block3dGetFieldSpacing (
		filehandle, field_name2, x, y, z );

	free ( field_name2 );
	return herr;
}

h5part_int64_t
h5bl_3d_set_field_spacing (
	h5part_int64_t *f,
	const char *field_name,
	const h5part_float64_t *x,
	const h5part_float64_t *y,
	const h5part_float64_t *z,
	const int l_field_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *field_name2 =  _H5Part_strdupfor2c ( field_name, l_field_name );

	h5part_int64_t herr = H5Block3dSetFieldSpacing (
		filehandle, field_name2, *x, *y, *z );

	free ( field_name2 );
	return herr;
}

h5part_int64_t
h5bl_3d_get_field_origin (
	h5part_int64_t *f,
	const char *field_name,
	h5part_float64_t *x,
	h5part_float64_t *y,
	h5part_float64_t *z,
	const int l_field_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *field_name2 =  _H5Part_strdupfor2c ( field_name, l_field_name );

	h5part_int64_t herr = H5Block3dGetFieldOrigin (
		filehandle, field_name2, x, y, z );

	free ( field_name2 );
	return herr;
}

h5part_int64_t
h5bl_3d_set_field_origin (
	h5part_int64_t *f,
	const char *field_name,
	const h5part_float64_t *x,
	const h5part_float64_t *y,
	const h5part_float64_t *z,
	const int l_field_name
	) {

	H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

	char *field_name2 =  _H5Part_strdupfor2c ( field_name, l_field_name );

	h5part_int64_t herr = H5Block3dSetFieldOrigin (
		filehandle, field_name2, *x, *y, *z );

	free ( field_name2 );
	return herr;
}
