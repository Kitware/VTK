/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_HDF5_H
#define __PRIVATE_H5_HDF5_H

#include <vtk_hdf5.h>

#include "h5core/h5_types.h"
#include "h5core/h5_err.h"
#include "h5core/h5_syscall.h"

#include "private/h5_log.h"
#include "private/h5_va_macros.h"

#include <stdlib.h>

ssize_t
hdf5_get_num_groups (
	const hid_t loc_id
	);

ssize_t
hdf5_get_num_groups_matching_prefix (
	const hid_t loc_id,
	char* prefix
	);

h5_err_t
hdf5_get_name_of_group_by_idx (
	hid_t loc_id,
	hsize_t idx,
	char *name,
	size_t len
	);

ssize_t
hdf5_get_num_datasets (
	const hid_t loc_id
	);

h5_err_t
hdf5_get_name_of_dataset_by_idx (
	hid_t loc_id,
	hsize_t idx,
	char *name,
	size_t len
	);

const char *
hdf5_get_objname (
	hid_t id
	);


/****** L i n k **************************************************************/

/*
   Determine whether a link with the specified name exists in a group.

   Result:
   TRUE		if link exists
   FALSE	if link doesn't exist
   H5_FAILURE	on error
*/
static inline h5_err_t
hdf5_link_exists (
        const hid_t loc_id,
        const char* name
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "loc_id=%lld (%s), name='%s'",
	                    (long long int)loc_id, hdf5_get_objname (loc_id), name);
	/* Save old error handler */
	H5E_auto2_t old_func;
	void *old_client_data;

	H5Eget_auto2(H5E_DEFAULT, &old_func, &old_client_data);

	/* Turn off error handling */
	H5Eset_auto(H5E_DEFAULT, NULL, NULL);

	/* Probe. Likely to fail, but that’s okay */
	htri_t exists = H5Lexists ( loc_id, name, H5P_DEFAULT );

	/* Restore previous error handler */
	H5Eset_auto(H5E_DEFAULT, old_func, old_client_data);

	if (exists < 0 )
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot query link %s/%s.",
			hdf5_get_objname (loc_id), name);
	H5_RETURN (exists);
}

static inline h5_err_t
hdf5_delete_link (
        hid_t loc_id,
        const char* name,
        hid_t lapl_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "loc_id=%lld (%s), name='%s', lapl_id=%lld",
	                    (long long int)loc_id, hdf5_get_objname (loc_id), name,
			    (long long int)lapl_id);
	if (H5Ldelete (loc_id, name, lapl_id)  < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot delete link %s/%s.",
			hdf5_get_objname (loc_id), name);

	H5_RETURN (H5_SUCCESS);
}

/****** G r o u p ************************************************************/

static inline hid_t
hdf5_open_group (
        const hid_t loc_id,
        const char* const group_name
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "loc_id=%lld (%s), group_name='%s'",
	                    (long long int)loc_id,
	                    hdf5_get_objname (loc_id),
	                    group_name);
	hid_t group_id = H5Gopen (loc_id, group_name, H5P_DEFAULT);
	if (group_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot open group '%s/%s'.",
			hdf5_get_objname (loc_id),
			group_name);
	H5_RETURN (group_id);
}

static inline hid_t
hdf5_create_group (
        const hid_t loc_id,
        const char* const group_name
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "loc_id=%lld (%s), group_name='%s'",
	                    (long long int)loc_id,
	                    hdf5_get_objname (loc_id),
	                    group_name);
	hid_t group_id = H5Gcreate (
	        loc_id, group_name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	if (group_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot create group '%s/%s'.",
			hdf5_get_objname (loc_id),
			group_name);
	H5_RETURN (group_id);
}

h5_err_t
h5priv_link_exists_ (
        const hid_t loc_id,
        const char *const path[],
        size_t size
        );
#define h5priv_link_exists_count(...) \
	PP_NARG(__VA_ARGS__)
#define h5priv_link_exists(loc_id, ...)                             \
        (h5priv_link_exists_(loc_id,                                \
                            (const char *const[]){__VA_ARGS__},     \
                            h5priv_link_exists_count(__VA_ARGS__)))


static inline h5_err_t
hdf5_close_group (
        const hid_t group_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "group_id=%lld (%s)",
	                    (long long int)group_id,
	                    hdf5_get_objname (group_id));

	if (group_id == 0 || group_id == -1)
		H5_LEAVE (H5_SUCCESS);
	if (H5Gclose (group_id) < 0 ) {
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot terminate access to group '%s').",
			hdf5_get_objname (group_id));
	}
	H5_RETURN (H5_SUCCESS);
}

static inline hid_t
h5priv_create_group (
        const hid_t loc_id,
        const char *const group_name
        ) {
	H5_PRIV_FUNC_ENTER (hid_t,
	                    "loc_id=%lld, (%s), group_name=%s",
			    (long long int)loc_id, hdf5_get_objname (loc_id),
	                    group_name);
	h5_err_t exists;
	TRY (exists = hdf5_link_exists (loc_id, group_name));
	if (exists) {
		TRY (ret_value = hdf5_open_group (loc_id, group_name));
	} else {
		TRY (ret_value = hdf5_create_group (loc_id, group_name));
	}
	H5_RETURN (ret_value);
}

