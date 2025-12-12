/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#include "h5core/h5_log.h"
#include "h5core/h5_file_attribs.h"
#include "h5core/h5_step_attribs.h"

#include "private/h5_hdf5.h"
#include "private/h5_attribs.h"

h5_err_t
h5_has_file_attrib (
	const h5_file_t f_,
	const char* const attrib_name
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, "
			   "attrib_name='%s'",
			   f,
			   attrib_name);
	CHECK_FILEHANDLE (f);
	TRY (ret_value = hdf5_attribute_exists(f->root_gid, attrib_name));
	H5_RETURN (ret_value);
}
	
h5_err_t
h5_has_iteration_attrib (
	const h5_file_t f_,
	const char* const attrib_name
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, "
			   "attrib_name='%s'",
			   f,
			   attrib_name);
	check_iteration_handle_is_valid (f);
	TRY (ret_value = hdf5_attribute_exists (f->iteration_gid, attrib_name));
	H5_RETURN (ret_value);
}

h5_ssize_t
h5_get_num_file_attribs (
	const h5_file_t f_                      /*!< handle to open file */
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_ssize_t, "f=%p", f);
	CHECK_FILEHANDLE (f);
	TRY (ret_value = hdf5_get_num_attribute (f->root_gid));
	H5_RETURN (ret_value);
}

h5_ssize_t
h5_get_num_iteration_attribs (
	const h5_file_t f_                     	/*!< handle to open file */
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_ssize_t, "f=%p", f);
	check_iteration_handle_is_valid (f);
	TRY (ret_value = hdf5_get_num_attribute (f->iteration_gid));
	H5_RETURN (ret_value);
}

h5_err_t
h5_get_file_attrib_info_by_idx (
	const h5_file_t f_,			/*!< handle to open file */
	const h5_size_t attrib_idx,		/*!< index of attribute */
	char* const attrib_name,		/*!< OUT: name of attribute */
	const h5_size_t len_attrib_name,	/*!< buffer length */
	h5_int64_t* const attrib_type,		/*!< OUT: H5 type of attribute */
	h5_size_t* const attrib_nelem		/*!< OUT: number of elements */
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, "
			   "attrib_idx=%llu, "
                           "attrib_name=%p, len_attrib_name=%llu, "
			   "attrib_type=%p, attrib_nelem=%p",
			   f,
			   (long long unsigned)attrib_idx,
			   attrib_name, (long long unsigned)len_attrib_name,
			   attrib_type, attrib_nelem);
	CHECK_FILEHANDLE (f);
	TRY (ret_value = h5priv_get_attrib_info_by_idx (
			f->root_gid,
			attrib_idx,
			attrib_name, len_attrib_name,
			attrib_type, attrib_nelem));
	H5_RETURN (ret_value);
}

h5_err_t
h5_get_file_attrib_info_by_name (
	const h5_file_t f_,			/*!< IN: handle to open file */
	const char* const attrib_name,		/*!< IN: name of attribute */
	h5_int64_t* attrib_type,		/*!< OUT: H5 type of attribute */
	h5_size_t* attrib_nelem			/*!< OUT: number of elements */
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, "
			   "attrib_name=%s, "
			   "attrib_type=%p, attrib_nelem=%p",
			   f,
			   attrib_name,
			   attrib_type, attrib_nelem);
	CHECK_FILEHANDLE (f);
	TRY (ret_value = h5priv_get_attrib_info_by_name (
		     f->root_gid,
		     attrib_name,
		     attrib_type, attrib_nelem));
	H5_RETURN (ret_value);
}

h5_err_t
h5_get_iteration_attrib_info_by_idx (
	const h5_file_t f_,			/*!< handle to open file */
	const h5_size_t attrib_idx,		/*!< index of attribute */
	char* attrib_name,			/*!< OUT: name of attribute */
	const h5_size_t len_attrib_name,	/*!< buffer length */
	h5_int64_t* attrib_type,		/*!< OUT: H5 type of attribute */
	h5_size_t* attrib_nelem			/*!< OUT: number of elements */
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, "
			   "attrib_idx=%llu, "
			   "attrib_name=%p, len_attrib_name=%llu, "
			   "attrib_type=%p, attrib_nelem=%p",
			   f,
			   (long long unsigned)attrib_idx,
			   attrib_name, (long long unsigned)len_attrib_name,
			   attrib_type, attrib_nelem);
	check_iteration_handle_is_valid (f);
	TRY (ret_value = h5priv_get_attrib_info_by_idx (
		     f->iteration_gid,
		     attrib_idx,
		     attrib_name, len_attrib_name,
		     attrib_type, attrib_nelem));
	H5_RETURN (ret_value);
}

