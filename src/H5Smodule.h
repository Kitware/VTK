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
 *
 * Use the functions in this module to manage HDF5 dataspaces \Emph{and} selections.
 *
 * HDF5 dataspaces describe the \Emph{shape} of datasets in memory or in HDF5
 * files. Dataspaces can be empty (#H5S_NULL), a singleton (#H5S_SCALAR), or
 * a multi-dimensional, regular grid (#H5S_SIMPLE). Dataspaces can be re-shaped.
 *
 * Subsets of dataspaces can be "book-marked" or used to restrict I/O operations
 * using \Emph{selections}. Furthermore, certain set operations are supported
 * for selections.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5S_examples.c create
 *   </td>
 *   <td>
 *   \snippet{lineno} H5S_examples.c read
 *   </td>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5S_examples.c update
 *   </td>
 *   <td>
 *   \snippet{lineno} H5S_examples.c delete
 *   </td>
 * </tr>
 * </table>
 *
 */

#endif /* H5Smodule_H */
