#ifndef __PRIVATE_H5_FILE_H
#define __PRIVATE_H5_FILE_H

#include "private/h5_types.h"
#include "private/h5_log.h"
#include "private/h5_err.h"
#include "h5core/h5_file.h"

#define H5_VFD_MPIO_POSIX       0x00000010
#define H5_VFD_MPIO_INDEPENDENT 0x00000020
#define H5_VFD_MPIO_COLLECTIVE  0x00000040
#define H5_VFD_CORE_IO		0x00000080

#define H5_FLUSH_FILE		0x00001000
#define H5_FLUSH_ITERATION	0x00002000
#define H5_FLUSH_DATASET	0x00004000

#define H5_FS_LUSTRE		0x00010000

static inline int
is_valid_file_handle(h5_file_p f) {
	return ((f != NULL) &&
		(f->file > 0) &&
		(f->u != NULL) &&
		(f->b != NULL));
}

static inline int
is_writable (h5_file_p f) {
	return (f->props->flags & (H5_O_RDWR | H5_O_WRONLY | H5_O_APPENDONLY));
}

static inline int
is_readable (h5_file_p f) {
	return (f->props->flags & (H5_O_RDWR | H5_O_RDONLY));
}

static inline int
is_readonly (h5_file_p f) {
	return (f->props->flags & H5_O_RDONLY);
}

static inline int
is_appendonly (h5_file_p f) {
	return (f->props->flags & H5_O_APPENDONLY);
}

#define CHECK_FILEHANDLE(f)			\
        TRY (is_valid_file_handle(f) ? H5_SUCCESS : h5_error (	\
		     H5_ERR_BADF,					\
		     "Called with bad filehandle."));


#define CHECK_WRITABLE_MODE(f)                                          \
	TRY (is_writable (f) ? H5_SUCCESS : h5_error (			\
                     H5_ERR_INVAL,                                      \
                     "Attempting to write to read-only file handle"));

#define CHECK_READABLE_MODE(f)                                          \
	TRY (is_readable (f) ? H5_SUCCESS : h5_error (                  \
                     H5_ERR_INVAL,                                      \
                     "Attempting to read from write-only file handle"));

#define CHECK_TIMEGROUP(f)                                             \
	TRY ((f->iteration_gid > 0) ? H5_SUCCESS : h5_error (               \
                     H5_ERR_INVAL,                                     \
                     "Iteration is invalid! Have you set the time step?"));

#define check_file_handle_is_valid(f)		\
	CHECK_FILEHANDLE(f);			\

#define check_file_is_writable(f)		\
	CHECK_FILEHANDLE(f);			\
	CHECK_WRITABLE_MODE(f);

#define check_iteration_handle_is_valid(f)	\
	CHECK_FILEHANDLE(f);			\
	CHECK_TIMEGROUP(f);
	
#define check_iteration_is_readable(f)		\
	CHECK_FILEHANDLE(f);			\
	CHECK_READABLE_MODE(f);			\
	CHECK_TIMEGROUP(f);

#define check_iteration_is_writable(f)		\
	CHECK_FILEHANDLE(f);			\
	CHECK_WRITABLE_MODE(f);			\
	CHECK_TIMEGROUP(f);

#endif
