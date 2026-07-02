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
 * Purpose:     Private routines for the internal passthru VOL connector.
 *
 *              Necessary for using internal library routines, which are
 *              disallowed within the actual passthru VOL connector code.
 *
 */

/****************/
/* Module Setup */
/****************/

#define H5VL_FRIEND /* Suppress error about including H5VLpkg   */

/***********/
/* Headers */
/***********/

#include "H5private.h"  /* Generic Functions                        */
#include "H5Eprivate.h" /* Error handling                           */
#include "H5Iprivate.h" /* IDs                                      */
#include "H5Pprivate.h" /* Property lists                           */
#include "H5VLpkg.h"    /* Virtual Object Layer                     */

#include "H5VLpassthru_private.h" /* Passthru  VOL connector        */

/* The native passthru VOL connector */
hid_t             H5VL_PASSTHRU_g      = H5I_INVALID_HID;
H5VL_connector_t *H5VL_PASSTHRU_conn_g = NULL;

/*-------------------------------------------------------------------------
 * Function:    H5VL__passthru_register
 *
 * Purpose:     Register the passthru VOL connector and set up an ID for it.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__passthru_register(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Register the passthru VOL connector, if it isn't already */
    if (NULL == H5VL_PASSTHRU_conn_g)
        if (NULL == (H5VL_PASSTHRU_conn_g =
                         H5VL__register_connector(&H5VL_pass_through_g, H5P_VOL_INITIALIZE_DEFAULT)))
            HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, FAIL, "can't register passthru VOL connector");

    /* Get ID for connector */
    if (H5I_VOL != H5I_get_type(H5VL_PASSTHRU_g)) {
        if ((H5VL_PASSTHRU_g = H5I_register(H5I_VOL, H5VL_PASSTHRU_conn_g, false)) < 0)
            HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, FAIL, "can't create ID for passthru VOL connector");

        /* ID is holding a reference to the connector */
        H5VL_conn_inc_rc(H5VL_PASSTHRU_conn_g);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__passthru_register() */

/*---------------------------------------------------------------------------
 * Function:    H5VL__passthru_unregister
 *
 * Purpose:     Shut down the passthru VOL
 *
 * Returns:     SUCCEED (Can't fail)
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VL__passthru_unregister(void)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Reset VOL connector info */
    H5VL_PASSTHRU_g      = H5I_INVALID_HID;
    H5VL_PASSTHRU_conn_g = NULL;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5VL__passthru_unregister() */
