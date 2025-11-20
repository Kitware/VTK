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

#ifndef H5Tconv_integer_H
#define H5Tconv_integer_H

/* Private headers needed by this file */
#include "H5Tpkg.h"

/***********************/
/* Function Prototypes */
/***********************/

/****************************************/
/* Soft (emulated) conversion functions */
/****************************************/

/* Conversion functions between integer datatypes */
H5_DLL herr_t H5T__conv_i_i(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);

/* Conversion functions from integer datatype to another datatype class */
H5_DLL herr_t H5T__conv_i_f(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);

/*********************************************/
/* Hard (compiler cast) conversion functions */
/*********************************************/

/* Conversion functions for 'signed char' */
H5_DLL herr_t H5T__conv_schar_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_schar__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_schar_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_schar_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'unsigned char' */
H5_DLL herr_t H5T__conv_uchar_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_uchar__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_uchar_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uchar_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'signed short' */
H5_DLL herr_t H5T__conv_short_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_short__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_short_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_short_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'unsigned short' */
H5_DLL herr_t H5T__conv_ushort_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_ushort__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_ushort_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ushort_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'signed int' */
H5_DLL herr_t H5T__conv_int_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                 const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                 size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                 const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                 size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_int__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_int_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_int_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'unsigned int' */
H5_DLL herr_t H5T__conv_uint_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                 const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                 size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_uint__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_uint_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_uint_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'signed long' */
H5_DLL herr_t H5T__conv_long_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                 const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                 size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_long__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_long_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_long_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'unsigned long' */
H5_DLL herr_t H5T__conv_ulong_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_ulong__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_ulong_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ulong_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'signed long long' */
H5_DLL herr_t H5T__conv_llong_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_llong__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_llong_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_llong_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'unsigned long long' */
H5_DLL herr_t H5T__conv_ullong_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_ullong__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_ullong_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ullong_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);

#endif /* H5Tconv_integer_H */
