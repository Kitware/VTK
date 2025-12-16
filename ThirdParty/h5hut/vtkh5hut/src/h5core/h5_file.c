/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "h5core/h5_log.h"

#include "private/h5_file.h"
#include "private/h5_hdf5.h"

#include "private/h5_model.h"
#include "private/h5_mpi.h"
#include "private/h5u_io.h"
#include "private/h5b_io.h"

#include "h5core/h5_err.h"
#include "h5core/h5_syscall.h"

/*!
  \ingroup h5_core
  \defgroup h5_core_filehandling
*/

/*!
  \ingroup h5_core_filehandling

  Check whether \c f points to a valid file handle.

  \return	H5_SUCCESS or error code
*/
h5_err_t
h5_check_filehandle (
	const h5_file_t f_               /*!< filehandle  to check validity of */
	) {
        h5_file_p f = (h5_file_p)f_;
	return is_valid_file_handle (f) ? H5_SUCCESS : H5_ERR;
}

hid_t
h5_get_hdf5_file(
	const h5_file_t f_		/*!< file handle		*/
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (hid_t, "f=%p", f);
	check_file_handle_is_valid (f);
	H5_RETURN (f->file);
}

/*!
  Initialize H5hut
*/
static herr_t
hdf5_error_handler (
	hid_t estack_id,
	void*  __f
	) {
	UNUSED_ARGUMENT (__f);
	if (h5_get_loglevel() >= 4) {
		H5Eprint (estack_id, stderr);
	}
	return 0;
}

