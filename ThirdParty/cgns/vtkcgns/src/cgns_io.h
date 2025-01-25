/* ------------------------------------------------------------------------- *
 * CGNS - CFD General Notation System (http://www.cgns.org)                  *
 * CGNS/MLL - Mid-Level Library header file                                  *
 * Please see cgnsconfig.h file for this local installation configuration    *
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *

  This software is provided 'as-is', without any express or implied warranty.
  In no event will the authors be held liable for any damages arising from
  the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must not
     be misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.

 * ------------------------------------------------------------------------- */

#ifndef CGNS_IO_H
#define CGNS_IO_H

#include "cgnstypes.h"

#if defined(_WIN32) && defined(BUILD_DLL)
# define CGEXTERN extern __declspec(dllexport)
#else
# define CGEXTERN extern
#endif

/* these should be the same as in cgnslib.h
   but are included here for completeness */

#define CGIO_MODE_READ   0
#define CGIO_MODE_WRITE  1
#define CGIO_MODE_MODIFY 2

#define CGIO_FILE_NONE   0
#define CGIO_FILE_ADF    1
#define CGIO_FILE_HDF5   2
#define CGIO_FILE_ADF2   3

#define CGIO_CONTIGUOUS 0
#define CGIO_COMPACT    1
#define CGIO_CHUNKED    2

/* currently these are the same as for ADF */

#define CGIO_MAX_DATATYPE_LENGTH  2
#define CGIO_MAX_DIMENSIONS      12
#define CGIO_MAX_NAME_LENGTH     32
#define CGIO_MAX_LABEL_LENGTH    32
#define CGIO_MAX_VERSION_LENGTH  32
#define CGIO_MAX_DATE_LENGTH     32
#define CGIO_MAX_ERROR_LENGTH    80
#define CGIO_MAX_LINK_DEPTH     100
#define CGIO_MAX_FILE_LENGTH   1024
#define CGIO_MAX_LINK_LENGTH   4096

/* these are the cgio error codes */

#define CGIO_ERR_NONE          0
#define CGIO_ERR_BAD_CGIO     -1
#define CGIO_ERR_MALLOC       -2
#define CGIO_ERR_FILE_MODE    -3
#define CGIO_ERR_FILE_TYPE    -4
#define CGIO_ERR_NULL_FILE    -5
#define CGIO_ERR_TOO_SMALL    -6
#define CGIO_ERR_NOT_FOUND    -7
#define CGIO_ERR_NULL_PATH    -8
#define CGIO_ERR_NO_MATCH     -9
#define CGIO_ERR_FILE_OPEN   -10
#define CGIO_ERR_READ_ONLY   -11
#define CGIO_ERR_NULL_STRING -12
#define CGIO_ERR_BAD_OPTION  -13
#define CGIO_ERR_FILE_RENAME -14
#define CGIO_ERR_TOO_MANY    -15
#define CGIO_ERR_DIMENSIONS  -16
#define CGIO_ERR_BAD_TYPE    -17
#define CGIO_ERR_NOT_HDF5    -18

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------*/

CGEXTERN int cgio_path_add (
    const char *path
);

CGEXTERN int cgio_path_delete (
    const char *path
);

CGEXTERN int cgio_find_file (
    const char *parentfile,
    const char *filename,
    int file_type,
    int max_path_len,
    char *pathname
);

/*---------------------------------------------------------*/

CGEXTERN int cgio_is_supported (
    int file_type
);

CGEXTERN int cgio_configure (
    int what,
    void *value
);

CGEXTERN void cgio_cleanup (void);

CGEXTERN int cgio_check_file (
    const char *filename,
    int *file_type
);

CGEXTERN int cgio_compute_data_size (
    const char *data_type,
    int num_dims,
    const cgsize_t *dim_vals,
    cglong_t *count
);

CGEXTERN int cgio_check_dimensions (
    int ndims,
    const cglong_t *dims
);

CGEXTERN int cgio_copy_dimensions (
    int ndims,
    const cglong_t *dims64,
    cgsize_t *dims
);

/*---------------------------------------------------------*/

CGEXTERN int cgio_open_file (
    const char *filename,
    int file_mode,
    int file_type,
    int *cgio_num
);

CGEXTERN int cgio_close_file (
    int cgio_num
);

CGEXTERN int cgio_compress_file (
    int cgio_num,
    const char *filename
);

CGEXTERN int cgio_copy_file (
    int cgio_num_inp,
    int cgio_num_out,
    int follow_links
);

CGEXTERN int cgio_flush_to_disk (
    int cgio_num
);

/*---------------------------------------------------------*/

CGEXTERN int cgio_library_version (
    int cgio_num,
    char *version
);

CGEXTERN int cgio_file_version (
    int cgio_num,
    char *file_version,
    char *creation_date,
    char *modified_date
);

CGEXTERN int cgio_get_root_id (
    int cgio_num,
    double *rootid
);

CGEXTERN int cgio_get_file_type (
    int cgio_num,
    int *file_type
);

/*---------------------------------------------------------*/

CGEXTERN void cgio_error_code (
    int *errcode,
    int *file_type
);

