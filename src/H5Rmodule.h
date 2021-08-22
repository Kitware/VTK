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

/* Purpose: This file contains declarations which define macros for the
 *          H5R package.  Including this header means that the source file
 *          is part of the H5R package.
 */
#ifndef H5Rmodule_H
#define H5Rmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5R_MODULE
#define H5_MY_PKG      H5R
#define H5_MY_PKG_ERR  H5E_REFERENCE
#define H5_MY_PKG_INIT YES

/**
 * \defgroup H5R H5R
 * \brief Reference Interface
 * \details The HDF5 Reference Interface, H5R, provides a mechanism for managing
 *          HDF5 referenced objects.
 */

#endif /* H5Rmodule_H */
