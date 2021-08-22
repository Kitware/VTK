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
 * Purpose:     Object callbacks for the native VOL connector
 *
 */

#define H5O_FRIEND /* Suppress error about including H5Opkg    */
#define H5F_FRIEND /* Suppress error about including H5Fpkg    */

#include "H5private.h"   /* Generic Functions                        */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Fpkg.h"      /* Files (pkg needed for id_exists)         */
#include "H5Gprivate.h"  /* Groups                                   */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5Opkg.h"      /* Object headers                           */
#include "H5Pprivate.h"  /* Property lists                           */
#include "H5VLprivate.h" /* Virtual Object Layer                     */

#include "H5VLnative_private.h" /* Native VOL connector                     */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_object_open
 *
 * Purpose:     Handles the object open callback
 *
 * Return:      Success:    object pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_object_open(void *obj, const H5VL_loc_params_t *loc_params, H5I_type_t *opened_type,
                         hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5G_loc_t loc;
    void *    ret_value = NULL;

    FUNC_ENTER_PACKAGE

    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file or file object")

    switch (loc_params->type) {
        case H5VL_OBJECT_BY_NAME: {
            /* Open the object */
            if (NULL == (ret_value = H5O_open_name(&loc, loc_params->loc_data.loc_by_name.name, opened_type)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, NULL, "unable to open object by name")
            break;
        }

        case H5VL_OBJECT_BY_IDX: {
            /* Open the object */
            if (NULL == (ret_value = H5O__open_by_idx(&loc, loc_params->loc_data.loc_by_idx.name,
                                                      loc_params->loc_data.loc_by_idx.idx_type,
                                                      loc_params->loc_data.loc_by_idx.order,
                                                      loc_params->loc_data.loc_by_idx.n, opened_type)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, NULL, "unable to open object by index")
            break;
        }

        case H5VL_OBJECT_BY_TOKEN: {
            H5O_token_t token = *loc_params->loc_data.loc_by_token.token;
            haddr_t     addr;

            /* Decode token */
            if (H5VL_native_token_to_addr(loc.oloc->file, H5I_FILE, token, &addr) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTUNSERIALIZE, NULL,
                            "can't deserialize object token into address")

            /* Open the object */
            if (NULL == (ret_value = H5O__open_by_addr(&loc, addr, opened_type)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, NULL, "unable to open object by address")
            break;
        }

        case H5VL_OBJECT_BY_SELF:
        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "unknown open parameters")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_object_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_object_copy
 *
 * Purpose:     Handles the object copy callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_object_copy(void *src_obj, const H5VL_loc_params_t *loc_params1, const char *src_name,
                         void *dst_obj, const H5VL_loc_params_t *loc_params2, const char *dst_name,
                         hid_t ocpypl_id, hid_t lcpl_id, hid_t H5_ATTR_UNUSED dxpl_id,
                         void H5_ATTR_UNUSED **req)
{
    H5G_loc_t src_loc; /* Source object group location */
    H5G_loc_t dst_loc; /* Destination group location */
    herr_t    ret_value = FAIL;

    FUNC_ENTER_PACKAGE

    /* get location for objects */
    if (H5G_loc_real(src_obj, loc_params1->obj_type, &src_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")
    if (H5G_loc_real(dst_obj, loc_params2->obj_type, &dst_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    /* Copy the object */
    if ((ret_value = H5O__copy(&src_loc, src_name, &dst_loc, dst_name, ocpypl_id, lcpl_id)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_object_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_object_get
 *
 * Purpose:     Handles the object get callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_object_get(void *obj, const H5VL_loc_params_t *loc_params, H5VL_object_get_t get_type,
                        hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req, va_list arguments)
{
    herr_t    ret_value = SUCCEED; /* Return value */
    H5G_loc_t loc;                 /* Location of group */

    FUNC_ENTER_PACKAGE

    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    switch (get_type) {

        /* Object file */
        case H5VL_OBJECT_GET_FILE: {
            void **ret = HDva_arg(arguments, void **);

            if (loc_params->type == H5VL_OBJECT_BY_SELF) {
                *ret = (void *)loc.oloc->file;

                /* TODO we currently need to set id_exists to TRUE because
                 * the upper layer will create an ID from the returned
                 * object. In theory this should not be needed and id_exists
                 * should be removed once the H5Fmount code gets fixed. */
                loc.oloc->file->id_exists = TRUE;
            }
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown get_file parameters")
            break;
        }

        /* Object name */
        case H5VL_OBJECT_GET_NAME: {
            ssize_t *ret  = HDva_arg(arguments, ssize_t *);
            char *   name = HDva_arg(arguments, char *);
            size_t   size = HDva_arg(arguments, size_t);

            if (loc_params->type == H5VL_OBJECT_BY_SELF) {
                /* Retrieve object's name */
                if ((*ret = H5G_get_name(&loc, name, size, NULL)) < 0)
                    HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't retrieve object name")
            } /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_TOKEN) {
                H5O_loc_t   obj_oloc; /* Object location */
                H5O_token_t token = *loc_params->loc_data.loc_by_token.token;

                /* Initialize the object location */
                H5O_loc_reset(&obj_oloc);
                obj_oloc.file = loc.oloc->file;

                /* Decode token */
                if (H5VL_native_token_to_addr(obj_oloc.file, H5I_FILE, token, &obj_oloc.addr) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTUNSERIALIZE, FAIL,
                                "can't deserialize object token into address")

                /* Retrieve object's name */
                if ((*ret = H5G_get_name_by_addr(loc.oloc->file, &obj_oloc, name, size)) < 0)
                    HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't determine object name")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown get_name parameters")
            break;
        }

        /* Object type */
        case H5VL_OBJECT_GET_TYPE: {
            H5O_type_t *obj_type = HDva_arg(arguments, H5O_type_t *);

            if (loc_params->type == H5VL_OBJECT_BY_TOKEN) {
                H5O_loc_t   obj_oloc; /* Object location */
                unsigned    rc;       /* Reference count of object */
                H5O_token_t token = *loc_params->loc_data.loc_by_token.token;

                /* Initialize the object location */
                H5O_loc_reset(&obj_oloc);
                obj_oloc.file = loc.oloc->file;

                /* Decode token */
                if (H5VL_native_token_to_addr(obj_oloc.file, H5I_FILE, token, &obj_oloc.addr) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTUNSERIALIZE, FAIL,
                                "can't deserialize object token into address")

                /* Get the # of links for object, and its type */
                /* (To check to make certain that this object hasn't been deleted) */
                if (H5O_get_rc_and_type(&obj_oloc, &rc, obj_type) < 0 || 0 == rc)
                    HGOTO_ERROR(H5E_REFERENCE, H5E_LINKCOUNT, FAIL, "dereferencing deleted object")
            }
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown get_type parameters")
            break;
        }

        /* H5Oget_info(_name|_by_idx)3 */
        case H5VL_OBJECT_GET_INFO: {
            H5O_info2_t *oinfo  = HDva_arg(arguments, H5O_info2_t *);
            unsigned     fields = HDva_arg(arguments, unsigned);

            /* Use the original H5Oget_info code to get the data */

            if (loc_params->type == H5VL_OBJECT_BY_SELF) { /* H5Oget_info */
                /* Retrieve the object's information */
                if (H5G_loc_info(&loc, ".", oinfo, fields) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
            }                                                   /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Oget_info_by_name */
                /* Retrieve the object's information */
                if (H5G_loc_info(&loc, loc_params->loc_data.loc_by_name.name, oinfo, fields) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
            }                                                  /* end else-if */
            else if (loc_params->type == H5VL_OBJECT_BY_IDX) { /* H5Oget_info_by_idx */
                H5G_loc_t  obj_loc;                            /* Location used to open group */
                H5G_name_t obj_path;                           /* Opened object group hier. path */
                H5O_loc_t  obj_oloc;                           /* Opened object object location */

                /* Set up opened group location to fill in */
                obj_loc.oloc = &obj_oloc;
                obj_loc.path = &obj_path;
                H5G_loc_reset(&obj_loc);

                /* Find the object's location, according to the order in the index */
                if (H5G_loc_find_by_idx(&loc, loc_params->loc_data.loc_by_idx.name,
                                        loc_params->loc_data.loc_by_idx.idx_type,
                                        loc_params->loc_data.loc_by_idx.order,
                                        loc_params->loc_data.loc_by_idx.n, &obj_loc /*out*/) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "group not found")

                /* Retrieve the object's information */
                if (H5O_get_info(obj_loc.oloc, oinfo, fields) < 0) {
                    H5G_loc_free(&obj_loc);
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't retrieve object info")
                } /* end if */

                /* Release the object location */
                if (H5G_loc_free(&obj_loc) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "can't free location")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_OHDR, H5E_UNSUPPORTED, FAIL, "unknown get info parameters")
            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get this type of information from object")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_object_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_object_specific
 *
 * Purpose:     Handles the object specific callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_object_specific(void *obj, const H5VL_loc_params_t *loc_params,
                             H5VL_object_specific_t specific_type, hid_t H5_ATTR_UNUSED dxpl_id,
                             void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5G_loc_t loc;
    herr_t    ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    switch (specific_type) {
        /* H5Oincr_refcount / H5Odecr_refcount */
        case H5VL_OBJECT_CHANGE_REF_COUNT: {
            int        update_ref = HDva_arg(arguments, int);
            H5O_loc_t *oloc       = loc.oloc;

            if (H5O_link(oloc, update_ref) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_LINKCOUNT, FAIL, "modifying object link count failed")

            break;
        }

        /* H5Oexists_by_name */
        case H5VL_OBJECT_EXISTS: {
            htri_t *ret = HDva_arg(arguments, htri_t *);

            if (loc_params->type == H5VL_OBJECT_BY_NAME) {
                /* Check if the object exists */
                if ((*ret = H5G_loc_exists(&loc, loc_params->loc_data.loc_by_name.name)) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to determine if '%s' exists",
                                loc_params->loc_data.loc_by_name.name)
            } /* end if */
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown object exists parameters")
            break;
        }

        /* Lookup object */
        case H5VL_OBJECT_LOOKUP: {
            H5O_token_t *token = HDva_arg(arguments, H5O_token_t *);

            HDassert(token);

            if (loc_params->type == H5VL_OBJECT_BY_NAME) {
                H5G_loc_t  obj_loc;  /* Group hier. location of object */
                H5G_name_t obj_path; /* Object group hier. path */
                H5O_loc_t  obj_oloc; /* Object object location */

                /* Set up opened group location to fill in */
                obj_loc.oloc = &obj_oloc;
                obj_loc.path = &obj_path;
                H5G_loc_reset(&obj_loc);

                /* Find the object */
                if (H5G_loc_find(&loc, loc_params->loc_data.loc_by_name.name, &obj_loc) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")

                /* Encode token */
                if (H5VL_native_addr_to_token(loc.oloc->file, H5I_FILE, obj_loc.oloc->addr, token) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTSERIALIZE, FAIL,
                                "can't serialize address into object token")

                /* Release the object location */
                if (H5G_loc_free(&obj_loc) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "can't free location")
            } /* end if */
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown object exists parameters")
            break;
        }

        case H5VL_OBJECT_VISIT: {
            H5_index_t      idx_type = (H5_index_t)HDva_arg(arguments, int);      /* enum work-around */
            H5_iter_order_t order    = (H5_iter_order_t)HDva_arg(arguments, int); /* enum work-around */
            H5O_iterate2_t  op       = HDva_arg(arguments, H5O_iterate2_t);
            void *          op_data  = HDva_arg(arguments, void *);
            unsigned        fields   = HDva_arg(arguments, unsigned);

            /* Call internal object visitation routine */
            if (loc_params->type == H5VL_OBJECT_BY_SELF) {
                /* H5Ovisit */
                if ((ret_value = H5O__visit(&loc, ".", idx_type, order, op, op_data, fields)) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "object visitation failed")
            } /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_NAME) {
                /* H5Ovisit_by_name */
                if ((ret_value = H5O__visit(&loc, loc_params->loc_data.loc_by_name.name, idx_type, order, op,
                                            op_data, fields)) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "object visitation failed")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown object visit params");
            break;
        }

        case H5VL_OBJECT_FLUSH: {
            hid_t oid = HDva_arg(arguments, hid_t);

            /* Flush the object's metadata */
            if (H5O_flush(loc.oloc, oid) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTFLUSH, FAIL, "unable to flush object")

            break;
        }

        case H5VL_OBJECT_REFRESH: {
            hid_t      oid  = HDva_arg(arguments, hid_t);
            H5O_loc_t *oloc = loc.oloc;

            /* Refresh the metadata */
            if (H5O_refresh_metadata(oid, *oloc) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, FAIL, "unable to refresh object")

            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't recognize this operation type")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_object_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_object_optional
 *
 * Purpose:     Handles the object optional callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_object_optional(void *obj, H5VL_object_optional_t optional_type, hid_t H5_ATTR_UNUSED dxpl_id,
                             void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5VL_loc_params_t *loc_params = HDva_arg(arguments, H5VL_loc_params_t *);
    H5G_loc_t          loc;                 /* Location of group */
    herr_t             ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    switch (optional_type) {
        /* H5Oget_comment / H5Oget_comment_by_name */
        case H5VL_NATIVE_OBJECT_GET_COMMENT: {
            char *   comment = HDva_arg(arguments, char *);
            size_t   bufsize = HDva_arg(arguments, size_t);
            ssize_t *ret     = HDva_arg(arguments, ssize_t *);

            /* Retrieve the object's comment */
            if (loc_params->type == H5VL_OBJECT_BY_SELF) { /* H5Oget_comment */
                if ((*ret = H5G_loc_get_comment(&loc, ".", comment /*out*/, bufsize)) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
            }                                                   /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Oget_comment_by_name */
                if ((*ret = H5G_loc_get_comment(&loc, loc_params->loc_data.loc_by_name.name, comment /*out*/,
                                                bufsize)) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown set_coment parameters")
            break;
        }

        /* H5Oset_comment */
        case H5VL_NATIVE_OBJECT_SET_COMMENT: {
            const char *comment = HDva_arg(arguments, char *);

            if (loc_params->type == H5VL_OBJECT_BY_SELF) { /* H5Oset_comment */
                /* (Re)set the object's comment */
                if (H5G_loc_set_comment(&loc, ".", comment) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
            }                                                   /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Oset_comment_by_name */
                /* (Re)set the object's comment */
                if (H5G_loc_set_comment(&loc, loc_params->loc_data.loc_by_name.name, comment) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown set_coment parameters")
            break;
        }

        /* H5Odisable_mdc_flushes */
        case H5VL_NATIVE_OBJECT_DISABLE_MDC_FLUSHES: {
            H5O_loc_t *oloc = loc.oloc;

            if (H5O_disable_mdc_flushes(oloc) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTCORK, FAIL, "unable to cork the metadata cache");

            break;
        }

        /* H5Oenable_mdc_flushes */
        case H5VL_NATIVE_OBJECT_ENABLE_MDC_FLUSHES: {
            H5O_loc_t *oloc = loc.oloc;

            if (H5O_enable_mdc_flushes(oloc) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTUNCORK, FAIL, "unable to uncork the metadata cache");

            break;
        }

        /* H5Oare_mdc_flushes_disabled */
        case H5VL_NATIVE_OBJECT_ARE_MDC_FLUSHES_DISABLED: {
            H5O_loc_t *oloc         = loc.oloc;
            hbool_t *  are_disabled = (hbool_t *)HDva_arg(arguments, hbool_t *);

            if (H5O_are_mdc_flushes_disabled(oloc, are_disabled) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to determine metadata cache cork status");

            break;
        }

        /* H5Oget_native_info(_name|_by_idx) */
        case H5VL_NATIVE_OBJECT_GET_NATIVE_INFO: {
            H5O_native_info_t *native_info = HDva_arg(arguments, H5O_native_info_t *);
            unsigned           fields      = HDva_arg(arguments, unsigned);

            /* Use the original H5Oget_info code to get the data */

            if (loc_params->type == H5VL_OBJECT_BY_SELF) { /* H5Oget_info */
                /* Retrieve the object's information */
                if (H5G_loc_native_info(&loc, ".", native_info, fields) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
            }                                                   /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Oget_info_by_name */
                /* Retrieve the object's information */
                if (H5G_loc_native_info(&loc, loc_params->loc_data.loc_by_name.name, native_info, fields) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
            }                                                  /* end else-if */
            else if (loc_params->type == H5VL_OBJECT_BY_IDX) { /* H5Oget_info_by_idx */
                H5G_loc_t  obj_loc;                            /* Location used to open group */
                H5G_name_t obj_path;                           /* Opened object group hier. path */
                H5O_loc_t  obj_oloc;                           /* Opened object object location */

                /* Set up opened group location to fill in */
                obj_loc.oloc = &obj_oloc;
                obj_loc.path = &obj_path;
                H5G_loc_reset(&obj_loc);

                /* Find the object's location, according to the order in the index */
                if (H5G_loc_find_by_idx(&loc, loc_params->loc_data.loc_by_idx.name,
                                        loc_params->loc_data.loc_by_idx.idx_type,
                                        loc_params->loc_data.loc_by_idx.order,
                                        loc_params->loc_data.loc_by_idx.n, &obj_loc /*out*/) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "group not found")

                /* Retrieve the object's information */
                if (H5O_get_native_info(obj_loc.oloc, native_info, fields) < 0) {
                    H5G_loc_free(&obj_loc);
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't retrieve object info")
                } /* end if */

                /* Release the object location */
                if (H5G_loc_free(&obj_loc) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "can't free location")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_OHDR, H5E_UNSUPPORTED, FAIL, "unknown get info parameters")

            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't perform this operation on object");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_object_optional() */
