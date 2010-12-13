/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Programmer:  Scott Wegner <swegner@hdfgroup.org>
 *				Based on code by Robb Matzke
 *              Thursday, May 24 2007
 *
 * Purpose:	The public header file for the windows driver.
 */
#ifndef H5FDwindows_H
#define H5FDwindows_H

#include "H5Ipublic.h"

#define H5FD_WINDOWS	(H5FD_windows_init())

#ifdef __cplusplus
extern "C" {
#endif

H5_DLL hid_t H5FD_windows_init(void);
H5_DLL void H5FD_windows_term(void);
H5_DLL herr_t H5Pset_fapl_windows(hid_t fapl_id);

#ifdef __cplusplus
}
#endif

#endif
