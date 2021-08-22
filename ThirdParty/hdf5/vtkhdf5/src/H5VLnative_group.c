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
 * Purpose:     Group callbacks for the native VOL connector
 *
 */

#define H5G_FRIEND /* Suppress error about including H5Gpkg    */

#include "H5private.h"   /* Generic Functions                        */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Gpkg.h"      /* Groups                                   */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5Oprivate.h"  /* Object headers                           */
#include "H5Pprivate.h"  /* Property lists                           */
#include "H5VLprivate.h" /* Virtual Object Layer                     */

#include "H5VLnative_private.h" /* Native VOL connector                     */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_group_create
 *
 * Purpose:     Handles the group create callback
 *
 * Return:      Success:    group pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_group_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id,
                          hid_t gcpl_id, hid_t H5_ATTR_UNUSED gapl_id, hid_t H5_ATTR_UNUSED dxpl_id,
                          void H5_ATTR_UNUSED **req)
{
    H5G_loc_t loc;        /* Location to create group     */
    H5G_t *   grp = NULL; /* New group created            */
    void *    ret_value;

    FUNC_ENTER_PACKAGE

    /* Set up the location */
    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file or file object")

    /* if name is NULL then this is from H5Gcreate_anon */
    if (name == NULL) {
        H5G_obj_create_t gcrt_info; /* Information for group creation */

        /* Set up group creation info */
        gcrt_info.gcpl_id    = gcpl_id;
        gcrt_info.cache_type = H5G_NOTHING_CACHED;
        HDmemset(&gcrt_info.cache, 0, sizeof(gcrt_info.cache));

        /* Create the new group & get its ID */
        if (NULL == (grp = H5G__create(loc.oloc->file, &gcrt_info)))
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, NULL, "unable to create group")
    } /* end if */
    /* otherwise it's from H5Gcreate */
    else {
        /* Create the new group & get its ID */
        if (NULL == (grp = H5G__create_named(&loc, name, lcpl_id, gcpl_id)))
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, NULL, "unable to create group")
    } /* end else */

    ret_value = (void *)grp;

