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
 *		H5A package.  Including this header means that the source file
 *		is part of the H5A package.
 */
#ifndef H5Amodule_H
#define H5Amodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5A_MODULE
#define H5_MY_PKG      H5A
#define H5_MY_PKG_ERR  H5E_ATTR
#define H5_MY_PKG_INIT YES

/**\defgroup H5A H5A
 *
 * Use the functions in this module to manage HDF5 attributes.
 *
 * Like HDF5 datasets, HDF5 attributes are array variables which have an element
 * datatype and a shape (dataspace). However, they perform a different function:
 * Attributes decorate other HDF5 objects, and are typically used to
 * represent application metadata. Unlike datasets, the HDF5 library does not
 * support partial I/O operations for attributes and they cannot be compressed
 * or extended.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5A_examples.c create
 *   </td>
 *   <td>
 *   \snippet{lineno} H5A_examples.c read
 *   </td>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5A_examples.c update
 *   </td>
 *   <td>
 *   \snippet{lineno} H5A_examples.c delete
 *   </td>
 * </tr>
 * </table>
 *
 */

#endif /* H5Amodule_H */
