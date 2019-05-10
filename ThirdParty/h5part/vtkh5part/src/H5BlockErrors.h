#ifndef _H5BLOCK_ERRORS_H_
#define _H5BLOCK_ERRORS_H_

extern h5part_error_handler _err_handler;

#define CHECK_LAYOUT( f ) \
	if ( ! f->block->have_layout ) \
		return (*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_LAYOUT, \
			"No layout defined." )

#define HANDLE_H5PART_LAYOUT_ERR \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_LAYOUT, \
			"Bad layout." );

#define HANDLE_H5PART_NOENT_ERR( name ) \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_NOENT, \
			"Object \"%s\" doesn't exists.", name );

#define HANDLE_H5PART_DATASET_RANK_ERR( m, n ) \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_INVAL, \
			"Wrong rank of dataset: Is %d, but should be %d", \
			m, n );

#define HANDLE_H5PART_GROUP_EXISTS_ERR( name ) \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_INVAL, \
			"Group \"%s\" already exists", name )

#define HANDLE_H5S_CREATE_SIMPLE_3D_ERR( dims ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot create 3d dataspace with dimension sizes " \
		"\"(%lld,%lld,%lld)\".", \
		 (long long)dims[0], (long long)dims[1], (long long)dims[2] );

#define HANDLE_H5S_GET_SIMPLE_EXTENT_DIMS_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot get dimension sizes of dataset" );

#endif
