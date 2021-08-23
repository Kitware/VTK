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
 * The Attribute Interface, H5A, provides a mechanism for attaching additional
 * information to a dataset, group, or named datatype.
 *
 * Attributes are accessed by opening the object that they are attached to and
 * are not independent objects. Typically an attribute is small in size and
 * contains user metadata about the object that it is attached to.
 *
 * Attributes look similar to HDF5 datasets in that they have a datatype and
 * dataspace. However, they do not support partial I/O operations and cannot be
 * compressed or extended.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet H5A_examples.c create
 *   </td>
 *   <td>
 *   \snippet H5A_examples.c read
 *   </td>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet H5A_examples.c update
 *   </td>
 *   <td>
 *   \snippet H5A_examples.c delete
 *   </td>
 * </tr>
 * </table>
 *
 */

#endif /* H5Amodule_H */