static inline hid_t
h5priv_open_group (
        const hid_t loc_id,
        const char *const group_name
        ) {
	H5_PRIV_FUNC_ENTER (hid_t,
	                    "loc_id=%lld, (%s), group_name=%s",
			    (long long int)loc_id, hdf5_get_objname (loc_id),
	                    group_name);
	h5_err_t exists;
	TRY (exists = hdf5_link_exists (loc_id, group_name));
	if (exists) {
		TRY (ret_value = hdf5_open_group (loc_id, group_name));
	} else {
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Group does not exist: '%s/%s'.",
			hdf5_get_objname (loc_id),
			group_name);
	}
	H5_RETURN (ret_value);
}

static inline hid_t
h5priv_create_group_with_intermediates (
        const hid_t loc_id,
	...
        ) {
	va_list ap;
	va_start(ap, loc_id);
	char* group_name = va_arg(ap, char*);
	H5_PRIV_FUNC_ENTER (hid_t,
	                    "loc_id=%lld (%s), "
			    "group_name=%s, ...",
			    (long long int)loc_id, hdf5_get_objname (loc_id),
			    group_name);
	hid_t parent_id = loc_id;
	while (group_name != NULL) {
		TRY (ret_value = h5priv_create_group (
			     parent_id,
			     group_name));
		if (parent_id != loc_id) {
			// close intermediate groups
			TRY (hdf5_close_group (parent_id));
		}
		group_name = va_arg(ap, char*);
		parent_id = ret_value;
	}
	va_end (ap);
	H5_RETURN (ret_value);
}

static inline hid_t
h5priv_open_group_with_intermediates (
        const hid_t loc_id,
	...
        ) {
	va_list ap;
	va_start(ap, loc_id);
	char* group_name = va_arg(ap, char*);
	H5_PRIV_FUNC_ENTER (hid_t,
	                    "loc_id=%lld (%s), "
			    "group_name=%s, ...",
			    (long long int)loc_id, hdf5_get_objname (loc_id),
			    group_name);
	hid_t parent_id = loc_id;
	while (group_name != NULL) {
		TRY (ret_value = h5priv_open_group (
			     parent_id,
			     group_name));
		if (parent_id != loc_id) {
			// close intermediate groups
			TRY (hdf5_close_group (parent_id));
		}
		group_name = va_arg(ap, char*);
		parent_id = ret_value;
	}
	va_end(ap);
	H5_RETURN (ret_value);
}

static inline h5_ssize_t
hdf5_get_num_objs_in_group (
        const hid_t group_id
        ) {
	HDF5_WRAPPER_ENTER (h5_ssize_t,
	                    "group_id=%lld (%s)",
	                    (long long int)group_id,
	                    hdf5_get_objname (group_id));
	H5G_info_t group_info;
	if (H5Gget_info (group_id, &group_info) < 0) {
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot get number of objects in group '%s'.",
			hdf5_get_objname(group_id));
	}
	H5_RETURN ((h5_ssize_t)group_info.nlinks);
}


static inline h5_ssize_t
hdf5_get_objname_by_idx (
        hid_t loc_id,
        hsize_t idx,
        char *name,
        size_t size
        ) {
	HDF5_WRAPPER_ENTER (h5_ssize_t,
	                    "loc_id=%lld (%s), idx=%lld",
	                    (long long int)loc_id,
	                    hdf5_get_objname (loc_id),
	                    (long long)idx);

	if (name == NULL) {
		size = 0;
	}
	ssize_t len = H5Lget_name_by_idx (loc_id, ".",
	                                  H5_INDEX_NAME, H5_ITER_INC,
	                                  idx,
	                                  name, size,
	                                  H5P_DEFAULT);
	if (len < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot get name of object %llu in group '%s'.",
			(unsigned long long)idx,
			hdf5_get_objname (loc_id));
	H5_RETURN (len);
}

/****** D a t a s p a c e ****************************************************/
/*!
   H5Screate_simple wrapper.
 */
static inline hid_t
hdf5_create_dataspace (
        const int rank,
        const hsize_t* dims,
        const hsize_t* maxdims
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "rank=%d, dims[0]=%lld,...,maxdims[0]=%lld,...",
	                    rank,
			    (dims ? (long long)dims[0] : -1),
			    (maxdims ? (long long)maxdims[0] : -1)
		);
	hid_t dataspace_id = H5Screate_simple (rank, dims, maxdims);
	if (dataspace_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot create dataspace with rank %d.",
			rank);
	H5_RETURN (dataspace_id);
}

