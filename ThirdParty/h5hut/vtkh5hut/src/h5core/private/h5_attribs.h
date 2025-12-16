/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_ATTRIBS_H
#define __PRIVATE_H5_ATTRIBS_H

#include "private/h5_types.h"
#include "private/h5_model.h"
#include "private/h5_hdf5.h"


static inline hid_t
h5priv_get_normalized_attribute_type (
	hid_t attr_id
	) {
	H5_PRIV_FUNC_ENTER (
		hid_t,
		"attr_id=%lld (%s)",
		(long long int)attr_id, hdf5_get_objname (attr_id));
	TRY (ret_value = hdf5_get_attribute_type (attr_id));
	TRY (ret_value = h5priv_normalize_type (ret_value));
	H5_RETURN (ret_value);
}

static inline h5_err_t
h5priv_read_attrib (
	const hid_t id,			/*!< HDF5 object ID */
	const char* attrib_name,	/*!< name of HDF5 attribute to read */
	const h5_types_t attrib_type,	/*!< H5hut enum type of attribute */
	void* const attrib_value	/*!< OUT: attribute value */
	) {
	H5_PRIV_API_ENTER (h5_err_t,
			   "id=%lld, attrib_name='%s', attrib_type=%lld, "
			   "attrib_value=%p",
			   (long long int)id,
			   attrib_name,
			   (long long int)attrib_type,
			   attrib_value);
	hid_t attrib_id;
	TRY (attrib_id = hdf5_open_attribute_by_name (id, attrib_name));
	hid_t mem_type;
	hid_t space_id;
	/*
	  attrib_type -> normalized HDF5 type
	  determine file type of attribute
	  compare normalized types
	  if not equal:
		error
	  if attribute type is string:
		set mem type to file type
	  else
		set mem type to normalized attrib_type

	 */	
	hid_t normalized_type;
	TRY (normalized_type = h5priv_map_enum_to_normalized_type (attrib_type));
	hid_t file_type;
	TRY (file_type = hdf5_get_attribute_type (attrib_id));
	hid_t normalized_file_type;
	TRY (normalized_file_type = h5priv_normalize_type (file_type));
	if (normalized_file_type != normalized_type)
		H5_RETURN_ERROR (
			H5_ERR_HDF5,
			"Attribute '%s' has type '%s' but "
			"was requested as '%s'.",
			attrib_name,
			hdf5_get_type_name (normalized_file_type),
			hdf5_get_type_name (normalized_type));
	if (normalized_type == H5_STRING) {
		mem_type = file_type;
	} else {
		mem_type = normalized_type;
	}
	TRY (space_id = hdf5_get_attribute_dataspace (attrib_id));
	TRY (hdf5_read_attribute (attrib_id, mem_type, attrib_value));
	TRY (hdf5_close_dataspace(space_id));
	TRY (hdf5_close_attribute (attrib_id));
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_write_attrib (
        const hid_t id,                 /*!< HDF5 object ID */
        const char* attrib_name,        /*!< name of HDF5 attribute to write */
        const h5_types_t attrib_type,   /*!< type of attribute */
        const void* attrib_value,       /*!< value of attribute */
        const hsize_t attrib_nelem      /*!< number of elements (dimension) */
        ) {
	H5_PRIV_API_ENTER (h5_err_t,
	                   "id=%lld, attrib_name='%s', attrib_type=%lld, "
	                   "attrib_value=%p, attrib_nelem=%llu",
	                   (long long int)id,
	                   attrib_name,
	                   (long long int)attrib_type,
	                   attrib_value,
	                   (long long unsigned)attrib_nelem);
	hid_t space_id;
	hid_t attrib_id;
	hid_t hdf5_type;
	if (attrib_type == H5_STRING_T) {
		TRY (hdf5_type = hdf5_create_string_type (attrib_nelem));
		TRY (space_id = hdf5_create_dataspace_scalar ());
	} else {
		hdf5_type = h5priv_map_enum_to_normalized_type (attrib_type);
		TRY (space_id = hdf5_create_dataspace (1, &attrib_nelem, NULL));
	}
	h5_err_t exists;
	TRY (exists = hdf5_attribute_exists (id, attrib_name));
	if (exists) {
		TRY (hdf5_delete_attribute (id, attrib_name));
	}
	TRY (attrib_id = hdf5_create_attribute (
	             id,
	             attrib_name,
	             hdf5_type,
	             space_id,
	             H5P_DEFAULT, H5P_DEFAULT));
	TRY (hdf5_write_attribute (attrib_id, hdf5_type, attrib_value));

	if (attrib_type == H5_STRING_T) {
		TRY (hdf5_close_type (hdf5_type));
	}
	TRY (hdf5_close_attribute (attrib_id));
	TRY (hdf5_close_dataspace (space_id));

	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_append_attrib (
        const hid_t id,                 /*!< HDF5 object ID */
        const char* attrib_name,        /*!< name of HDF5 attribute to write */
        const h5_types_t attrib_type,   /*!< type of attribute */
        const void* attrib_value,       /*!< value of attribute */
        const hsize_t attrib_nelem      /*!< number of elements (dimension) */
        ) {
	H5_PRIV_API_ENTER (h5_err_t,
	                   "id=%lld, attrib_name='%s', attrib_type=%lld, "
	                   "attrib_value=%p, attrib_nelem=%llu",
	                   (long long int)id,
	                   attrib_name,
	                   (long long int)attrib_type,
	                   attrib_value,
	                   (long long unsigned)attrib_nelem);
	h5_err_t exists;
	TRY (exists = hdf5_attribute_exists (id, attrib_name));
	if (exists) {
		H5_RETURN_ERROR (
			H5_ERR,
			"Cannot overwrite attribute %s/%s",
			hdf5_get_objname (id), attrib_name);
	}
	H5_RETURN (
		h5priv_write_attrib (
			id,
			attrib_name,
			attrib_type,
			attrib_value,
			attrib_nelem));
}


/*
  This is a helper function for 

  - h5priv_get_attrib_info_by_name()
  - h5priv_get_attrib_info_by_idx()
 */
static inline h5_err_t
get_attrib_info (
	hid_t attrib_id,
	h5_int64_t* attrib_type,	/*!< OUT: H5 type of attribute */
	h5_size_t* attrib_nelem		/*!< OUT: number of elements */
	) {
        H5_INLINE_FUNC_ENTER (h5_err_t);
	hid_t datatype_id;
	TRY (datatype_id = hdf5_get_attribute_type (attrib_id));
	if (attrib_nelem) {
                if (h5priv_normalize_type (datatype_id) == H5_STRING) {
                        *attrib_nelem = H5Tget_size (datatype_id);
                } else {
                        hid_t space_id;
                        TRY (space_id = hdf5_get_attribute_dataspace (attrib_id));
                        TRY (*attrib_nelem = hdf5_get_npoints_of_dataspace (
				     space_id));
                        TRY (hdf5_close_dataspace (space_id));
                }
	}
	if (attrib_type) {
		TRY (*attrib_type = h5priv_map_hdf5_type_to_enum (datatype_id));
	}
	TRY (hdf5_close_attribute (attrib_id));
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_get_attrib_info_by_name (
	const hid_t id,			/*!< IN: HDF5 object ID */
	const char* const attrib_name,	/*!< IN: name of attribute */
	h5_int64_t* attrib_type,	/*!< OUT: H5 type of attribute */
	h5_size_t* attrib_nelem		/*!< OUT: number of elements */
	) {
	H5_PRIV_API_ENTER (h5_err_t,
			   "id=%lld, "
			   "attrib_name=%s,"
			   "attrib_type=%p, attrib_nelem=%p",
			   (long long int)id,
			   attrib_name,
			   attrib_type, attrib_nelem);
	hid_t attrib_id;
        TRY (attrib_id = hdf5_open_attribute_by_name (id, attrib_name));
        H5_RETURN (
		get_attrib_info (
			attrib_id, attrib_type, attrib_nelem));
}

static inline h5_err_t
h5priv_get_attrib_info_by_idx (
	const hid_t id,			/*!< HDF5 object ID */
	const h5_size_t attrib_idx,	/*!< index of attribute */
	char* attrib_name,		/*!< OUT: name of attribute */
	const h5_size_t len_attrib_name,/*!< buffer length */
	h5_int64_t* attrib_type,	/*!< OUT: H5 type of attribute */
	h5_size_t* attrib_nelem		/*!< OUT: number of elements */
	) {
	H5_PRIV_API_ENTER (h5_err_t,
			   "id=%lld, "
			   "attrib_idx=%llu, "
			   "attrib_name=%p, len_attrib_name=%llu, "
			   "attrib_type=%p, attrib_nelem=%p",
			   (long long int)id,
			   (long long unsigned)attrib_idx,
			   attrib_name,
			   (long long unsigned)len_attrib_name,
			   attrib_type, attrib_nelem);
	hid_t attrib_id;
	TRY (attrib_id = hdf5_open_attribute_by_idx (
		      id,
		      (unsigned int)attrib_idx));

	if (attrib_name) {
		TRY (hdf5_get_attribute_name (
			     attrib_id,
			     (size_t)len_attrib_name,
			     attrib_name));
	}
        H5_RETURN (
		get_attrib_info (
			attrib_id, attrib_type, attrib_nelem));
}

#endif
