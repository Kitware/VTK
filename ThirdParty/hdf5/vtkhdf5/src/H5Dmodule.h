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
 *		H5D package.  Including this header means that the source file
 *		is part of the H5D package.
 */
#ifndef H5Dmodule_H
#define H5Dmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5D_MODULE
#define H5_MY_PKG      H5D
#define H5_MY_PKG_ERR  H5E_DATASET
#define H5_MY_PKG_INIT YES

/**\defgroup H5D H5D
 *
 * Use the functions in this module to manage HDF5 datasets, including the
 * transfer of data between memory and disk and the description of dataset
 * properties. Datasets are used by other HDF5 APIs and referenced either by
 * name or by a handle. Such handles can be obtained by either creating or
 * opening the dataset.
 *
 * Typical stages in the HDF5 dataset life cycle are shown below in introductory
 * examples.
 *
 * <table>
 * <tr><th>Create</th><th>Read</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet H5D_examples.c create
 *   </td>
 *   <td>
 *   \snippet H5D_examples.c read
 *   </td>
 * <tr><th>Update</th><th>Delete</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet H5D_examples.c update
 *   </td>
 *   <td>
 *   \snippet H5D_examples.c delete
 *   </td>
 * </tr>
 * </table>
 *
 */

#endif /* H5Dmodule_H */