static inline hid_t
hdf5_create_dataspace_scalar (
        void
        ) {
	HDF5_WRAPPER_ENTER (hid_t, "%s", "void");
	hid_t dataspace_id = H5Screate (H5S_SCALAR);
	if (dataspace_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot create scalar dataspace.");
	H5_RETURN (dataspace_id);
}

static inline h5_err_t
hdf5_select_hyperslab_of_dataspace (
        hid_t space_id,
        H5S_seloper_t op,
        const hsize_t* start,
        const hsize_t* stride,
        const hsize_t* count,
        const hsize_t* block
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t, "%lld", (long long int)space_id);
	herr_t herr = H5Sselect_hyperslab (
	        space_id,
	        op,
	        start,
	        stride,
	        count,
	        block);
	if (herr < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot set select hyperslap region or add the "
			"specified region");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hdf5_select_elements_of_dataspace (
        hid_t space_id,
        H5S_seloper_t op,
        hsize_t nelems,
        const hsize_t* indices
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t, "%lld", (long long int)space_id);
	herr_t herr;
	if ( nelems > 0 ) {
		herr = H5Sselect_elements (
		        space_id,
		        op,
		        (size_t)nelems,
		        indices);
	} else {
		herr = H5Sselect_none ( space_id );
	}
	if (herr < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot set select hyperslap region or add the "
			"specified region");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hdf5_select_none (
        hid_t space_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "%lld",
	                    (long long int)space_id);
	herr_t herr = H5Sselect_none (space_id);
	if (herr < 0) {
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Selection for writing zero-length data failed");
	}
	H5_RETURN (H5_SUCCESS);
}

static inline h5_ssize_t
hdf5_get_selected_npoints_of_dataspace (
        hid_t space_id
        ) {
	HDF5_WRAPPER_ENTER (h5_ssize_t, "%lld", (long long int)space_id);
	hssize_t size = H5Sget_select_npoints (space_id);
	if (size < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot determine number of "
			"selected elements in dataspace.");
	H5_RETURN (size);
}

static inline h5_ssize_t
hdf5_get_npoints_of_dataspace (
        hid_t space_id
        ) {
	HDF5_WRAPPER_ENTER (h5_ssize_t, "%lld", (long long int)space_id);
	hssize_t size = H5Sget_simple_extent_npoints (space_id);
	if (size < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot determine number of"
			"elements in dataspace.");
	H5_RETURN (size);
}

static inline int
hdf5_get_dims_of_dataspace (
        hid_t space_id,
        hsize_t* dims,
        hsize_t* maxdims
        ) {
	HDF5_WRAPPER_ENTER (int, "%lld", (long long int)space_id);
	int rank = H5Sget_simple_extent_dims (space_id, dims, maxdims);
	if (rank < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot determine rank of dataspace.");
	H5_RETURN (rank);
}


/*!
   H5Sclose() wrapper
 */
static inline h5_err_t
hdf5_close_dataspace (
        const hid_t dataspace_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t, "dataspace=%lld", (long long int)dataspace_id);
	if (dataspace_id <= 0 || dataspace_id == H5S_ALL)
		H5_LEAVE (H5_SUCCESS);

	herr_t herr = H5Sclose (dataspace_id);
	if (herr < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot terminate access to dataspace!");

	H5_RETURN (H5_SUCCESS);
}

/****** D a t a s e t ********************************************************/
/*!
   H5Dopen wrapper.
 */
static inline hid_t
hdf5_open_dataset_by_name (
        const hid_t loc_id,
        const char* const dataset_name
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "loc_id=%lld (%s), dataset_name='%s'",
	                    (long long int)loc_id,
	                    hdf5_get_objname (loc_id),
	                    dataset_name);
	hid_t dataset_id = H5Dopen (
	        loc_id,
	        dataset_name,
	        H5P_DEFAULT);
	if (dataset_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot open dataset '%s/%s'.",
			hdf5_get_objname (loc_id),
			dataset_name);
	H5_RETURN (dataset_id);
}

/*!
   H5Dcreate() wrapper.
 */
static inline hid_t
hdf5_create_dataset (
        hid_t loc_id,
        const char* dataset_name,
        const hid_t type_id,
        const hid_t dataspace_id,
        const hid_t create_proplist
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "loc_id=%lld (%s), dataset_name='%s'"
			    ", dataspace_id=%lld, create_proplist=%lld",
	                    (long long int)loc_id,
	                    hdf5_get_objname (loc_id),
	                    dataset_name,
			    (long long int)dataspace_id,
			    (long long int)create_proplist
		);
	hid_t dataset_id = H5Dcreate (
	        loc_id,
	        dataset_name,
	        type_id,
	        dataspace_id,
	        H5P_DEFAULT,
	        create_proplist,
	        H5P_DEFAULT);
	if (dataset_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot create dataset '%s/%s'",
			hdf5_get_objname (loc_id),
			dataset_name);
	H5_RETURN (dataset_id);
}

/*!
   H5Dclose() wrapper.
 */
static inline h5_err_t
hdf5_close_dataset (
        const hid_t dataset_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "dataset_id=%lld (%s)",
	                    (long long int)dataset_id,
	                    hdf5_get_objname (dataset_id));
	if (dataset_id < 0)
		H5_LEAVE (H5_SUCCESS);

	if (H5Dclose (dataset_id) < 0) {
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Close of dataset '%s' failed.",
			hdf5_get_objname (dataset_id));
	}
	H5_RETURN (H5_SUCCESS);
}