static inline h5_err_t
mpi_init (
	const h5_file_p f
	) {
	H5_INLINE_FUNC_ENTER (h5_err_t);
#ifdef H5_HAVE_PARALLEL
	TRY (h5priv_mpi_comm_size (f->props->comm, &f->nprocs));
	TRY (h5priv_mpi_comm_rank (f->props->comm, &f->myproc));

	/* xfer_prop:  also used for parallel I/O, during actual writes
	   rather than the access_prop which is for file creation. */
	TRY (f->props->xfer_prop = hdf5_create_property(H5P_DATASET_XFER));
	TRY (f->props->access_prop = hdf5_create_property(H5P_FILE_ACCESS));

	/* select the HDF5 VFD */
#if H5_VERSION_LE(1,8,12)
	if ((f->props->flags & H5_VFD_MPIO_POSIX)) {
		h5_info("Selecting MPI-POSIX VFD");
		hbool_t use_gpfs = 0; // TODO autodetect GPFS?
		TRY (hdf5_set_fapl_mpiposix_property (f->props->access_prop,
                                                     f->props->comm, use_gpfs));

        } else if ((f->props->flags & H5_VFD_CORE_IO)) {
		h5_info("Selecting CORE VFD");
                TRY (hdf5_set_fapl_core (f->props->access_prop,
                                         f->props->align, 1));
        } else if ((f->props->flags & H5_VFD_MPIO_INDEPENDENT)){
                h5_info("Selecting MPI-IO VFD, using independent mode");
		TRY (hdf5_set_fapl_mpio_property (f->props->access_prop,
                                                  f->props->comm, MPI_INFO_NULL));
                TRY (hdf5_set_dxpl_mpio_property (f->props->xfer_prop,
                                                  H5FD_MPIO_INDEPENDENT) );
        } else {
                // default is MPI-IO collective mode
		h5_info("Selecting MPI-IO VFD, using collective mode");
		TRY (hdf5_set_fapl_mpio_property (f->props->access_prop,
                                                  f->props->comm, MPI_INFO_NULL));
                TRY (hdf5_set_dxpl_mpio_property (f->props->xfer_prop,
                                                  H5FD_MPIO_COLLECTIVE) );
	}
#else
	// VFD_MPIO_POSIX has been removed in HDF5 1.8.13
        if ((f->props->flags & H5_VFD_CORE_IO)) {
		h5_info("Selecting CORE VFD");
                TRY (hdf5_set_fapl_core (f->props->access_prop,
                                         f->props->align, 1));
        } else if ((f->props->flags & H5_VFD_MPIO_INDEPENDENT)){
                h5_info("Selecting MPI-IO VFD, using independent mode");
		TRY (hdf5_set_fapl_mpio_property (f->props->access_prop,
                                                  f->props->comm, MPI_INFO_NULL));
                TRY (hdf5_set_dxpl_mpio_property (f->props->xfer_prop,
                                                  H5FD_MPIO_INDEPENDENT) );
        } else {
                // default is MPI-IO collective mode
		h5_info("Selecting MPI-IO VFD, using collective mode");
		TRY (hdf5_set_fapl_mpio_property (f->props->access_prop,
                                                  f->props->comm, MPI_INFO_NULL));
                TRY (hdf5_set_dxpl_mpio_property (f->props->xfer_prop,
                                                  H5FD_MPIO_COLLECTIVE) );
	}
#endif
#ifdef H5_USE_LUSTRE
	if (f->flags & H5_FS_LUSTRE) {
		TRY (h5_optimize_for_lustre(f, filename));
	}
#endif
#endif /* H5_HAVE_PARALLEL */
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
set_alignment (
	const h5_file_p f
	) {
	H5_INLINE_FUNC_ENTER (h5_err_t);
	if ( f->props->align != 0 ) {
		h5_info (
			"Setting HDF5 alignment to %lld bytes "
			"with threshold at half that many bytes.",
			(long long int)f->props->align);
		TRY (hdf5_set_alignment_property (
                             f->props->access_prop,
                             f->props->align / 2,
                             f->props->align));
		h5_info (
			"Setting HDF5 meta block to %lld bytes",
			(long long int)f->props->align);
		TRY (H5Pset_meta_block_size (f->props->access_prop, f->props->align));
	}
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
set_default_file_props (
        h5_prop_file_t* const _props
        ) {
        H5_INLINE_FUNC_ENTER (h5_err_t);
        h5_prop_file_t* props = (h5_prop_file_t*)_props;
        memset (props, 0, sizeof (*props));
        props->class = H5_PROP_FILE;
        TRY (props->prefix_iteration_name = h5_calloc (1, H5_ITERATION_NAME_LEN));
        strncpy (
                props->prefix_iteration_name,
                H5_ITERATION_NAME,
                H5_ITERATION_NAME_LEN - 1);
        props->width_iteration_idx = H5_ITERATION_NUM_WIDTH;
#ifdef H5_HAVE_PARALLEL
        props->comm = MPI_COMM_WORLD;
#endif
        H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_set_prop_file_mpio_collective (
        h5_prop_t _props,
        MPI_Comm* comm
        ) {
        h5_prop_file_t* props = (h5_prop_file_t*)_props;
        H5_CORE_API_ENTER (h5_err_t, "props=%p, comm=%p", props, comm);
        
        if (props->class != H5_PROP_FILE) {
                H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Invalid property class: %lld",
			(long long int)props->class);
        }
#ifdef H5_HAVE_PARALLEL
        props->flags &= ~(H5_VFD_MPIO_POSIX | H5_VFD_MPIO_INDEPENDENT | H5_VFD_CORE_IO);
        props->flags |= H5_VFD_MPIO_COLLECTIVE;
        props->comm = *comm;
	if (props->throttle > 0) {
		h5_warn ("Throttling is not permitted with collective VFD. Reset throttling.");
		props->throttle = 0;
	}
#else
	h5_info ("Setting MPIO collective property ignored in serial H5hut");
#endif
        H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_set_prop_file_mpio_independent (
        h5_prop_t _props,
        MPI_Comm* comm
        ) {
        h5_prop_file_t* props = (h5_prop_file_t*)_props;
        H5_CORE_API_ENTER (h5_err_t, "props=%p, comm=%p", props, comm);
        
        if (props->class != H5_PROP_FILE) {
                H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Invalid property class: %lld",
			(long long int)props->class);
        }
#ifdef H5_HAVE_PARALLEL
        props->flags &= ~(H5_VFD_MPIO_COLLECTIVE | H5_VFD_MPIO_POSIX | H5_VFD_CORE_IO);
        props->flags |= H5_VFD_MPIO_INDEPENDENT;
        props->comm = *comm;
#else
	h5_info ("Setting MPIO independent property ignored in serial H5hut");
#endif
        H5_RETURN (H5_SUCCESS);
}

#if H5_VERSION_LE(1,8,12)
h5_err_t
h5_set_prop_file_mpio_posix (
        h5_prop_t _props,
        MPI_Comm* comm
        ) {
        h5_prop_file_t* props = (h5_prop_file_t*)_props;
        H5_CORE_API_ENTER (h5_err_t, "props=%p, comm=%p", props, comm);
        
        if (props->class != H5_PROP_FILE) {
                H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Invalid property class: %lld",
			(long long int)props->class);
        }
#ifdef H5_HAVE_PARALLEL
        props->flags &= ~(H5_VFD_MPIO_COLLECTIVE | H5_VFD_MPIO_POSIX | H5_VFD_CORE_IO);
        props->flags |= H5_VFD_MPIO_INDEPENDENT;
        props->comm = *comm;
#else
	h5_info ("Setting MPIO POSIX property ignored in serial H5hut");
#endif
        H5_RETURN (H5_SUCCESS);
}
#endif

h5_err_t
h5_set_prop_file_core_vfd (
        h5_prop_t _props,
	const h5_int64_t increment
        ) {
        h5_prop_file_t* props = (h5_prop_file_t*)_props;
        H5_CORE_API_ENTER (h5_err_t, "props=%p, increment=%lld",
			   props, (long long int)increment);
        
        if (props->class != H5_PROP_FILE) {
                H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Invalid property class: %lld",
			(long long int)props->class);
        }
#ifdef H5_HAVE_PARALLEL
        props->flags &= ~(H5_VFD_MPIO_COLLECTIVE |
			  H5_VFD_MPIO_INDEPENDENT |
			  H5_VFD_MPIO_POSIX);
        props->flags |= H5_VFD_MPIO_INDEPENDENT;
        props->comm = MPI_COMM_SELF;
	props->increment = increment;
	if (props->throttle > 0) {
		h5_warn ("Throttling is not permitted with core VFD. Reset throttling.");
		props->throttle = 0;
	}
#else
	h5_info ("Setting MPIO core property ignored in serial H5hut");
#endif
        H5_RETURN (H5_SUCCESS);
}


