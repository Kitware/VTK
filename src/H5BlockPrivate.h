#ifndef _H5BLOCK_PRIVATE_H_
#define _H5BLOCK_PRIVATE_H_

#define H5BLOCK_GROUPNAME_BLOCK	"Block"

#define BLOCK_INIT( f ) { \
	h5part_int64_t herr = _H5Block_init ( f ); \
	if ( herr < 0 ) return herr; \
}

h5part_int64_t
_H5Block_init (
	H5PartFile *f
	);

h5part_int64_t
_H5Block_close (
	H5PartFile *f
	);

h5part_int64_t
_H5Block_open_field_group (
	H5PartFile *f,
	const char *name
	);

h5part_int64_t
_H5Block_close_field_group (
	H5PartFile *f
	);

h5part_int64_t
_H5Block_create_field_group (
	H5PartFile *f,
	const char *name
	);
 
h5part_int64_t
_H5Block_select_hyperslab_for_reading (
	H5PartFile *f,
	hid_t dataset
	);

h5part_int64_t
_H5Block_write_data (
	H5PartFile *f,
	const char *name,
	const void *data,
	const hid_t type
	);

h5part_int64_t
_H5Block_read_data (
	H5PartFile *f,
	const char *name,
	void *data,
	hid_t type
	);

h5part_int64_t
_write_field_attrib (
	H5PartFile *f,
	const char *field_name,
	const char *attrib_name,
	const hid_t attrib_type,
	const void *attrib_value,
	const h5part_int64_t attrib_nelem
	);

#endif