/*!
   H5Dget_space() wrapper.
 */
static inline hid_t
hdf5_get_dataset_space (
        const hid_t dataset_id
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "dataset_id=%lld (%s)",
	                    (long long int)dataset_id,
	                    hdf5_get_objname(dataset_id));
	hid_t dataspace_id = H5Dget_space (dataset_id);
	if (dataspace_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot get dataspace for dataset '%s'.",
			hdf5_get_objname (dataset_id));
	H5_RETURN (dataspace_id);
}

/*!
   H5Dwrite() wrapper
 */
static inline h5_err_t
hdf5_write_dataset (
        const hid_t dataset_id,
        const hid_t type_id,
        const hid_t memspace_id,
        const hid_t diskspace_id,
        const hid_t xfer_prop,
        const void* buf
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "dataset_id=%lld (%s) type_id=%lld",
	                    (long long int)dataset_id,
	                    hdf5_get_objname(dataset_id),
	                    (long long int)type_id);

	herr_t herr = H5Dwrite (
	        dataset_id,
	        type_id,
	        memspace_id,
	        diskspace_id,
	        xfer_prop,
	        buf);
	if (herr < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Write to dataset '%s' failed.",
			hdf5_get_objname (dataset_id));

	H5_RETURN (H5_SUCCESS);
}

/*
   H5Dread() write
 */
static inline h5_err_t
hdf5_read_dataset (
        const hid_t dataset_id,
        const hid_t type_id,
        const hid_t memspace_id,
        const hid_t diskspace_id,
        const hid_t xfer_prop,
        void* const buf ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "dataset_id=%lld (%s) type_id=%lld",
	                    (long long int)dataset_id,
	                    hdf5_get_objname(dataset_id),
	                    (long long int)type_id);
	herr_t herr = H5Dread (
	        dataset_id,
	        type_id,
	        memspace_id,
	        diskspace_id,
	        xfer_prop,
	        buf);
	if (herr < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Error reading dataset '%s'.",
			hdf5_get_objname (dataset_id));

	H5_RETURN (H5_SUCCESS);
}

static inline hid_t
hdf5_get_dataset_type (
        const hid_t dataset_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "dataset_id=%lld (%s)",
	                    (long long int)dataset_id,
	                    hdf5_get_objname(dataset_id));
	hid_t datatype_id = H5Dget_type (dataset_id);
	if (datatype_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot determine dataset type.");

	H5_RETURN (datatype_id);
}

static inline h5_err_t
hdf5_set_dataset_extent (
        hid_t dataset_id,
        const hsize_t* size
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "dataset_id=%lld (%s), size=%llu",
	                    (long long int)dataset_id,
	                    hdf5_get_objname(dataset_id),
	                    (long long unsigned int)*size);
	if (H5Dset_extent(dataset_id, size) < 0) {
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Changing size of dataset '%s' dimensions failed.",
			hdf5_get_objname (dataset_id));
	}
	H5_RETURN (H5_SUCCESS);
}

static inline h5_ssize_t
hdf5_get_npoints_of_dataset (
        hid_t dataset_id
        ) {
	HDF5_WRAPPER_ENTER (h5_ssize_t,
	                    "dataset_id=%lld (%s)",
	                    (long long int)dataset_id,
	                    hdf5_get_objname(dataset_id));
	hid_t dspace_id;
	hsize_t size;
	TRY (dspace_id = hdf5_get_dataset_space (dataset_id));
	TRY (size = hdf5_get_npoints_of_dataspace (dspace_id));
	TRY (hdf5_close_dataspace (dspace_id));
	H5_RETURN (size);
}

static inline h5_ssize_t
hdf5_get_npoints_of_dataset_by_name (
        const hid_t loc_id,
        const char* const name
        ) {
	HDF5_WRAPPER_ENTER (h5_ssize_t,
	                    "loc_id=%lld (%s), name='%s'",
	                    (long long int)loc_id,
	                    hdf5_get_objname(loc_id),
	                    name);
	hid_t dset_id;
	hsize_t size;
	TRY (dset_id = hdf5_open_dataset_by_name (loc_id, name));
	TRY (size = hdf5_get_npoints_of_dataset (dset_id));
	TRY (hdf5_close_dataset (dset_id));
	H5_RETURN (size);
}

/****** D a t a t y p e ******************************************************/
/*!
   H5Tarray_create() write
 */
static inline char_p
hdf5_get_type_name (
        hid_t type
        ) {
	HDF5_WRAPPER_ENTER (char_p,
			   "type=%lld",
			   (long long int)type);
	H5T_class_t tclass;
	int size;
	TRY (tclass = H5Tget_class (type));
	TRY (size = H5Tget_size (type));

	switch (tclass){
	case H5T_INTEGER:
		if (size==8) {
			ret_value = "H5_INT64_T";
		} else if (size==4) {
		        ret_value = "H5_INT32_T";
		} else if (size==2) {
		        ret_value = "H5_INT16_T";
		}
		break;
	case H5T_FLOAT:
		if (size==8) {
			ret_value = "H5_FLOAT64_T";
		} else if (size==4) {
			ret_value = "H5_FLOAT32_T";
		}
		break;
	case H5T_STRING:
		ret_value = "H5_STRING_T";
		break;
	default:
		ret_value = "unknown";
	}
	H5_RETURN (ret_value);
}

