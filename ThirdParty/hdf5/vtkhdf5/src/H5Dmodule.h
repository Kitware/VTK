/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
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
 * Programmer:	Quincey Koziol <koziol@hdfgroup.org>
 *		Saturday, September 12, 2015
 *
 * Purpose:	This file contains declarations which define macros for the
 *		H5D package.  Including this header means that the source file
 *		is part of the H5D package.
 */
#ifndef _H5Dmodule_H
#define _H5Dmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5D_MODULE
#define H5_MY_PKG       H5D
#define H5_MY_PKG_ERR   H5E_DATASET
#define H5_MY_PKG_INIT  YES

#endif /* _H5Dmodule_H */