CGEXTERN int cgio_error_message (
    char *error_msg
);

CGEXTERN void cgio_error_exit (
    const char *msg
);

CGEXTERN void cgio_error_abort (
    int abort_flag
);

/*---------------------------------------------------------*/

CGEXTERN int cgio_create_node (
    int cgio_num,
    double pid,
    const char *name,
    double *id
);

CGEXTERN int cgio_new_node (
    int cgio_num,
    double pid,
    const char *name,
    const char *label,
    const char *data_type,
    int ndims,
    const cgsize_t *dims,
    const void *data,
    double *id
);

CGEXTERN int cgio_delete_node (
    int cgio_num,
    double pid,
    double id
);

CGEXTERN int cgio_move_node (
    int cgio_num,
    double pid,
    double id,
    double new_pid
);

CGEXTERN int cgio_copy_node (
    int cgio_num_inp,
    double id_inp,
    int cgio_num_out,
    double id_out
);

CGEXTERN int cgio_release_id (
    int cgio_num,
    double id
);

/*---------------------------------------------------------*/

CGEXTERN int cgio_is_link (
    int cgio_num,
    double id,
    int *link_len
);

CGEXTERN int cgio_link_size (
    int cgio_num,
    double id,
    int *file_len,
    int *name_len
);

CGEXTERN int cgio_create_link (
    int cgio_num,
    double pid,
    const char *name,
    const char *filename,
    const char *name_in_file,
    double *id
);

CGEXTERN int cgio_get_link (
    int cgio_num,
    double id,
    char *filename,
    char *name_in_file
);

/*---------------------------------------------------------*/

CGEXTERN int cgio_number_children (
    int cgio_num,
    double id,
    int *num_children
);

CGEXTERN int cgio_children_ids (
    int cgio_num,
    double pid,
    int start,
    int max_ret,
    int *num_ret,
    double *ids
);

CGEXTERN int cgio_children_names (
    int cgio_num,
    double pid,
    int start,
    int max_ret,
    int name_len,
    int *num_ret,
    char *names
);

/*---------------------------------------------------------*/

CGEXTERN int cgio_get_node_id (
    int cgio_num,
    double pid,
    const char *name,
    double *id
);

CGEXTERN int cgio_get_name (
    int cgio_num,
    double id,
    char *name
);

CGEXTERN int cgio_get_label (
    int cgio_num,
    double id,
    char *label
);

CGEXTERN int cgio_get_data_type (
    int cgio_num,
    double id,
    char *data_type
);

CGEXTERN int cgio_get_data_size (
    int cgio_num,
    double id,
    cglong_t *data_size
);

CGEXTERN int cgio_get_dimensions (
    int cgio_num,
    double id,
    int *num_dims,
    cgsize_t *dims
);

CGEXTERN int cgio_read_all_data_type (
    int cgio_num,
    double id,
    const char *m_data_type,
    void *data
);

CGEXTERN int cgio_read_block_data_type (
    int cgio_num,
    double id,
    cgsize_t b_start,
    cgsize_t b_end,
    const char *m_data_type,
    void *data
);

CGEXTERN int cgio_read_data_type (
    int cgio_num,
    double id,
    const cgsize_t *s_start,
    const cgsize_t *s_end,
    const cgsize_t *s_stride,
    const char *m_data_type,
    int m_num_dims,
    const cgsize_t *m_dims,
    const cgsize_t *m_start,
    const cgsize_t *m_end,
    const cgsize_t *m_stride,
    void *data
);

/*---------------------------------------------------------*/

CGEXTERN int cgio_set_name (
    int cgio_num,
    double pid,
    double id,
    const char *name
);

CGEXTERN int cgio_set_label (
    int cgio_num,
    double id,
    const char *label
);

CGEXTERN int cgio_set_dimensions (
    int cgio_num,
    double id,
    const char *data_type,
    int num_dims,
    const cgsize_t *dims
);

CGEXTERN int cgio_write_all_data (
    int cgio_num,
    double id,
    const void *data
);

CGEXTERN int cgio_write_all_data_type (
    int cgio_num,
    double id,
    const char *m_data_type,
    const void *data
);

CGEXTERN int cgio_write_block_data (
    int cgio_num,
    double id,
    cgsize_t b_start,
    cgsize_t b_end,
    void *data
);

CGEXTERN int cgio_write_data (
    int cgio_num,
    double id,
    const cgsize_t *s_start,
    const cgsize_t *s_end,
    const cgsize_t *s_stride,
    int m_num_dims,
    const cgsize_t *m_dims,
    const cgsize_t *m_start,
    const cgsize_t *m_end,
    const cgsize_t *m_stride,
    const void *data
);

CGEXTERN int cgio_write_data_type (
    int cgio_num,
    double id,
    const cgsize_t *s_start,
    const cgsize_t *s_end,
    const cgsize_t *s_stride,
    const char *m_data_type,
    int m_num_dims,
    const cgsize_t *m_dims,
    const cgsize_t *m_start,
    const cgsize_t *m_end,
    const cgsize_t *m_stride,
    const void *data
);

#ifdef __cplusplus
}
#endif
#undef CGEXTERN
#endif