static inline const char*
get_class_type_name (
        const hid_t class_id
        ) {
	const char* const map[] = {
		[H5T_INTEGER]   = "H5T_INTEGER",
		[H5T_FLOAT]     = "H5T_FLOAT",
		[H5T_TIME]      = "H5T_TIME",
		[H5T_STRING]    = "H5T_STRING",
		[H5T_BITFIELD]  = "H5T_BITFIELD",
		[H5T_OPAQUE]    = "H5T_OPAQUE",
		[H5T_COMPOUND]  = "H5T_COMPOUND",
		[H5T_REFERENCE] = "H5T_REFERENCE",
		[H5T_ENUM]      = "H5T_ENUM",
		[H5T_VLEN]      = "H5T_VLEN",
		[H5T_ARRAY]     = "H5T_ARRAY"
	};
	if (class_id < 0 || class_id >= H5T_NCLASSES) {
		return ("[unknown]");
	}
	return map[class_id];
}

static inline hid_t
hdf5_create_array_type (
        const hid_t base_type_id,
        const int rank,
        const hsize_t* dims
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "base_type_id=%lld (%s), rank=%d",
	                    (long long int)base_type_id,
	                    hdf5_get_type_name (base_type_id),
	                    rank);
	hid_t type_id = H5Tarray_create (base_type_id, rank, dims);
	if (type_id < 0) {
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Can't create array datatype object with base "
			"type %s and rank %d",
			hdf5_get_type_name (base_type_id),
			rank);
	}
	H5_RETURN (type_id);
}

static inline hid_t
hdf5_create_type (
        H5T_class_t class,
        const size_t size
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "class=%d (%s)",
	                    class,
	                    get_class_type_name (class));
	hid_t type_id = H5Tcreate (class, size);
	if (type_id < 0) {
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Can't create datatype object of class %s.",
			get_class_type_name (class));
	}
	H5_RETURN (type_id);
}

static inline hid_t
hdf5_create_string_type(
        const hsize_t len
        ) {
  HDF5_WRAPPER_ENTER (hid_t, "len = %llu", (long long unsigned)len);
	hid_t type_id = H5Tcopy (H5T_C_S1);
	if (type_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Can't duplicate C string type.");

	herr_t herr = H5Tset_size (type_id, len);
	if (herr < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Can't set length of C string type.");
	H5_RETURN (type_id);
}

static inline h5_err_t
hdf5_insert_type (
        hid_t type_id,
        const char* name,
        size_t offset,
        hid_t field_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "type_id=%lld, name='%s'",
			    (long long int)type_id, name);
	herr_t herr = H5Tinsert (type_id, name, offset, field_id);
	if (herr < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Can't insert field %s to compound datatype.",
			name);
	H5_RETURN (H5_SUCCESS);
}

static inline H5T_class_t
hdf5_get_class_type (
        hid_t dtype_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t, "dtype_id=%lld", (long long int)dtype_id);
        H5T_class_t class = H5Tget_class (dtype_id);
	if (class < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Can't determine class of type %lld.",
			(long long int)dtype_id);
	H5_RETURN (class);
}

static inline h5_ssize_t
hdf5_get_sizeof_type (
        hid_t dtype_id
        ) {
	HDF5_WRAPPER_ENTER (h5_ssize_t,
			    "dtype_id=%lld",
			    (long long int)dtype_id);
        h5_ssize_t size = H5Tget_size (dtype_id);
        if (size == 0) {
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Can't determine size of type %lld.",
			(long long int)dtype_id);
        }
	H5_RETURN (size);
}


static inline h5_err_t
hdf5_close_type (
        hid_t dtype_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "dtype_id=%lld",
			    (long long int)dtype_id);
	herr_t herr = H5Tclose (dtype_id);
	if (herr < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot release datatype.");
	H5_RETURN (H5_SUCCESS);
}

/****** P r o p e r t y ******************************************************/

static inline hid_t
hdf5_create_property (
        hid_t cls_id
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
			    "cls_id=%lld",
			    (long long int)cls_id);
	hid_t prop_id = H5Pcreate (cls_id);
	if (prop_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot create property list.");
	H5_RETURN (prop_id);
}

/*!
   H5Dget_create_plist() wrapper.
 */
static inline hid_t
hdf5_get_dataset_create_plist (
        const hid_t dataset_id
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
			    "dataset_id=%lld (%s)",
	                    (long long int)dataset_id,
	                    hdf5_get_objname (dataset_id));
	hid_t plist_id = H5Dget_create_plist (dataset_id);
	if (plist_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot get create properties for dataset '%s'.",
			hdf5_get_objname (dataset_id));
	H5_RETURN (plist_id);
}

