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

#ifndef H5Tconv_array_H
#define H5Tconv_array_H

/* Private headers needed by this file */
#include "H5Tpkg.h"

/***********************/
/* Function Prototypes */
/***********************/

/****************************************/
/* Soft (emulated) conversion functions */
/****************************************/

/* Conversion functions between array datatypes */
H5_DLL herr_t H5T__conv_array(const H5T_t *src, const H5T_t *dst, H5T_cdata_t *cdata,
                              const H5T_conv_ctx_t *conv_ctx, size_t nelmts, size_t buf_stride,
                              size_t bkg_stride, void *buf, void *bkg);

/*********************************************/
/* Hard (compiler cast) conversion functions */
/*********************************************/

#endif /* H5Tconv_array_H */
