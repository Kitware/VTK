#ifndef __H5PART_PRIVATE_H
#define __H5PART_PRIVATE_H

#if H5_VERS_MAJOR == 1 && H5_VERS_MINOR == 6
#define H5PART_USE_HDF5_16
#define H5_USE_16_API
#endif

#if H5_VERS_MAJOR == 1 && H5_VERS_MINOR >= 8
#define H5PART_HAVE_HDF5_18
#endif

#define H5PART_GROUPNAME_STEP	"Step"

#define H5PART_SET_STEP_READ_ONLY 0

h5part_int64_t
_H5Part_file_is_valid (
	const H5PartFile *f
	);

/*!
  The functions declared here are not part of the API, but may be used
  in extensions like H5Block. We name these functions "private".

  \note
  Private function may change there interface even in stable versions.
  Don't use them in applications!
*/

struct _iter_op_data {
	int stop_idx;
	int count;
	int type;
	char *name;
	size_t len;
	char *pattern;
};

h5part_int64_t
_H5Part_set_step (
	H5PartFile *f,
	const h5part_int64_t step
	);

h5part_int64_t
_H5Part_get_step_name(
	H5PartFile *f,
	const h5part_int64_t step,
	char *name
	);

h5part_int64_t
_H5Part_get_num_particles (
	H5PartFile *f
	);

herr_t
_H5Part_iteration_operator (
	hid_t group_id,
	const char *member_name,
	void *operator_data
	);

#ifndef H5_USE_16_API
herr_t
_H5Part_iteration_operator2 (
	hid_t group_id,		 /*!< [in] parent object id */
	const char *member_name, /*!< [in] child object name */
	const H5L_info_t *linfo, /*!< link info */
	void *operator_data );   /*!< [in,out] data passed to the iterator */
#endif

void
_H5Part_set_funcname (
	char *fname
	);

char*
_H5Part_get_funcname (
	void
	);

#define INIT do {\
	if ( _init() < 0 ) {\
		HANDLE_H5PART_INIT_ERR;\
		return NULL;\
	}}while(0);

#define SET_FNAME( fname )	_H5Part_set_funcname( fname );

h5part_int64_t
_H5Part_make_string_type (
	hid_t *type,
	int size
	);

h5part_int64_t
_H5Part_normalize_h5_type (
	hid_t type
	);

h5part_int64_t
_H5Part_read_attrib (
	hid_t id,
	const char *attrib_name,
	void *attrib_value
	);

h5part_int64_t
_H5Part_write_attrib (
	hid_t id,
	const char *attrib_name,
	const hid_t attrib_type,
	const void *attrib_value,
	const hsize_t attrib_nelem
	);

h5part_int64_t
_H5Part_write_file_attrib (
	H5PartFile *f,
	const char *name,
	const hid_t type,
	const void *value,
	const hsize_t nelem
	);

h5part_int64_t
_H5Part_write_step_attrib (
	H5PartFile *f,
	const char *name,
	const hid_t type,
	const void *value,
	const hsize_t nelem
	);

h5part_int64_t
_H5Part_get_attrib_info (
	hid_t id,
	const h5part_int64_t attrib_idx,
	char *attrib_name,
	const h5part_int64_t len_attrib_name,
	h5part_int64_t *attrib_type,
	h5part_int64_t *attrib_nelem
	);

h5part_int64_t
_H5Part_get_num_objects (
	hid_t group_id,
	const char *group_name,
	const hid_t type
	);

h5part_int64_t
_H5Part_get_num_objects_matching_pattern (
	hid_t group_id,
	const char *group_name,
	const hid_t type,
	char * const pattern
	);

h5part_int64_t
_H5Part_get_object_name (
	hid_t group_id,
	const char *group_name,
	const hid_t type,
	const h5part_int64_t idx,
	char *obj_name,
	const h5part_int64_t len_obj_name
	);

h5part_int64_t
_H5Part_have_group (
	const hid_t id,
	const char *name
	);

h5part_int64_t
_H5Part_start_throttle (
	H5PartFile *f
	);

h5part_int64_t
_H5Part_end_throttle (
	H5PartFile *f
	);

h5part_error_handler
_H5Part_get_err_handle (
	void
	);

void
_H5Part_print_error (
	const char *fmt,
	... )
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

void
_H5Part_print_warn (
	const char *fmt,
	...
	)
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

void
_H5Part_print_info (
	const char *fmt,
	...
	)
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

void
_H5Part_print_debug (
	const char *fmt,
	...
	)
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

void
_H5Part_print_debug_detail (
	const char *fmt,
	...
	)
#ifdef __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
;

char *
_H5Part_strdupfor2c (
	const char *s,
	const ssize_t len
	);

char *
_H5Part_strc2for (
	char * const str,
	const ssize_t l_str
	);

#endif
