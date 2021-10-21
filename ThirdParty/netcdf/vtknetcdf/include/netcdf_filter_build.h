/* Copyright 2018, UCAR/Unidata and OPeNDAP, Inc.
   See the COPYRIGHT file for more information. */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/hdf5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* This include file is used if one wished to build a filter plugin
   independent of HDF5. See examples in the plugins directory
*/

#ifndef NETCDF_FILTER_BUILD_H
#define NETCDF_FILTER_BUILD_H 1

/**************************************************/
/* Build To the HDF5 C-API for Filters */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Support headers */
#include <netcdf.h>
#include <netcdf_filter.h>

#ifdef USE_HDF5
#include <hdf5.h>
/* Older versions of the hdf library may define H5PL_type_t here */
#include <H5PLextern.h>

#else /*!USE_HDF5*/ /* Provide replacement definitions */

/* WARNING: In order make NCZARR independent of HDF5,
   while still using HDF5-style filters, some HDF5
   declarations need to be duplicated here with
   different names. Watch out for changes in
   the underlying HDF5 declarations.

   See the file H5Zpublic.h for more detailed descriptions.

   Note that these declarations are always enabled because
   HDF5-style filters may have been created with these definitions
   but for use by HDF5.

   Note also that certain filters in the plugins directory will not build if HDF5 is not installed:
   notably blosc.
*/

/* H5Z_FILTER_RESERVED => H5Z_FILTER_RESERVED */
#define H5Z_FILTER_RESERVED 256 /*filter ids below this value are reserved for library use */

/* H5Z_FILTER_MAX => H5Z_FILTER_MAX */
#define H5Z_FILTER_MAX 65535 /*maximum filter id */

/* Only a limited set of definition and invocation flags are allowed */
#define H5Z_FLAG_MANDATORY      0x0000  /*filter is mandatory		*/
#define H5Z_FLAG_OPTIONAL	0x0001	/*filter is optional		*/
#define H5Z_FLAG_REVERSE	0x0100	/*reverse direction; read	*/

typedef int htri_t;
typedef int herr_t;
typedef size_t hsize_t;
typedef long long hid_t;

#define H5allocate_memory(size,n) malloc(size)
#define H5free_memory(buf) free(buf)

/* htri_t (*H5Z_can_apply_func_t)(hid_t dcpl_id, hid_t type_id, hid_t space_id) => currently not supported; must be NULL. */
typedef htri_t (*H5Z_can_apply_func_t)(long long, long long, long long);

/* herr_t (*H5Z_set_local_func_t)(hid_t dcpl_id, hid_t type_id, hid_t space_id); => currently not supported; must be NULL. */
typedef herr_t (*H5Z_set_local_func_t)(long long, long long, long long);

/* H5Z_funct_t => H5Z_filter_func_t */
typedef size_t (*H5Z_func_t)(unsigned int flags, size_t cd_nelmts,
			     const unsigned int cd_values[], size_t nbytes,
			     size_t *buf_size, void **buf);

typedef int H5Z_filter_t;

#define H5Z_CLASS_T_VERS 1

/*
 * The filter table maps filter identification numbers to structs that
 * contain a pointers to the filter function and timing statistics.
 */
typedef struct H5Z_class2_t {
    int version;                    /* Version number of the struct; should be H5Z_FILTER_CLASS_VER */
    H5Z_filter_t id;		            /* Filter ID number                             */
    unsigned encoder_present;       /* Does this filter have an encoder?            */
    unsigned decoder_present;       /* Does this filter have a decoder?             */
    const char *name;               /* Comment for debugging                        */
    H5Z_can_apply_func_t can_apply; /* The "can apply" callback for a filter        */
    H5Z_set_local_func_t set_local; /* The "set local" callback for a filter        */
    H5Z_func_t filter;              /* The actual filter function                   */
} H5Z_class2_t;

/* The HDF5/H5Zarr dynamic loader looks for the following:*/

/* Plugin type used by the plugin library */
typedef enum H5PL_type_t {
    H5PL_TYPE_ERROR         = -1,   /* Error                */
    H5PL_TYPE_FILTER        =  0,   /* Filter               */
    H5PL_TYPE_NONE          =  1    /* This must be last!   */
} H5PL_type_t;

#endif /*HAVE_HDF5_H*/

/* Following External Discovery Functions should be present for the dynamic loading of filters */

