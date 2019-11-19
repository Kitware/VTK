/*! \mainpage H5Part: A Portable High Performance Parallel Data Interface to HDF5

Particle based simulations of accelerator beam-lines, especially in
six dimensional phase space, generate vast amounts of data. Even
though a subset of statistical information regarding phase space or
analysis needs to be preserved, reading and writing such enormous
restart files on massively parallel supercomputing systems remains
challenging. 

H5Part consists of Particles and Block structured Fields.

Developed by:

<UL>
<LI> Andreas Adelmann (PSI) </LI>
<LI> Achim Gsell (PSI) </LI>
<LI> Benedikt Oswald (PSI) </LI>

<LI> Wes Bethel (NERSC/LBNL)</LI>
<LI> John Shalf (NERSC/LBNL)</LI>
<LI> Cristina Siegerist (NERSC/LBNL)</LI>
<LI> Mark Howison (NERSC/LBNL)</LI>
</UL>


Papers: 

<UL>
<LI> A. Adelmann, R.D. Ryne, C. Siegerist, J. Shalf,"From Visualization to Data Mining with Large Data Sets," <i>
<a href="http://www.sns.gov/pac05">Particle Accelerator Conference (PAC05)</a></i>, Knoxville TN., May 16-20, 2005. (LBNL-57603)
<a href="http://vis.lbl.gov/Publications/2005/FPAT082.pdf">FPAT082.pdf</a>
</LI>


<LI> A. Adelmann, R.D. Ryne, J. Shalf, C. Siegerist, "H5Part: A Portable High Performance Parallel Data Interface for Particle Simulations," <i>
<a href="http://www.sns.gov/pac05">Particle Accelerator Conference (PAC05)</a></i>, Knoxville TN., May 16-20, 2005.
<a href="http://vis.lbl.gov/Publications/2005/FPAT083.pdf">FPAT083.pdf</a>
</LI>
</UL>

For further information contact: <a href="mailto:h5part@lists.psi.ch">h5part</a>

*/


/*!
  \defgroup h5part_c_api H5Part C API

*/
/*!
  \ingroup h5part_c_api
  \defgroup h5part_open 	File Opening and Closing
*/
/*!
  \ingroup h5part_c_api
  \defgroup h5part_model	Setting up the Data Model
*/  
/*!
  \ingroup h5part_c_api
  \defgroup h5part_data		Readind and Writing Datasets
*/  
/*!
  \ingroup h5part_c_api
  \defgroup h5part_attrib	Reading and Writing Attributes
*/
/*!
  \ingroup h5part_c_api
  \defgroup h5part_errhandle	Error Handling
*/
/*!
  \internal
  \defgroup h5partkernel H5Part private functions 
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>	/* va_arg - System dependent ?! */
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <vtk_hdf5.h>

#ifndef WIN32
#include <unistd.h>
#else /* WIN32 */
#include <io.h>
#define open  _open
#define close _close
#endif /* WIN32 */

#include "H5Part.h"
#include "H5PartPrivate.h"
#include "H5PartErrors.h"

/********* Global Variable Declarations *************/
h5part_error_handler	_err_handler = H5PartReportErrorHandler;

/********* Private Variable Declarations *************/

static unsigned			_is_root_proc = 0;
static unsigned			_debug = H5PART_VERB_ERROR;
static h5part_int64_t		_h5part_errno = H5PART_SUCCESS;
static char *__funcname;

/********** Declaration of private functions ******/

static h5part_int64_t
_init(
	void
	);

static h5part_int64_t
_reset_view (
	H5PartFile *f
	);

/*
  error handler for hdf5
*/
static herr_t
_h5_error_handler (
#ifndef H5_USE_16_API
	hid_t,
#endif
	void *
	);

static void
_vprint (
	FILE* f,
	const char *prefix,
	const char *fmt,
	va_list ap
	);

/*========== File Opening/Closing ===============*/

static H5PartFile*
_H5Part_open_file (
	const char *filename,	/*!< [in] The name of the data file to open. */
	const char flags,	/*!< [in] The access mode for the file. */
	H5_Comm comm,		/*!< [in] MPI communicator */
	int f_parallel,		/*!< [in] 0 for serial io otherwise parallel */
	h5part_int64_t align	/*!< [in] Number of bytes for setting alignment,
					  metadata block size, etc.
					  Set to 0 to disable. */
	) {

	_h5part_errno = H5PART_SUCCESS;
	H5PartFile *f = NULL;

	f = (H5PartFile*) malloc( sizeof (H5PartFile) );
	if( f == NULL ) {
		HANDLE_H5PART_NOMEM_ERR;
		goto error_cleanup;
	}
	memset (f, 0, sizeof (H5PartFile));

	f->flags = flags;

	/* set default step name */
	strncpy ( f->groupname_step, H5PART_GROUPNAME_STEP, H5PART_STEPNAME_LEN );
	f->stepno_width = 0;

	f->xfer_prop = f->dcreate_prop = f->fcreate_prop = H5P_DEFAULT;

	f->access_prop = H5Pcreate (H5P_FILE_ACCESS);
	if (f->access_prop < 0) {
		HANDLE_H5P_CREATE_ERR;
		goto error_cleanup;
	}

	if ( f_parallel ) {
#ifdef PARALLEL_IO
		MPI_Info info = MPI_INFO_NULL;

		if (MPI_Comm_size (comm, &f->nprocs) != MPI_SUCCESS) {
			HANDLE_MPI_COMM_SIZE_ERR;
			goto error_cleanup;
		}
		if (MPI_Comm_rank (comm, &f->myproc) != MPI_SUCCESS) {
			HANDLE_MPI_COMM_RANK_ERR;
			goto error_cleanup;
		}

		if ( f-> myproc == 0 ) _is_root_proc = 1;
		else _is_root_proc = 0;

		f->pnparticles =
		  (h5part_int64_t*) malloc (f->nprocs * sizeof (h5part_int64_t));
		if (f->pnparticles == NULL) {
			HANDLE_H5PART_NOMEM_ERR;
			goto error_cleanup;
		}

		/* optional lustre optimizations */
		if ( flags & H5PART_FS_LUSTRE )
		{
			/* extend the btree size so that metadata pieces are
		 	* close to the alignment value */
			if ( align > 16384 )
			{
				unsigned int btree_ik = (align - 4096) / 96;
				unsigned int btree_bytes = 64 + 96*btree_ik;
				if ( btree_bytes > align ) {
					HANDLE_H5PART_INVALID_ERR(
						"btree_ik", btree_ik );
					goto error_cleanup;
				}

				_H5Part_print_info (
					"Setting HDF5 btree parameter to %u",
					btree_ik );
				_H5Part_print_info (
					"Extending HDF5 btree size to %u "
					"bytes at rank 3", btree_bytes );

				f->fcreate_prop = H5Pcreate(H5P_FILE_CREATE);
				if ( f->fcreate_prop < 0 ) {
					HANDLE_H5P_CREATE_ERR;
					goto error_cleanup;
				}

				H5Pset_istore_k (f->fcreate_prop, btree_ik);
			}

#ifdef H5PART_HAVE_HDF5_18
			/* defer metadata cache flushing until file close */
			H5AC_cache_config_t cache_config;
			cache_config.version = H5AC__CURR_CACHE_CONFIG_VERSION;
			H5Pget_mdc_config (f->access_prop, &cache_config);
			cache_config.set_initial_size = 1;
			cache_config.initial_size = 16 * 1024 * 1024;
			cache_config.evictions_enabled = 0;
			cache_config.incr_mode = H5C_incr__off;
			cache_config.flash_incr_mode = H5C_flash_incr__off;
			cache_config.decr_mode = H5C_decr__off;
			H5Pset_mdc_config (f->access_prop, &cache_config);
#else // H5_USE_16_API
			_H5Part_print_warn (
				"Unable to defer metadata write: need HDF5 1.8");
#endif // H5_USE_16_API
		}

		/* select the HDF5 VFD */
		if (flags & H5PART_VFD_MPIPOSIX) {
			_H5Part_print_info ( "Selecting MPI-POSIX VFD" );
			if (H5Pset_fapl_mpiposix ( f->access_prop, comm, 0 ) < 0) {
				HANDLE_H5P_SET_FAPL_ERR;
				goto error_cleanup;
			}
		} else if (flags & H5PART_VFD_CORE) {
			_H5Part_print_info ( "Selecting CORE VFD" );
			if (H5Pset_fapl_core ( f->access_prop, align, 1 ) < 0) {
				HANDLE_H5P_SET_FAPL_ERR;
				goto error_cleanup;
			}
		} else {
			_H5Part_print_info ( "Selecting MPI-IO VFD" );
			if (H5Pset_fapl_mpio ( f->access_prop, comm, info ) < 0) {
				HANDLE_H5P_SET_FAPL_ERR;
				goto error_cleanup;
			}
			if (flags & H5PART_VFD_MPIIO_IND) {
				_H5Part_print_info ( "Using independent mode" );
			} else {
				_H5Part_print_info ( "Using collective mode" );
				f->xfer_prop = H5Pcreate (H5P_DATASET_XFER);
				if (f->xfer_prop < 0) {
					HANDLE_H5P_CREATE_ERR;
					goto error_cleanup;
				}
				if (H5Pset_dxpl_mpio ( f->xfer_prop, H5FD_MPIO_COLLECTIVE ) < 0) {
					HANDLE_H5P_SET_DXPL_MPIO_ERR;
					goto error_cleanup;
				}
			}
		}

		f->comm = comm;
#endif // PARALLEL_IO
	} else {
		_is_root_proc = 1;
		f->comm = 0;
		f->nprocs = 1;
		f->myproc = 0;
		f->pnparticles = 
			(h5part_int64_t*) malloc (f->nprocs * sizeof (h5part_int64_t));
	}

	if ( align != 0 ) {
		_H5Part_print_info (
			"Setting HDF5 alignment to %lld bytes with threshold at half that many bytes",
			(long long)align );
		if (H5Pset_alignment ( f->access_prop, align/2, align ) < 0) {
			HANDLE_H5P_SET_FAPL_ERR;
			goto error_cleanup;
		}
		_H5Part_print_info (
			"Setting HDF5 meta block to %lld bytes",
			(long long)align );
		if (H5Pset_meta_block_size ( f->access_prop, align ) < 0) {
			HANDLE_H5P_SET_FAPL_ERR;
			goto error_cleanup;
		}
	}

	if ( flags & H5PART_READ ) {
		f->file = H5Fopen(filename, H5F_ACC_RDONLY, f->access_prop);
	}
	else if ( flags & H5PART_WRITE ){
		f->file = H5Fcreate (filename, H5F_ACC_TRUNC, f->fcreate_prop,
				f->access_prop);
		f->empty = 1;
	}
	else if ( flags & H5PART_APPEND ) {
		int fd = open(filename, O_RDONLY, 0);
		if ( (fd == -1) && (errno == ENOENT) ) {
			f->file = H5Fcreate(filename, H5F_ACC_TRUNC,
					f->fcreate_prop, f->access_prop);
			f->empty = 1;
		}
		else if (fd != -1) {
			close (fd);
			f->file = H5Fopen(
				filename, H5F_ACC_RDWR, f->access_prop);
			/*
			  The following function call returns an error,
			  if f->file < 0. But we can safely ignore this.
			*/
			f->timestep = _H5Part_get_num_objects_matching_pattern(
				f->file, "/", H5G_GROUP, f->groupname_step );
			if ( f->timestep < 0 ) goto error_cleanup;
		}
	}
	else {
		HANDLE_H5PART_FILE_ACCESS_TYPE_ERR ( flags );
		goto error_cleanup;
	}

	if (f->file < 0) {
		HANDLE_H5F_OPEN_ERR ( filename, flags );
		goto error_cleanup;
	}
	f->nparticles = 0;
	f->timegroup = -1;
	f->shape = H5S_ALL;
	f->diskshape = H5S_ALL;
	f->memshape = H5S_ALL;
	f->viewstart = -1;
	f->viewend = -1;
	f->viewindexed = 0;
	f->throttle = 0;

	_H5Part_print_debug (
		"Proc[%d]: Opened file \"%s\" val=%lld",
		f->myproc,
		filename,
		(long long)(size_t)f );

	return f;

 error_cleanup:
	if (f != NULL ) {
		if (f->pnparticles != NULL) {
			free (f->pnparticles);
		}
		free (f);
	}
	return NULL;
}

#ifdef PARALLEL_IO
/*!
  \ingroup h5part_open

  Opens file with specified filename. 

  Flags are bit values that can be combined with the bit operator \c |
  and include:

  - \c H5PART_WRITE - truncate file and open for writing
  - \c H5PART_APPEND - open file for writing without truncating
  - \c H5PART_READ - open file read-only
  - \c H5PART_FS_LUSTRE - enable optimizations for the Lustre file system
  - \c H5PART_VFD_MPIPOSIX - use the HDF5 MPI-POSIX virtual file driver
  - \c H5PART_VFD_MPIO_IND - use MPI-IO in indepedent mode

  The typical file extension is \c .h5.
  
  H5PartFile should be treated as an essentially opaque
  datastructure.  It acts as the file handle, but internally
  it maintains several key state variables associated with 
  the file.

  \return	File handle or \c NULL
 */