h5_err_t
h5_set_prop_file_align (
        h5_prop_t _props,
        const h5_int64_t align
        ) {
        h5_prop_file_t* props = (h5_prop_file_t*)_props;
        H5_CORE_API_ENTER (
		h5_err_t,
		"props=%p, align=%lld",
		props, (long long int)align);
        if (props->class != H5_PROP_FILE) {
                H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Invalid property class: %lld",
			(long long int)props->class);
        }
        props->align = align;
        H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_set_prop_file_flush_after_write (
        h5_prop_t _props
        ) {
        h5_prop_file_t* props = (h5_prop_file_t*)_props;
        H5_CORE_API_ENTER (
		h5_err_t,
		"props=%p",
		props);
        if (props->class != H5_PROP_FILE) {
                H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Invalid property class: %lld",
			(long long int)props->class);
        }
        props->flush = 1;
        H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_set_prop_file_throttle (
        h5_prop_t _props,
        const h5_int64_t throttle
        ) {
        h5_prop_file_t* props = (h5_prop_file_t*)_props;
        H5_CORE_API_ENTER (
		h5_err_t,
		"props=%p, throttle=%lld",
		props, (long long int)throttle);
        if (props->class != H5_PROP_FILE) {
                H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Invalid property class: %lld",
			(long long int)props->class);
        }
#ifdef H5_HAVE_PARALLEL
	// throttle only if VFD is MPIO independent od POSIX
	h5_int64_t mask = H5_VFD_MPIO_INDEPENDENT;
#if H5_VERSION_LE(1,8,12)
	mask |= H5_VFD_MPIO_POSIX;
#endif
	if (! (props->flags & mask)) {
#if H5_VERSION_LE(1,8,12)
		h5_warn (
			"Throttling is only permitted with the MPI-POSIX "
			"or MPI-IO Independent VFD. Property ignored." );
#else
		h5_warn (
			"Throttling is only permitted with "
			"the MPI-IO Independent VFD. Property ignored.");
#endif
		props->throttle = 0;
	}

        props->throttle = throttle;