/* returns specific constant H5ZP_TYPE_FILTER */
typedef H5PL_type_t (*H5PL_get_plugin_type_proto)(void);

/* return <pointer to instance of H5Z_filter_class> */
typedef const void* (*H5PL_get_plugin_info_proto)(void);

/**************************************************/
/* Build To a NumCodecs-style C-API for Filters */

/* Version of the NCZ_codec_t structure */
#define NCZ_CODEC_CLASS_VER 1

/* List of the kinds of NCZ_codec_t formats */
#define NCZ_CODEC_HDF5 1 /* HDF5 <-> Codec converter */

/* Defined flags for filter invocation (not stored); powers of two */
#define NCZ_FILTER_DECODE 0x00000001

/* External Discovery Function */

/*
Obtain a pointer to an instance of NCZ_codec_class_t.

NCZ_get_plugin_info(void) --  returns pointer to instance of NCZ_codec_class_t.
			      Instance an be recast based on version+sort to the plugin type specific info.
So the void* return value is typically actually of type NCZ_codec_class_t*.
*/
typedef const void* (*NCZ_get_plugin_info_proto)(void);

/* The current object returned by NCZ_get_plugin_info is a
   pointer to an instance of NCZ_codec_t.

The key to this struct are the four function pointers that do setup/reset/finalize
and conversion between codec JSON and HDF5 parameters.

Setup context state for the codec converter
int (*NCZ_codec_setup)(int ncid, int varid, void** contextp);

@param ncid -- (in) ncid of the variable's group
@param varid -- (in) varid of the variable
@params contextp -- (out) context for this (var,codec) combination.
@return -- a netcdf-c error code.

Reclaim any codec resources from setup. Not same as finalize.
int (*NCZ_codec_reset)(void* context);

@param context -- (in) context state

Finalize use of the plugin. Since HDF5 does not provide this functionality,
the codec may need to do it. See H5Zblosc.c for an example.
void (*NCZ_codec_finalize)(void);

@param context -- (in) context state

Convert a JSON representation to an HDF5 representation:
int (*NCZ_codec_to_hdf5)(void* context, const char* codec, int* nparamsp, unsigned** paramsp);

@param context -- (in) context state from setup.
@param codec   -- (in) ptr to JSON string representing the codec.
@param nparamsp -- (out) store the length of the converted HDF5 unsigned vector
@param paramsp -- (out) store a pointer to the converted HDF5 unsigned vector;
                  caller frees. Note the double indirection.
@return -- a netcdf-c error code.

Convert an HDF5 representation to a JSON representation
int (*NCZ_hdf5_to_codec)(void* context, int nparams, const unsigned* params, char** codecp);

@param context -- (in) context state from setup.
@param nparams -- (in) the length of the HDF5 unsigned vector
@param params -- (in) pointer to the HDF5 unsigned vector.
@param codecp -- (out) store the string representation of the codec; caller must free.
@return -- a netcdf-c error code.
*/

/* QUESTION? do we want to provide a netcdf-specific
  alternative to H5Z_set_local since NCZarr may not have HDF5 access?
  HDF5: herr_t set_local(hid_t dcpl, hid_t type, hid_t space);
  Proposed netcdf equivalent: int NCZ_set_local(int ncid, int varid, int* nparamsp, unsigned** paramsp);
  where ncid+varid is equivalent to the space.
*/

/*
The struct that provides the necessary filter info.
The combination of version + sort uniquely determines
the format of the remainder of the struct
*/
typedef struct NCZ_codec_t {
    int version; /* Version number of the struct */
    int sort; /* Format of remainder of the struct;
                 Currently always NCZ_CODEC_HDF5 */
    const char* codecid;            /* The name/id of the codec */
    unsigned int hdf5id; /* corresponding hdf5 id */
    int (*NCZ_codec_to_hdf5)(void* context, const char* codec, int* nparamsp, unsigned** paramsp);
    int (*NCZ_hdf5_to_codec)(void* context, int nparams, const unsigned* params, char** codecp);
    int (*NCZ_codec_setup)(int ncid, int varid, void** contextp);
    int (*NCZ_codec_reset)(void* context);
    void (*NCZ_codec_finalize)(void);
} NCZ_codec_t;

#ifndef NC_UNUSED
#define NC_UNUSED(var) (void)var
#endif

#ifndef DLLEXPORT
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif
#endif

#endif /*NETCDF_FILTER_BUILD_H*/