h5_err_t
h5_get_iteration_attrib_info_by_name (
	const h5_file_t f_,			/*!< handle to open file */
	const char* const attrib_name,		/*!< OUT: name of attribute */
	h5_int64_t* const attrib_type,		/*!< OUT: H5 type of attribute */
	h5_size_t* const attrib_nelem		/*!< OUT: number of elements */
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, "
			   "attrib_name=%p, "
			   "attrib_type=%p, attrib_nelem=%p",
			   f,
			   attrib_name,
			   attrib_type, attrib_nelem);
	check_iteration_handle_is_valid (f);
	TRY (ret_value = h5priv_get_attrib_info_by_name (
		     f->iteration_gid,
		     attrib_name,
		     attrib_type, attrib_nelem));
	H5_RETURN (ret_value);
}

h5_err_t
h5_read_file_attrib (
	const h5_file_t f_,
	const char* const attrib_name,
	const h5_types_t attrib_type,
	void* const attrib_value
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, attrib_name='%s', attrib_type=%lld, "
			   "attrib_value=%p",
			   f,
			   attrib_name,
			   (long long int)attrib_type,
			   attrib_value);
	CHECK_FILEHANDLE (f);
	TRY (ret_value = h5priv_read_attrib (
		     f->root_gid,
		     attrib_name,
		     attrib_type,
		     attrib_value));
	H5_RETURN (ret_value);
}

h5_err_t
h5_read_iteration_attrib (
	const h5_file_t f_,
	const char* const attrib_name,
	const h5_types_t attrib_type,
	void* const attrib_value
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, attrib_name='%s', attrib_type=%lld, "
			   "attrib_value=%p",
			   f,
			   attrib_name,
			   (long long int)attrib_type,
			   attrib_value);
	check_iteration_is_readable (f);
	
	TRY (ret_value = h5priv_read_attrib (
		     f->iteration_gid,
		     attrib_name,
		     attrib_type,
		     attrib_value));
	H5_RETURN (ret_value);
}

h5_err_t
h5_write_file_attrib (
	const h5_file_t f_,
	const char* const attrib_name,
	const h5_types_t attrib_type,
	const void* const attrib_value,
	const h5_size_t attrib_nelem
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, attrib_name='%s', attrib_type=%lld, "
			   "attrib_value=%p, attrib_nelem=%lld",
			   f,
			   attrib_name,
			   (long long int)attrib_type,
			   attrib_value,
			   (long long)attrib_nelem);
	CHECK_FILEHANDLE (f);
	CHECK_WRITABLE_MODE (f);
	if (is_appendonly (f)) {
		TRY (h5priv_append_attrib (
			     f->root_gid,
			     attrib_name,
			     attrib_type,
			     attrib_value,
			     attrib_nelem));
	} else {
		TRY (h5priv_write_attrib (
			     f->root_gid,
			     attrib_name,
			     attrib_type,
			     attrib_value,
			     attrib_nelem));
	}
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5_write_iteration_attrib (
	const h5_file_t f_,
	const char* const attrib_name,
	const h5_types_t attrib_type,
	const void* const attrib_value,
	const h5_size_t attrib_nelem
	) {
        h5_file_p f = (h5_file_p)f_;
	H5_CORE_API_ENTER (h5_err_t,
			   "f=%p, attrib_name='%s', attrib_type=%lld, "
			   "attrib_value=%p, attrib_nelem=%lld",
			   f,
			   attrib_name,
			   (long long int)attrib_type,
			   attrib_value,
			   (long long)attrib_nelem);
	check_iteration_is_writable (f);
	if (is_appendonly (f)) {
		TRY (h5priv_append_attrib (
			     f->iteration_gid,
			     attrib_name,
			     attrib_type,
			     attrib_value,
			     attrib_nelem));
	} else {
		TRY (h5priv_write_attrib (
			     f->iteration_gid,
			     attrib_name,
			     attrib_type,
			     attrib_value,
			     attrib_nelem));
	}
	H5_RETURN (H5_SUCCESS);
}
