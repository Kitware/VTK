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

/*
 * Purpose:	The private header file for the stdio VFD
 */

#ifndef H5FDstdio_private_H
#define H5FDstdio_private_H

/* Include VFD's public header */
#include "H5FDstdio.h" /* stdio VFD */

/* Private headers needed by this file */
#include "H5FDprivate.h" /* File drivers */

/**************************/
/* Library Private Macros */
/**************************/

/****************************/
/* Library Private Typedefs */
/****************************/

/*****************************/
/* Library Private Variables */
/*****************************/

/* stdio VFD's class struct */
H5_DLLVAR const H5FD_class_t H5FD_stdio_g;

/******************************/
/* Library Private Prototypes */
/******************************/

#endif /* H5FDstdio_private_H */
