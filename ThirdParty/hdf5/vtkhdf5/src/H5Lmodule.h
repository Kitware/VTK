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
 *		H5L package.  Including this header means that the source file
 *		is part of the H5L package.
 */
#ifndef H5Lmodule_H
#define H5Lmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5L_MODULE
#define H5_MY_PKG      H5L
#define H5_MY_PKG_ERR  H5E_LINK
#define H5_MY_PKG_INIT YES

/**\defgroup H5L H5L
 *
 * Use the functions in this module to manage HDF5 links and link types.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5L_examples.c create
 *   </td>
 *   <td>
 *   \snippet{lineno} H5L_examples.c iter_cb
 *   \snippet{lineno} H5L_examples.c read
 *   </td>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet{lineno} H5L_examples.c update
 *   </td>
 *   <td>
 *   \snippet{lineno} H5L_examples.c delete
 *   </td>
 * </tr>
 * </table>
 *
 * \defgroup TRAV Link Traversal
 * \ingroup H5L
 * \defgroup H5LA Advanced Link Functions
 * \ingroup H5L
 */

#endif /* H5Lmodule_H */
