/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef H5Tconv_complex_H
#define H5Tconv_complex_H

/* Private headers needed by this file */
#include "H5Tpkg.h"

/***********************/
/* Function Prototypes */
/***********************/

/* Helper functions shared between conversion modules */
H5_DLL herr_t H5T__conv_complex_f_matched(const H5T_t *src_p, const H5T_t *dst_p,
                                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                          void *buf);

/****************************************/
/* Soft (emulated) conversion functions */
/****************************************/

/* Conversion functions between complex number datatypes */
H5_DLL herr_t H5T__conv_complex(const H5T_t *src_p, const H5T_t *dst_p, H5T_cdata_t *cdata,
                                const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions from complex number datatype to another datatype class */
H5_DLL herr_t H5T__conv_complex_i(const H5T_t *src_p, const H5T_t *dst_p, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_complex_f(const H5T_t *src_p, const H5T_t *dst_p, H5T_cdata_t *cdata,
                                  const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                  size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_complex_compat(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *_buf, void *bkg);

/*********************************************/
/* Hard (compiler cast) conversion functions */
/*********************************************/

#ifdef H5_HAVE_COMPLEX_NUMBERS
/* Conversion functions for 'float _Complex' / '_Fcomplex' */
H5_DLL herr_t H5T__conv_fcomplex_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_fcomplex__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                          size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_fcomplex_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                         const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                         size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_dcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                          size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_fcomplex_lcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                          size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'double _Complex' / '_Dcomplex' */
H5_DLL herr_t H5T__conv_dcomplex_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
#ifdef H5_HAVE__FLOAT16
H5_DLL herr_t H5T__conv_dcomplex__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                          size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_dcomplex_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                         const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                         size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_fcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                          size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_dcomplex_lcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                          size_t bkg_stride, void *buf, void *bkg);

/* Conversion functions for 'long double _Complex' / '_Lcomplex' */
H5_DLL herr_t H5T__conv_lcomplex_schar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_lcomplex_uchar(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_lcomplex_short(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_lcomplex_ushort(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_lcomplex_int(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                     const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                     size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_lcomplex_uint(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_lcomplex_long(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                      const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                      size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_lcomplex_ulong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
#ifdef H5T_CONV_INTERNAL_LDOUBLE_LLONG
H5_DLL herr_t H5T__conv_lcomplex_llong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
#endif
#ifdef H5T_CONV_INTERNAL_LDOUBLE_ULLONG
H5_DLL herr_t H5T__conv_lcomplex_ullong(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
#endif
#if defined(H5_HAVE__FLOAT16) && defined(H5T_CONV_INTERNAL_LDOUBLE_FLOAT16)
H5_DLL herr_t H5T__conv_lcomplex__Float16(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                          size_t bkg_stride, void *buf, void *bkg);
#endif
H5_DLL herr_t H5T__conv_lcomplex_float(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                       const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                       size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_lcomplex_double(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                        const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                        size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_lcomplex_ldouble(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                         const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                         size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_lcomplex_fcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                          size_t bkg_stride, void *buf, void *bkg);
H5_DLL herr_t H5T__conv_lcomplex_dcomplex(const H5T_t *st, const H5T_t *dt, H5T_cdata_t *cdata,
                                          const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                                          size_t bkg_stride, void *buf, void *bkg);
#endif /* H5_HAVE_COMPLEX_NUMBERS */

#endif /* H5Tconv_complex_H */