H5PartFile*
H5PartOpenFileParallel (
	const char *filename,	/*!< [in] The name of the data file to open. */
	const char flags,	/*!< [in] The access mode for the file. */
	H5_Comm comm		/*!< [in] MPI communicator */
	) {
	INIT
	SET_FNAME ( "H5PartOpenFileParallel" );

	int f_parallel = 1;	/* parallel i/o */
	h5part_int64_t align = 0; /* no alignment tuning */

	return _H5Part_open_file ( filename, flags, comm, f_parallel, align );
}

/*!
  \ingroup h5part_open

  Opens file with specified filename, and also specifices an alignment
  value used for HDF5 tuning parameters.

  Flags are bit values that can be combined with the bit operator \c |
  and include:

  - \c H5PART_WRITE - truncate file and open for writing
  - \c H5PART_APPEND - open file for writing without truncating
  - \c H5PART_READ - open file read-only
  - \c H5PART_FS_LUSTRE - enable optimizations for the Lustre file system
  - \c H5PART_VFD_MPIPOSIX - use the HDF5 MPI-POSIX virtual file driver
  - \c H5PART_VFD_MPIO_IND - use MPI-IO in indepedent mode

  The typical file extension is \c .h5.
  
  H5PartFile should be treated as an essentially opaque
  datastructure.  It acts as the file handle, but internally
  it maintains several key state variables associated with 
  the file.

  \return	File handle or \c NULL
 */
H5PartFile*
H5PartOpenFileParallelAlign (
	const char *filename,	/*!< [in] The name of the data file to open. */
	const char flags,	/*!< [in] The access mode for the file. */
	H5_Comm comm,		/*!< [in] MPI communicator */
	h5part_int64_t align	/*!< [in] Alignment size in bytes. */
	) {
	INIT
	SET_FNAME ( "H5PartOpenFileParallelAlign" );

	int f_parallel = 1;	/* parallel i/o */

	return _H5Part_open_file ( filename, flags, comm, f_parallel, align );
}
#endif

/*!
  \ingroup  h5part_open

  Opens file with specified filename. 

  Flags are bit values that can be combined with the bit operator \c |
  and include:

  - \c H5PART_WRITE - truncate file and open for writing
  - \c H5PART_APPEND - open file for writing without truncating
  - \c H5PART_READ - open file read-only

  The typical file extension is \c .h5.
  
  H5PartFile should be treated as an essentially opaque
  datastructure.  It acts as the file handle, but internally
  it maintains several key state variables associated with 
  the file.

  \return	File handle or \c NULL
 */
H5PartFile*
H5PartOpenFile (
	const char *filename,	/*!< [in] The name of the data file to open. */
	const char flags	/*!< [in] The access mode for the file. */
	) {
	INIT
	SET_FNAME ( "H5PartOpenFile" );

	H5_Comm comm = 0;	/* dummy */
	int f_parallel = 0;	/* serial open */
	int align = 0;		/* no tuning parameters */

	return _H5Part_open_file ( filename, flags, comm, f_parallel, align );
}

/*!
  \ingroup h5part_open

  Opens file with specified filename, and also specifices an alignment
  value used for HDF5 tuning parameters.

  Flags are bit values that can be combined with the bit operator \c |
  and include:

  - \c H5PART_WRITE - truncate file and open for writing
  - \c H5PART_APPEND - open file for writing without truncating
  - \c H5PART_READ - open file read-only

  The typical file extension is \c .h5.
  
  H5PartFile should be treated as an essentially opaque
  datastructure.  It acts as the file handle, but internally
  it maintains several key state variables associated with 
  the file.

  \return	File handle or \c NULL
 */
H5PartFile*
H5PartOpenFileAlign (
	const char *filename,	/*!< [in] The name of the data file to open. */
	const char flags,	/*!< [in] The access mode for the file. */
	h5part_int64_t align	/*!< [in] Alignment size in bytes. */
) {
	INIT
	SET_FNAME ( "H5PartOpenFileAlign" );

	H5_Comm comm = 0;	/* dummy */
	int f_parallel = 0;	/* serial open */

	return _H5Part_open_file ( filename, flags, comm, f_parallel, align );
}

/*!
  Checks if a file was successfully opened.

  \return	\c H5PART_SUCCESS or error code
 */
h5part_int64_t
_H5Part_file_is_valid (
	const H5PartFile *f	/*!< filehandle  to check validity of */
	) {

	if( f == NULL )
		return H5PART_ERR_BADFD;
	else if(f->file > 0)
		return H5PART_SUCCESS;
	else
		return H5PART_ERR_BADFD;
}

/*!
  \ingroup h5part_open

  Closes an open file.

  \return	\c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartCloseFile (
	H5PartFile *f		/*!< [in] filehandle of the file to close */
	) {

	SET_FNAME ( "H5PartCloseFile" );
	herr_t r = 0;
	_h5part_errno = H5PART_SUCCESS;

	CHECK_FILEHANDLE ( f );

	if ( f->block && f->close_block ) {
		(*f->close_block) ( f );
		f->block = NULL;
		f->close_block = NULL;
	}

#ifdef PARALLEL_IO
	if ( f->multiblock && f->close_multiblock ) {
		(*f->close_multiblock) ( f );
		f->multiblock = NULL;
		f->close_multiblock = NULL;
	}
#endif

	if( f->shape != H5S_ALL ) {
		r = H5Sclose( f->shape );
		if ( r < 0 ) HANDLE_H5S_CLOSE_ERR;
		f->shape = 0;
	}
	if( f->timegroup >= 0 ) {
		r = H5Gclose( f->timegroup );
		if ( r < 0 ) HANDLE_H5G_CLOSE_ERR;
		f->timegroup = -1;
	}
	if( f->diskshape != H5S_ALL ) {
		r = H5Sclose( f->diskshape );
		if ( r < 0 ) HANDLE_H5S_CLOSE_ERR;
		f->diskshape = 0;
	}
	if( f->memshape != H5S_ALL ) {
		r = H5Sclose( f->memshape );
		if ( r < 0 ) HANDLE_H5S_CLOSE_ERR;
		f->memshape = 0;
	}
	if( f->xfer_prop != H5P_DEFAULT ) {
		r = H5Pclose( f->xfer_prop );
		if ( r < 0 ) HANDLE_H5P_CLOSE_ERR ( "f->xfer_prop" );
		f->xfer_prop = H5P_DEFAULT;
	}
	if( f->dcreate_prop != H5P_DEFAULT ) {
		r = H5Pclose( f->dcreate_prop );
		if ( r < 0 ) HANDLE_H5P_CLOSE_ERR ( "f->dcreate_prop" );
		f->dcreate_prop = H5P_DEFAULT;
	}


	if ( f->file ) {
		r = H5Fclose( f->file );
		if ( r < 0 ) HANDLE_H5F_CLOSE_ERR;
		f->file = 0;
	}
	if( f->access_prop != H5P_DEFAULT ) {
		r = H5Pclose( f->access_prop );
		if ( r < 0 ) HANDLE_H5P_CLOSE_ERR ( "f->access_prop" );
		f->access_prop = H5P_DEFAULT;
	}  
	if( f->fcreate_prop != H5P_DEFAULT ) {
		r = H5Pclose( f->fcreate_prop );
		if ( r < 0 ) HANDLE_H5P_CLOSE_ERR ( "f->fcreate_prop" );
		f->fcreate_prop = H5P_DEFAULT;
	}

	/* free memory from H5PartFile struct */
	if( f->pnparticles ) {
		free( f->pnparticles );
	}
	free( f );

	return _h5part_errno;
}

h5part_int64_t
H5PartFileIsValid (
	H5PartFile *f
	) {
	return _H5Part_file_is_valid(f);
}

/*============== File Writing Functions ==================== */

h5part_int64_t
_H5Part_get_step_name(
	H5PartFile *f,
	const h5part_int64_t step,
	char *name
	) {

	/* Work around sprintf bug on older systems */
	if (f->stepno_width == 0 && step == 0) {
		sprintf (
			name,
			"%s#%0*lld",
			f->groupname_step, 1, (long long) step );
	}
	else {
		sprintf (
			name,
			"%s#%0*lld",
			f->groupname_step, f->stepno_width, (long long) step );
	}

	return H5PART_SUCCESS;
}

h5part_int64_t
H5PartDefineStepName (
	H5PartFile *f,
	const char *name,
	const h5part_int64_t width
	) {

	CHECK_FILEHANDLE ( f );

	h5part_int64_t len = H5PART_STEPNAME_LEN - width - 2;
	if ( strlen(name) > len ) {
		_H5Part_print_warn (
			"Step name has been truncated to fit within %d chars.",
			H5PART_STEPNAME_LEN );
	}

	strncpy ( f->groupname_step, name, len );
	f->stepno_width = (int)width;

	_H5Part_print_debug ( "Step name defined as '%s'", f->groupname_step );
	
	return H5PART_SUCCESS;
}

