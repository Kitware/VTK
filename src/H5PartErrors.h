#ifndef __H5PART_ERRORS_H
#define __H5PART_ERRORS_H

extern h5part_error_handler _err_handler;

/***************** Error Handling ***************/

#define CHECK_FILEHANDLE( f ) \
	if ( _H5Part_file_is_valid ( f ) != H5PART_SUCCESS ) \
		return HANDLE_H5PART_BADFD_ERR;

#define CHECK_WRITABLE_MODE( f )  \
	if ( f->flags & H5PART_READ ) \
		return (*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_INVAL, \
			"Attempting to write to read-only file." );

#define CHECK_READONLY_MODE( f )  \
	if ( ! (f->flags & H5PART_READ) ) \
		return (*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_INVAL, \
			"Operation is not allowed on writable files." );

#define CHECK_TIMEGROUP( f ) \
	if ( f->timegroup <= 0 ) \
		return (*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_INVAL, \
			"Timegroup <= 0.");

/**************** H5Part *********************/

#define HANDLE_H5PART_BADFD_ERR \
	(*_err_handler)( \
		_H5Part_get_funcname(), \
		H5PART_ERR_BADFD, \
		"Called with bad filehandle." );

#define HANDLE_H5PART_INIT_ERR \
	(*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_INIT, \
		"Cannot initialize H5Part." );

#define HANDLE_H5PART_INVALID_ERR( name, value ) \
	(*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_INVAL, \
		"Invalid value '%lld' for '%s'.", \
		(long long)value, name);

#define HANDLE_H5PART_NOMEM_ERR \
	(*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_NOMEM, \
		"Out of memory." );

#define HANDLE_H5PART_SETSTEP_ERR( rc, step ) \
	(*_err_handler) ( \
		_H5Part_get_funcname(), \
		rc, \
		"Cannont set time-step to %lld.", (long long)step );

#define HANDLE_H5PART_FILE_ACCESS_TYPE_ERR( flags ) \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_INVAL, \
			"Invalid file access type \"%d\".", flags);

#define HANDLE_H5PART_STEP_EXISTS_ERR( step ) \
	(*_err_handler)( \
		_H5Part_get_funcname(), \
		H5PART_ERR_INVAL, \
		"Step #%lld already exists, step cannot be set to an existing" \
		" step in write and append mode", (long long)step );

#define HANDLE_H5PART_SET_VIEW_ERR( rc, start, end ) \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			rc, \
			"Cannot set view to (%lld, %lld).", \
			(long long)start, (long long)end );

#define HANDLE_H5PART_BAD_VIEW_ERR( start, end ) \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_BAD_VIEW, \
			"Problem with existing view (%lld, %lld).", \
			(long long)start, (long long)end );

#define HANDLE_H5PART_GET_NUM_PARTICLES_ERR( rc ) \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			rc, \
			"Cannot get number of particles." );

#define HANDLE_H5PART_NOENTRY_ERR( group_name, type, idx ) \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_NOENTRY, \
			"No entry with index %lld and type %d in group %s!", \
			(long long)idx, type, group_name );

#define HANDLE_H5PART_TYPE_ERR \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_NOTYPE, \
			"Encountered unkown data type!");

/**************** HDF5 *********************/
/* H5A: Attribute */
#define HANDLE_H5A_CLOSE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot terminate access to attribute." );

#define HANDLE_H5A_CREATE_ERR( s ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot create attribute \"%s\".", s );

#define HANDLE_H5A_GET_NAME_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot get attribute name." );

#define HANDLE_H5A_GET_NUM_ATTRS_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot get number of attributes." );

#define HANDLE_H5A_GET_SPACE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot get a copy of dataspace for attribute." );

#define HANDLE_H5A_GET_TYPE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot get attribute datatype." );

#define HANDLE_H5A_OPEN_IDX_ERR( n ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot open attribute specified by index \"%lld\".", \
		(long long)n );

#define HANDLE_H5A_OPEN_NAME_ERR( s ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot open attribute specified by name \"%s\".", s );

#define HANDLE_H5A_READ_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot read attribute" );

#define HANDLE_H5A_WRITE_ERR( s ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot write attribute \"%s\".", s );

/* H5D: Dataset */
#define HANDLE_H5D_CLOSE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Close of dataset failed." );