done:
    if (name == NULL) {
        /* Release the group's object header, if it was created */
        if (grp) {
            H5O_loc_t *oloc; /* Object location for group */

            /* Get the new group's object location */
            if (NULL == (oloc = H5G_oloc(grp)))
                HDONE_ERROR(H5E_SYM, H5E_CANTGET, NULL, "unable to get object location of group")

            /* Decrement refcount on group's object header in memory */
            if (H5O_dec_rc_by_loc(oloc) < 0)
                HDONE_ERROR(H5E_SYM, H5E_CANTDEC, NULL,
                            "unable to decrement refcount on newly created object")
        } /* end if */
    }     /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_group_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_group_open
 *
 * Purpose:     Handles the group open callback
 *
 * Return:      Success:    group pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_group_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name,
                        hid_t H5_ATTR_UNUSED gapl_id, hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5G_loc_t loc;        /* Location to open group   */
    H5G_t *   grp = NULL; /* New group opend          */
    void *    ret_value;

    FUNC_ENTER_PACKAGE

    /* Set up the location */
    if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file or file object")

    /* Open the group */
    if ((grp = H5G__open_name(&loc, name)) == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, NULL, "unable to open group")

    ret_value = (void *)grp;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_group_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_group_get
 *
 * Purpose:     Handles the group get callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_group_get(void *obj, H5VL_group_get_t get_type, hid_t H5_ATTR_UNUSED dxpl_id,
                       void H5_ATTR_UNUSED **req, va_list arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (get_type) {
        /* H5Gget_create_plist */
        case H5VL_GROUP_GET_GCPL: {
            hid_t *new_gcpl_id = HDva_arg(arguments, hid_t *);
            H5G_t *grp         = (H5G_t *)obj;

            if ((*new_gcpl_id = H5G_get_create_plist(grp)) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get creation property list for group")
            break;
        }

        /* H5Gget_info */
        case H5VL_GROUP_GET_INFO: {
            const H5VL_loc_params_t *loc_params = HDva_arg(arguments, const H5VL_loc_params_t *);
            H5G_info_t *             group_info = HDva_arg(arguments, H5G_info_t *);
            H5G_loc_t                loc;

            if (H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            if (loc_params->type == H5VL_OBJECT_BY_SELF) {
                /* H5Gget_info */

                /* Retrieve the group's information */
                if (H5G__obj_info(loc.oloc, group_info) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve group info")
            } /* end if */
            else if (loc_params->type == H5VL_OBJECT_BY_NAME) {
                /* H5Gget_info_by_name */

                /* Retrieve the group's information */
                if (H5G__get_info_by_name(&loc, loc_params->loc_data.loc_by_name.name, group_info) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve group info")
            } /* end else-if */
            else if (loc_params->type == H5VL_OBJECT_BY_IDX) {
                /* H5Gget_info_by_idx */

                /* Retrieve the group's information */
                if (H5G__get_info_by_idx(&loc, loc_params->loc_data.loc_by_idx.name,
                                         loc_params->loc_data.loc_by_idx.idx_type,
                                         loc_params->loc_data.loc_by_idx.order,
                                         loc_params->loc_data.loc_by_idx.n, group_info) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve group info")
            } /* end else-if */
            else
                HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown get info parameters")
            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get this type of information from group")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_group_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_group_specific
 *
 * Purpose:     Handles the group specific callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_group_specific(void *obj, H5VL_group_specific_t specific_type, hid_t H5_ATTR_UNUSED dxpl_id,
                            void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5G_t *grp       = (H5G_t *)obj;
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (specific_type) {
        case H5VL_GROUP_FLUSH: {
            hid_t group_id = HDva_arg(arguments, hid_t);

            /* Flush object's metadata to file */
            if (H5O_flush_common(&grp->oloc, group_id) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTFLUSH, FAIL, "unable to flush group")

            break;
        }

        case H5VL_GROUP_REFRESH: {
            hid_t group_id = HDva_arg(arguments, hid_t);

            /* Call private function to refresh group object */
            if ((H5O_refresh_metadata(group_id, grp->oloc)) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTLOAD, FAIL, "unable to refresh group")

            break;
        }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "invalid specific operation")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_group_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_group_optional
 *
 * Purpose:     Handles the group optional callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_group_optional(void H5_ATTR_UNUSED *obj, H5VL_group_optional_t optional_type,
                            hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req,
                            va_list H5_ATTR_DEPRECATED_USED arguments)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    switch (optional_type) {
#ifndef H5_NO_DEPRECATED_SYMBOLS
        /* H5Giterate (deprecated) */
        case H5VL_NATIVE_GROUP_ITERATE_OLD: {
            const H5VL_loc_params_t * loc_params = HDva_arg(arguments, const H5VL_loc_params_t *);
            hsize_t                   idx        = HDva_arg(arguments, hsize_t);
            hsize_t *                 last_obj   = HDva_arg(arguments, hsize_t *);
            const H5G_link_iterate_t *lnk_op     = HDva_arg(arguments, const H5G_link_iterate_t *);
            void *                    op_data    = HDva_arg(arguments, void *);
            H5G_loc_t                 grp_loc;

            /* Get the location struct for the object */
            if (H5G_loc_real(obj, loc_params->obj_type, &grp_loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            /* Call the actual iteration routine */
            if ((ret_value = H5G_iterate(&grp_loc, loc_params->loc_data.loc_by_name.name, H5_INDEX_NAME,
                                         H5_ITER_INC, idx, last_obj, lnk_op, op_data)) < 0)
                HERROR(H5E_VOL, H5E_BADITER, "error iterating over group's links");

            break;
        }

        /* H5Gget_objinfo (deprecated) */
        case H5VL_NATIVE_GROUP_GET_OBJINFO: {
            const H5VL_loc_params_t *loc_params  = HDva_arg(arguments, const H5VL_loc_params_t *);
            hbool_t                  follow_link = (hbool_t)HDva_arg(arguments, unsigned);
            H5G_stat_t *             statbuf     = HDva_arg(arguments, H5G_stat_t *);
            H5G_loc_t                grp_loc;

            /* Get the location struct for the object */
            if (H5G_loc_real(obj, loc_params->obj_type, &grp_loc) < 0)
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

            /* Call the actual group objinfo routine */
            if (H5G__get_objinfo(&grp_loc, loc_params->loc_data.loc_by_name.name, follow_link, statbuf) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "cannot stat object")

            break;
        }
#endif /* H5_NO_DEPRECATED_SYMBOLS */

        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "invalid optional operation")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_group_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__native_group_close
 *
 * Purpose:     Handles the group close callback
 *
 * Return:      Success:    SUCCEED
 *              Failure:    FAIL (group will not be closed)
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_group_close(void *grp, hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    if (H5G_close((H5G_t *)grp) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CLOSEERROR, FAIL, "can't close group")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_group_close() */
