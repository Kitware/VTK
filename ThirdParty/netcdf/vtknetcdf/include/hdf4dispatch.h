/* Copyright 2018, UCAR/Unidata. See netcdf/COPYRIGHT file for copying
 * and redistribution conditions. */
/**
 * @file @internal This header file contains the prototypes for the
 * HDF4 versions of the netCDF functions. This is part of the HDF4
 * dispatch layer and this header should not be included by any file
 * outside the libhdf4 directory.
 *
 * Ed Hartnett
 */
#ifndef _HDF4DISPATCH_H
#define _HDF4DISPATCH_H

#include "config.h"
#include "ncdispatch.h"

/** This is the max size of an SD dataset name in HDF4 (from HDF4
 * documentation).*/
#define NC_MAX_HDF4_NAME 64

/** This is the max number of dimensions for a HDF4 SD dataset (from
 * HDF4 documentation). */
#define NC_MAX_HDF4_DIMS 32

/* Stuff below is for hdf4 files. */
typedef struct NC_VAR_HDF4_INFO
{
    int sdsid;
    int hdf4_data_type;
} NC_VAR_HDF4_INFO_T;

typedef struct NC_HDF4_FILE_INFO
{
    int sdid;
} NC_HDF4_FILE_INFO_T;

#if defined(__cplusplus)
extern "C" {
#endif

    extern int
    NC_HDF4_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                 void *parameters, const NC_Dispatch *, int);

    extern int
    NC_HDF4_abort(int ncid);

    extern int
    NC_HDF4_close(int ncid, void *ignore);

    extern int
    NC_HDF4_inq_format(int ncid, int *formatp);

    extern int
    NC_HDF4_inq_format_extended(int ncid, int *formatp, int *modep);

    extern int
    NC_HDF4_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                     void *value, nc_type);

#if defined(__cplusplus)
}
#endif

#endif /*_HDF4DISPATCH_H */
