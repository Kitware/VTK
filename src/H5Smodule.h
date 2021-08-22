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
 * Programmer:	Quincey Koziol
 *		Saturday, September 12, 2015
 *
 * Purpose:	This file contains declarations which define macros for the
 *		H5S package.  Including this header means that the source file
 *		is part of the H5S package.
 */
#ifndef H5Smodule_H
#define H5Smodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5S_MODULE
#define H5_MY_PKG      H5S
#define H5_MY_PKG_ERR  H5E_DATASPACE
#define H5_MY_PKG_INIT YES

/**\defgroup H5S H5S
 * \brief Dataspace Interface
 *
 * \details The Dataspace Interface provides functions for creating and
 *          working with dataspaces.
 *
 *          A dataspace has two roles:
 *
 *          \li It contains the spatial information (logical layout) of a
 *              dataset stored in a file.
 *          \li It describes an application’s data buffers and data elements
 *              participating in I/O. In other words, it can be used to
 *              select a portion or subset of a dataset.
 *
 *          The spatial information of a dataset in a file includes the
 *          rank and dimensions of the dataset, which are a permanent part
 *          of the dataset definition. It can have dimensions that are fixed
 *          (unchanging) or unlimited, which means they can grow in size
 *          (or are extendible).
 *
 *          A dataspace can consist of:
 *          \li  no elements (NULL)
 *          \li  a single element (scalar), or
 *          \li  a simple array.
 *
 */

#endif /* H5Smodule_H */