#else
	h5_info ("Setting the throttle property in serial H5hut");
#endif
        H5_RETURN (H5_SUCCESS);
}


h5_prop_t
h5_create_prop (
        const h5_int64_t class
        ) {
        H5_CORE_API_ENTER (
		h5_prop_t,
		"class=%lld",
		(long long int)class);
        h5_prop_t* prop;
        switch (class) {
        case H5_PROP_FILE:
                TRY (prop = h5_calloc (1, sizeof (h5_prop_file_t)));
                set_default_file_props ((h5_prop_file_t*)prop);
                break;
        default:
                H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Invalid property class: %lld",
			(long long int)class);
        }
        H5_RETURN ((h5_prop_t)prop);
}

h5_err_t
h5_close_prop (
        h5_prop_t _prop
        ) {
        h5_prop_p prop = (h5_prop_p)_prop;
        H5_CORE_API_ENTER (h5_err_t, "prop=%p", prop);
        switch (prop->class) {
        case H5_PROP_FILE: {
                h5_prop_file_t* file_prop = (h5_prop_file_t*)prop;
                TRY (h5_free (file_prop->prefix_iteration_name));
                break;
        }
        default:
                H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Invalid property class: %lld",
			(long long int)prop->class);
        }
        H5_RETURN (h5_free (prop));
}

static inline h5_err_t
open_file (
	const h5_file_p f,
	const char* const filename,
	const h5_int32_t mode
	) {
	H5_INLINE_FUNC_ENTER (h5_err_t);
        h5_info ("Opening file %s.", filename);

        f->props->flags |= mode;

        f->nprocs = 1; // queried later
        f->myproc = 0; // queried later
        f->iteration_gid = -1;

        TRY (f->iteration_name = h5_calloc (2, H5_ITERATION_NAME_LEN));
        sprintf (
                f->iteration_name,
                "%s#%0*lld",
                f->props->prefix_iteration_name,
                f->props->width_iteration_idx, (long long)f->iteration_idx);

        TRY (hdf5_set_errorhandler (H5E_DEFAULT, hdf5_error_handler, NULL));
        
        f->props->xfer_prop = f->props->access_prop = H5P_DEFAULT;
        TRY (f->props->create_prop = hdf5_create_property (H5P_FILE_CREATE));
	TRY (mpi_init (f));              // noop if serial
	TRY (set_alignment (f));

	if (f->props->flags & H5_O_RDONLY) {
		f->file = H5Fopen (filename, H5F_ACC_RDONLY, f->props->access_prop);
	}
	else if (f->props->flags & H5_O_WRONLY){
		f->file = H5Fcreate (
                        filename, H5F_ACC_TRUNC, f->props->create_prop,
                        f->props->access_prop);
		f->empty = 1;
	}
	else if (f->props->flags & H5_O_APPENDONLY || f->props->flags & H5_O_RDWR) {
		int fd = open (filename, O_RDONLY, 0);
		if ((fd == -1) && (errno == ENOENT)) {
			f->file = H5Fcreate (
                                filename, H5F_ACC_TRUNC,
                                f->props->create_prop, f->props->access_prop);
			f->empty = 1;
		}
		else if (fd != -1) {
			close (fd);
			f->file = H5Fopen (filename, H5F_ACC_RDWR,
					   f->props->access_prop);
		}
	}
	else {
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Invalid file access mode '%lld'.",
			(long long int)f->props->flags & 0xff);
	}
	
	if (f->file < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot open file '%s' with mode '%s'",
			filename, H5_O_MODES[f->props->flags & 0xff]);
	TRY (f->root_gid = hdf5_open_group (f->file, "/" ));

	TRY (h5upriv_open_file (f));
	TRY (h5bpriv_open_file (f));

	H5_RETURN (H5_SUCCESS);
}

