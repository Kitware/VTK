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

/*
 * Programmer:  Quincey Koziol
 *              Saturday, September 12, 2015
 *
 * Purpose:     This file contains declarations which define macros for the
 *              H5I package.  Including this header means that the source file
 *              is part of the H5I package.
 */
#ifndef H5Imodule_H
#define H5Imodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 * reporting macros.
 */
#define H5I_MODULE
#define H5_MY_PKG      H5I
#define H5_MY_PKG_ERR  H5E_ATOM
#define H5_MY_PKG_INIT NO

/**\defgroup H5I H5I
 * \brief Identifier Interface
 * \todo Describe concisely what the functions in this module are about.
 */

#endif /* H5Imodule_H */