static inline h5_err_t
hdf5_set_chunk_property (
        hid_t plist,
        int rank,
        hsize_t* dims
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "plist=%lld, rank=%d, dims[0]=%llu ...",
	                    (long long int)plist, rank, (long long unsigned)dims[0]);
	if (H5Pset_chunk (plist, rank, dims) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot add chunking property to list.");

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hdf5_get_chunk_property (
        hid_t plist,
        int rank,
        hsize_t* dims
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "plist=%lld, rank=%d",
			    (long long int)plist, rank);
	if (H5Pget_chunk (plist, rank, dims) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot get chunking property from list.");

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hdf5_set_layout_property (
        hid_t plist,
        H5D_layout_t layout
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "plist=%lld",
			    (long long int)plist);
	if (H5Pset_layout (plist, layout) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot add layout property to list.");

	H5_RETURN (H5_SUCCESS);
}

#ifdef H5_HAVE_PARALLEL
static inline h5_err_t
hdf5_set_fapl_mpio_property (
        hid_t fapl_id,
        MPI_Comm comm,
        MPI_Info info
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "fapl_id=%lld, comm=..., info=...",
	                    (long long int)fapl_id);
	if (H5Pset_fapl_mpio (fapl_id, comm, info) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot store IO communicator information to the "
			"file access property list.");
	H5_RETURN (H5_SUCCESS);
}

#if H5_VERSION_LE(1,8,12)
static inline h5_err_t
hdf5_set_fapl_mpiposix_property (
        hid_t fapl_id,
        MPI_Comm comm,
        hbool_t use_gpfs
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "fapl_id=%lld, comm=..., use_gpfs=%d",
	                    (long long int)fapl_id, (int)use_gpfs);
	if ( H5Pset_fapl_mpiposix (fapl_id, comm, use_gpfs) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot store IO communicator information to"
			" the file access property list.");
	H5_RETURN (H5_SUCCESS);
}
#endif

static inline h5_err_t
hdf5_set_dxpl_mpio_property (
        hid_t dxpl_id,
        H5FD_mpio_xfer_t mode
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "dxpl_id=%lld, mode=%d",
			    (long long int)dxpl_id, (int)mode);
	if (H5Pset_dxpl_mpio (dxpl_id, mode) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot store IO communicator information to"
			" the dataset transfer property list.");
	H5_RETURN (H5_SUCCESS);
}
#endif

static inline h5_err_t
hdf5_set_mdc_property (
        hid_t fapl_id,
        H5AC_cache_config_t *config
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "fapl_id=%lld, config=%p",
			    (long long int)fapl_id, config);
	if (H5Pset_mdc_config (fapl_id, config) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot store metadata cache configuration in"
			" the file access property list.");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hdf5_get_mdc_property (
        hid_t fapl_id,
        H5AC_cache_config_t *config
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "fapl_id=%lld, config=%p",
			    (long long int)fapl_id, config);
	if (H5Pget_mdc_config (fapl_id, config) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot get metadata cache configuration in"
			" the file access property list.");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hdf5_set_btree_ik_property (
        const hid_t fcpl_id,
        const hsize_t btree_ik
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "fapl_id=%lld, btree_ik=%llu",
	                    (long long int)fcpl_id,
			    (long long unsigned)btree_ik);
	if (H5Pset_istore_k (fcpl_id, btree_ik) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot set btree size in the "
			"file access property list.");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hdf5_set_alignment_property (
        hid_t plist,
        hsize_t threshold,
        hsize_t alignment
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "plist=%lld, threshold=%llu, alignment=%llu",
	                    (long long int)plist,
			    (long long unsigned)threshold,
			    (long long unsigned)alignment);
	if (H5Pset_alignment (plist, threshold, alignment) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot set alignment property to %llu "
			"and threshold %llu",
			(long long unsigned)alignment,
			(long long unsigned)threshold);
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hdf5_set_meta_block_size (
        hid_t fapl_id,
        hsize_t size
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "fapl_id=%lld, size=%llu",
	                    (long long int)fapl_id,
			    (long long unsigned)size);
	if (H5Pset_meta_block_size (fapl_id, size) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot set meta block size property to %llu",
			(long long unsigned)size);
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hdf5_set_fapl_core (
        hid_t fapl_id,
        size_t increment,
        hbool_t backing_store
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "fapl_id=%lld, size=%zu, backing_store=%d",
			    (long long int)fapl_id, increment, backing_store);
        if (H5Pset_fapl_core (fapl_id, increment, backing_store))
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot set property to use the H5FD_CORE driver.");
        H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hdf5_close_property (
        hid_t prop
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "prop=%lld",
			    (long long int)prop);
	if (H5Pclose (prop) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot close property %lld.", (long long)prop);
	H5_RETURN (H5_SUCCESS);
}

/****** F i l e **************************************************************/
static inline h5_err_t
hdf5_close_object (
        hid_t object_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "object_id=%lld",
			    (long long int)object_id);
	if (H5Oclose (object_id) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot close object %lld.", (long long)object_id);
	H5_RETURN (H5_SUCCESS);
}