h5_file_t
h5_open_file2 (
	const char* const filename,
	const h5_int32_t mode,
        const h5_prop_t props_
	) {
        h5_prop_file_t* const props = (h5_prop_file_t*)props_;
	H5_CORE_API_ENTER (h5_file_t,
			   "filename='%s', mode=%d, props=%p",
			   filename, mode, props);
                
	h5_file_p f = NULL;
	TRY (f = h5_calloc (1, sizeof (*f)));
	
        TRY (f->props = (h5_prop_file_t*)h5_create_prop (H5_PROP_FILE));
                
        if (props != H5_PROP_DEFAULT) {
                if (props->class != H5_PROP_FILE) {
                        H5_RETURN_ERROR (
				H5_ERR_INVAL,
				"Invalid property class: %lld.",
				(long long int)props->class);
                }
#ifdef H5_HAVE_PARALLEL
                f->props->comm = props->comm;
#endif
                f->props->flags = props->flags;
                f->props->throttle = props->throttle;
                f->props->align = props->align;

                strncpy (
                        f->props->prefix_iteration_name,
                        props->prefix_iteration_name,
                        H5_ITERATION_NAME_LEN - 1);
                f->props->width_iteration_idx = props->width_iteration_idx;
        }

	TRY (open_file (f, filename, mode));

        TRY (h5_set_iteration_name_fmt ((uintptr_t)f,
					H5_ITERATION_NAME,
					H5_ITERATION_NUM_WIDTH));

	H5_RETURN ((h5_file_t)f);
}

/*!
  \ingroup h5_core_filehandling
  
  Open file with name \c filename. This function is available in the paralell
  and serial version. In the serial case \c comm may have any value.

  \param[in]	filename	The name of the data file to open.
  \param[in]	flags		The access mode for the file.
  \param[in]	comm		MPI communicator
  \param[in]	align		Number of bytes for setting alignment,
				metadata block size, etc. Set to 0 to disable.

  \return File handle.
  \return H5_ERR  on error.
*/

h5_file_p
h5_open_file1 (
	const char* filename,
	h5_int32_t mode,
	MPI_Comm comm,
	const h5_size_t align
	) {
	H5_CORE_API_ENTER (
		h5_file_p,
		"filename='%s', mode=%d, comm=?, align=%llu",
		filename, mode, (long long int)align);
        h5_prop_file_t* props;
        h5_file_t f;
        TRY (props = (h5_prop_file_t*)h5_create_prop (H5_PROP_FILE));
        TRY (h5_set_prop_file_mpio_collective ((h5_prop_t)props, &comm));
        TRY (h5_set_prop_file_align ((h5_prop_t)props, align));
        TRY (f = h5_open_file2 (filename, mode, (h5_prop_t)props));
        TRY (h5_close_prop ((h5_prop_t)props));
	h5_file_p _f = (h5_file_p)f;
        H5_RETURN (_f);

}

/*!
  \ingroup h5_core_filehandling

  The h5_close_file() call writes all buffered data to disk, releases 
  all previously allocated memory and terminates access to the associated
  HDF5 file.

  \return	H5_SUCCESS or error code
*/
h5_err_t
h5_close_file (
	const h5_file_t f_              /*!< file handle */
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t, "f=%p", f);
	h5_errno = H5_SUCCESS;

	check_file_handle_is_valid (f);

	TRY (h5priv_close_iteration (f));
	TRY (h5upriv_close_file (f));
	TRY (h5bpriv_close_file (f));
	TRY (hdf5_close_property (f->props->xfer_prop));
	TRY (hdf5_close_property (f->props->access_prop));
	TRY (hdf5_close_property (f->props->create_prop));
	TRY (hdf5_close_group (f->root_gid));
        TRY (hdf5_flush (f->file, H5F_SCOPE_GLOBAL));
        TRY (h5_close_prop ((h5_prop_t)f->props));
	TRY (hdf5_close_file (f->file));
        TRY (h5_free (f->iteration_name));
 	TRY (h5_free (f));
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_close_h5hut (
        void
        ) {
	H5_CORE_API_ENTER (h5_err_t, "%s", "");
	TRY (h5_finalize ());
	TRY (hdf5_close ());
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_flush_iteration (
	const h5_file_t f_
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t, "f=%p", f);
	check_iteration_is_writable (f);
	ret_value = H5_SUCCESS;
	if (f->iteration_gid >= 0) {
		TRY (ret_value = hdf5_flush (f->iteration_gid, H5F_SCOPE_LOCAL));
	}
	H5_RETURN (ret_value);
}

