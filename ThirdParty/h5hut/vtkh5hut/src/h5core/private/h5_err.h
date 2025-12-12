#ifndef __PRIVATE_H5_ERROR_H
#define __PRIVATE_H5_ERROR_H

#include "h5core/h5_err.h"

extern const char* const H5_O_MODES[];

#define h5priv_handle_file_mode_error( mode_id )                        \
        h5_error(                                                       \
                H5_ERR_BADF,                                            \
                "Operation not permitted in mode '%s'",                 \
                H5_O_MODES[mode_id & 0xff] );

#define HANDLE_H5_OVERFLOW_ERR( max )                                   \
        h5_error(                                                       \
                H5_ERR_INVAL,                                           \
                "Cannot store more than %lld items", (long long)max );

#define HANDLE_H5_PARENT_ID_ERR( parent_id  )                           \
        h5_error(                                                       \
                H5_ERR_INVAL,                                           \
                "Wrong parent_id %lld.",                                \
                (long long)parent_id );

#define HANDLE_H5_OUT_OF_RANGE_ERR( otype, oid )                        \
        h5_error(                                                       \
                H5_ERR_INVAL,                                           \
                "%s id %lld out of range",                              \
                otype, (long long)oid );

#endif
