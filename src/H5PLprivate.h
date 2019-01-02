/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5. The full HDF5 copyright notice, including      *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * This file contains private information about the H5PL module
 */

#ifndef _H5PLprivate_H
#define _H5PLprivate_H

/* Include package's public header */
#include "H5PLpublic.h"

/* Private headers needed by this file */
#include "H5private.h"          /* Generic Functions                    */


/**************************/
/* Library Private Macros */
/**************************/


/****************************/
/* Library Private Typedefs */
/****************************/

/* The key that will be used to find the plugin */
typedef union H5PL_key_t {
    int         id;         /* filters      */
} H5PL_key_t;


/*****************************/
/* Library-private Variables */
/*****************************/


/***************************************/
/* Library-private Function Prototypes */
/***************************************/

/* Internal API routines */
H5_DLL const void *H5PL_load(H5PL_type_t plugin_type, H5PL_key_t key);

#endif /* _H5PLprivate_H */

