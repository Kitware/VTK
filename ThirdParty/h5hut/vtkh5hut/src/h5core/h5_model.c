/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include <string.h>

#include "h5core/h5_model.h"
#include "private/h5_types.h"
#include "private/h5_hdf5.h"
#include "private/h5_model.h"

h5_err_t
h5priv_close_iteration (
	const h5_file_p f
	) {
	H5_PRIV_API_ENTER (h5_err_t, "f=%p", f);
	if (f->iteration_gid <= 0)
		H5_LEAVE (H5_SUCCESS);
	TRY (hdf5_close_group (f->iteration_gid));

	f->iteration_gid = -1;

	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_set_iteration (
	const h5_file_t f_,		/*!< [in]  Handle to open file */
	const h5_id_t iteration_idx	/*!< [in]  Iteration to set. */
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
                           "f=%p, iteration_idx=%lld",
                           f, (long long)iteration_idx);
	CHECK_FILEHANDLE (f);
	TRY (h5priv_close_iteration (f));
	f->iteration_idx = iteration_idx;

	sprintf (
		f->iteration_name,
		"%s#%0*lld",
		f->props->prefix_iteration_name, f->props->width_iteration_idx,
                (long long) f->iteration_idx);
	h5_info (
		"Open iteration #%lld in file %lld",
		(long long)f->iteration_idx,
		(long long)(size_t) f);

	h5_err_t exists;
	TRY (exists = hdf5_link_exists (f->file, f->iteration_name));
	if (exists) {
		TRY (f->iteration_gid = h5priv_open_group (
			     f->file,
			     f->iteration_name));
	} else if (is_writable (f)) {
		TRY (f->iteration_gid = h5priv_create_group (
			     f->file,
			     f->iteration_name));
	}
	H5_RETURN (H5_SUCCESS);
}

/*
  returns:
  TRUE (value > 0): if iteration exists
  FALSE (i.e. 0):   if iteration does not exist
  H5_FAILURE: on error
 */
h5_err_t
h5_has_iteration (
	const h5_file_t f_,		/*!< [in]  Handle to open file */
	const h5_id_t iteration_idx	/*!< [in]  Step number to query */
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, iteration_idx=%lld",
			   f, (long long)iteration_idx);
	CHECK_FILEHANDLE (f);
	char name[2*H5_ITERATION_NAME_LEN];
	sprintf (name,
		"%s#%0*lld",
		f->props->prefix_iteration_name, f->props->width_iteration_idx,
		 (long long)iteration_idx);
        TRY (ret_value = hdf5_link_exists (f->file, name));
	H5_RETURN (ret_value);
}

h5_err_t
h5priv_normalize_dataset_name (
	char* const name
	) {
	H5_CORE_API_ENTER (h5_err_t, "name='%s'", name);
	if (strlen(name) > H5_DATANAME_LEN-1) {
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Dataset name '%s' to long! "
			"Must be less then %d characters.",
			 name, H5_DATANAME_LEN);
	}
	if (strcmp (name, H5BLOCK_GROUPNAME_BLOCK) == 0) {
		H5_RETURN_ERROR (
			H5_ERR_INVAL,
			"Can't create dataset or field with name '%s'"
			" because it is reserved by H5Block.",
			H5BLOCK_GROUPNAME_BLOCK);
	}
	H5_RETURN (H5_SUCCESS);
}
