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
 * Purpose:     Attribute callbacks for the native VOL connector
 *
 */

#define H5A_FRIEND /* Suppress error about including H5Apkg    */

#include "H5private.h"   /* Generic Functions                        */
#include "H5Apkg.h"      /* Attributes                               */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Fprivate.h"  /* Files                                    */
#include "H5Gprivate.h"  /* Groups                                   */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5Pprivate.h"  /* Property lists                           */
#include "H5Sprivate.h"  /* Dataspaces                               */
#include "H5Tprivate.h"  /* Datatypes                                */
#include "H5VLprivate.h" /* Virtual Object Layer                     */

#include "H5VLnative_private.h" /* Native VOL connector                     */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_attr_create
 *
 * Purpose:     Handles the attribute create callback
 *
 * Return:      Success:    attribute pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_attr_create(void *obj, const H5VL_loc_params_t *loc_params, const char *attr_name, hid_t type_id,
                         hid_t space_id, hid_t acpl_id, hid_t H5_ATTR_UNUSED aapl_id,
                         hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5G_loc_t loc;     /* Object location */
    H5G_loc_t obj_loc; /* Location used to open group */
    hbool_t   loc_found = FALSE;
    H5T_t *   type, *dt; /* Datatype to use for attribute */
    H5S_t *   space;     /* Dataspace to use for attribute */
    H5A_t *   attr      = NULL;
    void *    ret_value = NULL;

    FUNC_ENTER_PACKAGE

    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file or file object")
    if (0 == (H5F_INTENT(loc.oloc->file) & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_ARGS, H5E_WRITEERROR, NULL, "no write intent on file")

    if (NULL == (dt = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a datatype")
    /* If this is a named datatype, get the connector's pointer to the datatype */
    type = H5T_get_actual_type(dt);

    if (NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a data space")

    if (loc_params->type == H5VL_OBJECT_BY_SELF) {
        /* H5Acreate */
        /* Go do the real work for attaching the attribute to the dataset */
        if (NULL == (attr = H5A__create(&loc, attr_name, type, space, acpl_id)))
            HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, NULL, "unable to create attribute")
    } /* end if */
    else if (loc_params->type == H5VL_OBJECT_BY_NAME) {
        /* H5Acreate_by_name */
        if (NULL == (attr = H5A__create_by_name(&loc, loc_params->loc_data.loc_by_name.name, attr_name, type,
                                                space, acpl_id)))
            HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, NULL, "unable to create attribute")
    } /* end else-if */
    else
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "unknown attribute create parameters")

    ret_value = (void *)attr;

done:
    /* Release resources */
    if (loc_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTRELEASE, NULL, "can't free location")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_attr_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_attr_open
 *
 * Purpose:     Handles the attribute open callback
 *
 * Return:      Success:    attribute pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_attr_open(void *obj, const H5VL_loc_params_t *loc_params, const char *attr_name,
                       hid_t H5_ATTR_UNUSED aapl_id, hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5G_loc_t loc;         /* Object location */
    H5A_t *   attr = NULL; /* Attribute opened */
    void *    ret_value;

    FUNC_ENTER_PACKAGE

    /* check arguments */
    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file or file object")

    if (loc_params->type == H5VL_OBJECT_BY_SELF) {
        /* H5Aopen */
        /* Open the attribute */
        if (NULL == (attr = H5A__open(&loc, attr_name)))
            HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, NULL, "unable to open attribute: '%s'", attr_name)
    } /* end if */
    else if (loc_params->type == H5VL_OBJECT_BY_NAME) {
        /* H5Aopen_by_name */
        /* Open the attribute on the object header */
        if (NULL == (attr = H5A__open_by_name(&loc, loc_params->loc_data.loc_by_name.name, attr_name)))
            HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, NULL, "can't open attribute")
    } /* end else-if */
    else if (loc_params->type == H5VL_OBJECT_BY_IDX) {
        /* H5Aopen_by_idx */
        /* Open the attribute in the object header */
        if (NULL == (attr = H5A__open_by_idx(
                         &loc, loc_params->loc_data.loc_by_idx.name, loc_params->loc_data.loc_by_idx.idx_type,
                         loc_params->loc_data.loc_by_idx.order, loc_params->loc_data.loc_by_idx.n)))
            HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, NULL, "unable to open attribute")
    } /* end else-if */
    else
        HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "unknown attribute open parameters")

    ret_value = (void *)attr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_attr_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_attr_read
 *
 * Purpose:     Handles the attribute read callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_attr_read(void *attr, hid_t dtype_id, void *buf, hid_t H5_ATTR_UNUSED dxpl_id,
                       void H5_ATTR_UNUSED **req)
{
    H5T_t *mem_type;  /* Memory datatype */
    herr_t ret_value; /* Return value */

    FUNC_ENTER_PACKAGE

    if (NULL == (mem_type = (H5T_t *)H5I_object_verify(dtype_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Go write the actual data to the attribute */
    if ((ret_value = H5A__read((H5A_t *)attr, mem_type, buf)) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_READERROR, FAIL, "unable to read attribute")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_attr_read() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_attr_write
 *
 * Purpose:     Handles the attribute write callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_attr_write(void *attr, hid_t dtype_id, const void *buf, hid_t H5_ATTR_UNUSED dxpl_id,
                        void H5_ATTR_UNUSED **req)
{
    H5T_t *mem_type;  /* Memory datatype */
    herr_t ret_value; /* Return value */

    FUNC_ENTER_PACKAGE

    if (NULL == (mem_type = (H5T_t *)H5I_object_verify(dtype_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Go write the actual data to the attribute */
    if ((ret_value = H5A__write((H5A_t *)attr, mem_type, buf)) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_WRITEERROR, FAIL, "unable to write attribute")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_attr_write() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_attr_get
 *
 * Purpose:     Handles the attribute get callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_attr_get(void *obj, H5VL_attr_get_t get_type, hid_t H5_ATTR_UNUSED dxpl_id,
                      void H5_ATTR_UNUSED **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (get_type) {
        /* H5Aget_space */
        case H5VL_ATTR_GET_SPACE: {
            hid_t *ret_id = HDva_arg(arguments, hid_t *);
            H5A_t *attr   = (H5A_t *)obj;

            if ((*ret_id = H5A_get_space(attr)) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get space ID of attribute")
            break;
        }

        /* H5Aget_type */
        case H5VL_ATTR_GET_TYPE: {
            hid_t *ret_id = HDva_arg(arguments, hid_t *);
            H5A_t *attr   = (H5A_t *)obj;

            if ((*ret_id = H5A__get_type(attr)) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get datatype ID of attribute")
            break;
        }

        /* H5Aget_create_plist */
        case H5VL_ATTR_GET_ACPL: {
            hid_t *ret_id = HDva_arg(arguments, hid_t *);
            H5A_t *attr   = (H5A_t *)obj;

            if ((*ret_id = H5A__get_create_plist(attr)) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get creation property list for attr")

            break;
        }

        /* H5Aget_name */
        case H5VL_ATTR_GET_NAME: {
            const H5VL_loc_params_t *loc_params = HDva_arg(arguments, const H5VL_loc_params_t *);
            size_t                   buf_size   = HDva_arg(arguments, size_t);
            char *                   buf        = HDva_arg(arguments, char *);
            ssize_t *                ret_val    = HDva_arg(arguments, ssize_t *);
            H5A_t *                  attr       = NULL;

            if (H5VL_OBJECT_BY_SELF == loc_params->type) {
                attr = (H5A_t *)obj;
                /* Call private function in turn */
                if (0 > (*ret_val = H5A__get_name(attr, buf_size, buf)))
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "can't get attribute name")
            }
            else if (H5VL_OBJECT_BY_IDX == loc_params->type) {
                H5G_loc_t loc;

                /* check arguments */
                if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

                /* Open the attribute on the object header */
                if (NULL == (attr = H5A__open_by_idx(&loc, loc_params->loc_data.loc_by_idx.name,
                                                     loc_params->loc_data.loc_by_idx.idx_type,
                                                     loc_params->loc_data.loc_by_idx.order,
                                                     loc_params->loc_data.loc_by_idx.n)))
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "can't open attribute")

                /* Get the length of the name */
                *ret_val = (ssize_t)HDstrlen(attr->shared->name);

                /* Copy the name into the user's buffer, if given */
                if (buf) {
                    HDstrncpy(buf, attr->shared->name, MIN((size_t)(*ret_val + 1), buf_size));
                    if ((size_t)(*ret_val) >= buf_size)
                        buf[buf_size - 1] = '\0';
                } /* end if */

                /* Release resources */
                if (attr && H5A__close(attr) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")
            }
            else
                HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't get name of attr")

            break;
        }

        /* H5Aget_info */
        case H5VL_ATTR_GET_INFO: {
            const H5VL_loc_params_t *loc_params = HDva_arg(arguments, const H5VL_loc_params_t *);
            H5A_info_t *             ainfo      = HDva_arg(arguments, H5A_info_t *);
            H5A_t *                  attr       = NULL;

            if (H5VL_OBJECT_BY_SELF == loc_params->type) {
                attr = (H5A_t *)obj;
                if (H5A__get_info(attr, ainfo) < 0)
                    HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get attribute info")
            }
            else if (H5VL_OBJECT_BY_NAME == loc_params->type) {
                char *    attr_name = HDva_arg(arguments, char *);
                H5G_loc_t loc;

                /* check arguments */
                if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

                /* Open the attribute on the object header */
                if (NULL ==
                    (attr = H5A__open_by_name(&loc, loc_params->loc_data.loc_by_name.name, attr_name)))
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "can't open attribute")

                /* Get the attribute information */
                if (H5A__get_info(attr, ainfo) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "unable to get attribute info")

                /* Release resources */
                if (attr && H5A__close(attr) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")
            }
            else if (H5VL_OBJECT_BY_IDX == loc_params->type) {
                H5G_loc_t loc;

                /* check arguments */
                if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

                /* Open the attribute on the object header */
                if (NULL == (attr = H5A__open_by_idx(&loc, loc_params->loc_data.loc_by_idx.name,
                                                     loc_params->loc_data.loc_by_idx.idx_type,
                                                     loc_params->loc_data.loc_by_idx.order,
                                                     loc_params->loc_data.loc_by_idx.n)))
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "can't open attribute")

                /* Get the attribute information */
                if (H5A__get_info(attr, ainfo) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "unable to get attribute info")

                /* Release resources */
                if (attr && H5A__close(attr) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")
            }
            else
                HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't get name of attr")

            break;
        }

        case H5VL_ATTR_GET_STORAGE_SIZE: {
            hsize_t *ret  = HDva_arg(arguments, hsize_t *);
            H5A_t *  attr = (H5A_t *)obj;

            /* Set return value */
            *ret = attr->shared->data_size;
            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get this type of information from attr")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_attr_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_attr_specific
 *
 * Purpose:     Handles the attribute specific callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_attr_specific(void *obj, const H5VL_loc_params_t *loc_params, H5VL_attr_specific_t specific_type,
                           hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5G_loc_t loc;
    herr_t    ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Get location for passed-in object */
    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    switch (specific_type) {
        case H5VL_ATTR_DELETE: {
            char *attr_name = HDva_arg(arguments, char *);

            if (H5VL_OBJECT_BY_SELF == loc_params->type) {
                /* H5Adelete */
                /* Delete the attribute from the location */
                if (H5O__attr_remove(loc.oloc, attr_name) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTDELETE, FAIL, "unable to delete attribute")
            } /* end if */
            else if (H5VL_OBJECT_BY_NAME == loc_params->type) {
                /* H5Adelete_by_name */
                /* Delete the attribute */
                if (H5A__delete_by_name(&loc, loc_params->loc_data.loc_by_name.name, attr_name) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTDELETE, FAIL, "unable to delete attribute")
            } /* end else-if */
            else if (H5VL_OBJECT_BY_IDX == loc_params->type) {
                /* H5Adelete_by_idx */
                /* Delete the attribute from the location */
                if (H5A__delete_by_idx(
                        &loc, loc_params->loc_data.loc_by_idx.name, loc_params->loc_data.loc_by_idx.idx_type,
                        loc_params->loc_data.loc_by_idx.order, loc_params->loc_data.loc_by_idx.n) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTDELETE, FAIL, "unable to delete attribute")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown attribute remove parameters")
            break;
        }

        case H5VL_ATTR_EXISTS: {
            const char *attr_name = HDva_arg(arguments, const char *);
            htri_t *    ret       = HDva_arg(arguments, htri_t *);

            if (loc_params->type == H5VL_OBJECT_BY_SELF) { /* H5Aexists */
                /* Check if the attribute exists */
                if ((*ret = H5O__attr_exists(loc.oloc, attr_name)) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "unable to determine if attribute exists")
            }                                                   /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Aexists_by_name */
                /* Check if the attribute exists */
                if ((*ret = H5A__exists_by_name(loc, loc_params->loc_data.loc_by_name.name, attr_name)) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "unable to determine if attribute exists")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown parameters")
            break;
        }

        case H5VL_ATTR_ITER: {
            H5_index_t      idx_type = (H5_index_t)HDva_arg(arguments, int);      /* enum work-around */
            H5_iter_order_t order    = (H5_iter_order_t)HDva_arg(arguments, int); /* enum work-around */
            hsize_t *       idx      = HDva_arg(arguments, hsize_t *);
            H5A_operator2_t op       = HDva_arg(arguments, H5A_operator2_t);
            void *          op_data  = HDva_arg(arguments, void *);

            if (loc_params->type == H5VL_OBJECT_BY_SELF) { /* H5Aiterate2 */
                /* Iterate over attributes */
                if ((ret_value = H5A__iterate(&loc, ".", idx_type, order, idx, op, op_data)) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_BADITER, FAIL, "error iterating over attributes")
            }                                                   /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Aiterate_by_name */
                /* Iterate over attributes by name */
                if ((ret_value = H5A__iterate(&loc, loc_params->loc_data.loc_by_name.name, idx_type, order,
                                              idx, op, op_data)) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_BADITER, FAIL, "attribute iteration failed");
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown parameters")
            break;
        }

        /* H5Arename/rename_by_name */
        case H5VL_ATTR_RENAME: {
            const char *old_name = HDva_arg(arguments, const char *);
            const char *new_name = HDva_arg(arguments, const char *);

            if (loc_params->type == H5VL_OBJECT_BY_SELF) { /* H5Arename */
                /* Call attribute rename routine */
                if (H5O__attr_rename(loc.oloc, old_name, new_name) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTRENAME, FAIL, "can't rename attribute")
            }                                                   /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Arename_by_name */
                /* Call attribute rename routine */
                if (H5A__rename_by_name(loc, loc_params->loc_data.loc_by_name.name, old_name, new_name) < 0)
                    HGOTO_ERROR(H5E_ATTR, H5E_CANTRENAME, FAIL, "can't rename attribute")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown attribute rename parameters")
            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "invalid specific operation")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_attr_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_attr_optional
 *
 * Purpose:     Handles the attribute optional callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_attr_optional(void H5_ATTR_UNUSED *obj, H5VL_attr_optional_t opt_type,
                           hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req,
                           va_list H5_ATTR_DEPRECATED_USED arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (opt_type) {
#ifndef H5_NO_DEPRECATED_SYMBOLS
        case H5VL_NATIVE_ATTR_ITERATE_OLD: {
            hid_t           loc_id   = HDva_arg(arguments, hid_t);
            unsigned *      attr_num = HDva_arg(arguments, unsigned *);
            H5A_operator1_t op       = HDva_arg(arguments, H5A_operator1_t);
            void *          op_data  = HDva_arg(arguments, void *);

            /* Call the actual iteration routine */
            if ((ret_value = H5A__iterate_old(loc_id, attr_num, op, op_data)) < 0)
                HERROR(H5E_VOL, H5E_BADITER, "error iterating over attributes");

            break;
        }
#endif /* H5_NO_DEPRECATED_SYMBOLS */

        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "invalid optional operation")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_attr_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_attr_close
 *
 * Purpose:     Handles the attribute close callback
 *
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL (attribute will not be closed)
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_attr_close(void *attr, hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    if (H5A__close((H5A_t *)attr) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTDEC, FAIL, "can't close attribute")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_attr_close() */
