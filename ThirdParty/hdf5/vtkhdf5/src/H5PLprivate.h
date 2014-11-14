/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5. The full HDF5 copyright notice, including      *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic document set and is     *
 * linked from the top-level documents page.  It can also be found at        *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have access   *
 * to either file, you may request a copy from help@hdfgroup.org.            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Programmer:  Raymond Lu <songyulu@hdfgroup.org>
 *              13 February 2013
 */

#ifndef _H5PLprivate_H
#define _H5PLprivate_H

/* Include package's "external" header */
#include "H5PLextern.h"

/* Private headers needed by this file */
#include "H5private.h"          /* Generic Functions                    */

#ifndef H5_VMS

/**************************/
/* Library Private Macros */
/**************************/


/****************************/
/* Library Private Typedefs */
/****************************/


/*****************************/
/* Library-private Variables */
/*****************************/


/***************************************/
/* Library-private Function Prototypes */
/***************************************/

/* Internal API routines */
H5_DLL const void *H5PL_load(H5PL_type_t plugin_type, int type_id);
H5_DLL htri_t H5PL_no_plugin(void);
#endif /*H5_VMS*/

#endif /* _H5PLprivate_H */

