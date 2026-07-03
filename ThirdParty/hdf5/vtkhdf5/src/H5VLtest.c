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

/*-------------------------------------------------------------------------
 *
 * Created:		H5VLtest.c
 *
 * Purpose:		Virtual Object Layer (VOL) testing routines.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5VLmodule.h" /* This source code file is part of the H5VL module */
#define H5VL_TESTING    /* Suppress warning about H5VL testing funcs        */

/***********/
/* Headers */
/***********/
#include "H5private.h"  /* Generic Functions                    */
#include "H5Eprivate.h" /* Error handling                       */
#include "H5Iprivate.h" /* IDs                                  */
#include "H5VLpkg.h"    /* Virtual Object Layer                 */

/* VOL connectors */
#include "H5VLnative_private.h" /* Native VOL connector                 */

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
 * Function:    H5VL__reparse_def_vol_conn_variable_test
 *
 * Purpose:     Re-parse the default VOL connector environment variable.
 *
 *              Since getenv(3) is fairly expensive, we only parse it once,
 *              when the library opens. This test function is used to
 *              re-parse the environment variable after we've changed it
 *              with setenv(3).
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__reparse_def_vol_conn_variable_test(void)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    /* Re-check for the HDF5_VOL_CONNECTOR environment variable */
    if (H5VL__set_def_conn() < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTINIT, FAIL, "unable to initialize default VOL connector");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__reparse_def_vol_conn_variable_test() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__is_native_connector_test
 *
 * Purpose:     Check if connector is the native connector
 *
 * Return:      true/false/FAIL
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5VL__is_native_connector_test(hid_t vol_id)
{
    H5VL_connector_t *native, *connector;
    int               cmp_value;           /* Comparison result */
    htri_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    if (NULL == (connector = H5I_object_verify(vol_id, H5I_VOL)))
        HGOTO_ERROR(H5E_VOL, H5E_BADTYPE, FAIL, "not a VOL connector ID");

    /* For the time being, we disallow unregistering the native VOL connector */
    native = H5VL_NATIVE_conn_g;
    if (H5VL_cmp_connector_cls(&cmp_value, connector->cls, native->cls) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTCOMPARE, FAIL, "can't compare connector classes");
    ret_value = (0 == cmp_value);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__is_native_connector_test() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__register_using_vol_id_test
 *
 * Purpose:     Test infra wrapper around H5VL_register
 *
 * Return:      Success:    A valid HDF5 ID
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5VL__register_using_vol_id_test(H5I_type_t type, void *object, hid_t vol_id)
{
    H5VL_connector_t *connector;
    hid_t             ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    if (NULL == (connector = H5I_object_verify(vol_id, H5I_VOL)))
        HGOTO_ERROR(H5E_VOL, H5E_BADTYPE, H5I_INVALID_HID, "not a VOL connector ID");

    /* Get an ID for the object */
    if ((ret_value = H5VL_register(type, object, connector, true)) < 0)
        HGOTO_ERROR(H5E_VOL, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to get an ID for the object");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__register_using_vol_id_test() */
