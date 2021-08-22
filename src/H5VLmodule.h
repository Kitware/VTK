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
 * Purpose:	This file contains declarations which define macros for the
 *          H5VL package.  Including this header means that the source file
 *          is part of the H5VL package.
 */

#ifndef H5VLmodule_H
#define H5VLmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5VL_MODULE
#define H5_MY_PKG      H5VL
#define H5_MY_PKG_ERR  H5E_VOL
#define H5_MY_PKG_INIT YES

/**
 * \defgroup H5VL H5VL
 * \brief Virtual Object Layer Interface
 * \todo Describe concisely what the functions in this module are about.
 *
 * \defgroup H5VLDEF Definitions
 * \ingroup H5VL
 * \defgroup H5VLDEV VOL Developer
 * \ingroup H5VL
 * \defgroup H5VLNAT Native VOL
 * \ingroup H5VL
 * \defgroup H5VLPT Pass-through VOL
 * \ingroup H5VL
 */

#endif /* H5VLmodule_H */
