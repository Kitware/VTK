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
 *              Monday, April  6, 2020
 *
 * Purpose:     This file contains declarations which define macros for the
 *              H5ES package.  Including this header means that the source file
 *              is part of the H5ES package.
 */
#ifndef H5ESmodule_H
#define H5ESmodule_H

/* Define the proper control macros for the generic FUNC_ENTER/LEAVE and error
 *      reporting macros.
 */
#define H5ES_MODULE
#define H5_MY_PKG      H5ES
#define H5_MY_PKG_ERR  H5E_EVENTSET
#define H5_MY_PKG_INIT YES

/**\defgroup H5ES H5ES
 *
 * \todo Add the event set life cycle.
 *
 * \brief Event Set Interface
 *
 * \details \Bold{This interface can be only used with the HDF5 VOL connectors that
 *          enable the asynchronous feature in HDF5.} The native HDF5 library has
 *          only synchronous operations.
 *
 *          HDF5 VOL connectors with support for asynchronous operations:
 *          - ASYNC
 *          - DAOS
 *
 * \par Example:
 * \code
 * fid = H5Fopen(..);
 * gid = H5Gopen(fid, ..);  //Starts when H5Fopen completes
 * did = H5Dopen(gid, ..);  //Starts when H5Gopen completes
 *
 * es_id = H5EScreate();  // Create event set for tracking async operations
 * status = H5Dwrite_async(did, .., es_id);  //Asynchronous, starts when H5Dopen completes,
 *                                           // may run concurrently with other H5Dwrite_async
 *                                           // in event set.
 * status = H5Dwrite_async(did, .., es_id);  //Asynchronous, starts when H5Dopen completes,
 *                                           // may run concurrently with other H5Dwrite_async
 *                                           // in event set....
 * <other user code>
 * ...
 * H5ESwait(es_id); // Wait for operations in event set to complete, buffers
 *                  // used for H5Dwrite_async must only be changed after wait
 *                  // returns.
 * \endcode
 */

#endif /* H5ESmodule_H */
