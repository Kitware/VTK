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

#ifndef H5Tconv_float_H
#define H5Tconv_float_H

/* Private headers needed by this file */
#include "H5Tpkg.h"

/***********************/
/* Function Prototypes */
/***********************/

/****************************************/
/* Soft (emulated) conversion functions */
/****************************************/

/* Conversion functions between float datatypes */
H5_DLL herr_t H5T__conv_f_f(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);

/* Conversion functions from float datatype to another datatype class */
H5_DLL herr_t H5T__conv_f_i(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                            const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg);

/*********************************************/
/* Hard (compiler cast) conversion functions */
/*********************************************/

/* Conversion functions for '_Float16' */
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv__Float16_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv__Float16_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                         const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                         size_t bkg_stride, void *buf, void *bkg);
#endif

/* Conversion functions for 'float' */
H5_DLL herr_t H5T__conv_float_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_float__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_float_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_float_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'double' */
H5_DLL herr_t H5T__conv_double_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                   const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                   size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_double__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_double_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_double_ldouble(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'long double' */
H5_DLL herr_t H5T__conv_ldouble_schar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_uchar(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_short(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_ushort(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_int(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                    const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                    size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_uint(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_long(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_ulong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_llong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_ullong(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
#ifdef H5T_CONV_INTERNAL_LDOUBLE_FLOAT16
H5_DLL herr_t H5T__conv_ldouble__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                         const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                         size_t bkg_stride, void *buf, void *bkg);
#endif
#endif
H5_DLL herr_t H5T__conv_ldouble_float(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_ldouble_double(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);

#endif /* H5Tconv_float_H */