h5_err_t
h5_flush_file (
	const h5_file_t f_
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t, "f=%p", f);
	check_file_is_writable (f);
	TRY (ret_value = hdf5_flush (f->file, H5F_SCOPE_GLOBAL));
	H5_RETURN (ret_value);
}


/*!
  \ingroup h5_core_filehandling

  Define format of the iteration names.

  Example: ==h5_set_iteration_name_fmt (f, "Step", 6)==
  defines iteration names like ==Step#000042==.

  \return \c H5_SUCCESS or error code
*/
h5_err_t
h5_set_iteration_name_fmt (
	const h5_file_t f_,
	const char* name,
	int width
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
                           "f=%p, name='%s', width=%d",
                           f, name, width);
	check_file_handle_is_valid (f);
	if (width < 0) width = 0;
	else if (width > H5_ITERATION_NAME_LEN - 1) width = H5_ITERATION_NAME_LEN - 1;
	strncpy (
		f->props->prefix_iteration_name,
		name,
		H5_ITERATION_NAME_LEN - 1);
	f->props->width_iteration_idx = width;

	H5_RETURN (H5_SUCCESS);
}

/*!
  \ingroup h5_core_filehandling

  Get format of the iteration names.

  \return \c H5_SUCCESS or error code
*/
h5_err_t
h5_get_iteration_name_fmt (
	const h5_file_t f_,		/*!< Handle to file		*/
	char* const name,		/*!< OUT: Prefix		*/
	const int l_name,		/*!< length of buffer name	*/
	int* const width		/*!< OUT: Width of the number	*/
	) {
        h5_file_p f = (h5_file_p)f_;
	UNUSED_ARGUMENT (f);
	UNUSED_ARGUMENT (name);
	UNUSED_ARGUMENT (l_name);
	UNUSED_ARGUMENT (width);
	return h5_error_not_implemented ();
}

/*!
  \ingroup h5_core_filehandling

  Get current iteration number.

  \return Current iteration number or error code
*/
h5_id_t
h5_get_iteration (
	const h5_file_t f_		/*!< file handle		*/
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_id_t, "f=%p", f);
	check_iteration_is_readable (f);
	H5_RETURN (f->iteration_idx);
}

/*!
  \ingroup h5_core_filehandling

  Get number of processes.

  \return Number of processes or error code
*/
int
h5_get_num_procs (
	const h5_file_t f_		/*!< file handle		*/
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (int, "f=%p", f);
	check_file_handle_is_valid (f);
	H5_RETURN (f->nprocs);
}

/*!
  \ingroup h5_core_filehandling

  Get number of iterations.

  \return Number of iterations or error code
*/
h5_ssize_t
h5_get_num_iterations (
	const h5_file_t f_		/*!< file handle		*/
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (int, "f=%p", f);
	check_file_handle_is_valid (f);
	TRY (ret_value = hdf5_get_num_groups_matching_prefix (
		     f->root_gid,
		     f->props->prefix_iteration_name));
	H5_RETURN (ret_value);
}

/*!
  \ingroup h5_core_filehandling

  Start traversing iterations.

  \return \c H5_SUCCESS or error code 
*/
h5_err_t
h5_start_traverse_iterations (
	const h5_file_t f_		/*!< file handle		*/
	) {
	H5_CORE_API_ENTER (int, "f=%p", (void*)f_);

	/*
	  fast test: Does Step#0 or Step#1 exist?
	  otherwise
	  loop over all steps and get smallest step number
	 */
	
	H5_RETURN (h5_error_not_implemented ());
}

/*!
  \ingroup h5_core_filehandling

  Go to next iteration.

  \return \c H5_SUCCESS or error code 
*/
h5_err_t
h5_traverse_iterations (
	const h5_file_t f_		/*!< file handle		*/
	) {
        h5_file_p f = (h5_file_p)f_;
	UNUSED_ARGUMENT (f);
	return h5_error_not_implemented ();
}
