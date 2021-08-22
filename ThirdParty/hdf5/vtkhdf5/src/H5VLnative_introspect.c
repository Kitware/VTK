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
 * Purpose:     Connector/container introspection callbacks for the native VOL connector
 *
 */

#include "H5private.h"   /* Generic Functions                        */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5VLprivate.h" /* Virtual Object Layer                     */

#include "H5VLnative_private.h" /* Native VOL connector                     */

/* Note: H5VL__native_introspect_get_conn_cls is in src/H5VLnative.c so that
 *      it can return the address of the staticly declared class struct.
 */

/*---------------------------------------------------------------------------
 * Function:    H5VL__native_introspect_opt_query
 *
 * Purpose:     Query if an optional operation is supported by this connector
 *
 * Returns:     SUCCEED (Can't fail)
 *
 *---------------------------------------------------------------------------
 */
herr_t
H5VL__native_introspect_opt_query(void H5_ATTR_UNUSED *obj, H5VL_subclass_t H5_ATTR_UNUSED cls,
                                  int H5_ATTR_UNUSED opt_type, hbool_t *supported)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity check */
    HDassert(supported);

    /* The native VOL connector supports all optional operations */
    *supported = TRUE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5VL__native_introspect_opt_query() */