static h5part_int64_t
_set_num_particles (
	H5PartFile *f,				/*!< [in] Handle to open file */
	const h5part_int64_t nparticles,	/*!< [in] Number of particles */
	const h5part_int64_t _stride
	) {

	int ret;
	h5part_int64_t herr;

	hsize_t count;
#ifdef HDF5V160
	hssize_t start;
#else
	hsize_t start;
#endif
	hsize_t stride;
	hsize_t dmax = H5S_UNLIMITED;

#ifdef PARALLEL_IO
	hsize_t total;
	register int i;
#endif

#ifdef PARALLEL_IO
	if ( nparticles < 0 )
#else
	if ( nparticles <= 0 )
#endif
		return HANDLE_H5PART_INVALID_ERR ( "nparticles", nparticles );

	/* prevent invalid stride value */	
	if (_stride < 1)
	{
		_H5Part_print_warn (
			"Stride < 1 was specified: changing to 1." );
		stride = 1;
	} else {
		stride = (hsize_t) _stride;
	}

	if ( nparticles == 0 ) stride = 1;

#ifndef PARALLEL_IO
	/*
	  if we are not using parallel-IO, there is enough information
	   to know that we can short circuit this routine.  However,
	   for parallel IO, this is going to cause problems because
	   we don't know if things have changed globally
	*/
	if ( f->nparticles == nparticles && stride == 1 ) {
		_H5Part_print_debug (
			"Serial mode: skipping unnecessary view creation" );
		return H5PART_SUCCESS;
	}
#endif

	herr = _reset_view ( f );
	if ( herr < 0 ) return herr;

	if ( f->shape != H5S_ALL ) {
		herr = H5Sclose ( f->shape );
		if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
		f->shape = H5S_ALL;
	}

	f->nparticles = (hsize_t) nparticles;

	if ( f->nparticles > 0 )
	{
		/* declare local memory datasize with striding */
		count = f->nparticles * stride;
		f->memshape = H5Screate_simple ( 1, &count, &dmax );
		if ( f->memshape < 0 )
			return HANDLE_H5S_CREATE_SIMPLE_ERR ( f->nparticles );
	}

	/* we need a hyperslab selection if there is striding
	 * (otherwise, the default H5S_ALL selection is ok)
	 */
	if ( stride > 1 )
	{
		start = 0;
		count = f->nparticles;
		herr = H5Sselect_hyperslab (
			f->memshape,
			H5S_SELECT_SET,
			&start,
			&stride,
			&count, NULL );
		if ( herr < 0 ) return HANDLE_H5S_SELECT_HYPERSLAB_ERR;
	}

#ifndef PARALLEL_IO
	count = f->nparticles;
	f->shape = H5Screate_simple ( 1, &count, NULL );
	if ( f->shape < 0 ) HANDLE_H5S_CREATE_SIMPLE_ERR ( count );
	f->viewstart = 0;
	f->viewend   = nparticles - 1; // view range is *inclusive*
#else /* PARALLEL_IO */
	/*
	  The Gameplan here is to declare the overall size of the on-disk
	  data structure the same way we do for the serial case.  But
	  then we must have additional "DataSpace" structures to define
	  our in-memory layout of our domain-decomposed portion of the particle
	  list as well as a "selection" of a subset of the on-disk 
	  data layout that will be written in parallel to mutually exclusive
	  regions by all of the processors during a parallel I/O operation.
	  These are f->shape, f->memshape and f->diskshape respectively.
	*/

	/*
	  acquire the number of particles to be written from each MPI process
	*/

	ret = MPI_Allgather (
		(void*)&nparticles, 1, MPI_LONG_LONG,
		f->pnparticles, 1, MPI_LONG_LONG,
		f->comm );
	if ( ret != MPI_SUCCESS) return HANDLE_MPI_ALLGATHER_ERR;

	if ( f->myproc == 0 ) {
		for ( i=0; i<f->nprocs; i++ ) 
			_H5Part_print_debug_detail (
				"[%d] np=%lld",
				i, (long long) f->pnparticles[i] );
	}

	/* compute start offsets */
	start = 0;
	for (i=0; i<f->myproc; i++) {
		start += f->pnparticles[i];
	}
	f->viewstart = start;
	f->viewend   = start + f->nparticles - 1; // view range is *inclusive*
	
	/* compute total nparticles */
	total = 0;
	for (i=0; i < f->nprocs; i++) {
		total += f->pnparticles[i];
	}

	/* declare overall datasize */
	count = total;
	f->shape = H5Screate_simple (1, &count, NULL);
	if ( f->shape < 0 ) return HANDLE_H5S_CREATE_SIMPLE_ERR ( count );

	/* declare overall data size  but then will select a subset */
	f->diskshape = H5Screate_simple (1, &count, NULL);
	if ( f->diskshape < 0 ) return HANDLE_H5S_CREATE_SIMPLE_ERR ( count );

	count = nparticles;
	stride = 1;
	if ( count > 0 ) {
		herr = H5Sselect_hyperslab (
			f->diskshape,
			H5S_SELECT_SET,
			&start,
			&stride,
			&count, NULL );
	} else {
		herr = H5Sselect_none ( f->diskshape );
	}
	if ( herr < 0 ) return HANDLE_H5S_SELECT_HYPERSLAB_ERR;
#endif

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_model

  Set the number of particles for the current time-step.
  After you call this subroutine, all subsequent 
  operations will assume this number of particles will be written.

  For the parallel library, the \c nparticles value is the number of
  particles that the \e individual task will write. You can use
  a different value on different tasks.
  This function uses an \c MPI_Allgather
  call to aggregate each tasks number of particles and determine
  the appropiate offsets. Because of the use of this MPI collective,
  it is advisable to call this function as
  few times as possible when running at large concurrency.

  This function assumes that your particles' data fields are in stored in
  contiguous 1D arrays.
  For instance, the fields \e x and \e y for your particles are stored
  in separate arrays \c x[] and \c y[].
  
  If instead you store your particles as tuples, so that the values
  are arranged \f$ x_1,y_1,x_2,y_2\f$... than you need to setup striding
  (in this case with value 2) using \ref H5PartSetNumParticlesStrided.

  \return	\c H5PART_SUCCESS or error code
 */
h5part_int64_t
H5PartSetNumParticles (
	H5PartFile *f,			/*!< [in] Handle to open file */
	const h5part_int64_t nparticles	/*!< [in] Number of particles */
	) {

	SET_FNAME ( "H5PartSetNumParticles" );
	CHECK_FILEHANDLE( f );

	h5part_int64_t herr;
	h5part_int64_t stride = 1;

	herr = _set_num_particles ( f, nparticles, stride );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_model

  Set the number of particles for the current time-step.
  After you call this subroutine, all subsequent 
  operations will assume this number of particles will be written.

  For the parallel library, the \c nparticles value is the number of
  particles that the \e individual task will write. You can use
  a different value on different tasks.
  This function uses an \c MPI_Allgather
  call to aggregate each tasks number of particles and determine
  the appropiate offsets. Because of the use of this MPI collective,
  it is advisable to call this function as
  few times as possible when running at large concurrency.

  This function assumes that your particles' data fields are
  stored tuples. For instance, the fields \e x and \e y of your
  particles are arranged \f$x_1,y_1,x_2,y_2\f$... in a single data
  array. In this example, the stride value would be 2.
  
  If you instead have a separate array for each fields,
  such as \c x[] and \c y[],
  use \ref H5PartSetNumParticles.

  \return	\c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartSetNumParticlesStrided (
	H5PartFile *f,				/*!< [in] Handle to open file */
	const h5part_int64_t nparticles,	/*!< [in] Number of particles */
	const h5part_int64_t stride		/*!< [in] Stride value (e.g. number of fields in the particle array) */
	) {

	SET_FNAME ( "H5PartSetNumParticlesStrided" );
	CHECK_FILEHANDLE( f );

	h5part_int64_t herr;

	herr = _set_num_particles ( f, nparticles, stride );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_model

  Define the chunk \c size and enables chunking in the underlying
  HDF5 layer. When combined with the \c align value in the
  \ref H5PartOpenFileAlign or \ref H5PartOpenFileParallelAlign
  function, this causes each group of \c size particles to be
  padded on disk out to the nearest multiple of \c align bytes.

  Note that this policy wastes disk space, but can improve write
  bandwidth on parallel filesystems that are sensitive to alignment
  to stripe boundaries (e.g. lustre).

  \return	\c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartSetChunkSize (
	H5PartFile *f,
	const h5part_int64_t size
	) {

	SET_FNAME ( "H5PartSetChunkSize" );
	CHECK_FILEHANDLE( f );

	_H5Part_print_info (
		"Setting chunk size to %lld elements",
		(long long)size );

	if ( f->dcreate_prop == H5P_DEFAULT ) {
		f->dcreate_prop = H5Pcreate (H5P_DATASET_CREATE);
		if ( f->dcreate_prop < 0 ) return HANDLE_H5P_CREATE_ERR;
	}

	hsize_t hsize = (hsize_t)size;

	herr_t herr = H5Pset_chunk ( f->dcreate_prop, 1, &hsize );
	if ( herr < 0 ) return HANDLE_H5P_SET_CHUNK_ERR;

	return H5PART_SUCCESS;
}

static void
_normalize_dataset_name (
	const char *name,
	char *name2
	) {

	if ( strlen(name) > H5PART_DATANAME_LEN ) {
		strncpy ( name2, name, H5PART_DATANAME_LEN - 1 );
		name2[H5PART_DATANAME_LEN-1] = '\0';
		_H5Part_print_warn (
			"Dataset name '%s' is longer than maximum %d chars. "
			"Truncated to: '%s'",
			name, H5PART_DATANAME_LEN, name2 );
	} else {
		strcpy ( name2, name );
	}
}

static h5part_int64_t
_write_data (
	H5PartFile *f,		/*!< IN: Handle to open file */
	const char *name,	/*!< IN: Name to associate array with */
	const void *array,	/*!< IN: Array to commit to disk */
	const hid_t type	/*!< IN: Type of data */
	) {

	herr_t herr;
	hid_t dataset_id;

	char name2[H5PART_DATANAME_LEN];
	_normalize_dataset_name ( name, name2 );

	_H5Part_print_debug (
			"Create a dataset[%s] mounted on "
			"timestep %lld",
			name2, (long long)f->timestep );

	if ( f->shape == H5S_ALL ) {
		_H5Part_print_warn (
			"The view is unset or invalid: please "
			"set the view or specify a number of particles." );
		return HANDLE_H5PART_BAD_VIEW_ERR ( f->viewstart, f->viewend );
	}

	H5E_BEGIN_TRY
	dataset_id = H5Dopen ( f->timegroup, name2
#ifndef H5_USE_16_API
		, H5P_DEFAULT
#endif
		);
	H5E_END_TRY

	if ( dataset_id > 0 ) {
		_H5Part_print_warn (
			"Dataset[%s] at timestep %lld "
			"already exists", name2, (long long)f->timestep );
	} else {
		dataset_id = H5Dcreate ( 
			f->timegroup,
			name2,
			type,
			f->shape,
#ifndef H5_USE_16_API
			H5P_DEFAULT,
			f->dcreate_prop,
			H5P_DEFAULT
#else
			f->dcreate_prop
#endif
                );
		if ( dataset_id < 0 )
			return HANDLE_H5D_CREATE_ERR ( name2, f->timestep );
	}

#ifdef PARALLEL_IO
	herr = _H5Part_start_throttle ( f );
	if ( herr < 0 ) return herr;
#endif

	herr = H5Dwrite (
		dataset_id,
		type,
		f->memshape,
		f->diskshape,
		f->xfer_prop,
		array );

#ifdef PARALLEL_IO
	herr = _H5Part_end_throttle ( f );
	if ( herr < 0 ) return herr;
#endif

	if ( herr < 0 ) return HANDLE_H5D_WRITE_ERR ( name2, f->timestep );

	herr = H5Dclose ( dataset_id );
	if ( herr < 0 ) return HANDLE_H5D_CLOSE_ERR;

	f->empty = 0;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_data

  Write array of 64 bit floating point data to file.

  After setting the number of particles with \c H5PartSetNumParticles() and
  the current timestep using \c H5PartSetStep(), you can start writing datasets
  into the file. Each dataset has a name associated with it (chosen by the
  user) in order to facilitate later retrieval. The name of the dataset is
  specified in the parameter \c name, which must be a null-terminated string.

  There are no restrictions on naming of datasets, but it is useful to arrive
  at some common naming convention when sharing data with other groups.

  The writing routines also implicitly store the datatype of the array so that
  the array can be reconstructed properly on other systems with incompatible
  type representations.

  All data that is written after setting the timestep is associated with that
  timestep. While the number of particles can change for each timestep, you
  cannot change the number of particles in the middle of a given timestep.

  The data is committed to disk before the routine returns.

  \return	\c H5PART_SUCCESS or error code
 */
h5part_int64_t
H5PartWriteDataFloat64 (
	H5PartFile *f,		/*!< [in] Handle to open file */
	const char *name,	/*!< [in] Name to associate array with */
	const h5part_float64_t *array	/*!< [in] Array to commit to disk */
	) {

	SET_FNAME ( "H5PartWriteDataFloat64" );
	h5part_int64_t herr;

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	herr = _write_data ( f, name, (void*)array, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_data

  Write array of 32 bit floating point data to file.

  After setting the number of particles with \c H5PartSetNumParticles() and
  the current timestep using \c H5PartSetStep(), you can start writing datasets
  into the file. Each dataset has a name associated with it (chosen by the
  user) in order to facilitate later retrieval. The name of the dataset is
  specified in the parameter \c name, which must be a null-terminated string.

  There are no restrictions on naming of datasets, but it is useful to arrive
  at some common naming convention when sharing data with other groups.

  The writing routines also implicitly store the datatype of the array so that
  the array can be reconstructed properly on other systems with incompatible
  type representations.

  All data that is written after setting the timestep is associated with that
  timestep. While the number of particles can change for each timestep, you
  cannot change the number of particles in the middle of a given timestep.

  The data is committed to disk before the routine returns.

  \return	\c H5PART_SUCCESS or error code
 */
h5part_int64_t
H5PartWriteDataFloat32 (
	H5PartFile *f,		/*!< [in] Handle to open file */
	const char *name,	/*!< [in] Name to associate array with */
	const h5part_float32_t *array	/*!< [in] Array to commit to disk */
	) {

	SET_FNAME ( "H5PartWriteDataFloat32" );
	h5part_int64_t herr;

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	herr = _write_data ( f, name, (void*)array, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_data

  Write array of 64 bit integer data to file.

  After setting the number of particles with \c H5PartSetNumParticles() and
  the current timestep using \c H5PartSetStep(), you can start writing datasets
  into the file. Each dataset has a name associated with it (chosen by the
  user) in order to facilitate later retrieval. The name of the dataset is
  specified in the parameter \c name, which must be a null-terminated string.

  There are no restrictions on naming of datasets, but it is useful to arrive
  at some common naming convention when sharing data with other groups.

  The writing routines also implicitly store the datatype of the array so that
  the array can be reconstructed properly on other systems with incompatible
  type representations.

  All data that is written after setting the timestep is associated with that
  timestep. While the number of particles can change for each timestep, you
  cannot change the number of particles in the middle of a given timestep.

  The data is committed to disk before the routine returns.

  \return	\c H5PART_SUCCESS or error code
 */
h5part_int64_t
H5PartWriteDataInt64 (
	H5PartFile *f,		/*!< [in] Handle to open file */
	const char *name,	/*!< [in] Name to associate array with */
	const h5part_int64_t *array	/*!< [in] Array to commit to disk */
	) {

	SET_FNAME ( "H5PartWriteDataInt64" );

	h5part_int64_t herr;

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	herr = _write_data ( f, name, (void*)array, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_data

  Write array of 32 bit integer data to file.

  After setting the number of particles with \c H5PartSetNumParticles() and
  the current timestep using \c H5PartSetStep(), you can start writing datasets
  into the file. Each dataset has a name associated with it (chosen by the
  user) in order to facilitate later retrieval. The name of the dataset is
  specified in the parameter \c name, which must be a null-terminated string.

  There are no restrictions on naming of datasets, but it is useful to arrive
  at some common naming convention when sharing data with other groups.

  The writing routines also implicitly store the datatype of the array so that
  the array can be reconstructed properly on other systems with incompatible
  type representations.

  All data that is written after setting the timestep is associated with that
  timestep. While the number of particles can change for each timestep, you
  cannot change the number of particles in the middle of a given timestep.

  The data is committed to disk before the routine returns.

  \return	\c H5PART_SUCCESS or error code
 */
h5part_int64_t
H5PartWriteDataInt32 (
	H5PartFile *f,		/*!< [in] Handle to open file */
	const char *name,	/*!< [in] Name to associate array with */
	const h5part_int32_t *array	/*!< [in] Array to commit to disk */
	) {

	SET_FNAME ( "H5PartWriteDataInt32" );

	h5part_int64_t herr;

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	herr = _write_data ( f, name, (void*)array, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/********************** reading and writing attribute ************************/

/********************** private functions to handle attributes ***************/

/*!
  \ingroup h5partkernel
  @{
*/

h5part_int64_t
_H5Part_make_string_type(
	hid_t *stype,
	int size
	) {
	*stype = H5Tcopy ( H5T_C_S1 );
	if ( *stype < 0 ) return HANDLE_H5T_STRING_ERR;
	herr_t herr = H5Tset_size ( *stype, size );
	if ( herr < 0 ) return HANDLE_H5T_STRING_ERR;
	return H5PART_SUCCESS;
}

/*!
   Normalize HDF5 type
*/
h5part_int64_t
_H5Part_normalize_h5_type (
	hid_t type
	) {

	H5T_class_t tclass = H5Tget_class ( type );
	int size = H5Tget_size ( type );

	switch ( tclass ) {
	case H5T_INTEGER:
		if ( size==8 ) {
			return H5PART_INT64;
		}
	  	else if ( size==4 ) {
			return H5PART_INT32;
		}
	  	else if ( size==1 ) {
			return H5PART_CHAR;
		}
		break;
	case H5T_FLOAT:
		if ( size==8 ) {
			return H5PART_FLOAT64;
		}
		else if ( size==4 ) {
			return H5PART_FLOAT32;
		}
		break;
	case H5T_STRING:
		return H5PART_STRING;
	default:
		;/* NOP */
	}
	
	return HANDLE_H5PART_TYPE_ERR;
}

h5part_int64_t
_H5Part_read_attrib (
	hid_t id,
	const char *attrib_name,
	void *attrib_value
	) {

	herr_t herr;
	hid_t attrib_id;
	hid_t space_id;
	hid_t type_id;

#ifdef H5PART_HAVE_HDF5_18
	if (! H5Aexists ( id, attrib_name )) {
		_H5Part_print_warn ( "Attribute '%s' does not exist!", attrib_name );
	}
	attrib_id = H5Aopen ( id, attrib_name, H5P_DEFAULT );
#else
	attrib_id = H5Aopen_name ( id, attrib_name );
#endif
	if ( attrib_id <= 0 ) return HANDLE_H5A_OPEN_NAME_ERR( attrib_name );

	type_id = H5Aget_type ( attrib_id );
	if ( type_id < 0 ) return HANDLE_H5A_GET_TYPE_ERR;

	space_id = H5Aget_space ( attrib_id );
	if ( space_id < 0 ) return HANDLE_H5A_GET_SPACE_ERR;

	herr = H5Aread ( attrib_id, type_id, attrib_value );
	if ( herr < 0 ) return HANDLE_H5A_READ_ERR;

	herr = H5Sclose ( space_id );
	if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;

	herr = H5Tclose ( type_id );
	if ( herr < 0 ) return HANDLE_H5T_CLOSE_ERR;

	herr = H5Aclose ( attrib_id );
	if ( herr < 0 ) return HANDLE_H5A_CLOSE_ERR;

	return H5PART_SUCCESS;
}

h5part_int64_t
_H5Part_write_attrib (
	hid_t id,
	const char *attrib_name,
	const hid_t attrib_type,
	const void *attrib_value,
	const hsize_t attrib_nelem
	) {

	h5part_int64_t herr;
	hid_t space_id;
	hid_t attrib_id;
	hid_t type = attrib_type;

	if ( attrib_type == H5PART_STRING )
	{
		herr = _H5Part_make_string_type ( &type, attrib_nelem );
		if ( herr < 0 ) return herr;

		space_id = H5Screate (H5S_SCALAR);
		if ( space_id < 0 )
			return HANDLE_H5S_CREATE_SCALAR_ERR;
	}
	else {
		space_id = H5Screate_simple ( 1, &attrib_nelem, NULL );
		if ( space_id < 0 )
			return HANDLE_H5S_CREATE_SIMPLE_ERR ( attrib_nelem );
	}

	attrib_id = H5Acreate(
		id,
		attrib_name,
		type,
		space_id,
		H5P_DEFAULT
#ifndef H5_USE_16_API
		, H5P_DEFAULT
#endif
		);

	if ( attrib_id < 0 ) return HANDLE_H5A_CREATE_ERR ( attrib_name );

	herr = H5Awrite ( attrib_id, type, attrib_value);
	if ( herr < 0 ) return HANDLE_H5A_WRITE_ERR ( attrib_name );

	herr = H5Aclose ( attrib_id );
	if ( herr < 0 ) return HANDLE_H5A_CLOSE_ERR;

	herr = H5Sclose ( space_id );
	if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;

	if ( attrib_type == H5PART_STRING ) {
		herr = H5Tclose ( type );
		if ( herr < 0 ) return HANDLE_H5T_CLOSE_ERR;
	}

	return H5PART_SUCCESS;
}

h5part_int64_t
_H5Part_write_file_attrib (
	H5PartFile *f,
	const char *name,
	const hid_t type,
	const void *value,
	const hsize_t nelem
	) {

	hid_t group_id = H5Gopen ( f->file, "/"
#ifndef H5_USE_16_API
				  , H5P_DEFAULT
#endif
				  );

	if ( group_id < 0 ) return HANDLE_H5G_OPEN_ERR( "/" );

	h5part_int64_t herr = _H5Part_write_attrib (
		group_id,
		name,
		type,
		value,
		nelem );
	if ( herr < 0 ) return herr;

	herr = H5Gclose ( group_id );
	if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;

	return H5PART_SUCCESS;
}

h5part_int64_t
_H5Part_write_step_attrib (
	H5PartFile *f,
	const char *name,
	const hid_t type,
	const void *value,
	const hsize_t nelem
	) {

	CHECK_TIMEGROUP( f );

	h5part_int64_t herr = _H5Part_write_attrib (
		f->timegroup,
		name,
		type,
		value,
		nelem );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

h5part_int64_t
_H5Part_get_attrib_info (
	hid_t id,
	const h5part_int64_t attrib_idx,
	char *attrib_name,
	const h5part_int64_t len_attrib_name,
	h5part_int64_t *attrib_type,
	h5part_int64_t *attrib_nelem
	) {

	herr_t herr;
	hid_t attrib_id;
	hid_t mytype;
	hid_t space_id;

	attrib_id = H5Aopen_idx ( id, (unsigned int)attrib_idx );
	if ( attrib_id < 0 ) return HANDLE_H5A_OPEN_IDX_ERR ( attrib_idx );

	if ( attrib_nelem ) {
		space_id =  H5Aget_space ( attrib_id );
		if ( space_id < 0 ) return HANDLE_H5A_GET_SPACE_ERR;

		*attrib_nelem = H5Sget_simple_extent_npoints ( space_id );
		if ( *attrib_nelem < 0 )
			return HANDLE_H5S_GET_SIMPLE_EXTENT_NPOINTS_ERR;

		herr = H5Sclose ( space_id );
		if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
	}
	if ( attrib_name ) {
		herr = H5Aget_name (
			attrib_id,
			(size_t)len_attrib_name,
			attrib_name );
		if ( herr < 0 ) return HANDLE_H5A_GET_NAME_ERR;
	}
	if ( attrib_type ) {
		mytype = H5Aget_type ( attrib_id );
		if ( mytype < 0 ) return HANDLE_H5A_GET_TYPE_ERR;

		*attrib_type = _H5Part_normalize_h5_type ( mytype );
		if ( *attrib_type < 0 ) return *attrib_type;

		herr = H5Tclose ( mytype );
		if ( herr < 0 ) return HANDLE_H5T_CLOSE_ERR;
	}
	herr = H5Aclose ( attrib_id);
	if ( herr < 0 ) return HANDLE_H5A_CLOSE_ERR;

	return H5PART_SUCCESS;
}
/*!
  }@
*/

/********************** attribute API ****************************************/

/*!
  \ingroup h5part_attrib

  Writes an attribute \c name with the string \c value to
  the file root ("/").

  \return	\c H5PART_SUCCESS or error code   
*/
h5part_int64_t
H5PartWriteFileAttribString (
	H5PartFile *f,		/*!< [in] Handle to open file */
	const char *name,	/*!< [in] Name of attribute to create */
	const char *value	/*!< [in] Value of attribute */ 
	) {

	SET_FNAME ( "H5PartWriteFileAttribString" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );

	h5part_int64_t herr = _H5Part_write_file_attrib (
		f,
		name,
		H5PART_STRING,
		value,
		strlen ( value ) + 1 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes an attribute \c name with the string \c value to
  the current timestep.

  \return	\c H5PART_SUCCESS or error code   
*/
h5part_int64_t
H5PartWriteStepAttribString (
	H5PartFile *f,		/*!< [in] Handle to open file */
	const char *name,	/*!< [in] Name of attribute to create */
	const char *value	/*!< [in] Value of attribute */ 
	) {

	SET_FNAME ( "H5PartWriteStepAttribString" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );

	h5part_int64_t herr = _H5Part_write_step_attrib (
		f,
		name,
		H5PART_STRING,
		value,
		strlen ( value ) + 1 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes an attribute \c name with values in the array \c data
  of \c nelem elements to
  the current timestep.

  The type of \c data must ve specified using one of the folowing
  macros:

  - \c H5PART_FLOAT64 (for \c h5part_float64_t)
  - \c H5PART_FLOAT32 (for \c h5part_float32_t)
  - \c H5PART_INT64 (for \c h5part_int64_t)
  - \c H5PART_INT32 (for \c h5part_int32_t)
  - \c H5PART_CHAR (for \c char)
  - \c H5PART_STRING (for \c char*)

  \return	\c H5PART_SUCCESS or error code   
*/
h5part_int64_t
H5PartWriteStepAttrib (
	H5PartFile *f,			/*!< [in] Handle to open file */
	const char *name,		/*!< [in] Name of attribute */
	const h5part_int64_t type,	/*!< [in] Type of values */
	const void *data,		/*!< [in] Array of attribute values */ 
	const h5part_int64_t nelem	/*!< [in] Number of array elements */
	){

	SET_FNAME ( "H5PartWriteStepAttrib" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	h5part_int64_t herr = _H5Part_write_step_attrib (
		f,
		name,
		(hid_t)type,
		data,
		(hsize_t)nelem );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes an attribute \c name with values in the array \c data
  of \c nelem elements to
  the file root ("/").

  The type of \c data must ve specified using one of the folowing
  macros:

  - \c H5PART_FLOAT64 (for \c h5part_float64_t)
  - \c H5PART_FLOAT32 (for \c h5part_float32_t)
  - \c H5PART_INT64 (for \c h5part_int64_t)
  - \c H5PART_INT32 (for \c h5part_int32_t)
  - \c H5PART_CHAR (for \c char)
  - \c H5PART_STRING (for \c char*)

  \return	\c H5PART_SUCCESS or error code   
*/
h5part_int64_t
H5PartWriteFileAttrib (
	H5PartFile *f,			/*!< [in] Handle to open file */
	const char *name,		/*!< [in] Name of attribute */
	const h5part_int64_t type,	/*!< [in] Type of values */
	const void *data,		/*!< [in] Array of attribute values */ 
	const h5part_int64_t nelem	/*!< [in] Number of array elements */
	) {

	SET_FNAME ( "H5PartWriteFileAttrib" );

	CHECK_FILEHANDLE ( f );
	CHECK_WRITABLE_MODE ( f );

	h5part_int64_t herr = _H5Part_write_file_attrib (
		f,
		name,
		(hid_t)type,
		data,
		(hsize_t)nelem );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Gets the number of attributes bound to the current step.

  \return	Number of attributes bound to current time step or error code.
*/
h5part_int64_t
H5PartGetNumStepAttribs (
	H5PartFile *f			/*!< [in] Handle to open file */
	) {

	SET_FNAME ( "H5PartGetNumStepAttribs" );
	CHECK_FILEHANDLE ( f );

	h5part_int64_t nattribs;

	nattribs = H5Aget_num_attrs(f->timegroup);
	if ( nattribs < 0 ) HANDLE_H5A_GET_NUM_ATTRS_ERR;

	return nattribs;
}

/*!
  \ingroup h5part_attrib

  Gets the number of attributes bound to the file.

  \return	Number of attributes bound to file \c f or error code.
*/
h5part_int64_t
H5PartGetNumFileAttribs (
	H5PartFile *f			/*!< [in] Handle to open file */
	) {

	SET_FNAME ( "H5PartGetNumFileAttribs" );
	herr_t herr;
	h5part_int64_t nattribs;

	CHECK_FILEHANDLE ( f );


	hid_t group_id = H5Gopen ( f->file, "/"
#ifndef H5_USE_16_API
			   , H5P_DEFAULT
#endif
			   );

	if ( group_id < 0 ) HANDLE_H5G_OPEN_ERR ( "/" );

	nattribs = H5Aget_num_attrs ( group_id );
	if ( nattribs < 0 ) HANDLE_H5A_GET_NUM_ATTRS_ERR;

	herr = H5Gclose ( group_id );
	if ( herr < 0 ) HANDLE_H5G_CLOSE_ERR;
	return nattribs;
}

/*!
  \ingroup h5part_attrib

  Gets the name, type and number of elements of the step attribute
  specified by its index.

  This function can be used to retrieve all attributes bound to the
  current time-step by looping from \c 0 to the number of attribute
  minus one.  The number of attributes bound to the current
  time-step can be queried by calling the function
  \c H5PartGetNumStepAttribs().

  \return	\c H5PART_SUCCESS or error code 
*/
h5part_int64_t
H5PartGetStepAttribInfo (
	H5PartFile *f,			/*!< [in]  Handle to open file */
	const h5part_int64_t attrib_idx,/*!< [in]  Index of attribute to
						   get infos about */
	char *attrib_name,		/*!< [out] Name of attribute */
	const h5part_int64_t len_of_attrib_name,
					/*!< [in]  length of buffer \c name */
	h5part_int64_t *attrib_type,	/*!< [out] Type of value. */
	h5part_int64_t *attrib_nelem	/*!< [out] Number of elements */
	) {
	
	SET_FNAME ( "H5PartGetStepAttribInfo" );
	h5part_int64_t herr;

	CHECK_FILEHANDLE( f );

	herr = _H5Part_get_attrib_info (
		f->timegroup,
		attrib_idx,
		attrib_name,
		len_of_attrib_name,
		attrib_type,
		attrib_nelem );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Gets the name, type and number of elements of the file attribute
  specified by its index.

  This function can be used to retrieve all attributes bound to the
  file \c f by looping from \c 0 to the number of attribute minus
  one.  The number of attributes bound to file \c f can be queried
  by calling the function \c H5PartGetNumFileAttribs().

  \return	\c H5PART_SUCCESS or error code 
*/

h5part_int64_t
H5PartGetFileAttribInfo (
	H5PartFile *f,			/*!< [in]  Handle to open file */
	const h5part_int64_t attrib_idx,/*!< [in]  Index of attribute to get
						   infos about */
	char *attrib_name,		/*!< [out] Name of attribute */
	const h5part_int64_t len_of_attrib_name,
					/*!< [in]  length of buffer \c name */
	h5part_int64_t *attrib_type,	/*!< [out] Type of value. */
	h5part_int64_t *attrib_nelem	/*!< [out] Number of elements */
	) {

	SET_FNAME ( "H5PartGetFileAttribInfo" );
	hid_t group_id;
	h5part_int64_t herr;

	CHECK_FILEHANDLE( f );

	group_id = H5Gopen(f->file,"/"
#ifndef H5_USE_16_API
			   , H5P_DEFAULT
#endif
			   );

	if ( group_id < 0 ) return HANDLE_H5G_OPEN_ERR( "/" );

	herr = _H5Part_get_attrib_info (
		group_id,
		attrib_idx,
		attrib_name,
		len_of_attrib_name,
		attrib_type,
		attrib_nelem );
	if ( herr < 0 ) return herr;

	herr = H5Gclose ( group_id );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Reads an attribute bound to current time-step.

  \return \c H5PART_SUCCESS or error code 
*/
h5part_int64_t
H5PartReadStepAttrib (
	H5PartFile *f,			/*!< [in]  Handle to open file */
	const char *attrib_name,	/*!< [in] Name of attribute to read */
	void *attrib_value		/*!< [out] Value of attribute */
	) {

	SET_FNAME ( "H5PartReadStepAttrib" );
	CHECK_FILEHANDLE( f );

	h5part_int64_t herr;

	herr = _H5Part_read_attrib ( f->timegroup, attrib_name, attrib_value );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Reads an attribute bound to file \c f.

  \return \c H5PART_SUCCESS or error code 
*/
h5part_int64_t
H5PartReadFileAttrib ( 
	H5PartFile *f,
	const char *attrib_name,
	void *attrib_value
	) {

	SET_FNAME ( "H5PartReadFileAttrib" );

	hid_t group_id;
	h5part_int64_t herr;

	CHECK_FILEHANDLE( f );

	group_id = H5Gopen(f->file,"/"
#ifndef H5_USE_16_API
			   , H5P_DEFAULT
#endif
			   );

	if ( group_id < 0 ) return HANDLE_H5G_OPEN_ERR( "/" );

	herr = _H5Part_read_attrib ( group_id, attrib_name, attrib_value );
	if ( herr < 0 ) return herr;

	herr = H5Gclose ( group_id );
	if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;

	return H5PART_SUCCESS;
}


/*================== File Reading Routines =================*/
/*
  H5PartSetStep:


  So you use this to random-access the file for a particular timestep.
  Failure to explicitly set the timestep on each read will leave you
  stuck on the same timestep for *all* of your reads.  That is to say
  the writes auto-advance the file pointer, but the reads do not
  (they require explicit advancing by selecting a particular timestep).
*/

h5part_int64_t
_H5Part_set_step (
	H5PartFile *f,			/*!< [in]  Handle to open file */
	const h5part_int64_t step	/*!< [in]  Time-step to set. */
	) {

	char stepname[H5PART_STEPNAME_LEN];
	_H5Part_get_step_name(f, step, stepname);

#if H5PART_SET_STEP_READ_ONLY
	if ( (!(f->flags & H5PART_READ)) && _H5Part_have_group ( f->file, stepname ) ) {
		return HANDLE_H5PART_STEP_EXISTS_ERR ( step );
	}
#endif

	if ( f->timegroup >= 0 ) {
		herr_t herr = H5Gclose ( f->timegroup );
		if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;
	}
	f->timegroup = -1;
	f->timestep = step;

#if H5PART_SET_STEP_READ_ONLY
	// in this mode, existing steps can be selecting only
	// for a READ file handle
	if ( f->flags & H5PART_READ ) {
		_H5Part_print_debug (
			"Proc[%d]: Set step to #%lld for file %lld",
			f->myproc,
			(long long)step,
			(long long)(size_t) f );

		f->timegroup = H5Gopen ( f->file, stepname
#ifndef H5_USE_16_API
					  , H5P_DEFAULT
#endif
					  );
		if ( f->timegroup < 0 )
			return HANDLE_H5G_OPEN_ERR ( stepname );
	}
	else {
		_H5Part_print_debug (
			"Proc[%d]: Create step #%lld for file %lld", 
			f->myproc,
			(long long)step,
			(long long)(size_t) f );

		f->timegroup = H5Gcreate( f->file, stepname, 0
#ifndef H5_USE_16_API
					  , H5P_DEFAULT, H5P_DEFAULT
#endif
					  );
		if ( f->timegroup < 0 )
			return HANDLE_H5G_CREATE_ERR ( stepname );
	}

#else // H5PART_SET_STEP_READ_ONLY

	// in this mode, existing steps can be selected for all file
	// handles: first try to open the step, and create it if it
	// doesn't exist
	H5E_BEGIN_TRY
	f->timegroup = H5Gopen ( f->file, stepname
#ifndef H5_USE_16_API
					  , H5P_DEFAULT
#endif
					  );
	H5E_END_TRY

	
	if ( f->timegroup < 0 )
	{
		f->timegroup = H5Gcreate( f->file, stepname, 0
#ifndef H5_USE_16_API
					  , H5P_DEFAULT, H5P_DEFAULT
#endif
					  );
		if ( f->timegroup < 0 )
			return HANDLE_H5G_CREATE_ERR ( stepname );
	}

#endif // H5PART_SET_STEP_READ_ONLY

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_model

  Set the current time-step.

  When writing data to a file the current time step must be set first
  (even if there is only one). In write-mode this function creates a new
  time-step! You are not allowed to step to an already existing time-step.
  This prevents you from overwriting existing data. Another consequence is,
  that you \b must write all data before going to the next time-step.

  In read-mode you can use this function to random-access the file for a
  particular timestep.

  \return \c H5PART_SUCCESS or error code 
*/
h5part_int64_t
H5PartSetStep (
	H5PartFile *f,			/*!< [in]  Handle to open file */
	const h5part_int64_t step	/*!< [in]  Time-step to set. */
	) {

	SET_FNAME ( "H5PartSetStep" );

	CHECK_FILEHANDLE ( f );

	return _H5Part_set_step ( f, step );
}

/********************** query file structure *********************************/

/*!
  \ingroup h5part_kernel

  Test whether a group named \c name exists at location \c id.
*/
h5part_int64_t
_H5Part_have_group (
	const hid_t id,
	const char *name
	) {
#ifdef H5PART_HAVE_HDF5_18
	return (H5Lexists( id, name, H5P_DEFAULT ) ? 1 : 0);
#else
	herr_t exists = 0;
	H5E_BEGIN_TRY
	exists = H5Gget_objinfo( id, name, 1, NULL );
	H5E_END_TRY
	return (exists >= 0 ? 1 : 0);
#endif
}


/********************** query file structure *********************************/

/*!
  \ingroup h5part_kernel

  Iterator for \c H5Giterate().
*/
#ifdef H5PART_HAVE_HDF5_18
herr_t
_H5Part_iteration_operator2 (
	hid_t group_id,			/*!< [in] parent object id */
	const char *member_name,	/*!< [in] child object name */
	const H5L_info_t *linfo,	/*!< link info */
	void *operator_data		/*!< [in,out] data passed to the iterator */
	) {

  switch (linfo->type) {
    case H5L_TYPE_HARD: {

      H5O_info_t objinfo;
      if( H5Oget_info_by_name( group_id, member_name, &objinfo, H5P_DEFAULT ) < 0 ) {
	return (herr_t)HANDLE_H5G_GET_OBJINFO_ERR ( member_name );
      }

      switch(objinfo.type)
      {
        case H5O_TYPE_GROUP:
        case H5O_TYPE_DATASET:
          return _H5Part_iteration_operator( group_id,
					     member_name,
					     operator_data );
          break;

        default:
	  return (herr_t)HANDLE_H5G_GET_OBJINFO_ERR ( member_name );
          break;
      }

      break;
    }

    case H5L_TYPE_EXTERNAL: {
      char *targbuf = (char*) malloc( linfo->u.val_size );
      if ( targbuf == NULL ) return (herr_t)HANDLE_H5PART_NOMEM_ERR;

      if(H5Lget_val(group_id, member_name, targbuf, linfo->u.val_size,
              H5P_DEFAULT) < 0) {

        // DO TO - THROW AN ERROR
      } else {
        const char *filename;
        const char *targname;

        if(H5Lunpack_elink_val(targbuf, linfo->u.val_size, 0,
                &filename, &targname) < 0) {
          // DO TO - THROW AN ERROR
        } else {

/* 	  std::cout << "VsFilter::visitLinks: node '" << member_name << "' is an external link." << std::endl; */

/* 	  std::cout << "VsFilter::visitLinks: node '" << targname << "' the is an external target group." << std::endl; */

          free(targbuf);

          // Open the linked object. 
          H5O_info_t objinfo;
          hid_t obj_id = H5Oopen(group_id, member_name, H5P_DEFAULT);
          if ( obj_id < 0 ) {
	    return (herr_t)HANDLE_H5G_OPEN_ERR ( member_name );
          }
          else if ( H5Oget_info ( obj_id, &objinfo ) < 0 ) {
	    return (herr_t)HANDLE_H5G_GET_OBJINFO_ERR ( member_name );
          }
          else {
	    
	    H5Oclose( obj_id );

	    switch(objinfo.type)
	    {
	    case H5O_TYPE_GROUP:
	    case H5O_TYPE_DATASET:
	      return _H5Part_iteration_operator( group_id,
						 member_name,
						 operator_data );
	      break;
	      
	    default:
	      return (herr_t)HANDLE_H5G_GET_OBJINFO_ERR ( member_name );
	      break;
	    }
	  }
        }
      }
    }
    break;


    default:
      return (herr_t)HANDLE_H5G_GET_OBJINFO_ERR ( member_name );
      break;
  }

  return 0;
}
#endif // H5_USE_16_API


/********************** query file structure *********************************/

/*!
  \ingroup h5part_kernel

  Iterator for \c H5Giterate().
*/
herr_t
_H5Part_iteration_operator (
	hid_t group_id,		/*!< [in] parent object id */
	const char *member_name,/*!< [in] child object name */
	void *operator_data	/*!< [in,out] data passed to the iterator */
	) {

	struct _iter_op_data *data = (struct _iter_op_data*)operator_data;
	herr_t herr;

/*   printf( "%d GROUP ITERATOR %s %d\n", __LINE__, member_name, data->type ); */

	if ( data->type != H5G_UNKNOWN )
	{
#ifdef H5PART_HAVE_HDF6_18
		H5O_info_t objinfo;

		hid_t obj_id = H5Oopen(group_id, member_name, H5P_DEFAULT);
		if ( obj_id < 0 )
			return (herr_t)HANDLE_H5G_OPEN_ERR ( member_name );

		herr = H5Oget_info ( obj_id, &objinfo );
		if ( herr < 0 )
			return (herr_t)HANDLE_H5G_GET_OBJINFO_ERR ( member_name );

		H5Oclose( obj_id );
#else
		H5G_stat_t objinfo;
		herr = H5Gget_objinfo( group_id, member_name, 1, &objinfo );
		if ( herr < 0 )
			return (herr_t)HANDLE_H5G_GET_OBJINFO_ERR ( member_name );
#endif
		if ( objinfo.type != data->type )
			return 0; /* don't count, continue iteration */
	}

	if ( data->name && (data->stop_idx == data->count) )
	{
		memset ( data->name, 0, data->len );
		strncpy ( data->name, member_name, data->len-1 );
		
		return 1;	/* stop iteration */
	}

	/* count only if pattern is NULL or member name matches */
  	if ( !data->pattern ||
 		(strncmp (member_name, data->pattern, strlen(data->pattern)) == 0)
		) {
		data->count++;
	}

	return 0;		/* continue iteration */
}

/*!
  \ingroup h5part_kernel

  Iterator for \c H5Giterate().
*/
h5part_int64_t
_H5Part_get_num_objects (
	hid_t group_id,
	const char *group_name,
	const hid_t type
	) {

	return _H5Part_get_num_objects_matching_pattern (
		group_id,
		group_name,
		type,
		NULL );
}

/*!
  \ingroup h5part_kernel

  Iterator for \c H5Giterate().
*/
h5part_int64_t
_H5Part_get_num_objects_matching_pattern (
	hid_t group_id,
	const char *group_name,
	const hid_t type,
	char * const pattern
	) {

	h5part_int64_t herr;
	int idx = 0;
	struct _iter_op_data data;

	memset ( &data, 0, sizeof ( data ) );
	data.type = type;
	data.pattern = pattern;

#ifdef H5PART_HAVE_HDF5_18
	hid_t child_id = H5Gopen( group_id, group_name
#ifndef H5_USE_16_API
		, H5P_DEFAULT
#endif
		);

	if ( child_id < 0 ) return child_id;
 	herr = H5Literate( child_id, H5_INDEX_NAME, H5_ITER_INC, 0,
 			_H5Part_iteration_operator2, &data );
#else
	herr = H5Giterate ( group_id, group_name, &idx,
			_H5Part_iteration_operator, &data );
#endif
	if ( herr < 0 ) return herr;

#ifdef H5PART_HAVE_HDF5_18
	herr = H5Gclose ( child_id );
	if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;
#endif

	return data.count;
}

/*!
  \ingroup h5part_kernel

  \return	1 on success
  		0 for no entry
*/
h5part_int64_t
_H5Part_get_object_name (
	hid_t group_id,
	const char *group_name,
	const hid_t type,
	const h5part_int64_t idx,
	char *obj_name,
	const h5part_int64_t len_obj_name
	) {

	herr_t herr;
	struct _iter_op_data data;
	int iterator_idx = 0;

	memset ( &data, 0, sizeof ( data ) );
	data.stop_idx = (hid_t)idx;
	data.type = type;
	data.name = obj_name;
	data.len = (size_t)len_obj_name;

#ifdef H5PART_HAVE_HDF5_18
	hid_t child_id = H5Gopen ( group_id, group_name
#ifndef H5_USE_16_API
		, H5P_DEFAULT
#endif
		);
	if ( child_id < 0 ) return child_id;
 	herr = H5Literate ( child_id, H5_INDEX_NAME, H5_ITER_INC, 0,
 			_H5Part_iteration_operator2, &data );
#else
	herr = H5Giterate ( group_id, group_name, &iterator_idx,
			_H5Part_iteration_operator, &data );
#endif
	if ( herr < 0 ) {
		return HANDLE_H5L_ITERATE_ERR;
	}

#ifdef H5PART_HAVE_HDF5_18
	herr_t herr2 = H5Gclose ( child_id );
	if ( herr2 < 0 ) return HANDLE_H5G_CLOSE_ERR;
#endif

	if ( herr == 0 ) return 0;

	return 1;
}

/*!
  \ingroup h5part_model

  Query whether a particular \a step already exists in
  the file \a f.

  \return	0 or 1
*/
h5part_int64_t
H5PartHasStep (
	H5PartFile *f,		/*!< [in]  Handle to open file */
	h5part_int64_t step	/*!< [in]  Step number to query */
	) {
  
	SET_FNAME ( "H5PartHasStep" );

	CHECK_FILEHANDLE( f );

	char stepname[H5PART_STEPNAME_LEN];
	_H5Part_get_step_name(f, step, stepname);

	return _H5Part_have_group ( f->file, stepname );
}


/*!
  \ingroup h5part_model

  Get the number of time-steps that are currently stored in the file
  \c f.

  It works for both reading and writing of files, but is probably
  only typically used when you are reading.

  \return	number of time-steps or error code
*/
h5part_int64_t
H5PartGetNumSteps (
	H5PartFile *f			/*!< [in]  Handle to open file */
	) {

	SET_FNAME ( "H5PartGetNumSteps" );

	CHECK_FILEHANDLE( f );

	return _H5Part_get_num_objects_matching_pattern (
		f->file,
		"/",
		H5G_UNKNOWN,
		f->groupname_step );
}

/*!
  \ingroup h5part_model

  Get the number of datasets that are stored at the current time-step.

  \return	number of datasets in current timestep or error code
*/

h5part_int64_t
H5PartGetNumDatasets (
	H5PartFile *f			/*!< [in]  Handle to open file */
	) {

	SET_FNAME ( "H5PartGetNumDatasets" );

	CHECK_FILEHANDLE( f );
	
	char stepname[H5PART_STEPNAME_LEN];
	_H5Part_get_step_name(f, f->timestep, stepname);

	return _H5Part_get_num_objects ( f->file, stepname, H5G_DATASET );
}

/*!
  \ingroup h5part_model

  This reads the name of a dataset specified by it's index in the current
  time-step.

  If the number of datasets is \c n, the range of \c _index is \c 0 to \c n-1.

  \result	\c H5PART_SUCCESS
*/
h5part_int64_t
H5PartGetDatasetName (
	H5PartFile *f,			/*!< [in]  Handle to open file */
	const h5part_int64_t idx,	/*!< [in]  Index of the dataset */
	char *name,			/*!< [out] Name of dataset */
	const h5part_int64_t len_of_name/*!< [in]  Size of buffer \c name */
	) {

	SET_FNAME ( "H5PartGetDatasetName" );

	CHECK_FILEHANDLE ( f );
	CHECK_TIMEGROUP ( f );

	char stepname[H5PART_STEPNAME_LEN];
	_H5Part_get_step_name(f, f->timestep, stepname);

	h5part_int64_t herr = _H5Part_get_object_name (
		f->file,
		stepname,
		H5G_DATASET,
		idx,
		name,
		len_of_name );
	if ( herr == 0 ) HANDLE_H5PART_NOENTRY_ERR(
					stepname, H5G_DATASET, idx );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_model

  Gets the name, type and number of elements of a dataset based on its
  index in the current timestep.

  Type is one of the following macros:

  - \c H5PART_FLOAT64 (for \c h5part_float64_t)
  - \c H5PART_FLOAT32 (for \c h5part_float32_t)
  - \c H5PART_INT64 (for \c h5part_int64_t)
  - \c H5PART_INT32 (for \c h5part_int32_t)
  - \c H5PART_CHAR (for \c char)
  - \c H5PART_STRING (for \c char*)

  \return	\c H5PART_SUCCESS
*/
h5part_int64_t
H5PartGetDatasetInfo (
	H5PartFile *f,		/*!< [in]  Handle to open file */
	const h5part_int64_t idx,/*!< [in]  Index of the dataset */
	char *dataset_name,	/*!< [out] Name of dataset */
	const h5part_int64_t len_dataset_name,
				/*!< [in]  Size of buffer \c dataset_name */
	h5part_int64_t *type,	/*!< [out] Type of data in dataset */
	h5part_int64_t *nelem	/*!< [out] Number of elements. */
	) {

	SET_FNAME ( "H5PartGetDatasetInfo" );

	h5part_int64_t herr;
	hid_t dataset;
	hid_t space;
	hid_t h5type;

	CHECK_FILEHANDLE ( f );
	CHECK_TIMEGROUP ( f );

	char stepname[H5PART_STEPNAME_LEN];
	_H5Part_get_step_name(f, f->timestep, stepname);

	herr = _H5Part_get_object_name (
		f->file,
		stepname,
		H5G_DATASET,
		idx,
		dataset_name,
		len_dataset_name );
	if ( herr == 0 ) {
		return HANDLE_H5PART_NOENTRY_ERR( stepname, H5G_DATASET, idx );
	}
	else if ( herr < 0 ) return herr;

	dataset = H5Dopen( f->timegroup, dataset_name
#ifndef H5_USE_16_API
			, H5P_DEFAULT
#endif
			);
	if ( dataset < 0 ) return HANDLE_H5D_OPEN_ERR ( dataset_name );

	h5type = H5Dget_type ( dataset );
	if ( h5type < 0 ) return HANDLE_H5D_GET_TYPE_ERR;

	if ( type ) {
		*type = _H5Part_normalize_h5_type ( h5type );
		if ( *type < 0 ) return *type;
	}

	if ( nelem )
	{
		space = H5Dget_space ( dataset );
		if ( space < 0 ) return HANDLE_H5D_GET_SPACE_ERR;

		*nelem = H5Sget_simple_extent_npoints ( space );
		if ( *nelem < 0 )
			return HANDLE_H5S_GET_SIMPLE_EXTENT_NPOINTS_ERR;

		herr = H5Sclose ( space );
		if ( herr <  0 ) return HANDLE_H5S_CLOSE_ERR;
	}

	herr = H5Tclose ( h5type );
	if ( herr < 0 ) HANDLE_H5T_CLOSE_ERR;

	herr = H5Dclose ( dataset );
	if ( herr < 0 ) HANDLE_H5D_CLOSE_ERR;

	return H5PART_SUCCESS;
}

static h5part_int64_t
_H5Part_has_view (
	H5PartFile *f
	) {

	return  ( f->viewindexed || ( f->viewstart >= 0 && f->viewend >= 0 ));
}

h5part_int64_t
_H5Part_get_num_particles (
	H5PartFile *f			/*!< [in]  Handle to open file */
	) {

	h5part_int64_t herr;
	h5part_int64_t nparticles;
	char dataset_name[H5PART_DATANAME_LEN];
	char stepname[H5PART_STEPNAME_LEN];
	_H5Part_get_step_name(f, f->timestep, stepname);

	herr = _H5Part_get_object_name (
		f->file,
		stepname,
		H5G_DATASET,
		0,
		dataset_name, H5PART_DATANAME_LEN );
	if ( herr < 0 ) return herr;

	/* if a view exists, use its size as the number of particles */
	if ( _H5Part_has_view ( f ) )
	{
		nparticles = H5Sget_select_npoints ( f->diskshape );
		if ( nparticles < 0 ) return HANDLE_H5S_GET_SELECT_NPOINTS_ERR;

		_H5Part_print_debug (
			"Found %lld points with H5Sget_select_npoints",
			(long long)nparticles );

#if 0 // this does not work for indices
		/* double check that the size of the diskshape agrees with
		 * the size of the view */
		if ( nparticles != f->viewend - f->viewstart + 1 ) {
			_H5Part_print_warn (
				"Number of particles (%lld) does not agree "
				"with view range.", (long long)nparticles );
			return HANDLE_H5PART_BAD_VIEW_ERR (
					f->viewstart, f->viewend);
		}
#endif
	}
	/* herr is 0 if there are no datasets on disk */
	else if ( herr == 0 )
	{
		/* try to recover number of particles from a previous
		 * H5PartSetNumParticles call. */
#ifdef PARALLEL_IO
		nparticles = 0;
		int i;
		for (i=0; i < f->nprocs; i++) {
			nparticles += f->pnparticles[i];
		}
#else
		nparticles = f->nparticles;
#endif

		if ( nparticles > 0 ) {
			_H5Part_print_debug (
				"Using existing view to report "
				"nparticles = %lld", (long long)nparticles );
			return nparticles;
		}
		else {
			_H5Part_print_warn ( 
				"There are no datasets in timestep %s "
				"or existing views: "
				"reporting 0 particles.", stepname );
			return 0;
		}
	}
	/* otherwise, report all particles on disk in the first dataset
	 * for this timestep */
	else
	{
		hid_t space_id;
		hid_t dataset_id;

		dataset_id = H5Dopen ( f->timegroup, dataset_name
#ifndef H5_USE_16_API
				, H5P_DEFAULT
#endif
				 );

		if ( dataset_id < 0 ) 
			return HANDLE_H5D_OPEN_ERR ( dataset_name );

		space_id = H5Dget_space ( dataset_id );
		if ( space_id < 0 ) return HANDLE_H5D_GET_SPACE_ERR;

		nparticles = H5Sget_simple_extent_npoints ( space_id );
		if ( nparticles < 0 )
			return HANDLE_H5S_GET_SIMPLE_EXTENT_NPOINTS_ERR;

		herr = H5Sclose ( space_id );
		if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;

		herr = H5Dclose ( dataset_id );
		if ( herr < 0 ) return HANDLE_H5D_CLOSE_ERR;
	}

	return nparticles;
}

/*!
  \ingroup h5part_model

  This function returns the number of particles in the first dataset of
  the current timestep (or in the first timestep if none has been set).

  If you have neither set the number of particles (read or write)
  nor set a view (read-only), then this returns the total number of
  elements on disk of the first dataset if is exists. Otherwise,
  it returns 0.

  If you have set a view, this return the number of particles
  in the view.

  \return	number of particles in current timestep or an error
		code.
 */
h5part_int64_t
H5PartGetNumParticles (
	H5PartFile *f			/*!< [in]  Handle to open file */
	) {

	SET_FNAME ( "H5PartGetNumParticles" );

	CHECK_FILEHANDLE( f );

	if ( f->timegroup < 0 ) {
		h5part_int64_t herr = _H5Part_set_step ( f, 0 );
		if ( herr < 0 ) return herr;
	}

	return _H5Part_get_num_particles ( f );
}

static h5part_int64_t
_reset_view (
 	H5PartFile *f			/*!< [in]  Handle to open file */
	) {

	herr_t herr = 0;

	f->viewstart = -1;
	f->viewend = -1;
	f->viewindexed = 0;

	if ( f->diskshape != H5S_ALL ) {
		herr = H5Sclose ( f->diskshape );
		if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
		f->diskshape = H5S_ALL;
	}

	if ( f->memshape != H5S_ALL ){
		herr = H5Sclose ( f->memshape );
		if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
		f->memshape = H5S_ALL;
	}

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_model
*/
h5part_int64_t
H5PartResetView (
 	H5PartFile *f			/*!< [in]  Handle to open file */
	) {
	SET_FNAME ( "H5PartResetView" );

	CHECK_FILEHANDLE( f );
	CHECK_READONLY_MODE ( f );

	return _reset_view ( f );
}

/*!
  \ingroup h5part_model
*/
h5part_int64_t
H5PartHasView (
 	H5PartFile *f			/*!< [in]  Handle to open file */
	) {
	SET_FNAME ( "H5PartHasView" );

	CHECK_FILEHANDLE ( f );
	CHECK_READONLY_MODE ( f );

	return _H5Part_has_view ( f );
}

static h5part_int64_t
_set_view (
	H5PartFile *f,			/*!< [in]  Handle to open file */
	h5part_int64_t start,		/*!< [in]  Start particle */
	h5part_int64_t end		/*!< [in]  End particle */
	) {

	h5part_int64_t herr = 0;
	hsize_t total;
	hsize_t stride = 1;
	hsize_t hstart;
	hsize_t dmax = H5S_UNLIMITED;

	_H5Part_print_debug (
		"Set view (%lld,%lld).",
		(long long)start,(long long)end);

	herr = _reset_view ( f );
	if ( herr < 0 ) return herr;

	if ( start == -1 && end == -1 ) return H5PART_SUCCESS;

	/*
	  View has been reset so H5PartGetNumParticles will tell
	  us the total number of particles.

	  For now, we interpret start=-1 to mean 0 and 
	  end==-1 to mean end of file
	*/
	total = (hsize_t) _H5Part_get_num_particles ( f );
	if ( total == 0 ) {
		/* No datasets have been created yet and no veiws are set.
		 * We have to leave the view empty because we don't know how
		 * many particles there should be! */
		return H5PART_SUCCESS;
	}

	if ( start == -1 ) start = 0;
	if ( end == -1 )   end = total - 1; // range is *inclusive*

	/* so, is this selection inclusive or exclusive? 
	   it appears to be inclusive for both ends of the range.
	*/
	if ( end < start ) {
		_H5Part_print_warn (
			"Nonfatal error. "
			"End of view (%lld) is less than start (%lld).",
			(long long)end, (long long)start );
		end = start; /* ensure that we don't have a range error */
	}
	/* setting up the new view */
	f->viewstart =  start;
	f->viewend =    end;
	f->nparticles = end - start + 1;
	
	_H5Part_print_debug ( "nparticles=%lld", (long long)f->nparticles );

	/* declare overall data size but then select a subset */
	f->diskshape = H5Screate_simple ( 1, &total, NULL );
	if ( f->diskshape < 0 )
		return HANDLE_H5S_CREATE_SIMPLE_ERR ( total );

	total = (hsize_t)f->nparticles;
	hstart = (size_t)start;
	herr = H5Sselect_hyperslab ( 
		f->diskshape,
		H5S_SELECT_SET,
		&hstart,
		&stride,
		&total,
		NULL );
	if ( herr < 0 ) return HANDLE_H5S_SELECT_HYPERSLAB_ERR;

	/* declare local memory datasize */
	f->memshape = H5Screate_simple ( 1, &total, &dmax );
	if ( f->memshape < 0 )
		return HANDLE_H5S_CREATE_SIMPLE_ERR ( f->nparticles );

	return H5PART_SUCCESS;
}

static h5part_int64_t
_set_view_indices (
	H5PartFile *f,			/*!< [in]  Handle to open file */
	const h5part_int64_t *indices,	/*!< [in]  List of indices */
	h5part_int64_t nelems		/*!< [in]  Size of list */
	) {

	h5part_int64_t herr = 0;
	hsize_t total;
	hsize_t dmax = H5S_UNLIMITED;

	herr = _reset_view ( f );
	if ( herr < 0 ) return herr;

	if ( indices == NULL || nelems < 0 ) {
		_H5Part_print_warn (
			"View indices array is null or size is < 0: reseting view." );
		return H5PART_SUCCESS;
	}

	/*
	  View has been reset so H5PartGetNumParticles will tell
	  us the total number of particles.

	  For now, we interpret start=-1 to mean 0 and 
	  end==-1 to mean end of file
	*/
	total = (hsize_t) _H5Part_get_num_particles ( f );
	if ( total == 0 ) {
		/* No datasets have been created yet and no veiws are set.
		 * We have to leave the view empty because we don't know how
		 * many particles there should be! */
		return H5PART_SUCCESS;
	}

	_H5Part_print_debug ( "Total nparticles=%lld", (long long)total );

	if ( total == 0 ) return H5PART_SUCCESS;

	f->nparticles = (hsize_t) nelems;

	/* declare overall data size  but then will select a subset */
	f->diskshape = H5Screate_simple ( 1, &total, NULL );
	if ( f->diskshape < 0 )
		return HANDLE_H5S_CREATE_SIMPLE_ERR ( total );

	/* declare local memory datasize */
	total = (size_t)f->nparticles;
	f->memshape = H5Screate_simple (1, &total, &dmax );
	if ( f->memshape < 0 )
		return HANDLE_H5S_CREATE_SIMPLE_ERR ( f->nparticles );

	if ( nelems > 0 ) {
		herr = H5Sselect_elements ( 
			f->diskshape,
			H5S_SELECT_SET,
			nelems,
			(hsize_t*)indices );
	} else {
		herr = H5Sselect_none ( f->diskshape );
	}
	if ( herr < 0 ) return HANDLE_H5S_SELECT_ELEMENTS_ERR;

	f->viewindexed = 1;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_model

  For parallel I/O or for subsetting operations on the datafile, the
  \c H5PartSetView() function allows you to define a subset of the total
  particle dataset to operate on.
  The concept of "view" works for both serial
  and for parallel I/O.  The "view" will remain in effect until a new view
  is set, or the number of particles in a dataset changes, or the view is
  "unset" by calling \c H5PartSetView(file,-1,-1);

  Before you set a view, the \c H5PartGetNumParticles() will return the
  total number of particles in the current time-step (even for the parallel
  reads).  However, after you set a view, it will return the number of
  particles contained in the view.

  The range is \e inclusive: the end value is the last index of the
  data.

  \return	\c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartSetView (
	H5PartFile *f,			/*!< [in]  Handle to open file */
	const h5part_int64_t start,	/*!< [in]  Start particle */
	const h5part_int64_t end	/*!< [in]  End particle */
	) {

	SET_FNAME ( "H5PartSetView" );

	CHECK_FILEHANDLE( f );

	if ( f->timegroup < 0 ) {
		h5part_int64_t herr = _H5Part_set_step ( f, 0 );
		if ( herr < 0 ) return herr;
	}

	return _set_view ( f, start, end );
}

/*!
  \ingroup h5part_model

  For parallel I/O or for subsetting operations on the datafile,
  this function allows you to define a subset of the total
  dataset to operate on by specifying a list of indices.
  The concept of "view" works for both serial
  and for parallel I/O.  The "view" will remain in effect until a new view
  is set, or the number of particles in a dataset changes, or the view is
  "unset" by calling \c H5PartSetViewIndices(NULL,0);

  Before you set a view, the \c H5PartGetNumParticles() will return the
  total number of particles in the current time-step (even for the parallel
  reads).  However, after you set a view, it will return the number of
  particles contained in the view.

  \return	\c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartSetViewIndices (
	H5PartFile *f,			/*!< [in]  Handle to open file */
	const h5part_int64_t *indices,	/*!< [in]  List of indices */
	h5part_int64_t nelems		/*!< [in]  Size of list */
	) {

	SET_FNAME ( "H5PartSetViewIndices" );

	CHECK_FILEHANDLE( f );

	if ( f->timegroup < 0 ) {
		h5part_int64_t herr = _H5Part_set_step ( f, 0 );
		if ( herr < 0 ) return herr;
	}

	return _set_view_indices ( f, indices, nelems );
}

/*!
  \ingroup h5part_model

   Allows you to query the current view. Start and End
   will be \c -1 if there is no current view established.
   Use \c H5PartHasView() to see if the view is smaller than the
   total dataset.

   \return	number of elements in the view or error code
*/
h5part_int64_t
H5PartGetView (
	H5PartFile *f,			/*!< [in]  Handle to open file */
	h5part_int64_t *start,		/*!< [out]  Start particle */
	h5part_int64_t *end		/*!< [out]  End particle */
	) {

	SET_FNAME ( "H5PartGetView" );

	CHECK_FILEHANDLE( f );

	if ( f->viewindexed ) {
		_H5Part_print_error (
			"The current view has an index selection, but "
			"this function only works for ranged views." );
		return H5PART_ERR_INVAL;
	}

	if ( f->timegroup < 0 ) {
		h5part_int64_t herr = _H5Part_set_step ( f, 0 );
		if ( herr < 0 ) return herr;
	}

	h5part_int64_t viewstart = 0;
	h5part_int64_t viewend = 0;

	if ( f->viewstart >= 0 )
		viewstart = f->viewstart;

	if ( f->viewend >= 0 ) {
		viewend = f->viewend;
	}
	else {
		viewend = _H5Part_get_num_particles ( f );
		if ( viewend < 0 )
			return HANDLE_H5PART_GET_NUM_PARTICLES_ERR ( viewend );
	}

	if ( start ) *start = viewstart;
	if ( end )   *end   = viewend;

	return viewend - viewstart + 1; // view range is *inclusive*
}

/*!
  \ingroup h5part_model

  If it is too tedious to manually set the start and end coordinates
  for a view, the \c H5SetCanonicalView() will automatically select an
  appropriate domain decomposition of the data arrays for the degree
  of parallelism and set the "view" accordingly.

  \return		H5PART_SUCCESS or error code
*/
/*
  \note
  There is a bug in this function:
  If (NumParticles % f->nprocs) != 0  then
  the last  (NumParticles % f->nprocs) particles are not handled!
*/

h5part_int64_t
H5PartSetCanonicalView (
	H5PartFile *f			/*!< [in]  Handle to open file */
	) {

	SET_FNAME ( "H5PartSetCanonicalView" );

	h5part_int64_t herr;

	CHECK_FILEHANDLE( f );

	herr = _reset_view ( f );
	if ( herr < 0 ) return HANDLE_H5PART_SET_VIEW_ERR( herr, -1, -1 );

#ifdef PARALLEL_IO
	h5part_int64_t start = 0;
	h5part_int64_t end = 0;
	h5part_int64_t total = 0;
	h5part_int64_t pertask = 0;
	int i = 0;
	
	if ( f->timegroup < 0 ) {
		herr = _H5Part_set_step ( f, 0 );
		if ( herr < 0 ) return herr;
	}

	/* returns all particles (aggregated across all tasks) */
	total = _H5Part_get_num_particles ( f );
	if ( total < 0 )
		return HANDLE_H5PART_GET_NUM_PARTICLES_ERR ( total );

#if 0
	/* now lets query the attributes for this group to see if there
	   is a 'pnparticles' group that contains the offsets for the
	   processors.
	*/
	if ( _H5Part_read_attrib (
		     f->timegroup,
		     "pnparticles", f->pnparticles ) < 0) {
		/*
		  Attribute "pnparticles" is not available.  So
		  subdivide the view into NP mostly equal pieces
		*/
		n /= f->nprocs;
		for ( i=0; i<f->nprocs; i++ ) {
			f->pnparticles[i] = n;
		}
	}
#endif

	total /= f->nprocs;
	for ( i=0; i<f->nprocs; i++ ) {
		f->pnparticles[i] = total;
	}

	for ( i = 0; i < f->myproc; i++ ){
		start += f->pnparticles[i];
	}
	end = start + f->pnparticles[f->myproc] - 1;
	herr = _set_view ( f, start, end );
	if ( herr < 0 ) return HANDLE_H5PART_SET_VIEW_ERR ( herr, start, end );

#endif

	return H5PART_SUCCESS;
}

static h5part_int64_t
_read_data (
	H5PartFile *f,		/*!< [in] Handle to open file */
	const char *name,	/*!< [in] Name to associate dataset with */
	void *array,		/*!< [out] Array of data */
	const hid_t type
	) {

	h5part_int64_t herr;
	hssize_t ndisk, nread, nmem;
	hid_t dataset_id;
	hid_t space_id;
	hid_t memspace_id;

	if ( f->timegroup < 0 ) {
		herr = _H5Part_set_step ( f, f->timestep );
		if ( herr < 0 ) return herr;
	}

	char name2[H5PART_DATANAME_LEN];
	_normalize_dataset_name ( name, name2 );

	dataset_id = H5Dopen ( f->timegroup, name2
#ifndef H5_USE_16_API
				, H5P_DEFAULT
#endif
				);

	if ( dataset_id < 0 ) return HANDLE_H5D_OPEN_ERR ( name2 );

	/* default spaces, if not using a view selection */
	memspace_id = H5S_ALL;
	space_id = H5Dget_space ( dataset_id );
	if ( space_id < 0 ) return HANDLE_H5D_GET_SPACE_ERR;

	/* get the number of elements on disk for the datset */
	ndisk = H5Sget_simple_extent_npoints ( space_id );
	if ( ndisk < 0 ) return HANDLE_H5S_GET_SIMPLE_EXTENT_NPOINTS_ERR;

	if ( f->diskshape != H5S_ALL )
	{
		nread = H5Sget_select_npoints ( f->diskshape );
		if ( nread < 0 ) return HANDLE_H5S_GET_SELECT_NPOINTS_ERR;

		/* make sure the disk space selected by the view doesn't
		 * exceed the size of the dataset */
		if ( nread <= ndisk ) {
			/* we no longer need the dataset space... */
			herr = H5Sclose ( space_id );
			if ( herr < 0 ) HANDLE_H5S_CLOSE_ERR;
			/* ...because it's safe to use the view selection */
			space_id = f->diskshape;
		} else {
			/* the view selection is too big?
			 * fall back to using the dataset space */
			_H5Part_print_warn (
				"Ignoring view: dataset[%s] has fewer "
				"elements on disk (%lld) than are selected "
				"(%lld).",
				name2, (long long)ndisk, (long long)nread );
			nread = ndisk;
		}
	}
	else {
		/* since the view selection is H5S_ALL, we will
		 * read all available elements in the dataset space */
		nread = ndisk;
	}

	if ( f->memshape != H5S_ALL )
	{
		nmem = H5Sget_simple_extent_npoints ( f->memshape );
		if ( nmem <  0 ) return HANDLE_H5S_GET_SELECT_NPOINTS_ERR;

		/* make sure the memory space selected by the view has
		 * enough capacity for the read */
		if ( nmem >= nread ) {
			memspace_id = f->memshape;
		} else {
			/* the view selection is too small?
			 * fall back to using H5S_ALL */
			_H5Part_print_warn (
				"Ignoring view: dataset[%s] has more "
				"elements selected (%lld) than are available "
				"in memory (%lld).",
				name2, (long long)nread, (long long)nmem );
		}
	}

#ifdef PARALLEL_IO
	herr = _H5Part_start_throttle ( f );
	if ( herr < 0 ) return herr;
#endif

	herr = H5Dread (
		dataset_id,
		type,
		memspace_id,		/* shape/size of data in memory (the
					   complement to disk hyperslab) */
		space_id,		/* shape/size of data on disk 
					   (get hyperslab if needed) */
		f->xfer_prop,		/* ignore... its for parallel reads */
		array );
	if ( herr < 0 ) return HANDLE_H5D_READ_ERR ( name2, f->timestep );

#ifdef PARALLEL_IO
	herr = _H5Part_end_throttle ( f );
	if ( herr < 0 ) return herr;
#endif

	if ( space_id != f->diskshape ) {
		herr = H5Sclose ( space_id );
		if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
	}

	herr = H5Dclose ( dataset_id );
	if ( herr < 0 ) return HANDLE_H5D_CLOSE_ERR;
	
	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_data

  Read array of 64 bit floating point data from file.

  When retrieving datasets from disk, you ask for them
  by name. There are no restrictions on naming of arrays,
  but it is useful to arrive at some common naming
  convention when sharing data with other groups.

  \return	\c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartReadDataFloat64 (
	H5PartFile *f,		/*!< [in] Handle to open file */
	const char *name,	/*!< [in] Name to associate dataset with */
	h5part_float64_t *array	/*!< [out] Array of data */
	) {

	SET_FNAME ( "H5PartReadDataFloat64" );

	h5part_int64_t herr;

	CHECK_FILEHANDLE( f );

	herr = _read_data ( f, name, array, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_data

  Read array of 32 bit floating point data from file.

  When retrieving datasets from disk, you ask for them
  by name. There are no restrictions on naming of arrays,
  but it is useful to arrive at some common naming
  convention when sharing data with other groups.

  \return	\c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartReadDataFloat32 (
	H5PartFile *f,		/*!< [in] Handle to open file */
	const char *name,	/*!< [in] Name to associate dataset with */
	h5part_float32_t *array	/*!< [out] Array of data */
	) {

	SET_FNAME ( "H5PartReadDataFloat32" );

	h5part_int64_t herr;

	CHECK_FILEHANDLE( f );

	herr = _read_data ( f, name, array, H5T_NATIVE_FLOAT );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_data

  Read array of 64 bit integer data from file.

  When retrieving datasets from disk, you ask for them
  by name. There are no restrictions on naming of arrays,
  but it is useful to arrive at some common naming
  convention when sharing data with other groups.

  \return	\c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartReadDataInt64 (
	H5PartFile *f,		/*!< [in] Handle to open file */
	const char *name,	/*!< [in] Name to associate dataset with */
	h5part_int64_t *array	/*!< [out] Array of data */
	) {

	SET_FNAME ( "H5PartReadDataInt64" );

	h5part_int64_t herr;

	CHECK_FILEHANDLE( f );

	herr = _read_data ( f, name, array, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_data

  Read array of 32 bit integer data from file.

  When retrieving datasets from disk, you ask for them
  by name. There are no restrictions on naming of arrays,
  but it is useful to arrive at some common naming
  convention when sharing data with other groups.

  \return	\c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartReadDataInt32 (
	H5PartFile *f,		/*!< [in] Handle to open file */
	const char *name,	/*!< [in] Name to associate dataset with */
	h5part_int32_t *array	/*!< [out] Array of data */
	) {

	SET_FNAME ( "H5PartReadDataInt64" );

	h5part_int64_t herr;

	CHECK_FILEHANDLE( f );

	herr = _read_data ( f, name, array, H5T_NATIVE_INT32 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_data

  This is an aggregate read function that pulls in all of the data for a
  typical particle timestep in one shot.
  It also takes the timestep as an argument
  and will call \ref H5PartSetStep internally.

  \return	\c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartReadParticleStep (
	H5PartFile *f,		/*!< [in]  Handle to open file */
	h5part_int64_t step,	/*!< [in]  Step to read */
	h5part_float64_t *x,	/*!< [out] Buffer for dataset named "x" */
	h5part_float64_t *y,	/*!< [out] Buffer for dataset named "y" */
	h5part_float64_t *z,	/*!< [out] Buffer for dataset named "z" */
	h5part_float64_t *px,	/*!< [out] Buffer for dataset named "px" */
	h5part_float64_t *py,	/*!< [out] Buffer for dataset named "py" */
	h5part_float64_t *pz,	/*!< [out] Buffer for dataset named "pz" */
	h5part_int64_t *id	/*!< [out] Buffer for dataset named "id" */
	) {

	SET_FNAME ( "H5PartReadParticleStep" );
	h5part_int64_t herr;

	CHECK_FILEHANDLE( f );

	herr = _H5Part_set_step ( f, step );
	if ( herr < 0 ) return herr;

	herr = _read_data ( f, "x", (void*)x, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	herr = _read_data ( f, "y", (void*)y, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	herr = _read_data ( f, "z", (void*)z, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	herr = _read_data ( f, "px", (void*)px, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	herr = _read_data ( f, "py", (void*)py, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	herr = _read_data ( f, "pz", (void*)pz, H5T_NATIVE_DOUBLE );
	if ( herr < 0 ) return herr;

	herr = _read_data ( f, "id", (void*)id, H5T_NATIVE_INT64 );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/************ error handling and configuration ************/

/*!
  \ingroup h5part_errhandle

  Set the `throttle` factor, which causes HDF5 write and read
  calls to be issued in that number of batches.

  This can prevent large cuncurrency parallel applications that
  use independent writes from overwhelming the underlying
  parallel file system.

  Throttling only works with the H5PART_VFD_MPIPOSIX or
  H5PART_VFD_MPIIO_IND drivers and is only available in
  the parallel library.

  \return \c H5PART_SUCCESS
*/
#ifdef PARALLEL_IO
h5part_int64_t
H5PartSetThrottle (
	H5PartFile *f,
	int factor
	) {

	SET_FNAME( "H5PartSetThrottle" );
	CHECK_FILEHANDLE ( f );

	if ( f->flags & H5PART_VFD_MPIIO_IND || f->flags & H5PART_VFD_MPIPOSIX ) {
		f->throttle = factor;
		_H5Part_print_info (
			"Throttling set with factor = %d", f->throttle );
	} else {
		_H5Part_print_warn (
			"Throttling is only permitted with the MPI-POSIX "
			"or MPI-IO Independent VFD." );
	}

	return H5PART_SUCCESS;
}

h5part_int64_t
_H5Part_start_throttle (
	H5PartFile *f
	) {

	if (f->throttle > 0) {
		int ret;
		int token = 1;
		_H5Part_print_info (
			"Throttling with factor = %d",
			f->throttle);
		if (f->myproc / f->throttle > 0) {
			_H5Part_print_debug_detail (
				"[%d] throttle: waiting on token from %d",
				f->myproc, f->myproc - f->throttle);
			// wait to receive token before continuing with read
			ret = MPI_Recv(
				&token, 1, MPI_INT,
				f->myproc - f->throttle, // receive from previous proc
				f->myproc, // use this proc id as message tag
				f->comm,
				MPI_STATUS_IGNORE
				);
			if ( ret != MPI_SUCCESS ) return HANDLE_MPI_SENDRECV_ERR;
		}
		_H5Part_print_debug_detail (
			"[%d] throttle: received token",
			f->myproc);
	}
	return H5PART_SUCCESS;
}

h5part_int64_t
_H5Part_end_throttle (
	H5PartFile *f
	) {

	if (f->throttle > 0) {
		int ret;
		int token;
		if (f->myproc + f->throttle < f->nprocs) {
			// pass token to next proc 
			_H5Part_print_debug_detail (
				"[%d] throttle: passing token to %d",
				f->myproc, f->myproc + f->throttle);
			ret = MPI_Send(
				&token, 1, MPI_INT,
				f->myproc + f->throttle, // send to next proc
				f->myproc + f->throttle, // use the id of the target as tag
				f->comm
				);
			if ( ret != MPI_SUCCESS ) return HANDLE_MPI_SENDRECV_ERR;
		}
	}
	return H5PART_SUCCESS;
}
#endif // PARALLEL_IO

/*!
  \ingroup h5part_open

  Set verbosity level to \c level.

  \return \c H5PART_SUCCESS
*/
h5part_int64_t
H5PartSetVerbosityLevel (
	h5part_int64_t level
	) {

	_debug = (unsigned int)level;
	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_errhandle

  Set error handler to \c handler.

  \return \c H5PART_SUCCESS
*/
h5part_int64_t
H5PartSetErrorHandler (
	h5part_error_handler handler
	) {
	_err_handler = handler;
	return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_errhandle

  Get current error handler.

  \return Pointer to error handler.
*/
h5part_error_handler
H5PartGetErrorHandler (
	void
	) {
	return _err_handler;
}

/*!
  \ingroup h5part_errhandle

  Get last error code.

  \return error code
*/
h5part_int64_t
H5PartGetErrno (
	void
	) {
	return _h5part_errno;
}

/*!
  \ingroup h5part_errhandle

  This is the H5Part default error handler.  If an error occures, an
  error message will be printed and an error number will be returned.

  \return value given in \c eno
*/
h5part_int64_t
H5PartReportErrorHandler (
	const char *funcname,
	const h5part_int64_t eno,
	const char *fmt,
	...
	) {

	_h5part_errno = eno;
	if ( _debug > 0 && _is_root_proc ) {
		va_list ap;
		va_start ( ap, fmt );
		_vprint ( stderr, "E", fmt, ap );
		va_end ( ap );
	}
	return _h5part_errno;
}

/*!
  \ingroup h5part_errhandle

  If an error occures, an error message will be printed and the
  program exists with the error code given in \c eno.
*/
h5part_int64_t
H5PartAbortErrorHandler (
	const char *funcname,
	const h5part_int64_t eno,
	const char *fmt,
	...
	) {

	_h5part_errno = eno;
	if ( _debug > 0 && _is_root_proc ) {
		va_list ap;
		va_start ( ap, fmt );
		fprintf ( stderr, "%s: ", funcname );
		vfprintf ( stderr, fmt, ap );
		fprintf ( stderr, "\n" );
	}
	exit ( (int)_h5part_errno );
}

/*!
  Initialize H5Part
*/
static h5part_int64_t
_init ( void ) {
	static int __init = 0;

	herr_t r5;
	if ( ! __init ) {
		_H5Part_set_funcname ( "NONE" );
#ifndef H5_USE_16_API
		r5 = H5Eset_auto ( H5E_DEFAULT, _h5_error_handler, NULL );
#else
		r5 = H5Eset_auto ( _h5_error_handler, NULL );
#endif
		if ( r5 < 0 ) return H5PART_ERR_INIT;
	}
	__init = 1;
	return H5PART_SUCCESS;
}
/*! @} */

static herr_t
#ifndef H5_USE_16_API
_h5_error_handler ( hid_t estack, void* unused ) {
#else
_h5_error_handler ( void* unused ) {
#endif
	
	if ( _debug >= 1 ) {
#ifndef H5_USE_16_API
		H5Eprint (H5E_DEFAULT,stderr);
#else
		H5Eprint (stderr);
#endif
	}
	return 0;
}

static void
_vprint (
	FILE* f,
	const char *prefix,
	const char *fmt,
	va_list ap
	) {
	char *fmt2 = (char*)malloc( strlen ( prefix ) +strlen ( fmt ) + strlen ( __funcname ) + 16 );
	if ( fmt2 == NULL ) return;
	sprintf ( fmt2, "%s: %s: %s\n", prefix, __funcname, fmt ); 
	vfprintf ( stderr, fmt2, ap );
	free ( fmt2 );
}

void
_H5Part_print_error (
	const char *fmt,
	...
	) {

	if ( _debug < 1 || !_is_root_proc) return;
	va_list ap;
	va_start ( ap, fmt );
	_vprint ( stderr, "E", fmt, ap );
	va_end ( ap );
}

void
_H5Part_print_warn (
	const char *fmt,
	...
	) {

	if ( _debug < 2 || !_is_root_proc ) return;
	va_list ap;
	va_start ( ap, fmt );
	_vprint ( stderr, "W", fmt, ap );
	va_end ( ap );
}

void
_H5Part_print_info (
	const char *fmt,
	...
	) {

	if ( _debug < 3 || !_is_root_proc ) return;
	va_list ap;
	va_start ( ap, fmt );
	_vprint ( stdout, "I", fmt, ap );
	va_end ( ap );
}

void
_H5Part_print_debug (
	const char *fmt,
	...
	) {

	if ( _debug < 4 || !_is_root_proc ) return;
	va_list ap;
	va_start ( ap, fmt );
	_vprint ( stdout, "D", fmt, ap );
	va_end ( ap );
}

void
_H5Part_print_debug_detail (
	const char *fmt,
	...
	) {

	if ( _debug < 5 ) return;
	va_list ap;
	va_start ( ap, fmt );
	_vprint ( stdout, "DD", fmt, ap );
	va_end ( ap );
}

void
_H5Part_set_funcname (
	char *fname
	) {
	__funcname = fname;
	_H5Part_print_debug ("(entered function)");
}

char*
_H5Part_get_funcname (
	void
	) {
	return __funcname;
}

