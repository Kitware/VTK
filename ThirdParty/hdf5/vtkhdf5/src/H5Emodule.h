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
 *		H5E package.  Including this header means that the source file
 *		is part of the H5E package.
 */
#ifndef H5Emodule_H
#define H5Emodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5E_MODULE
#define H5_MY_PKG      H5E
#define H5_MY_PKG_ERR  H5E_ERROR
#define H5_MY_PKG_INIT YES

/**
 * \defgroup H5E H5E
 * \brief Error Handling Interface
 *
 * \details The Error interface provides error handling in the form of a stack.
 *          The \Code{FUNC_ENTER} macro clears the error stack whenever an
 *          interface function is entered. When an error is detected, an entry
 *          is pushed onto the stack. As the functions unwind, additional
 *          entries are pushed onto the stack. The API function will return some
 *          indication that an error occurred and the application can print the
 *          error stack.
 *
 *          Certain API functions in the \c H5E package, such as H5Eprint1(), do
 *          not clear the error stack. Otherwise, any function which does not
 *          have an underscore immediately after the package name will clear the
 *          error stack. For instance, H5Fopen() clears the error stack while
 *          \Code{H5F_open} does not.
 *
 *          An error stack has a fixed maximum size. If this size is exceeded
 *          then the stack will be truncated and only the inner-most functions
 *          will have entries on the stack. This is expected to be a rare
 *          condition.
 *
 *          Each thread has its own error stack, but since multi-threading has
 *          not been added to the library yet, this package maintains a single
 *          error stack. The error stack is statically allocated to reduce the
 *          complexity of handling errors within the \c H5E package.
 */

#endif /* H5Emodule_H */
