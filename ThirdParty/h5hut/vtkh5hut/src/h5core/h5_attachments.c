/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "private/h5_types.h"

#include "private/h5_err.h"
#include "private/h5_hdf5.h"
#include "private/h5_model.h"
#include "h5core/h5_syscall.h"
#include "private/h5_va_macros.h"

#include "h5core/h5_file.h"

#include <string.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif
#include <fcntl.h>
#include <errno.h>

h5_err_t
h5_add_attachment (
	const h5_file_t f_,
	const char* const fname
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t, "f=%p, fname='%s'", f, fname);
	CHECK_FILEHANDLE (f);
	CHECK_WRITABLE_MODE (f);

	struct stat st;
        if (stat (fname, &st) < 0) {
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot stat file '%s'",
			fname);
	}
	hsize_t fsize = st.st_size;
	hsize_t write_length;
	char* buf = NULL;
	if (f->myproc == 0) {
		TRY (buf = h5_calloc (1, fsize));
		write_length = fsize;
		int fd;
		if ((fd = open (fname, O_RDONLY)) < 0) {
			H5_RETURN_ERROR (
				H5_ERR_HDF5,
				"Cannot open file '%s' for reading",
				fname);
		}
	again:
		if (read (fd, buf, fsize) < 0) {
			if (errno == EINTR) {
				goto again;
			} else {
				H5_RETURN_ERROR (
					H5_ERR_HDF5,
					"Cannot read file '%s'",
					fname);
			}
		}
		if (close (fd) < 0) {
			H5_RETURN_ERROR (
				H5_ERR_HDF5,
				"Cannot close file '%s'",
				fname);
		}

	} else {
		TRY (buf = h5_calloc (1, 1));
		write_length = 0;
	}

	hid_t loc_id;
	TRY (loc_id = h5priv_create_group (f->file, H5_ATTACHMENT));
	h5_err_t exists;
	TRY (exists = hdf5_link_exists (loc_id, fname));
        if (exists && (f->props->flags & H5_O_APPENDONLY)) {
		H5_LEAVE (
			h5priv_handle_file_mode_error (f->props->flags));
	}
	hid_t diskspace_id;
	TRY (diskspace_id = hdf5_create_dataspace (1, &fsize, &fsize));
	hid_t dataset_id;
	TRY (dataset_id = hdf5_create_dataset (loc_id,
					       fname,
					       H5T_NATIVE_CHAR,
					       diskspace_id,
					       H5P_DEFAULT));
	hsize_t start = 0;
	TRY (hdf5_select_hyperslab_of_dataspace (
		     diskspace_id,
		     H5S_SELECT_SET,
		     &start,
		     NULL,
		     &write_length,
		     NULL));

	hid_t memspace_id;
	hsize_t max = H5S_UNLIMITED;
	TRY (memspace_id = hdf5_create_dataspace (1, &write_length, &max));
	TRY (hdf5_write_dataset (dataset_id,
				 H5T_NATIVE_CHAR,
				 memspace_id,
				 diskspace_id,
				 f->props->xfer_prop,
				 buf));

	TRY (hdf5_close_dataspace (diskspace_id));
	TRY (hdf5_close_dataspace (memspace_id));
	TRY (hdf5_close_dataset (dataset_id));
	TRY (hdf5_close_group (loc_id));

	TRY (h5_free (buf));

	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_has_attachments (
	const h5_file_t f_
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_ssize_t, "f=%p", f);
	CHECK_FILEHANDLE (f);
	TRY  (ret_value = hdf5_link_exists (f->file, H5_ATTACHMENT));
	H5_RETURN (ret_value);
}

h5_ssize_t
h5_get_num_attachments (
	const h5_file_t f_
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_ssize_t, "f=%p", f);
	CHECK_FILEHANDLE (f);
	h5_err_t exists;
	TRY  (exists = hdf5_link_exists (f->file, H5_ATTACHMENT));
	if (exists == 0) {
		return 0;
	}
	hid_t group_id;
	TRY (group_id = hdf5_open_group (f->file, H5_ATTACHMENT));
	TRY (ret_value = hdf5_get_num_datasets (group_id));
	TRY (hdf5_close_group (group_id));
	H5_RETURN (ret_value);
}

