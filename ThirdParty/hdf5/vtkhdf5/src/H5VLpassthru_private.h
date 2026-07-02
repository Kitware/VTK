/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose:	The private header file for the passthru VOL connector.
 */

#ifndef H5VLpassthru_private_H
#define H5VLpassthru_private_H

/* Include connector's public header */
#include "H5VLpassthru.h" /* Passthru VOL connector     */

/* Private headers needed by this file */
#include "H5VLprivate.h" /* Virtual Object Layer        */

/**************************/
/* Library Private Macros */
/**************************/

/****************************/
/* Library Private Typedefs */
/****************************/

/*****************************/
/* Library Private Variables */
/*****************************/

/* Passthru VOL connector's class struct */
H5_DLLVAR const H5VL_class_t H5VL_pass_through_g;

/* The native VOL connector */
H5_DLLVAR H5VL_connector_t *H5VL_PASSTHRU_conn_g;

/******************************/
/* Library Private Prototypes */
/******************************/

#endif /* H5VLpassthru_private_H */