static inline ssize_t
hdf5_get_object_count (
	hid_t file_id,
	unsigned int types
	) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "file_id=%lld (%s), types=%u",
	                    (long long int)file_id, hdf5_get_objname (file_id),
			    types);
	if ((ret_value = H5Fget_obj_count (file_id, types)) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot get open object count for file %lld.",
			(long long)file_id);
	H5_RETURN (ret_value);

}

static inline ssize_t
hdf5_get_object_ids (
	hid_t file_id,
	unsigned int types,
	size_t max_objs,
	hid_t *obj_id_list
	) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "file_id=%lld (%s), "
			    "type=%u, max_objs=%zd, "
			    "obj_id_list=%p",
	                    (long long int)file_id, hdf5_get_objname (file_id),
			    types, max_objs, obj_id_list);
	if ((ret_value = H5Fget_obj_ids (file_id, types, max_objs, obj_id_list)) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot get object id list for file %lld.",
			(long long)file_id);
	H5_RETURN (ret_value);
}

static inline h5_err_t
hdf5_close_file (
        hid_t file_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
			    "file_id=%lld (%s)",
	                    (long long int)file_id, hdf5_get_objname (file_id));
	if (H5Fclose (file_id) < 0) {
		// close open objects used by file
		ssize_t max_objs;
		unsigned int types = H5F_OBJ_DATASET|H5F_OBJ_GROUP|H5F_OBJ_DATATYPE;
		TRY (max_objs = hdf5_get_object_count (file_id, types));
		hid_t* obj_id_list = (hid_t*)malloc (sizeof (hid_t) * max_objs);
		TRY (hdf5_get_object_ids (file_id, types, max_objs, obj_id_list));
		for (ssize_t i = 0; i < max_objs; i++) {
			hid_t object_id = obj_id_list [i];
			h5_debug ("Open object: %lld", (long long)object_id);
#if defined(H5Oget_info_vers) && H5Oget_info_vers > 1
			H5O_info_t object_info;
			if (H5Oget_info (object_id, &object_info, H5O_INFO_ALL) < 0)
				continue;
#else
			H5O_info_t object_info;
			if (H5Oget_info (object_id, &object_info) < 0)
				continue;
#endif
			switch (object_info.type) {
			case H5O_TYPE_GROUP:
			case H5O_TYPE_DATASET:
			case H5O_TYPE_NAMED_DATATYPE:
				TRY (hdf5_close_object (object_id));
			default:
				// we cannot close other objects
				break;
			}
		}
		free (obj_id_list);
		// try again
		if (H5Fclose (file_id) < 0)
			goto return_with_error;
	}
	H5_RETURN (H5_SUCCESS);

return_with_error:
	H5_RETURN_ERROR (
		H5_ERR_HDF5,
		"Cannot close file '%s'.",
		hdf5_get_objname (file_id));
}

static inline h5_err_t
hdf5_close (
	void
	) {
	HDF5_WRAPPER_ENTER (h5_err_t, "%s", "void");
	if (H5close () < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot close HDF5 library.");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
hdf5_flush (
        hid_t obj_id,
        H5F_scope_t scope
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "obj_id=%lld (%s)",
	                    (long long int)obj_id,
	                    hdf5_get_objname (obj_id));
	if (H5Fflush (obj_id, scope) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot flush data \"%s\".",
			hdf5_get_objname (obj_id));
	H5_RETURN (H5_SUCCESS);
}

/****** E r r o r h a n d l i n g ********************************************/

static inline h5_err_t
hdf5_set_errorhandler (
        hid_t estack_id,
        H5E_auto_t func,
        void* client_data
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "estack_id=%lld, func=%p, client_data=%p",
	                    (long long int)estack_id, func, client_data);
	if (H5Eset_auto (estack_id, func, client_data) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot initialize H5.");
	H5_RETURN (H5_SUCCESS);
}

/****** A t t r i b u t e ****************************************************/
static inline hid_t
hdf5_attribute_exists (
        hid_t loc_id,
        const char* attrib_name
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "loc_id=%lld (%s), attr_name='%s'",
	                    (long long int)loc_id,
			    hdf5_get_objname (loc_id), attrib_name);
	htri_t exists = H5Aexists (loc_id, attrib_name);
	if (exists < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot query attribute '%s' of '%s'.",
			attrib_name,
			hdf5_get_objname (loc_id));
	H5_RETURN (exists);
}

static inline hid_t
hdf5_open_attribute_by_name (
        hid_t loc_id,
        const char* attrib_name
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "loc_id=%lld (%s), attr_name='%s'",
	                    (long long int)loc_id,
			    hdf5_get_objname (loc_id), attrib_name);
	hid_t attrib_id = H5Aopen (loc_id, attrib_name, H5P_DEFAULT);
	if (attrib_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot open attribute '%s' of '%s'.",
			attrib_name,
			hdf5_get_objname (loc_id));
	H5_RETURN (attrib_id);
}

