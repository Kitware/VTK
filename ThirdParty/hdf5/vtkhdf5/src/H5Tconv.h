/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef H5Tconv_H
#define H5Tconv_H

/* Private headers needed by this file */
#include "H5private.h"
#include "H5Sprivate.h"
#include "H5Tprivate.h"

/**************************/
/* Library Private Macros */
/**************************/

/* Length of debugging name buffer */
#define H5T_NAMELEN 32

/****************************/
/* Library Private Typedefs */
/****************************/

/* Forward reference of H5S_t */
struct H5S_t;

/* Structure for conversion callback property */
typedef struct H5T_conv_cb_t {
    H5T_conv_except_func_t func;
    void                  *user_data;
} H5T_conv_cb_t;

/* Context struct for information used during datatype conversions.
 * Which union member is valid to read from is dictated by the
 * accompanying H5T_cdata_t structure's H5T_cmd_t member value.
 */
typedef struct H5T_conv_ctx_t {
    union {
        /*
         * Fields only valid during conversion function initialization
         * (H5T_cmd_t H5T_CONV_INIT)
         */
        struct H5T_conv_ctx_init_fields {
            H5T_conv_cb_t cb_struct;
        } init;

        /*
         * Fields only valid during conversion function conversion
         * process (H5T_cmd_t H5T_CONV_CONV)
         */
        struct H5T_conv_ctx_conv_fields {
            H5T_conv_cb_t cb_struct;
            hid_t         dxpl_id;
            hid_t         src_type_id;
            hid_t         dst_type_id;

            /* Is conversion currently being done on a member of
             * a container type, like a compound datatype? If so,
             * cached information can be reused rather than creating
             * and tearing it down for every compound element.
             */
            bool recursive;
        } conv;

        /*
         * Fields only valid during conversion function free process
         * (H5T_cmd_t H5T_CONV_FREE)
         */
        struct H5T_conv_ctx_free_fields {
            hid_t src_type_id;
            hid_t dst_type_id;
        } free;
    } u;
} H5T_conv_ctx_t;

/* Library internal datatype conversion functions are... */
typedef herr_t (*H5T_lib_conv_t)(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                 const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                 size_t bkg_stride, void *buf, void *bkg);

/* Conversion callbacks */
typedef struct H5T_conv_func_t {
    bool is_app; /* Whether conversion function is registered from application */
    union {
        H5T_conv_t     app_func; /* Application data conversion function */
        H5T_lib_conv_t lib_func; /* Library internal data conversion function */
    } u;
} H5T_conv_func_t;

#ifdef H5T_DEBUG
/* Statistics about a conversion function */
typedef struct H5T_stats_t {
    unsigned      ncalls; /*num calls to conversion function   */
    hsize_t       nelmts; /*total data points converted      */
    H5_timevals_t times;  /*total time for conversion        */
} H5T_stats_t;
#endif

/* The datatype conversion database */
typedef struct H5T_path_t {
    char            name[H5T_NAMELEN]; /*name for debugging only         */
    H5T_t          *src;               /*source datatype             */
    H5T_t          *dst;               /*destination datatype            */
    H5T_conv_func_t conv;              /* Conversion function  */
    bool            is_hard;           /*is it a hard function?      */
    bool            is_noop;           /*is it the noop conversion?      */
    H5T_cdata_t     cdata;             /*data for this function      */

#ifdef H5T_DEBUG
    H5T_stats_t stats; /*statistics for the conversion       */
#endif
} H5T_path_t;

/* The master list of soft conversion functions */
typedef struct H5T_soft_t {
    char            name[H5T_NAMELEN]; /*name for debugging only         */
    H5T_class_t     src;               /*source datatype class       */
    H5T_class_t     dst;               /*destination datatype class      */
    H5T_conv_func_t conv;              /*the conversion function         */
} H5T_soft_t;

/* Values for the optimization of compound data reading and writing.  They indicate
 * whether the fields of the source and destination are subset of each other and
 * there is no conversion needed.
 */
typedef enum {
    H5T_SUBSET_BADVALUE = -1, /* Invalid value */
    H5T_SUBSET_FALSE    = 0,  /* Source and destination aren't subset of each other */
    H5T_SUBSET_SRC,           /* Source is the subset of dest and no conversion is needed */
    H5T_SUBSET_DST,           /* Dest is the subset of source and no conversion is needed */
    H5T_SUBSET_CAP            /* Must be the last value */
} H5T_subset_t;

typedef struct H5T_subset_info_t {
    H5T_subset_t subset;    /* See above */
    size_t       copy_size; /* Size in bytes, to copy for each element */
} H5T_subset_info_t;

/*****************************/
/* Library-private Variables */
/*****************************/

/***************************************/
/* Library-private Function Prototypes */
/***************************************/

H5_DLL herr_t H5T_convert(H5T_path_t *tpath, const H5T_t *src_type, const H5T_t *dst_type, size_t nelmts,
                          size_t buf_stride, size_t bkg_stride, void *buf, void *bkg);

/* Helper function for H5T_convert that accepts a pointer to a H5T_conv_ctx_t structure */
H5_DLL herr_t H5T_convert_with_ctx(H5T_path_t *tpath, const H5T_t *src_type, const H5T_t *dst_type,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);

/* Conversion path and path table routines */
H5_DLL H5T_path_t        *H5T_path_find(const H5T_t *src, const H5T_t *dst);
H5_DLL bool               H5T_path_noop(const H5T_path_t *p);
H5_DLL bool               H5T_noop_conv(const H5T_t *src, const H5T_t *dst);
H5_DLL H5T_bkg_t          H5T_path_bkg(const H5T_path_t *p);
H5_DLL H5T_subset_info_t *H5T_path_compound_subset(const H5T_path_t *p);

/* Generic routines */
H5_DLL herr_t H5T_reclaim(const H5T_t *type, struct H5S_t *space, void *buf);
H5_DLL herr_t H5T_reclaim_cb(void *elem, const H5T_t *dt, unsigned ndim, const hsize_t *point, void *op_data);
H5_DLL bool   H5T_get_force_conv(const H5T_t *dt);

/* Conversion functions */
H5_DLL herr_t H5T__conv_noop(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                             const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                             size_t bkg_stride, void *_buf, void *bkg);
H5_DLL herr_t H5T__conv_order(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                              const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                              size_t bkg_stride, void *_buf, void *bkg);
H5_DLL herr_t H5T__conv_order_opt(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *_buf, void *bkg);

/* Utility functions */
H5_DLL herr_t H5T__reverse_order(uint8_t *rev, uint8_t *s, size_t size, H5T_order_t order);

/* Debugging functions */
H5_DLL herr_t H5T__print_path_stats(H5T_path_t *path, int *nprint /*in,out*/);

/* Testing functions */
H5_DLL int H5T__get_path_table_npaths(void);

#endif /* H5Tconv_H */
