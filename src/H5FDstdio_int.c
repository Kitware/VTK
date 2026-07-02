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
 * Purpose:     Private routines for the stdio VFD.
 *
 *              Necessary for using internal library routines, which are
 *              disallowed within the actual stdio VFD code.
 *
 */

/****************/
/* Module Setup */
/****************/

#define H5FD_FRIEND /* Suppress error about including H5FDpkg   */

/***********/
/* Headers */
/***********/

#include "H5private.h"  /* Generic Functions                        */
#include "H5Eprivate.h" /* Error handling                           */
#include "H5Iprivate.h" /* IDs                                      */
#include "H5FDpkg.h"    /* File drivers                             */

#include "H5FDstdio_private.h" /* stdio VFD */

/* The driver identification number, initialized at runtime */
hid_t H5FD_STDIO_id_g = H5I_INVALID_HID;

/*-------------------------------------------------------------------------
 * Function:    H5FD__stdio_register
 *
 * Purpose:     Register the driver with the library.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__stdio_register(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    if (H5I_VFL != H5I_get_type(H5FD_STDIO_id_g))
        if ((H5FD_STDIO_id_g = H5FD_register(&H5FD_stdio_g, sizeof(H5FD_class_t), false)) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTREGISTER, FAIL, "unable to register stdio driver");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__stdio_register() */

/*---------------------------------------------------------------------------
 * Function:    H5FD_stdio_unregister
 *
 * Purpose:     Reset library driver info.
 *
 * Returns:     SUCCEED (Can't fail)
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5FD__stdio_unregister(void)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Reset VFL ID */
    H5FD_STDIO_id_g = H5I_INVALID_HID;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_stdio_unregister() */
