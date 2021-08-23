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
 *		H5F package.  Including this header means that the source file
 *		is part of the H5F package.
 */
#ifndef H5Fmodule_H
#define H5Fmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5F_MODULE
#define H5_MY_PKG      H5F
#define H5_MY_PKG_ERR  H5E_FILE
#define H5_MY_PKG_INIT YES

/**
 * \defgroup H5F H5F
 *
 * Use the functions in this module to manage HDF5 files.
 *
 * In the code snippets below, we show the skeletal life cycle of an HDF5 file,
 * when creating a new file (left) or when opening an existing file (right).
 * File creation is essentially controlled through \ref FCPL, and file access to
 * new and existing files is controlled through \ref FAPL. The file \c name and
 * creation or access \c mode control the interaction with the underlying
 * storage such as file systems.
 *
 * \Emph{Proper error handling is part of the life cycle.}
 * <table>
 * <tr><th>Create</th><th>Open</th></tr>
 * <tr valign="top">
 *   <td>
 *   \snippet H5F_examples.c life_cycle
 *   </td>
 *   <td>
 *   \snippet H5F_examples.c life_cycle_w_open
 *   </td>
 * </tr>
 * </table>
 *
 * In addition to general file management functions, there are three categories
 * of functions that deal with advanced file management tasks and use cases:
 * 1. The control of the HDF5 \ref MDC
 * 2. The use of (MPI-) \ref PH5F HDF5
 * 3. The \ref SWMR pattern
 *
 * \defgroup MDC Metadata Cache
 * \ingroup H5F
 * \defgroup PH5F Parallel
 * \ingroup H5F
 * \defgroup SWMR Single Writer Multiple Readers
 * \ingroup H5F
 */

#endif /* H5Fmodule_H */
