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
 * Purpose:     Link callbacks for the native VOL connector
 *
 */

#define H5L_FRIEND /* Suppress error about including H5Lpkg    */

#include "H5private.h"   /* Generic Functions                        */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Gprivate.h"  /* Groups                                   */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5Lpkg.h"      /* Links                                    */
#include "H5Pprivate.h"  /* Property lists                           */
#include "H5VLprivate.h" /* Virtual Object Layer                     */

#include "H5VLnative_private.h" /* Native VOL connector                     */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_link_create
 *
 * Purpose:     Handles the link create callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_link_create(H5VL_link_create_type_t create_type, void *obj, const H5VL_loc_params_t *loc_params,
                         hid_t lcpl_id, hid_t H5_ATTR_UNUSED lapl_id, hid_t H5_ATTR_UNUSED dxpl_id,
                         void H5_ATTR_UNUSED **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (create_type) {
        case H5VL_LINK_CREATE_HARD: {
            H5G_loc_t          cur_loc;
            H5G_loc_t          link_loc;
            void *             cur_obj    = HDva_arg(arguments, void *);
            H5VL_loc_params_t *cur_params = HDva_arg(arguments, H5VL_loc_params_t *);

            if (NULL != cur_obj && H5G_loc_real(cur_obj, cur_params->obj_type, &cur_loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")
            if (NULL != obj && H5G_loc_real(obj, loc_params->obj_type, &link_loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            /* H5Lcreate_hard */
            if (H5VL_OBJECT_BY_NAME == cur_params->type) {
                H5G_loc_t *cur_loc_p, *link_loc_p;

                /* Set up current & new location pointers */
                cur_loc_p  = &cur_loc;
                link_loc_p = &link_loc;
                if (NULL == cur_obj)
                    cur_loc_p = link_loc_p;
                else if (NULL == obj)
                    link_loc_p = cur_loc_p;
                else if (cur_loc_p->oloc->file != link_loc_p->oloc->file)
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                                "source and destination should be in the same file.")

                /* Create the link */
                if ((ret_value =
                         H5L__create_hard(cur_loc_p, cur_params->loc_data.loc_by_name.name, link_loc_p,
                                          loc_params->loc_data.loc_by_name.name, lcpl_id)) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create link")
            }      /* end if */
            else { /* H5Olink */
                /* Link to the object */
                if (H5L_link(&link_loc, loc_params->loc_data.loc_by_name.name, &cur_loc, lcpl_id) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to create link")
            } /* end else */
            break;
        }

        case H5VL_LINK_CREATE_SOFT: {
            char *    target_name = HDva_arg(arguments, char *);
            H5G_loc_t link_loc; /* Group location for new link */

            if (H5G_loc_real(obj, loc_params->obj_type, &link_loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            /* Create the link */
            if ((ret_value = H5L__create_soft(target_name, &link_loc, loc_params->loc_data.loc_by_name.name,
                                              lcpl_id)) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create link")
            break;
        }

        case H5VL_LINK_CREATE_UD: {
            H5G_loc_t  link_loc; /* Group location for new link */
            H5L_type_t link_type  = (H5L_type_t)HDva_arg(arguments, int);
            void *     udata      = HDva_arg(arguments, void *);
            size_t     udata_size = HDva_arg(arguments, size_t);

            if (H5G_loc_real(obj, loc_params->obj_type, &link_loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            /* Create link */
            if (H5L__create_ud(&link_loc, loc_params->loc_data.loc_by_name.name, udata, udata_size, link_type,
                               lcpl_id) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create link")
            break;
        }

        default:
            HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "invalid link creation call")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_link_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_link_copy
 *
 * Purpose:     Handles the link copy callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_link_copy(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
                       const H5VL_loc_params_t *loc_params2, hid_t lcpl_id, hid_t H5_ATTR_UNUSED lapl_id,
                       hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5G_loc_t src_loc, *src_loc_p;
    H5G_loc_t dst_loc, *dst_loc_p;
    herr_t    ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    if (NULL != src_obj && H5G_loc_real(src_obj, loc_params1->obj_type, &src_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")
    if (NULL != dst_obj && H5G_loc_real(dst_obj, loc_params2->obj_type, &dst_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    /* Set up src & dst location pointers */
    src_loc_p = &src_loc;
    dst_loc_p = &dst_loc;
    if (NULL == src_obj)
        src_loc_p = dst_loc_p;
    else if (NULL == dst_obj)
        dst_loc_p = src_loc_p;

    /* Copy the link */
    if (H5L__move(src_loc_p, loc_params1->loc_data.loc_by_name.name, dst_loc_p,
                  loc_params2->loc_data.loc_by_name.name, TRUE, lcpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTCOPY, FAIL, "unable to copy link")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_link_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_link_move
 *
 * Purpose:     Handles the link move callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_link_move(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj,
                       const H5VL_loc_params_t *loc_params2, hid_t lcpl_id, hid_t H5_ATTR_UNUSED lapl_id,
                       hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5G_loc_t src_loc, *src_loc_p;
    H5G_loc_t dst_loc, *dst_loc_p;
    herr_t    ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    if (NULL != src_obj && H5G_loc_real(src_obj, loc_params1->obj_type, &src_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")
    if (NULL != dst_obj && H5G_loc_real(dst_obj, loc_params2->obj_type, &dst_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    /* Set up src & dst location pointers */
    src_loc_p = &src_loc;
    dst_loc_p = &dst_loc;
    if (NULL == src_obj)
        src_loc_p = dst_loc_p;
    else if (NULL == dst_obj)
        dst_loc_p = src_loc_p;

    /* Move the link */
    if (H5L__move(src_loc_p, loc_params1->loc_data.loc_by_name.name, dst_loc_p,
                  loc_params2->loc_data.loc_by_name.name, FALSE, lcpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTMOVE, FAIL, "unable to move link")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_link_move() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_link_get
 *
 * Purpose:     Handles the link get callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_link_get(void *obj, const H5VL_loc_params_t *loc_params, H5VL_link_get_t get_type,
                      hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5G_loc_t loc;
    herr_t    ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    switch (get_type) {
        /* H5Lget_info/H5Lget_info_by_idx */
        case H5VL_LINK_GET_INFO: {
            H5L_info2_t *linfo2 = HDva_arg(arguments, H5L_info2_t *);

            /* Get the link information */
            if (loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Lget_info */
                if (H5L_get_info(&loc, loc_params->loc_data.loc_by_name.name, linfo2) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "unable to get link info")
            }                                                  /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_IDX) { /* H5Lget_info_by_idx */
                if (H5L__get_info_by_idx(
                        &loc, loc_params->loc_data.loc_by_idx.name, loc_params->loc_data.loc_by_idx.idx_type,
                        loc_params->loc_data.loc_by_idx.order, loc_params->loc_data.loc_by_idx.n, linfo2) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "unable to get link info")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "unable to get link info")

            break;
        }

        /* H5Lget_name_by_idx */
        case H5VL_LINK_GET_NAME: {
            char *   name = HDva_arg(arguments, char *);
            size_t   size = HDva_arg(arguments, size_t);
            ssize_t *ret  = HDva_arg(arguments, ssize_t *);

            /* Get the link name */
            if ((*ret = H5L__get_name_by_idx(&loc, loc_params->loc_data.loc_by_idx.name,
                                             loc_params->loc_data.loc_by_idx.idx_type,
                                             loc_params->loc_data.loc_by_idx.order,
                                             loc_params->loc_data.loc_by_idx.n, name, size)) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "unable to get link info")

            break;
        }

        /* H5Lget_val/H5Lget_val_by_idx */
        case H5VL_LINK_GET_VAL: {
            void * buf  = HDva_arg(arguments, void *);
            size_t size = HDva_arg(arguments, size_t);

            /* Get the link information */
            if (loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Lget_val */
                if (H5L__get_val(&loc, loc_params->loc_data.loc_by_name.name, buf, size) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "unable to get link value")
            }
            else if (loc_params->type == H5VL_OBJECT_BY_IDX) { /* H5Lget_val_by_idx */

                if (H5L__get_val_by_idx(&loc, loc_params->loc_data.loc_by_idx.name,
                                        loc_params->loc_data.loc_by_idx.idx_type,
                                        loc_params->loc_data.loc_by_idx.order,
                                        loc_params->loc_data.loc_by_idx.n, buf, size) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "unable to get link val")
            }
            else
                HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "unable to get link val")

            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get this type of information from link")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_link_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_link_specific
 *
 * Purpose:     Handles the link specific callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_link_specific(void *obj, const H5VL_loc_params_t *loc_params, H5VL_link_specific_t specific_type,
                           hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (specific_type) {
        case H5VL_LINK_EXISTS: {
            htri_t *  ret = HDva_arg(arguments, htri_t *);
            H5G_loc_t loc;

            if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            /* Check for the existence of the link */
            if ((*ret = H5L__exists(&loc, loc_params->loc_data.loc_by_name.name)) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "unable to specific link info")
            break;
        }

        case H5VL_LINK_ITER: {
            H5G_loc_t       loc;
            hbool_t         recursive = (hbool_t)HDva_arg(arguments, unsigned);
            H5_index_t      idx_type  = (H5_index_t)HDva_arg(arguments, int);      /* enum work-around */
            H5_iter_order_t order     = (H5_iter_order_t)HDva_arg(arguments, int); /* enum work-around */
            hsize_t *       idx_p     = HDva_arg(arguments, hsize_t *);
            H5L_iterate2_t  op        = HDva_arg(arguments, H5L_iterate2_t);
            void *          op_data   = HDva_arg(arguments, void *);

            /* Get the location */
            if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

            /* Visit or iterate over the links */
            if (loc_params->type == H5VL_OBJECT_BY_SELF) {
                if (recursive) {
                    /* H5Lvisit */
                    if ((ret_value = H5G_visit(&loc, ".", idx_type, order, op, op_data)) < 0)
                        HGOTO_ERROR(H5E_LINK, H5E_BADITER, FAIL, "link visitation failed")
                } /* end if */
                else {
                    /* H5Literate */
                    if ((ret_value = H5L_iterate(&loc, ".", idx_type, order, idx_p, op, op_data)) < 0)
                        HGOTO_ERROR(H5E_LINK, H5E_BADITER, FAIL, "error iterating over links")
                } /* end else */
            }     /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_NAME) {
                if (recursive) {
                    /* H5Lvisit_by_name */
                    if ((ret_value = H5G_visit(&loc, loc_params->loc_data.loc_by_name.name, idx_type, order,
                                               op, op_data)) < 0)
                        HGOTO_ERROR(H5E_LINK, H5E_BADITER, FAIL, "link visitation failed")
                } /* end if */
                else {
                    /* H5Literate_by_name */
                    if ((ret_value = H5L_iterate(&loc, loc_params->loc_data.loc_by_name.name, idx_type, order,
                                                 idx_p, op, op_data)) < 0)
                        HGOTO_ERROR(H5E_LINK, H5E_BADITER, FAIL, "error iterating over links")
                } /* end else */
            }     /* end else-if */
            else
                HGOTO_ERROR(H5E_LINK, H5E_UNSUPPORTED, FAIL, "unknown link iterate params")

            break;
        }

        case H5VL_LINK_DELETE: {
            H5G_loc_t loc;

            if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            /* Unlink */
            if (loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Ldelete */
                if (H5L__delete(&loc, loc_params->loc_data.loc_by_name.name) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_CANTDELETE, FAIL, "unable to delete link")
            }                                                  /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_IDX) { /* H5Ldelete_by_idx */
                if (H5L__delete_by_idx(
                        &loc, loc_params->loc_data.loc_by_idx.name, loc_params->loc_data.loc_by_idx.idx_type,
                        loc_params->loc_data.loc_by_idx.order, loc_params->loc_data.loc_by_idx.n) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_CANTDELETE, FAIL, "unable to delete link")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_LINK, H5E_CANTDELETE, FAIL, "unable to delete link")
            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "invalid specific operation")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_link_specific() */