h5_err_t
h5_get_attachment_info_by_idx (
	const h5_file_t f_,
	const h5_size_t idx,		// IN
	char* const fname,		// OUT
	h5_size_t len_fname,		// IN
	h5_size_t* const fsize		// OUT
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, idx=%llu, fname=%s, len_fname=%llu, fsize=%p",
			   f, (unsigned long long)idx,
			   fname, (unsigned long long)len_fname,
			   fsize);
	CHECK_FILEHANDLE (f);
	hid_t loc_id;
	TRY (loc_id = hdf5_open_group (f->file, H5_ATTACHMENT));
	TRY (hdf5_get_name_of_dataset_by_idx (
		     loc_id,
		     idx,
		     fname, len_fname));

	if (fsize) {
		// get number of elements, do not change value on error
		h5_ssize_t ssize;
		TRY (ssize = hdf5_get_npoints_of_dataset_by_name (loc_id, fname));
		*fsize = ssize;
	}
	TRY (hdf5_close_group (loc_id));
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_has_attachment (
	const h5_file_t f_,		// [in]
	const char* const fname		// [in]
	) {
	h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t, "f=%p, fname='%s'", f, fname);
	CHECK_FILEHANDLE (f);
	hid_t loc_id;
	TRY (loc_id = hdf5_open_group (f->file, H5_ATTACHMENT));
        TRY (ret_value = hdf5_link_exists (f->file, fname));
	H5_RETURN (ret_value);
}

h5_err_t
h5_get_attachment_info_by_name (
	const h5_file_t f_,
	const char* const fname,	// IN
	h5_size_t* const fsize		// OUT
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t, "f=%p, fname='%s', fsize=%p", f, fname, fsize);
	CHECK_FILEHANDLE (f);
	
	hid_t loc_id;
	TRY (loc_id = hdf5_open_group (f->file, H5_ATTACHMENT));
	if (fsize) {
		// get number of elements, do not change value on error
		h5_ssize_t ssize;
		TRY (ssize = hdf5_get_npoints_of_dataset_by_name (loc_id, fname));
		*fsize = ssize;
	}
	TRY (hdf5_close_group (loc_id));
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_get_attachment (
	const h5_file_t f_,
	const char* const fname
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t, "f=%p, fname='%s'", f, fname);
	CHECK_FILEHANDLE (f);
	// allowed modes: O_RDWR, O_RDONLY; O_APPEND
	// forbidden modes: O_WRONLY
	if (f->props->flags & H5_O_WRONLY) {
		H5_LEAVE (
			h5priv_handle_file_mode_error (f->props->flags));
	}

	hid_t loc_id;
	TRY (loc_id = hdf5_open_group (f->file, H5_ATTACHMENT));

	// read dataset
	hid_t dataset_id, diskspace_id;
	h5_ssize_t fsize;
	TRY (dataset_id = hdf5_open_dataset_by_name (loc_id, fname));
	TRY (diskspace_id = hdf5_get_dataset_space (dataset_id));
	TRY (fsize = hdf5_get_npoints_of_dataspace (diskspace_id));

	hsize_t read_length;
	char* buf = NULL;
	if (f->myproc == 0) {
		buf = h5_calloc (1, fsize);
		read_length = fsize;

	} else {
		buf = h5_calloc (1, 1);
		read_length = 0;
	}

	hsize_t start = 0;
	TRY (hdf5_select_hyperslab_of_dataspace (
		     diskspace_id,
		     H5S_SELECT_SET,
		     &start,
		     NULL,
		     &read_length,
		     NULL));

	hid_t memspace_id;
	hsize_t max = H5S_UNLIMITED;
	TRY (memspace_id = hdf5_create_dataspace (1, &read_length, &max));
	TRY (hdf5_read_dataset (dataset_id,
				 H5T_NATIVE_CHAR,
				 memspace_id,
				 diskspace_id,
				 f->props->xfer_prop,
				 buf));

	TRY (hdf5_close_dataspace (diskspace_id));
	TRY (hdf5_close_dataspace (memspace_id));
	TRY (hdf5_close_dataset (dataset_id));
	TRY (hdf5_close_group (loc_id));

	// write file
	if (f->myproc == 0) {
		int fd;
		if ((fd = open (fname, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0) {
			H5_RETURN_ERROR (
				H5_ERR_H5,
				"Error opening file '%s': %s",
				fname, strerror(errno));
		}
		if (write (fd, buf, fsize) != fsize) {
			H5_RETURN_ERROR (
				H5_ERR_H5,
				"Error writing to file '%s': %s",
				fname, strerror(errno));
		}
		if (close (fd) < 0) {
			H5_RETURN_ERROR (
				H5_ERR_H5,
				"Error closing file '%s': %s",
				fname, strerror(errno));
		}
	}
	TRY (h5_free (buf));

	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_delete_attachment (
	const h5_file_t f_,
	const char* const fname
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t, "f=%p, fname='%s'", f, fname);
	CHECK_FILEHANDLE (f);
	// allowed file modes: O_RDWR, O_WRONLY; O_APPEND
	if (f->props->flags & H5_O_RDONLY) {
		H5_LEAVE (
			h5priv_handle_file_mode_error (f->props->flags));
	}

	hid_t loc_id;
	TRY (loc_id = hdf5_open_group (f->file, H5_ATTACHMENT));
	TRY (hdf5_delete_link (loc_id, fname, H5P_DEFAULT));
	TRY (hdf5_close_group (loc_id));
	H5_RETURN (H5_SUCCESS);
}
