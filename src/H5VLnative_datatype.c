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
 * Purpose:     Datatype callbacks for the native VOL connector
 *
 */

#define H5T_FRIEND /* Suppress error about including H5Tpkg    */

#include "H5private.h"   /* Generic Functions                        */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Gprivate.h"  /* Groups                                   */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5Oprivate.h"  /* Object headers                           */
#include "H5Pprivate.h"  /* Property lists                           */
#include "H5Tpkg.h"      /* Datatypes                                */
#include "H5VLprivate.h" /* Virtual Object Layer                     */

#include "H5VLnative_private.h" /* Native VOL connector                     */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_datatype_commit
 *
 * Purpose:     Handles the datatype commit callback
 *
 * Return:      Success:    datatype pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_datatype_commit(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t type_id,
                             hid_t lcpl_id, hid_t tcpl_id, hid_t H5_ATTR_UNUSED tapl_id,
                             hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5G_loc_t loc;              /* Location to commit datatype */
    H5T_t *   dt;               /* Datatype for ID */
    H5T_t *   type      = NULL; /* copy of the original type which will be committed */
    void *    ret_value = NULL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* check arguments */
    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file or file object")

    if (NULL == (dt = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a datatype")

    /* Check arguments.  We cannot commit an immutable type because H5Tclose()
     * normally fails on such types (try H5Tclose(H5T_NATIVE_INT)) but closing
     * a named type should always succeed.
     */
    if (H5T_STATE_NAMED == dt->shared->state || H5T_STATE_OPEN == dt->shared->state)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "datatype is already committed")
    if (H5T_STATE_IMMUTABLE == dt->shared->state)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "datatype is immutable")

    /* Check for a "sensible" datatype to store on disk */
    if (H5T_is_sensible(dt) <= 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "datatype is not sensible")

    /* Copy the datatype - the copied one will be the type that is
     * committed, and attached to original datatype above the VOL
     * layer
     */
    if (NULL == (type = H5T_copy(dt, H5T_COPY_TRANSIENT)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to copy");

    /* Commit the datatype */
    if (NULL != name) {
        /* H5Tcommit */
        if (H5T__commit_named(&loc, name, type, lcpl_id, tcpl_id) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to commit datatype")
    } /* end if */
    else {
        /* H5Tcommit_anon */
        if (H5T__commit_anon(loc.oloc->file, type, tcpl_id) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to commit datatype")
    } /* end else */

    ret_value = (void *)type;

done:
    if (NULL == ret_value && type)
        H5T_close(type);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_datatype_commit() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_datatype_open
 *
 * Purpose:     Handles the datatype open callback
 *
 * Return:      Success:    datatype pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_datatype_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
                           hid_t H5_ATTR_UNUSED tapl_id, hid_t H5_ATTR_UNUSED dxpl_id,
                           void H5_ATTR_UNUSED **req)
{
    H5T_t *   type = NULL; /* Datatype opened in file */
    H5G_loc_t loc;         /* Group location of object to open */
    void *    ret_value = NULL;

    FUNC_ENTER_PACKAGE

    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file or file object")

    /* Open the datatype */
    if (NULL == (type = H5T__open_name(&loc, name)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, NULL, "unable to open named datatype")

    type->vol_obj = NULL;

    ret_value = (void *)type;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_datatype_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_datatype_get
 *
 * Purpose:     Handles the datatype get callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_datatype_get(void *obj, H5VL_datatype_get_t get_type, hid_t H5_ATTR_UNUSED dxpl_id,
                          void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5T_t *dt        = (H5T_t *)obj;
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (get_type) {
        case H5VL_DATATYPE_GET_BINARY: {
            ssize_t *nalloc = HDva_arg(arguments, ssize_t *);
            void *   buf    = HDva_arg(arguments, void *);
            size_t   size   = HDva_arg(arguments, size_t);

            if (H5T_encode(dt, (unsigned char *)buf, &size) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't determine serialized length of datatype")

            *nalloc = (ssize_t)size;
            break;
        }

        /* H5Tget_create_plist */
        case H5VL_DATATYPE_GET_TCPL: {
            hid_t *ret_id = HDva_arg(arguments, hid_t *);

            if (H5I_INVALID_HID == (*ret_id = H5T__get_create_plist(dt)))
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get object creation info");

            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get this type of information from datatype")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_datatype_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_datatype_specific
 *
 * Purpose:     Handles the datatype specific callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_datatype_specific(void *obj, H5VL_datatype_specific_t specific_type,
                               hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5T_t *dt        = (H5T_t *)obj;
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (specific_type) {
        case H5VL_DATATYPE_FLUSH: {
            hid_t type_id = HDva_arg(arguments, hid_t);

            /* To flush metadata and invoke flush callback if there is */
            if (H5O_flush_common(&dt->oloc, type_id) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTFLUSH, FAIL, "unable to flush datatype")

            break;
        }

        case H5VL_DATATYPE_REFRESH: {
            hid_t type_id = HDva_arg(arguments, hid_t);

            /* Call private function to refresh datatype object */
            if ((H5O_refresh_metadata(type_id, dt->oloc)) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTLOAD, FAIL, "unable to refresh datatype")

            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "invalid specific operation")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_datatype_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_datatype_close
 *
 * Purpose:     Handles the datatype close callback
 *
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL (datatype will not be closed)
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_datatype_close(void *dt, hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    if (H5T_close((H5T_t *)dt) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTDEC, FAIL, "can't close datatype")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_datatype_close() */
