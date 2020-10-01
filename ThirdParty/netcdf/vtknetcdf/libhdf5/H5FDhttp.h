/*********************************************************************
*    Copyright 2018, UCAR/Unidata
*    See netcdf/COPYRIGHT file for copying and redistribution conditions.
* ********************************************************************/

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Programmer:  Dennis Heimbigner (dmh@ucar.edu)
 *              Dec. 26 2018
 *
 * Purpose:	The public header file for the s3 driver.
 * 
 * Derived from the HDF5 Source file H5FDstdio.c
 */

#ifndef H5FDHTTP_H
#define H5FDHTTP_H

#include "H5Ipublic.h"

#define H5FD_HTTP	(H5FD_http_init())

#ifdef __cplusplus
extern "C" {
#endif

#if 0
H5_DLL hid_t H5FD_http_init(void);
H5_DLL herr_t H5Pset_fapl_http(hid_t fapl_id);
#else
EXTERNL hid_t H5FD_http_init(void);
EXTERNL herr_t H5Pset_fapl_http(hid_t fapl_id);
#endif

#ifdef __cplusplus
}
#endif

#endif /*H5FDHTTP_H*/