#define HANDLE_H5D_CREATE_ERR( s, n ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot create dataset for name \"%s\", step \"%lld\".", \
		s, (long long) n );

#define HANDLE_H5D_EXISTS_ERR( s, n ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Dataset already exists with name \"%s\", step \"%lld\".", \
		s, (long long) n );

#define HANDLE_H5D_GET_SPACE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot get dataspace identifier.");

#define HANDLE_H5D_GET_PLIST_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot get dataspace property list.");

#define HANDLE_H5D_GET_TYPE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot determine dataset type.");

#define HANDLE_H5D_OPEN_ERR( s ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot open dataset \"%s\".", s );

#define HANDLE_H5D_READ_ERR( s, n ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Read from dataset \"%s\" failed, step \"%lld\".", \
		s, (long long) n );

#define HANDLE_H5D_WRITE_ERR( s, n ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Write to dataset \"%s\" failed, step \"%lld\".", \
		s, (long long)n );

/* H5F: file */
#define HANDLE_H5F_CLOSE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot terminate access to file." );

#define HANDLE_H5F_OPEN_ERR( filename, flags ) \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_HDF5, \
			"Cannot open file \"%s\" with mode \"%d\"", filename, flags );



/* H5G: group */
#define HANDLE_H5G_CLOSE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot terminate access to datagroup." );

#define HANDLE_H5G_CREATE_ERR( s ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot create datagroup \"%s\".", s );

#define HANDLE_H5G_GET_OBJINFO_ERR( s ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot get information about object \"%s\".", s );

#define HANDLE_H5G_OPEN_ERR( s ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot open group \"%s\".", s );

#define HANDLE_H5O_OPEN_ERR( s ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot open object \"%s\".", s );

/* H5P: property */
#define HANDLE_H5P_CLOSE_ERR( s ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot terminate access to property list \"%s\".", s );

#define HANDLE_H5P_CREATE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot create property list." );

#define HANDLE_H5P_SET_DXPL_MPIO_ERR \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_HDF5, \
			"MPI: Cannot set data transfer mode." );


#define HANDLE_H5P_SET_FAPL_ERR \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_HDF5, \
			"Cannot store IO communicator information to the " \
			"file access property list." );

#define HANDLE_H5P_SET_CHUNK_ERR \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_HDF5, \
			"Cannot set chunk dimensions." );

#define HANDLE_H5P_GET_CHUNK_ERR \
		(*_err_handler) ( \
			_H5Part_get_funcname(), \
			H5PART_ERR_HDF5, \
			"Cannot get chunk dimensions." );

/* H5S: dataspace */
#define HANDLE_H5S_CREATE_SCALAR_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot create scalar dataspace." );

#define HANDLE_H5S_CREATE_SIMPLE_ERR( n ) \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot create dataspace with len \"%lld\".", (long long) n );

#define HANDLE_H5S_CLOSE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot terminate access to dataspace." ); 

#define HANDLE_H5S_GET_SELECT_NPOINTS_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot determine number of elements in dataspace selection." ); 

#define HANDLE_H5S_GET_SIMPLE_EXTENT_NPOINTS_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot determine number of elements in dataspace." ); 

#define HANDLE_H5S_SELECT_HYPERSLAB_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot select hyperslap region of dataspace." );

#define HANDLE_H5S_SELECT_ELEMENTS_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot select elements in dataspace." );

/* H5T:  type */
#define HANDLE_H5T_STRING_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot create string datatype." );

#define HANDLE_H5T_CLOSE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot release datatype." );

/* H5L */
#define HANDLE_H5L_ITERATE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_HDF5, \
		"Cannot iterate through group." );

/* MPI */
#define HANDLE_MPI_ALLGATHER_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_MPI, \
		"Cannot gather data." );

#define HANDLE_MPI_SENDRECV_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_MPI, \
		"Unable to perform point-to-point MPI send/receive." );

#define HANDLE_MPI_COMM_SIZE_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_MPI, \
		"Cannot get number of processes in my group." );

#define HANDLE_MPI_COMM_RANK_ERR \
	 (*_err_handler) ( \
		_H5Part_get_funcname(), \
		H5PART_ERR_MPI, \
		"Cannot get rank of the calling process in my group." );

#endif
