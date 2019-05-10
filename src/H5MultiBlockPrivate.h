#ifndef _H5MULTIBLOCK_PRIVATE_H_
#define _H5MULTIBLOCK_PRIVATE_H_

#define MULTIBLOCK_INIT( f ) { \
	h5part_int64_t herr = _H5MultiBlock_init ( f ); \
	if ( herr < 0 ) return herr; \
}

h5part_int64_t
_H5MultiBlock_init (
	H5PartFile *f
	);

h5part_int64_t
_H5MultiBlock_close (
	H5PartFile *f
	);

h5part_int64_t
_H5MultiBlock_write_data (
	H5PartFile *f,
	const char *field_name,
	const void *data,
	const hid_t type
	);

h5part_int64_t
_H5MultiBlock_read_data (
	H5PartFile *f,
	const char *field_name,
	char **data,
	hid_t type
	);

#endif
