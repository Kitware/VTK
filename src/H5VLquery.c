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
 * Purpose:     The Virtual Object Layer as described in documentation.
 *              The purpose is to provide an abstraction on how to access the
 *              underlying HDF5 container, whether in a local file with
 *              a specific file format, or remotely on other machines, etc...
 */

/****************/
/* Module Setup */
/****************/

#include "H5VLmodule.h" /* This source code file is part of the H5VL module */

/***********/
/* Headers */
/***********/

#include "H5private.h" /* Generic Functions                                */
#include "H5VLpkg.h"   /* Virtual Object Layer                             */

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Package Typedefs */
/********************/

/********************/
/* Local Prototypes */
/********************/

/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/*-------------------------------------------------------------------------
 * Function:    Retrieve the refcount for a VOL object
 *
 * Purpose:     Quick and dirty routine to retrieve the VOL object's refcount.
 *              (Mainly added to stop non-file routines from poking about in the
 *              H5VL_object_t data structure)
 *
 * Return:      Refcount on success/abort on failure (shouldn't fail)
 *
 *-------------------------------------------------------------------------
 */
size_t
H5VL_obj_get_rc(const H5VL_object_t *vol_obj)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    assert(vol_obj);

    FUNC_LEAVE_NOAPI(vol_obj->rc)
} /* end H5VL_obj_get_rc() */

/*-------------------------------------------------------------------------
 * Function:    Retrieve the connector for a VOL object
 *
 * Purpose:     Quick and dirty routine to retrieve the VOL object's connector.
 *              (Mainly added to stop non-file routines from poking about in the
 *              H5VL_object_t data structure)
 *
 * Return:      Pointer to connector on success/abort on failure (shouldn't fail)
 *
 *-------------------------------------------------------------------------
 */
H5VL_connector_t *
H5VL_obj_get_connector(const H5VL_object_t *vol_obj)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    assert(vol_obj);

    FUNC_LEAVE_NOAPI(vol_obj->connector)
} /* end H5VL_obj_get_connector() */

/*-------------------------------------------------------------------------
 * Function:    Retrieve the data for a VOL object
 *
 * Purpose:     Quick and dirty routine to retrieve the VOL object's data.
 *              (Mainly added to stop non-file routines from poking about in the
 *              H5VL_object_t data structure)
 *
 * Return:      Pointer to data on success/abort on failure (shouldn't fail)
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_obj_get_data(const H5VL_object_t *vol_obj)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    assert(vol_obj);

    FUNC_LEAVE_NOAPI(vol_obj->data)
} /* end H5VL_obj_get_data() */

/*-------------------------------------------------------------------------
 * Function:    Resetthe data for a VOL object
 *
 * Purpose:     Quick and dirty routine to reset the VOL object's data.
 *              (Mainly added to stop non-file routines from poking about in the
 *              H5VL_object_t data structure)
 *
 * Return:      none
 *
 *-------------------------------------------------------------------------
 */
void
H5VL_obj_reset_data(H5VL_object_t *vol_obj)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    assert(vol_obj);

    vol_obj->data = NULL;

    FUNC_LEAVE_NOAPI_VOID
} /* end H5VL_obj_reset_data() */