static inline hid_t
hdf5_open_attribute_by_idx (
        hid_t loc_id,
        unsigned int idx
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "loc_id=%lld (%s), idx=%u",
	                    (long long int)loc_id, hdf5_get_objname (loc_id), idx);
	hid_t attr_id = H5Aopen_idx (loc_id, idx);
	if (attr_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot open attribute '%u' of '%s'.",
			idx,
			hdf5_get_objname (loc_id));
	H5_RETURN (attr_id);
}

static inline hid_t
hdf5_create_attribute (
        hid_t loc_id,
        const char* attr_name,
        hid_t type_id,
        hid_t space_id,
        hid_t acpl_id,
        hid_t aapl_id
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "loc_id=%lld (%s), attr_name='%s', type_id=%lld",
	                    (long long int)loc_id, hdf5_get_objname (loc_id),
	                    attr_name, (long long int)type_id);
	hid_t attr_id = H5Acreate (
	        loc_id,
	        attr_name,
	        type_id,
	        space_id,
	        acpl_id,
	        aapl_id);
	if (attr_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot create attribute '%s' for '%s'.",
			attr_name,
			hdf5_get_objname (loc_id));
	H5_RETURN (attr_id);
}

static inline h5_err_t
hdf5_read_attribute (
        hid_t attr_id,
        hid_t mem_type_id,
        void* buf
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "attr_id=%lld (%s), mem_type_id=%lld, buf=%p",
	                    (long long int)attr_id, hdf5_get_objname (attr_id),
	                    (long long int)mem_type_id, buf);
	if (H5Aread (attr_id, mem_type_id, buf) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot read attribute '%s'.",
			hdf5_get_objname (attr_id));
	H5_RETURN (H5_SUCCESS);
}

/*
   Wrapper for H5Awrite.
 */
static inline h5_err_t
hdf5_write_attribute (
        hid_t attr_id,
        hid_t mem_type_id,
        const void* buf
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "attr_id=%lld (%s), mem_type_id=%lld, buf=%p",
	                    (long long int)attr_id, hdf5_get_objname (attr_id),
	                    (long long int)mem_type_id, buf);
	if (H5Awrite (attr_id, mem_type_id, buf) < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot write attribute '%s'.",
			hdf5_get_objname (attr_id));

	H5_RETURN (H5_SUCCESS);
}

static inline h5_ssize_t
hdf5_get_attribute_name (
        hid_t attr_id,
        size_t buf_size,
        char *buf
        ) {
	HDF5_WRAPPER_ENTER (h5_ssize_t,
	                    "attr_id=%lld (%s), buf_size=%llu, buf=%p",
	                    (long long int)attr_id, hdf5_get_objname (attr_id),
	                    (unsigned long long)buf_size, buf);
	ssize_t size = H5Aget_name ( attr_id, buf_size, buf );
	if (size < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"%s",
			"Cannot get attribute name." );
	H5_RETURN ((h5_size_t)size);
}

static inline hid_t
hdf5_get_attribute_type (
        hid_t attr_id
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "attr_id=%lld (%s)",
	                    (long long int)attr_id, hdf5_get_objname (attr_id));
	hid_t datatype_id = H5Aget_type (attr_id);
	if (datatype_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot get type of attribute '%s'.",
			hdf5_get_objname (attr_id));
	H5_RETURN (datatype_id);
}

static inline hid_t
hdf5_get_attribute_dataspace (
        hid_t attr_id
        ) {
	HDF5_WRAPPER_ENTER (hid_t,
	                    "attr_id=%lld (%s)",
	                    (long long int)attr_id, hdf5_get_objname (attr_id));
	hid_t space_id = H5Aget_space (attr_id);
	if (space_id < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot get dataspace of attribute '%s'.",
			hdf5_get_objname (attr_id));
	H5_RETURN (space_id);
}

static inline int
hdf5_get_num_attribute (
        hid_t loc_id
        ) {
	HDF5_WRAPPER_ENTER (int,
	                    "loc_id=%lld (%s)",
	                    (long long int)loc_id, hdf5_get_objname (loc_id));
	int num = H5Aget_num_attrs (loc_id);
	if (num < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot get number of attributes of '%s'.",
			hdf5_get_objname (loc_id));
	H5_RETURN (num);
}

static inline herr_t
hdf5_delete_attribute (
        hid_t loc_id,
        const char* attrib_name
        ) {
	HDF5_WRAPPER_ENTER (herr_t,
	                    "loc_id=%lld (%s), attr_name='%s'",
	                    (long long int)loc_id, hdf5_get_objname (loc_id), attrib_name);
	herr_t herr = H5Adelete (loc_id, attrib_name);
	if (herr < 0)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot delete attribute '%s' of '%s'.",
			attrib_name,
			hdf5_get_objname (loc_id));
	H5_RETURN (herr);
}

static inline h5_err_t
hdf5_close_attribute (
        hid_t attr_id
        ) {
	HDF5_WRAPPER_ENTER (h5_err_t,
	                    "attr_id=%lld (%s)",
	                    (long long int)attr_id, hdf5_get_objname (attr_id));
	if (H5Aclose (attr_id))
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Cannot close attribute '%s'.",
			hdf5_get_objname (attr_id));

	H5_RETURN (H5_SUCCESS);
}

#endif
