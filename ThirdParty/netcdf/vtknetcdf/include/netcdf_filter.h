/* Copyright 2018, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */

/*
 * In order to use any of the netcdf_XXX.h files, it is necessary
 * to include netcdf.h followed by any netcdf_XXX.h files.
 * Various things (like EXTERNL) are defined in netcdf.h
 * to make them available for use by the netcdf_XXX.h files.
*/

#ifndef NETCDF_FILTER_H
#define NETCDF_FILTER_H 1

/* API for libdispatch/dfilter.c */

/* Must match values in <H5Zpublic.h> */
#ifndef H5Z_FILTER_SZIP
#define H5Z_FILTER_SZIP 4
#endif

/* Define the known filter formats */
#define NC_FILTER_FORMAT_HDF5 1 /* Use the H5Z_class2_t format */

/* Note that this structure can be extended
   in the usual C way if the first field of the extended
   struct is of type NC_FILTER_INFO
*/
typedef struct NC_FILTER_INFO {
    int version; /* Of this structure */
#      define NC_FILTER_INFO_VERSION 1
    int format; /* Controls actual type of this structure */
    int id;     /* Must be unique WRT format */
    void* info; /* The filter info as defined by the format.
                   For format == NC_FILTER_FORMAT_HDF5,
                   this must conform to H5Z_class2_t in H5Zpublic.h;
                   Defined as void* to avoid specifics.
                */
} NC_FILTER_INFO;

#if defined(__cplusplus)
extern "C" {
#endif

/* Provide consistent filter spec parser */
EXTERNL int NC_parsefilterspec(const char* spec, unsigned int* idp, size_t* nparamsp, unsigned int** paramsp);

EXTERNL void NC_filterfix8(unsigned char* mem, int decode);

/* Support direct user defined filters */
EXTERNL int nc_filter_register(NC_FILTER_INFO* filter_info);
EXTERNL int nc_filter_unregister(int format, int id);
EXTERNL int nc_filter_inq(int format, int id, NC_FILTER_INFO* filter_info);

#if defined(__cplusplus)
}
#endif

#endif /* NETCDF_FILTER_H */
