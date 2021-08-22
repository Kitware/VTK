/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/****************/
/* Module Setup */
/****************/

#include "H5Lmodule.h" /* This source code file is part of the H5L module */

/***********/
/* Headers */
/***********/
#include "H5private.h"          /* Generic Functions                        */
#include "H5ACprivate.h"        /* Metadata cache                           */
#include "H5CXprivate.h"        /* API Contexts                             */
#include "H5Dprivate.h"         /* Datasets                                 */
#include "H5Eprivate.h"         /* Error handling                           */
#include "H5Fprivate.h"         /* File access                              */
#include "H5Gprivate.h"         /* Groups                                   */
#include "H5Iprivate.h"         /* IDs                                      */
#include "H5Lpkg.h"             /* Links                                    */
#include "H5MMprivate.h"        /* Memory management                        */
#include "H5Oprivate.h"         /* File objects                             */
#include "H5Pprivate.h"         /* Property lists                           */
#include "H5VLprivate.h"        /* Virtual Object Layer                     */
#include "H5VLnative_private.h" /* Native VOL                               */

/****************/
/* Local Macros */
/****************/

#define H5L_MIN_TABLE_SIZE 32 /* Minimum size of the user-defined link type table if it is allocated */

/******************/
/* Local Typedefs */
/******************/

/* User data for path traversal routine for getting link info by name */
typedef struct {
    H5L_info2_t *linfo; /* Buffer to return to user */
} H5L_trav_gi_t;

/* User data for path traversal callback to creating a link */
typedef struct {
    H5F_t *           file;      /* Pointer to the file */
    H5P_genplist_t *  lc_plist;  /* Link creation property list */
    H5G_name_t *      path;      /* Path to object being linked */
    H5O_obj_create_t *ocrt_info; /* Pointer to object creation info */
    H5O_link_t *      lnk;       /* Pointer to link information to insert */
} H5L_trav_cr_t;

/* User data for path traversal routine for moving and renaming a link */
typedef struct {
    const char *     dst_name;         /* Destination name for moving object */
    H5T_cset_t       cset;             /* Char set for new name */
    const H5G_loc_t *dst_loc;          /* Destination location for moving object */
    unsigned         dst_target_flags; /* Target flags for destination object */
    hbool_t          copy;             /* TRUE if this is a copy operation */
    size_t           orig_nlinks; /* The original value for the # of soft / UD links that can be traversed */
} H5L_trav_mv_t;

/* User data for path traversal routine for moving and renaming an object */
typedef struct {
    H5F_t *     file; /* Pointer to the file */
    H5O_link_t *lnk;  /* Pointer to link information to insert */
    hbool_t     copy; /* TRUE if this is a copy operation */
} H5L_trav_mv2_t;

/* User data for path traversal routine for checking if a link exists */
typedef struct {
    /* Down */
    char *sep; /* Pointer to next separator in the string */

    /* Up */
    hbool_t exists; /* Whether the link exists or not */
} H5L_trav_le_t;

/* User data for path traversal routine for getting link value */
typedef struct {
    size_t size; /* Size of user buffer */
    void * buf;  /* User buffer */
} H5L_trav_gv_t;

/********************/
/* Local Prototypes */
/********************/

static int    H5L__find_class_idx(H5L_type_t id);
static herr_t H5L__link_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                           H5G_loc_t *obj_loc, void *_udata /*in,out*/, H5G_own_loc_t *own_loc /*out*/);
static herr_t H5L__create_real(const H5G_loc_t *link_loc, const char *link_name, H5G_name_t *obj_path,
                               H5F_t *obj_file, H5O_link_t *lnk, H5O_obj_create_t *ocrt_info, hid_t lcpl_id);
static herr_t H5L__get_val_real(const H5O_link_t *lnk, void *buf, size_t size);
static herr_t H5L__get_val_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                              H5G_loc_t *obj_loc, void *_udata /*in,out*/, H5G_own_loc_t *own_loc /*out*/);
static herr_t H5L__get_val_by_idx_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                                     H5G_loc_t *obj_loc, void *_udata /*in,out*/,
                                     H5G_own_loc_t *own_loc /*out*/);
static herr_t H5L__delete_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                             H5G_loc_t *obj_loc, void *_udata /*in,out*/, H5G_own_loc_t *own_loc /*out*/);
static herr_t H5L__delete_by_idx_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                                    H5G_loc_t *obj_loc, void *_udata /*in,out*/,
                                    H5G_own_loc_t *own_loc /*out*/);
static herr_t H5L__move_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                           H5G_loc_t *obj_loc, void *_udata /*in,out*/, H5G_own_loc_t *own_loc /*out*/);
static herr_t H5L__move_dest_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                                H5G_loc_t *obj_loc, void *_udata /*in,out*/, H5G_own_loc_t *own_loc /*out*/);
static herr_t H5L__exists_final_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                                   H5G_loc_t *obj_loc, void *_udata /*in,out*/,
                                   H5G_own_loc_t *own_loc /*out*/);
static herr_t H5L__exists_inter_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                                   H5G_loc_t *obj_loc, void *_udata /*in,out*/,
                                   H5G_own_loc_t *own_loc /*out*/);
static herr_t H5L__get_info_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                               H5G_loc_t *obj_loc, void *_udata /*in,out*/, H5G_own_loc_t *own_loc /*out*/);
static herr_t H5L__get_info_by_idx_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                                      H5G_loc_t *obj_loc, void *_udata /*in,out*/,
                                      H5G_own_loc_t *own_loc /*out*/);
static herr_t H5L__get_name_by_idx_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                                      H5G_loc_t *obj_loc, void *_udata /*in,out*/,
                                      H5G_own_loc_t *own_loc /*out*/);

/*********************/
/* Package Variables */
/*********************/

/* Package initialization variable */
hbool_t H5_PKG_INIT_VAR = FALSE;

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/* Information about user-defined links */
static size_t       H5L_table_alloc_g = 0;
static size_t       H5L_table_used_g  = 0;
static H5L_class_t *H5L_table_g       = NULL;

/*-------------------------------------------------------------------------
 * Function:    H5L_init
 *
 * Purpose:     Initialize the interface from some other package.
 *
 * Return:      Success:    non-negative
 *
 *              Failure:    negative
 *
 * Programmer:  James Laird
 *              Thursday, July 13, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_init(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_init() */

/*-------------------------------------------------------------------------
 * Function:    H5L__init_package
 *
 * Purpose:     Initialize information specific to H5L interface.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Tuesday, January 24, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L__init_package(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Initialize user-defined link classes */
    if (H5L_register_external() < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "unable to register external link class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_init_package() */

/*-------------------------------------------------------------------------
 * Function:    H5L_term_package
 *
 * Purpose:     Terminate any resources allocated in H5L__init_package.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Tuesday, January 24, 2006
 *
 *-------------------------------------------------------------------------
 */
int
H5L_term_package(void)
{
    int n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if (H5_PKG_INIT_VAR) {
        /* Free the table of link types */
        if (H5L_table_g) {
            H5L_table_g      = (H5L_class_t *)H5MM_xfree(H5L_table_g);
            H5L_table_used_g = H5L_table_alloc_g = 0;
            n++;
        } /* end if */

        /* Mark the interface as uninitialized */
        if (0 == n)
            H5_PKG_INIT_VAR = FALSE;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* H5L_term_package() */

/*-------------------------------------------------------------------------
 * Function:    H5Lmove
 *
 * Purpose:     Renames an object within an HDF5 file and moves it to a new
 *              group.  The original name SRC is unlinked from the group graph
 *              and then inserted with the new name DST (which can specify a
 *              new path for the object) as an atomic operation. The names
 *              are interpreted relative to SRC_LOC_ID and DST_LOC_ID,
 *              which are either file IDs or group ID.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Wednesday, March 29, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lmove(hid_t src_loc_id, const char *src_name, hid_t dst_loc_id, const char *dst_name, hid_t lcpl_id,
        hid_t lapl_id)
{
    H5VL_object_t *   vol_obj1 = NULL; /* Object of src_id */
    H5VL_loc_params_t loc_params1;
    H5VL_object_t *   vol_obj2 = NULL; /* Object of dst_id */
    H5VL_loc_params_t loc_params2;
    H5VL_object_t     tmp_vol_obj;         /* Temporary object */
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "i*si*sii", src_loc_id, src_name, dst_loc_id, dst_name, lcpl_id, lapl_id);

    /* Check arguments */
    if (src_loc_id == H5L_SAME_LOC && dst_loc_id == H5L_SAME_LOC)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "source and destination should not both be H5L_SAME_LOC")
    if (!src_name || !*src_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no current name specified")
    if (!dst_name || !*dst_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no destination name specified")
    if (lcpl_id != H5P_DEFAULT && (TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a link creation property list")

    /* Check the link create property list */
    if (H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;

    /* Set the LCPL for the API context */
    H5CX_set_lcpl(lcpl_id);

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, ((src_loc_id != H5L_SAME_LOC) ? src_loc_id : dst_loc_id), TRUE) <
        0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Set location parameter for source object */
    loc_params1.type                         = H5VL_OBJECT_BY_NAME;
    loc_params1.loc_data.loc_by_name.name    = src_name;
    loc_params1.loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params1.obj_type                     = H5I_get_type(src_loc_id);

    /* Set location parameter for destination object */
    loc_params2.type                         = H5VL_OBJECT_BY_NAME;
    loc_params2.loc_data.loc_by_name.name    = dst_name;
    loc_params2.loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params2.obj_type                     = H5I_get_type(dst_loc_id);

    if (H5L_SAME_LOC != src_loc_id)
        /* Get the location object */
        if (NULL == (vol_obj1 = (H5VL_object_t *)H5I_object(src_loc_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")
    if (H5L_SAME_LOC != dst_loc_id)
        /* Get the location object */
        if (NULL == (vol_obj2 = (H5VL_object_t *)H5I_object(dst_loc_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Make sure that the VOL connectors are the same */
    if (vol_obj1 && vol_obj2)
        if (vol_obj1->connector->cls->value != vol_obj2->connector->cls->value)
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                        "Objects are accessed through different VOL connectors and can't be linked")

    /* Construct a temporary source VOL object */
    tmp_vol_obj.data      = (vol_obj1 ? vol_obj1->data : NULL);
    tmp_vol_obj.connector = (vol_obj1 ? vol_obj1->connector : vol_obj2->connector);

    /* Move the link */
    if (H5VL_link_move(&tmp_vol_obj, &loc_params1, vol_obj2, &loc_params2, lcpl_id, lapl_id,
                       H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTMOVE, FAIL, "unable to move link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lmove() */

/*-------------------------------------------------------------------------
 * Function:    H5Lcopy
 *
 * Purpose:     Creates an identical copy of a link with the same creation
 *              time and target.  The new link can have a different name
 *              and be in a different location than the original.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Wednesday, March 29, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lcopy(hid_t src_loc_id, const char *src_name, hid_t dst_loc_id, const char *dst_name, hid_t lcpl_id,
        hid_t lapl_id)
{
    H5VL_object_t *   vol_obj1 = NULL; /* Object of src_id */
    H5VL_loc_params_t loc_params1;
    H5VL_object_t *   vol_obj2 = NULL; /* Object of dst_id */
    H5VL_loc_params_t loc_params2;
    H5VL_object_t     tmp_vol_obj;         /* Temporary object */
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "i*si*sii", src_loc_id, src_name, dst_loc_id, dst_name, lcpl_id, lapl_id);

    /* Check arguments */
    if (src_loc_id == H5L_SAME_LOC && dst_loc_id == H5L_SAME_LOC)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "source and destination should not both be H5L_SAME_LOC")
    if (!src_name || !*src_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no current name specified")
    if (!dst_name || !*dst_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no destination name specified")
    if (lcpl_id != H5P_DEFAULT && (TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a link creation property list")

    /* Check the link create property list */
    if (H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;

    /* Set the LCPL for the API context */
    H5CX_set_lcpl(lcpl_id);

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, ((src_loc_id != H5L_SAME_LOC) ? src_loc_id : dst_loc_id), TRUE) <
        0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Set location parameter for source object */
    loc_params1.type                         = H5VL_OBJECT_BY_NAME;
    loc_params1.loc_data.loc_by_name.name    = src_name;
    loc_params1.loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params1.obj_type                     = H5I_get_type(src_loc_id);

    /* Set location parameter for destination object */
    loc_params2.type                         = H5VL_OBJECT_BY_NAME;
    loc_params2.loc_data.loc_by_name.name    = dst_name;
    loc_params2.loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params2.obj_type                     = H5I_get_type(dst_loc_id);

    if (H5L_SAME_LOC != src_loc_id)
        /* Get the location object */
        if (NULL == (vol_obj1 = (H5VL_object_t *)H5I_object(src_loc_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")
    if (H5L_SAME_LOC != dst_loc_id)
        /* Get the location object */
        if (NULL == (vol_obj2 = (H5VL_object_t *)H5I_object(dst_loc_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Make sure that the VOL connectors are the same */
    if (vol_obj1 && vol_obj2)
        if (vol_obj1->connector->cls->value != vol_obj2->connector->cls->value)
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                        "Objects are accessed through different VOL connectors and can't be linked")

    /* Construct a temporary source VOL object */
    tmp_vol_obj.data      = (vol_obj1 ? vol_obj1->data : NULL);
    tmp_vol_obj.connector = (vol_obj1 ? vol_obj1->connector : vol_obj2->connector);

    /* Copy the link */
    if (H5VL_link_copy(&tmp_vol_obj, &loc_params1, vol_obj2, &loc_params2, lcpl_id, lapl_id,
                       H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTMOVE, FAIL, "unable to copy link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lcopy() */

/*-------------------------------------------------------------------------
 * Function:    H5Lcreate_soft
 *
 * Purpose:     Creates a soft link from LINK_NAME to LINK_TARGET.
 *
 *              LINK_TARGET can be anything and is interpreted at lookup
 *              time relative to the group which contains the final component
 *              of LINK_NAME.  For instance, if LINK_TARGET is `./foo' and
 *              LINK_NAME is `./x/y/bar' and a request is made for `./x/y/bar'
 *              then the actual object looked up is `./x/y/./foo'.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, April  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lcreate_soft(const char *link_target, hid_t link_loc_id, const char *link_name, hid_t lcpl_id,
               hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "*si*sii", link_target, link_loc_id, link_name, lcpl_id, lapl_id);

    /* Check arguments */
    if (link_loc_id == H5L_SAME_LOC)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "link location id should not be H5L_SAME_LOC")
    if (!link_target)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "link_target parameter cannot be NULL")
    if (!*link_target)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "link_target parameter cannot be an empty string")
    if (!link_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "link_name parameter cannot be NULL")
    if (!*link_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "link_name parameter cannot be an empty string")
    if (lcpl_id != H5P_DEFAULT && (TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a link creation property list")

    /* Get the link creation property list */
    if (H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;

    /* Set the LCPL for the API context */
    H5CX_set_lcpl(lcpl_id);

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, link_loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Set location fields */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.loc_data.loc_by_name.name    = link_name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params.obj_type                     = H5I_get_type(link_loc_id);

    /* get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5VL_vol_object(link_loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Create the link */
    if (H5VL_link_create(H5VL_LINK_CREATE_SOFT, vol_obj, &loc_params, lcpl_id, lapl_id,
                         H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, link_target) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTCREATE, FAIL, "unable to create soft link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lcreate_soft() */

/*-------------------------------------------------------------------------
 * Function:    H5Lcreate_hard
 *
 * Purpose:     Creates a hard link from NEW_NAME to CUR_NAME.
 *
 *              CUR_NAME must name an existing object.  CUR_NAME and
 *              NEW_NAME are interpreted relative to CUR_LOC_ID and
 *              NEW_LOC_ID, which are either file IDs or group IDs.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, April  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lcreate_hard(hid_t cur_loc_id, const char *cur_name, hid_t new_loc_id, const char *new_name, hid_t lcpl_id,
               hid_t lapl_id)
{
    H5VL_object_t *   vol_obj1 = NULL; /* Object of cur_loc_id */
    H5VL_object_t *   vol_obj2 = NULL; /* Object of new_loc_id */
    H5VL_object_t     tmp_vol_obj;     /* Temporary object */
    H5VL_loc_params_t loc_params1;
    H5VL_loc_params_t loc_params2;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "i*si*sii", cur_loc_id, cur_name, new_loc_id, new_name, lcpl_id, lapl_id);

    /* Check arguments */
    if (cur_loc_id == H5L_SAME_LOC && new_loc_id == H5L_SAME_LOC)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "source and destination should not be both H5L_SAME_LOC")
    if (!cur_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cur_name parameter cannot be NULL")
    if (!*cur_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cur_name parameter cannot be an empty string")
    if (!new_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "new_name parameter cannot be NULL")
    if (!*new_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "new_name parameter cannot be an empty string")
    if (lcpl_id != H5P_DEFAULT && (TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a link creation property list")

    /* Check the link create property list */
    if (H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;

    /* Set the LCPL for the API context */
    H5CX_set_lcpl(lcpl_id);

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, cur_loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Set up current & new location structs */
    loc_params1.type                         = H5VL_OBJECT_BY_NAME;
    loc_params1.obj_type                     = H5I_get_type(cur_loc_id);
    loc_params1.loc_data.loc_by_name.name    = cur_name;
    loc_params1.loc_data.loc_by_name.lapl_id = lapl_id;

    loc_params2.type                         = H5VL_OBJECT_BY_NAME;
    loc_params2.obj_type                     = H5I_get_type(new_loc_id);
    loc_params2.loc_data.loc_by_name.name    = new_name;
    loc_params2.loc_data.loc_by_name.lapl_id = lapl_id;

    if (H5L_SAME_LOC != cur_loc_id)
        /* Get the current location object */
        if (NULL == (vol_obj1 = (H5VL_object_t *)H5VL_vol_object(cur_loc_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")
    if (H5L_SAME_LOC != new_loc_id)
        /* Get the new location object */
        if (NULL == (vol_obj2 = (H5VL_object_t *)H5VL_vol_object(new_loc_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Make sure that the VOL connectors are the same */
    if (vol_obj1 && vol_obj2)
        if (vol_obj1->connector->cls->value != vol_obj2->connector->cls->value)
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                        "Objects are accessed through different VOL connectors and can't be linked")

    /* Construct a temporary VOL object */
    tmp_vol_obj.data      = (vol_obj2 ? (vol_obj2->data) : NULL);
    tmp_vol_obj.connector = (vol_obj1 != NULL ? vol_obj1->connector : vol_obj2->connector);

    /* Create the link */
    if (H5VL_link_create(H5VL_LINK_CREATE_HARD, &tmp_vol_obj, &loc_params2, lcpl_id, lapl_id,
                         H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, (vol_obj1 ? vol_obj1->data : NULL),
                         &loc_params1) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTCREATE, FAIL, "unable to create hard link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lcreate_hard() */

/*-------------------------------------------------------------------------
 * Function:    H5Lcreate_ud
 *
 * Purpose:     Creates a user-defined link of type LINK_TYPE named LINK_NAME
 *              with user-specified data UDATA.
 *
 *              The format of the information pointed to by UDATA is
 *              defined by the user. UDATA_SIZE holds the size of this buffer.
 *
 *              LINK_NAME is interpreted relative to LINK_LOC_ID.
 *
 *              The property list specified by LCPL_ID holds properties used
 *              to create the link.
 *
 *              The link class of the new link must already be registered
 *              with the library.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Tuesday, December 13, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lcreate_ud(hid_t link_loc_id, const char *link_name, H5L_type_t link_type, const void *udata,
             size_t udata_size, hid_t lcpl_id, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* Object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "i*sLl*xzii", link_loc_id, link_name, link_type, udata, udata_size, lcpl_id, lapl_id);

    /* Check arguments */
    if (!link_name || !*link_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no link name specified")
    if (link_type < H5L_TYPE_UD_MIN || link_type > H5L_TYPE_MAX)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid link class")
    if (!udata && udata_size)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "udata cannot be NULL if udata_size is non-zero")

    /* Get the link creation property list */
    if (H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;

    /* Set the LCPL for the API context */
    H5CX_set_lcpl(lcpl_id);

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, link_loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.loc_data.loc_by_name.name    = link_name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params.obj_type                     = H5I_get_type(link_loc_id);

    /* get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(link_loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Create external link */
    if (H5VL_link_create(H5VL_LINK_CREATE_UD, vol_obj, &loc_params, lcpl_id, lapl_id,
                         H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, (int)link_type, udata, udata_size) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lcreate_ud() */

/*-------------------------------------------------------------------------
 * Function:    H5Ldelete
 *
 * Purpose:     Removes the specified NAME from the group graph and
 *              decrements the link count for the object to which NAME
 *              points. If the link count reaches zero then all file-space
 *              associated with the object will be reclaimed (but if the
 *              object is open, then the reclamation of the file space is
 *              delayed until all handles to the object are closed).
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, April  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ldelete(hid_t loc_id, const char *name, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* Object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*si", loc_id, name, lapl_id);

    /* Check arguments */
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Fill in the location struct fields */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.obj_type                     = H5I_get_type(loc_id);
    loc_params.loc_data.loc_by_name.name    = name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;

    /* get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Unlink */
    if (H5VL_link_specific(vol_obj, &loc_params, H5VL_LINK_DELETE, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTDELETE, FAIL, "unable to delete link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ldelete() */

/*-------------------------------------------------------------------------
 * Function:    H5Ldelete_by_idx
 *
 * Purpose:     Removes the specified link from the group graph and
 *              decrements the link count for the object to which it
 *              points, according to the order within an index.
 *
 *              If the link count reaches zero then all file-space
 *              associated with the object will be reclaimed (but if the
 *              object is open, then the reclamation of the file space is
 *              delayed until all handles to the object are closed).
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, November 13, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ldelete_by_idx(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
                 hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "i*sIiIohi", loc_id, group_name, idx_type, order, n, lapl_id);

    /* Check arguments */
    if (!group_name || !*group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    loc_params.type                         = H5VL_OBJECT_BY_IDX;
    loc_params.loc_data.loc_by_idx.name     = group_name;
    loc_params.loc_data.loc_by_idx.idx_type = idx_type;
    loc_params.loc_data.loc_by_idx.order    = order;
    loc_params.loc_data.loc_by_idx.n        = n;
    loc_params.loc_data.loc_by_idx.lapl_id  = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Delete the link */
    if (H5VL_link_specific(vol_obj, &loc_params, H5VL_LINK_DELETE, H5P_DATASET_XFER_DEFAULT,
                           H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTDELETE, FAIL, "unable to delete link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ldelete_by_idx() */

/*-------------------------------------------------------------------------
 * Function:    H5Lget_val
 *
 * Purpose:     Returns the link value of a link whose name is NAME.  For
 *              symbolic links, this is the path to which the link points,
 *              including the null terminator.  For user-defined links, it
 *              is the link buffer.
 *
 *              At most SIZE bytes are copied to the BUF result buffer.
 *
 * Return:      Success:    Non-negative with the link value in BUF.
 *
 *              Failure:    Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, April 13, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lget_val(hid_t loc_id, const char *name, void *buf /*out*/, size_t size, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "i*sxzi", loc_id, name, buf, size, lapl_id);

    /* Check arguments */
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.obj_type                     = H5I_get_type(loc_id);
    loc_params.loc_data.loc_by_name.name    = name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get the link value */
    if (H5VL_link_get(vol_obj, &loc_params, H5VL_LINK_GET_VAL, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, buf,
                      size) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to get link value for '%s'", name)

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lget_val() */

/*-------------------------------------------------------------------------
 * Function:    H5Lget_val_by_idx
 *
 * Purpose:     Returns the link value of a link, according to the order of
 *              an index.  For symbolic links, this is the path to which the
 *              link points, including the null terminator.  For user-defined
 *              links, it is the link buffer.
 *
 *              At most SIZE bytes are copied to the BUF result buffer.
 *
 * Return:      Success:    Non-negative with the link value in BUF.
 *              Failure:    Negative
 *
 * Programmer:  Quincey Koziol
 *              Monday, November 13, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lget_val_by_idx(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
                  void *buf /*out*/, size_t size, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE8("e", "i*sIiIohxzi", loc_id, group_name, idx_type, order, n, buf, size, lapl_id);

    /* Check arguments */
    if (!group_name || !*group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    loc_params.type                         = H5VL_OBJECT_BY_IDX;
    loc_params.loc_data.loc_by_idx.name     = group_name;
    loc_params.loc_data.loc_by_idx.idx_type = idx_type;
    loc_params.loc_data.loc_by_idx.order    = order;
    loc_params.loc_data.loc_by_idx.n        = n;
    loc_params.loc_data.loc_by_idx.lapl_id  = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get the link value */
    if (H5VL_link_get(vol_obj, &loc_params, H5VL_LINK_GET_VAL, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, buf,
                      size) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to get link value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lget_val_by_idx() */

/*-------------------------------------------------------------------------
 * Function:    H5Lexists
 *
 * Purpose:     Checks if a link of a given name exists in a group
 *
 * Return:      Success:    TRUE/FALSE/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Friday, March 16, 2007
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Lexists(hid_t loc_id, const char *name, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    htri_t            ret_value = FAIL; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("t", "i*si", loc_id, name, lapl_id);

    /* Check arguments */
    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be NULL")
    if (!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be an empty string")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Set location struct fields */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.obj_type                     = H5I_get_type(loc_id);
    loc_params.loc_data.loc_by_name.name    = name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Check for the existence of the link */
    if (H5VL_link_specific(vol_obj, &loc_params, H5VL_LINK_EXISTS, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                           &ret_value) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to get link info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lexists() */

/*-------------------------------------------------------------------------
 * Function:    H5Lget_info2
 *
 * Purpose:     Gets metadata for a link.
 *
 * Return:      Success:    Non-negative with information in LINFO
 *              Failure:    Negative
 *
 * Programmer:  James Laird
 *              Wednesday, June 21, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lget_info2(hid_t loc_id, const char *name, H5L_info2_t *linfo /*out*/, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "i*sxi", loc_id, name, linfo, lapl_id);

    /* Check arguments */
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.obj_type                     = H5I_get_type(loc_id);
    loc_params.loc_data.loc_by_name.name    = name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;

    /* get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get the link information */
    if (H5VL_link_get(vol_obj, &loc_params, H5VL_LINK_GET_INFO, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                      linfo) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to get link info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lget_info2() */

/*-------------------------------------------------------------------------
 * Function:    H5Lget_info_by_idx2
 *
 * Purpose:     Gets metadata for a link, according to the order within an
 *              index.
 *
 * Return:      Success:    Non-negative with information in LINFO
 *              Failure:    Negative
 *
 * Programmer:  Quincey Koziol
 *              Monday, November  6, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lget_info_by_idx2(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order,
                    hsize_t n, H5L_info2_t *linfo /*out*/, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "i*sIiIohxi", loc_id, group_name, idx_type, order, n, linfo, lapl_id);

    /* Check arguments */
    if (!group_name || !*group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    loc_params.type                         = H5VL_OBJECT_BY_IDX;
    loc_params.loc_data.loc_by_idx.name     = group_name;
    loc_params.loc_data.loc_by_idx.idx_type = idx_type;
    loc_params.loc_data.loc_by_idx.order    = order;
    loc_params.loc_data.loc_by_idx.n        = n;
    loc_params.loc_data.loc_by_idx.lapl_id  = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get the link information */
    if (H5VL_link_get(vol_obj, &loc_params, H5VL_LINK_GET_INFO, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                      linfo) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to get link info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lget_info_by_idx2() */

/*-------------------------------------------------------------------------
 * Function:    H5Lregister
 *
 * Purpose:     Registers a class of user-defined links, or changes the
 *              behavior of an existing class.
 *
 *              The link class passed in will override any existing link
 *              class for the specified link class ID. It must at least
 *              include a H5L_class_t version (which should be
 *              H5L_LINK_CLASS_T_VERS), a link class ID, and a traversal
 *              function.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lregister(const H5L_class_t *cls)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "*x", cls);

    /* Check args */
    if (cls == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid link class")

    /* Check H5L_class_t version number; this is where a function to convert
     * from an outdated version should be called.
     *
     * v0 of the H5L_class_t is only different in the parameters to the
     * traversal callback, which is handled in H5G_traverse_ud()
     * (in src/H5Gtraverse.c), so it's allowed to to pass through here. - QAK, 2018/02/06
     */
    if (cls->version > H5L_LINK_CLASS_T_VERS)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid H5L_class_t version number")
#ifdef H5_NO_DEPRECATED_SYMBOLS
    if (cls->version < H5L_LINK_CLASS_T_VERS)
        HGOTO_ERROR(
            H5E_ARGS, H5E_BADVALUE, FAIL,
            "deprecated H5L_class_t version number (%d) and library built without deprecated symbol support",
            cls->version)
#endif /* H5_NO_DEPRECATED_SYMBOLS */

    if (cls->id < H5L_TYPE_UD_MIN || cls->id > H5L_TYPE_MAX)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid link identification number")
    if (cls->trav_func == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no traversal function specified")

    /* Do it */
    if (H5L_register(cls) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "unable to register link type")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lregister() */

/*-------------------------------------------------------------------------
 * Function:    H5Lunregister
 *
 * Purpose:     Unregisters a class of user-defined links, preventing them
 *              from being traversed, queried, moved, etc.
 *
 *              A link class can be re-registered using H5Lregister().
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lunregister(H5L_type_t id)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "Ll", id);

    /* Check args */
    if (id < 0 || id > H5L_TYPE_MAX)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid link type")

    /* Do it */
    if (H5L_unregister(id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "unable to unregister link type")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lunregister() */

/*-------------------------------------------------------------------------
 * Function:    H5Lis_registered
 *
 * Purpose:     Tests whether a user-defined link class has been registered
 *              or not.
 *
 * Return:      TRUE if the link class has been registered
 *              FALSE if it is unregistered
 *              FAIL on error (if the class is not a valid UD class ID)
 *
 * Programmer:  James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Lis_registered(H5L_type_t id)
{
    size_t i;                 /* Local index variable */
    htri_t ret_value = FALSE; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("t", "Ll", id);

    /* Check args */
    if (id < 0 || id > H5L_TYPE_MAX)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid link type id number")

    /* Is the link class already registered? */
    for (i = 0; i < H5L_table_used_g; i++)
        if (H5L_table_g[i].id == id) {
            ret_value = TRUE;
            break;
        } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lis_registered() */

/*-------------------------------------------------------------------------
 * Function:    H5Lget_name_by_idx
 *
 * Purpose:     Gets name for a link, according to the order within an
 *              index.
 *
 *              Same pattern of behavior as H5Iget_name.
 *
 * Return:      Success:    Non-negative length of name, with information
 *                          in NAME buffer
 *
 *              Failure:    -1
 *
 * Programmer:  Quincey Koziol
 *              Saturday, November 11, 2006
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Lget_name_by_idx(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order,
                   hsize_t n, char *name /*out*/, size_t size, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    ssize_t           ret_value = -1; /* Return value */

    FUNC_ENTER_API((-1))
    H5TRACE8("Zs", "i*sIiIohxzi", loc_id, group_name, idx_type, order, n, name, size, lapl_id);

    /* Check arguments */
    if (!group_name || !*group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "no name specified")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "invalid iteration order specified")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, (-1), "can't set access property list info")

    /* Fill in location struct fields */
    loc_params.type                         = H5VL_OBJECT_BY_IDX;
    loc_params.loc_data.loc_by_idx.name     = group_name;
    loc_params.loc_data.loc_by_idx.idx_type = idx_type;
    loc_params.loc_data.loc_by_idx.order    = order;
    loc_params.loc_data.loc_by_idx.n        = n;
    loc_params.loc_data.loc_by_idx.lapl_id  = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "invalid location identifier")

    /* Get the link information */
    if (H5VL_link_get(vol_obj, &loc_params, H5VL_LINK_GET_NAME, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                      name, size, &ret_value) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, (-1), "unable to get link name")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lget_name_by_idx() */

/*-------------------------------------------------------------------------
 * Function:    H5Literate2
 *
 * Purpose:     Iterates over links in a group, with user callback routine,
 *              according to the order within an index.
 *
 *              Same pattern of behavior as H5Giterate.
 *
 * Return:      Success:    The return value of the first operator that
 *                          returns non-zero, or zero if all members were
 *                          processed with no operator returning non-zero.
 *
 *              Failure:    Negative if something goes wrong within the
 *                          library, or the negative value returned by one
 *                          of the operators.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Literate2(hid_t group_id, H5_index_t idx_type, H5_iter_order_t order, hsize_t *idx_p, H5L_iterate2_t op,
            void *op_data)
{
    H5VL_object_t *   vol_obj = NULL; /* Object of loc_id */
    H5VL_loc_params_t loc_params;
    H5I_type_t        id_type;   /* Type of ID */
    herr_t            ret_value; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "iIiIo*hx*x", group_id, idx_type, order, idx_p, op, op_data);

    /* Check arguments */
    id_type = H5I_get_type(group_id);
    if (!(H5I_GROUP == id_type || H5I_FILE == id_type))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if (!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no operator specified")

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(group_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Set location struct fields */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(group_id);

    /* Iterate over the links */
    if ((ret_value = H5VL_link_specific(vol_obj, &loc_params, H5VL_LINK_ITER, H5P_DATASET_XFER_DEFAULT,
                                        H5_REQUEST_NULL, (unsigned)FALSE, (int)idx_type, (int)order, idx_p,
                                        op, op_data)) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_BADITER, FAIL, "link iteration failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Literate2() */

/*-------------------------------------------------------------------------
 * Function:    H5Literate_by_name2
 *
 * Purpose:     Iterates over links in a group, with user callback routine,
 *              according to the order within an index.
 *
 *              Same pattern of behavior as H5Giterate.
 *
 * Return:      Success:    The return value of the first operator that
 *                          returns non-zero, or zero if all members were
 *                          processed with no operator returning non-zero.
 *
 *              Failure:    Negative if something goes wrong within the
 *                          library, or the negative value returned by one
 *                          of the operators.
 *
 *
 * Programmer:  Quincey Koziol
 *              Thursday, November 16, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Literate_by_name2(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order,
                    hsize_t *idx_p, H5L_iterate2_t op, void *op_data, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* Object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE8("e", "i*sIiIo*hx*xi", loc_id, group_name, idx_type, order, idx_p, op, op_data, lapl_id);

    /* Check arguments */
    if (!group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "group_name parameter cannot be NULL")
    if (!*group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "group_name parameter cannot be an empty string")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if (!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no operator specified")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Set location struct fields */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.obj_type                     = H5I_get_type(loc_id);
    loc_params.loc_data.loc_by_name.name    = group_name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;

    /* Iterate over the links */
    if ((ret_value = H5VL_link_specific(vol_obj, &loc_params, H5VL_LINK_ITER, H5P_DATASET_XFER_DEFAULT,
                                        H5_REQUEST_NULL, FALSE, idx_type, order, idx_p, op, op_data)) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_BADITER, FAIL, "link iteration failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Literate_by_name() */

/*-------------------------------------------------------------------------
 * Function:    H5Lvisit2
 *
 * Purpose:     Recursively visit all the links in a group and all
 *              the groups that are linked to from that group.  Links within
 *              each group are visited according to the order within the
 *              specified index (unless the specified index does not exist for
 *              a particular group, then the "name" index is used).
 *
 *              NOTE: Each _link_ reachable from the initial group will only be
 *              visited once.  However, because an object may be reached from
 *              more than one link, the visitation may call the application's
 *              callback with more than one link that points to a particular
 *              _object_.
 *
 * Return:      Success:    The return value of the first operator that
 *                          returns non-zero, or zero if all members were
 *                          processed with no operator returning non-zero.
 *
 *              Failure:    Negative if something goes wrong within the
 *                          library, or the negative value returned by one
 *                          of the operators.
 *
 * Programmer:  Quincey Koziol
 *              November 24 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lvisit2(hid_t group_id, H5_index_t idx_type, H5_iter_order_t order, H5L_iterate2_t op, void *op_data)
{
    H5VL_object_t *   vol_obj = NULL; /* Object of loc_id */
    H5VL_loc_params_t loc_params;
    H5I_type_t        id_type;   /* Type of ID */
    herr_t            ret_value; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "iIiIox*x", group_id, idx_type, order, op, op_data);

    /* Check args */
    id_type = H5I_get_type(group_id);
    if (!(H5I_GROUP == id_type || H5I_FILE == id_type))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if (!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no callback operator specified")

    /* Set location struct fields */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(group_id);

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(group_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Iterate over the links */
    if ((ret_value = H5VL_link_specific(vol_obj, &loc_params, H5VL_LINK_ITER, H5P_DATASET_XFER_DEFAULT,
                                        H5_REQUEST_NULL, TRUE, idx_type, order, NULL, op, op_data)) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_BADITER, FAIL, "link visitation failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lvisit2() */

/*-------------------------------------------------------------------------
 * Function:    H5Lvisit_by_name2
 *
 * Purpose:     Recursively visit all the links in a group and all
 *              the groups that are linked to from that group.  Links within
 *              each group are visited according to the order within the
 *              specified index (unless the specified index does not exist for
 *              a particular group, then the "name" index is used).
 *
 *              NOTE: Each _link_ reachable from the initial group will only be
 *              visited once.  However, because an object may be reached from
 *              more than one link, the visitation may call the application's
 *              callback with more than one link that points to a particular
 *              _object_.
 *
 * Return:      Success:    The return value of the first operator that
 *                          returns non-zero, or zero if all members were
 *                          processed with no operator returning non-zero.
 *
 *              Failure:    Negative if something goes wrong within the
 *                          library, or the negative value returned by one
 *                          of the operators.
 *
 * Programmer:  Quincey Koziol
 *              November 3 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lvisit_by_name2(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order,
                  H5L_iterate2_t op, void *op_data, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL; /* Object of loc_id */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "i*sIiIox*xi", loc_id, group_name, idx_type, order, op, op_data, lapl_id);

    /* Check args */
    if (!group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "group_name parameter cannot be NULL")
    if (!*group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "group_name parameter cannot be an empty string")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if (!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no callback operator specified")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't set access property list info")

    /* get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Set location struct fields */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.obj_type                     = H5I_get_type(loc_id);
    loc_params.loc_data.loc_by_name.name    = group_name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;

    /* Visit the links */
    if ((ret_value = H5VL_link_specific(vol_obj, &loc_params, H5VL_LINK_ITER, H5P_DATASET_XFER_DEFAULT,
                                        H5_REQUEST_NULL, TRUE, idx_type, order, NULL, op, op_data)) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_BADITER, FAIL, "link visitation failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lvisit_by_name2() */

/*
 *-------------------------------------------------------------------------
 *-------------------------------------------------------------------------
 *   N O   A P I   F U N C T I O N S   B E Y O N D   T H I S   P O I N T
 *-------------------------------------------------------------------------
 *-------------------------------------------------------------------------
 */

/*-------------------------------------------------------------------------
 * Function:    H5L__find_class_idx
 *
 * Purpose:     Given a link class ID, return the offset in the global array
 *              that holds all the registered link classes.
 *
 * Return:      Success:    Non-negative index of entry in global
 *                              link class table.
 *              Failure:    Negative
 *
 * Programmer:  James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
static int
H5L__find_class_idx(H5L_type_t id)
{
    size_t i;                /* Local index variable */
    int    ret_value = FAIL; /* Return value */

    FUNC_ENTER_STATIC_NOERR

    for (i = 0; i < H5L_table_used_g; i++)
        if (H5L_table_g[i].id == id)
            HGOTO_DONE((int)i)

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__find_class_idx */

/*-------------------------------------------------------------------------
 * Function:    H5L_find_class
 *
 * Purpose:     Given a link class ID return a pointer to a global struct that
 *              defines the link class.
 *
 * Return:      Success:    Ptr to entry in global link class table.
 *              Failure:    NULL
 *
 * Programmer:  James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
const H5L_class_t *
H5L_find_class(H5L_type_t id)
{
    int          idx;              /* Filter index in global table */
    H5L_class_t *ret_value = NULL; /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Get the index in the global table */
    if ((idx = H5L__find_class_idx(id)) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, NULL, "unable to find link class")

    /* Set return value */
    ret_value = H5L_table_g + idx;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_find_class */

/*-------------------------------------------------------------------------
 * Function:    H5L_register
 *
 * Purpose:     Registers a class of user-defined links, or changes the
 *              behavior of an existing class.
 *
 *              See H5Lregister for full documentation.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_register(const H5L_class_t *cls)
{
    size_t i;                   /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(cls);
    HDassert(cls->id >= 0 && cls->id <= H5L_TYPE_MAX);

    /* Is the link type already registered? */
    for (i = 0; i < H5L_table_used_g; i++)
        if (H5L_table_g[i].id == cls->id)
            break;

    /* Filter not already registered */
    if (i >= H5L_table_used_g) {
        if (H5L_table_used_g >= H5L_table_alloc_g) {
            size_t       n     = MAX(H5L_MIN_TABLE_SIZE, (2 * H5L_table_alloc_g));
            H5L_class_t *table = (H5L_class_t *)H5MM_realloc(H5L_table_g, (n * sizeof(H5L_class_t)));
            if (!table)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to extend link type table")
            H5L_table_g       = table;
            H5L_table_alloc_g = n;
        } /* end if */

        /* Initialize */
        i = H5L_table_used_g++;
    } /* end if */

    /* Copy link class info into table */
    H5MM_memcpy(H5L_table_g + i, cls, sizeof(H5L_class_t));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_register */

/*-------------------------------------------------------------------------
 * Function:    H5L_unregister
 *
 * Purpose:     Unregisters a class of user-defined links.
 *
 *              See H5Lunregister for full documentation.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_unregister(H5L_type_t id)
{
    size_t i;                   /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(id >= 0 && id <= H5L_TYPE_MAX);

    /* Is the filter already registered? */
    for (i = 0; i < H5L_table_used_g; i++)
        if (H5L_table_g[i].id == id)
            break;

    /* Fail if filter not found */
    if (i >= H5L_table_used_g)
        HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "link class is not registered")

    /* Remove filter from table */
    /* Don't worry about shrinking table size (for now) */
    HDmemmove(&H5L_table_g[i], &H5L_table_g[i + 1], sizeof(H5L_class_t) * ((H5L_table_used_g - 1) - i));
    H5L_table_used_g--;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_unregister() */

/*-------------------------------------------------------------------------
 * Function:    H5L_link
 *
 * Purpose:     Creates a link from OBJ_ID to CUR_NAME.  See H5Olink() for
 *              full documentation.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Tuesday, December 13, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_link(const H5G_loc_t *new_loc, const char *new_name, H5G_loc_t *obj_loc, hid_t lcpl_id)
{
    H5O_link_t lnk;                 /* Link to insert */
    herr_t     ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(new_loc);
    HDassert(obj_loc);
    HDassert(new_name && *new_name);

    /* The link callback will check that the object isn't being hard linked
     * into a different file, so we don't need to do it here (there could be
     * external links along the path).
     */

    /* Construct link information for eventual insertion */
    lnk.type        = H5L_TYPE_HARD;
    lnk.u.hard.addr = obj_loc->oloc->addr;

    /* Create the link */
    if (H5L__create_real(new_loc, new_name, obj_loc->path, obj_loc->oloc->file, &lnk, NULL, lcpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link to object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_link() */

/*-------------------------------------------------------------------------
 * Function:    H5L_link_object
 *
 * Purpose:     Creates a new object and a link to it.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, April 9, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_link_object(const H5G_loc_t *new_loc, const char *new_name, H5O_obj_create_t *ocrt_info, hid_t lcpl_id)
{
    H5O_link_t lnk;                 /* Link to insert */
    herr_t     ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(new_loc);
    HDassert(new_name && *new_name);
    HDassert(ocrt_info);

    /* The link callback will check that the object isn't being hard linked
     * into a different file, so we don't need to do it here (there could be
     * external links along the path).
     */

    /* Construct link information for eventual insertion */
    lnk.type = H5L_TYPE_HARD;

    /* Create the link */
    if (H5L__create_real(new_loc, new_name, NULL, NULL, &lnk, ocrt_info, lcpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link to object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_link_object() */

/*-------------------------------------------------------------------------
 * Function:    H5L__link_cb
 *
 * Purpose:     Callback for creating a link to an object.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, September 19, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__link_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t H5_ATTR_UNUSED *lnk,
             H5G_loc_t *obj_loc, void *_udata /*in,out*/, H5G_own_loc_t *own_loc /*out*/)
{
    H5L_trav_cr_t *udata  = (H5L_trav_cr_t *)_udata; /* User data passed in */
    H5G_t *        grp    = NULL;    /* H5G_t for this group, opened to pass to user callback */
    hid_t          grp_id = FAIL;    /* Id for this group (passed to user callback */
    H5G_loc_t      temp_loc;         /* For UD callback */
    hbool_t temp_loc_init = FALSE;   /* Temporary location for UD callback (temp_loc) has been initialized */
    hbool_t obj_created   = FALSE;   /* Whether an object was created (through a hard link) */
    herr_t  ret_value     = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the name in this group resolved to a valid location */
    /* (which is not what we want) */
    if (obj_loc != NULL)
        HGOTO_ERROR(H5E_LINK, H5E_EXISTS, FAIL, "name already exists")

    /* Check for crossing file boundaries with a new hard link */
    if (udata->lnk->type == H5L_TYPE_HARD) {
        /* Check for creating an object */
        /* (only for hard links) */
        if (udata->ocrt_info) {
            H5G_loc_t new_loc; /* Group location for new object */

            /* Create new object at this location */
            if (NULL ==
                (udata->ocrt_info->new_obj = H5O_obj_create(grp_loc->oloc->file, udata->ocrt_info->obj_type,
                                                            udata->ocrt_info->crt_info, &new_loc)))
                HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create object")

            /* Set address for hard link */
            udata->lnk->u.hard.addr = new_loc.oloc->addr;

            /* Set object path to use for setting object name (below) */
            udata->path = new_loc.path;

            /* Indicate that an object was created */
            obj_created = TRUE;
        } /* end if */
        else {
            /* Check that both objects are in same file */
            if (!H5F_SAME_SHARED(grp_loc->oloc->file, udata->file))
                HGOTO_ERROR(H5E_LINK, H5E_BADVALUE, FAIL, "interfile hard links are not allowed")
        } /* end else */
    }     /* end if */

    /* Set 'standard' aspects of link */
    udata->lnk->corder =
        0; /* Will be re-written during group insertion, if the group is tracking creation order */
    udata->lnk->corder_valid = FALSE; /* Creation order not valid (yet) */

    /* Check for non-default link creation properties */
    if (udata->lc_plist) {
        /* Get character encoding property */
        if (H5CX_get_encoding(&udata->lnk->cset) < 0)
            HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't get 'character set' property")
    } /* end if */
    else
        udata->lnk->cset = H5F_DEFAULT_CSET; /* Default character encoding for link */

    /* Set the link's name correctly */
    /* Casting away const OK -QAK */
    udata->lnk->name = (char *)name;

    /* Insert link into group */
    if (H5G_obj_insert(grp_loc->oloc, name, udata->lnk, TRUE,
                       udata->ocrt_info ? udata->ocrt_info->obj_type : H5O_TYPE_UNKNOWN,
                       udata->ocrt_info ? udata->ocrt_info->crt_info : NULL) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link for object")

    /* Set object's path if it has been passed in and is not set */
    if (udata->path != NULL && udata->path->user_path_r == NULL)
        if (H5G_name_set(grp_loc->path, udata->path, name) < 0)
            HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "cannot set name")

    /* If link is a user-defined link, trigger its creation callback if it has one */
    if (udata->lnk->type >= H5L_TYPE_UD_MIN) {
        const H5L_class_t *link_class; /* User-defined link class */

        /* Get the link class for this type of link. */
        if (NULL == (link_class = H5L_find_class(udata->lnk->type)))
            HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "unable to get class of UD link")

        if (link_class->create_func != NULL) {
            H5O_loc_t  temp_oloc;
            H5G_name_t temp_path;

            /* Create a temporary location (or else H5G_open will do a shallow
             * copy and wipe out grp_loc)
             */
            H5G_name_reset(&temp_path);
            if (H5O_loc_copy_deep(&temp_oloc, grp_loc->oloc) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTCOPY, FAIL, "unable to copy object location")

            temp_loc.oloc = &temp_oloc;
            temp_loc.path = &temp_path;
            temp_loc_init = TRUE;

            /* Set up location for user-defined callback */
            if (NULL == (grp = H5G_open(&temp_loc)))
                HGOTO_ERROR(H5E_LINK, H5E_CANTOPENOBJ, FAIL, "unable to open group")
            if ((grp_id = H5VL_wrap_register(H5I_GROUP, grp, TRUE)) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTREGISTER, FAIL, "unable to register ID for group")

            /* Make callback */
            if ((link_class->create_func)(name, grp_id, udata->lnk->u.ud.udata, udata->lnk->u.ud.size,
                                          H5P_DEFAULT) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CALLBACK, FAIL, "link creation callback failed")
        } /* end if */
    }     /* end if */

done:
    /* Check if an object was created */
    if (obj_created) {
        H5O_loc_t oloc; /* Object location for created object */

        /* Set up object location */
        HDmemset(&oloc, 0, sizeof(oloc));
        oloc.file = grp_loc->oloc->file;
        oloc.addr = udata->lnk->u.hard.addr;

        /* Decrement refcount on new object's object header in memory */
        if (H5O_dec_rc_by_loc(&oloc) < 0)
            HDONE_ERROR(H5E_LINK, H5E_CANTDEC, FAIL, "unable to decrement refcount on newly created object")
    } /* end if */

    /* Close the location given to the user callback if it was created */
    if (grp_id >= 0) {
        if (H5I_dec_app_ref(grp_id) < 0)
            HDONE_ERROR(H5E_LINK, H5E_CANTRELEASE, FAIL, "unable to close atom from UD callback")
    } /* end if */
    else if (grp != NULL) {
        if (H5G_close(grp) < 0)
            HDONE_ERROR(H5E_LINK, H5E_CANTRELEASE, FAIL, "unable to close group given to UD callback")
    } /* end if */
    else if (temp_loc_init)
        H5G_loc_free(&temp_loc);

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__link_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L__create_real
 *
 * Purpose:     Creates a link at a path location
 *
 *              lnk should have linkclass-specific information already
 *              set, but this function will take care of setting name.
 *
 *              obj_path can be NULL if the object's path doesn't need to
 *              be set, and obj_file can be NULL if the object is not a
 *              hard link.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, December  5, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__create_real(const H5G_loc_t *link_loc, const char *link_name, H5G_name_t *obj_path, H5F_t *obj_file,
                 H5O_link_t *lnk, H5O_obj_create_t *ocrt_info, hid_t lcpl_id)
{
    char *          norm_link_name = NULL;              /* Pointer to normalized link name */
    unsigned        target_flags   = H5G_TARGET_NORMAL; /* Flags to pass to group traversal function */
    H5P_genplist_t *lc_plist       = NULL;              /* Link creation property list */
    H5L_trav_cr_t   udata;                              /* User data for callback */
    herr_t          ret_value = SUCCEED;                /* Return value */

    FUNC_ENTER_STATIC

    /* Check args */
    HDassert(link_loc);
    HDassert(link_name && *link_name);
    HDassert(lnk);
    HDassert(lnk->type >= H5L_TYPE_HARD && lnk->type <= H5L_TYPE_MAX);

    /* Get normalized link name */
    if ((norm_link_name = H5G_normalize(link_name)) == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_BADVALUE, FAIL, "can't normalize name")

    /* Check for flags present in creation property list */
    if (lcpl_id != H5P_DEFAULT) {
        unsigned crt_intmd_group;

        /* Get link creation property list */
        if (NULL == (lc_plist = (H5P_genplist_t *)H5I_object(lcpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

        /* Get intermediate group creation property */
        if (H5CX_get_intermediate_group(&crt_intmd_group) < 0)
            HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't get 'create intermediate group' property")

        if (crt_intmd_group > 0)
            target_flags |= H5G_CRT_INTMD_GROUP;
    } /* end if */

    /* Set up user data
     * FILE is used to make sure that hard links don't cross files, and
     * should be NULL for other link types.
     * LC_PLIST is a pointer to the link creation property list.
     * PATH is a pointer to the path of the object being inserted if this is
     * a hard link; this is used to set the paths to objects when they are
     * created.  For other link types, this is NULL.
     * OCRT_INFO is a pointer to the structure for object creation.
     * LNK is the link struct passed into this function.  At this point all
     * of its fields should be populated except for name, which is set when
     * inserting it in the callback.
     */
    udata.file      = obj_file;
    udata.lc_plist  = lc_plist;
    udata.path      = obj_path;
    udata.ocrt_info = ocrt_info;
    udata.lnk       = lnk;

    /* Traverse the destination path & create new link */
    if (H5G_traverse(link_loc, link_name, target_flags, H5L__link_cb, &udata) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINSERT, FAIL, "can't insert link")

done:
    /* Free the normalized path name */
    if (norm_link_name)
        H5MM_xfree(norm_link_name);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__create_real() */

/*-------------------------------------------------------------------------
 * Function:    H5L__create_hard
 *
 * Purpose:     Creates a hard link from NEW_NAME to CUR_NAME.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, April  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L__create_hard(H5G_loc_t *cur_loc, const char *cur_name, const H5G_loc_t *link_loc, const char *link_name,
                 hid_t lcpl_id)
{
    char *     norm_cur_name = NULL; /* Pointer to normalized current name */
    H5F_t *    link_file     = NULL; /* Pointer to file to link to */
    H5O_link_t lnk;                  /* Link to insert */
    H5G_loc_t  obj_loc;              /* Location of object to link to */
    H5G_name_t path;                 /* obj_loc's path*/
    H5O_loc_t  oloc;                 /* obj_loc's oloc */
    hbool_t    loc_valid = FALSE;
    herr_t     ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(cur_loc);
    HDassert(cur_name && *cur_name);
    HDassert(link_loc);
    HDassert(link_name && *link_name);

    /* Get normalized copy of the current name */
    if ((norm_cur_name = H5G_normalize(cur_name)) == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_BADVALUE, FAIL, "can't normalize name")

    /* Set up link data specific to hard links */
    lnk.type = H5L_TYPE_HARD;

    /* Get object location for object pointed to */
    obj_loc.path = &path;
    obj_loc.oloc = &oloc;
    H5G_loc_reset(&obj_loc);
    if (H5G_loc_find(cur_loc, norm_cur_name, &obj_loc) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "source object not found")
    loc_valid = TRUE;

    /* Construct link information for eventual insertion */
    lnk.u.hard.addr = obj_loc.oloc->addr;

    /* Set destination's file information */
    link_file = obj_loc.oloc->file;

    /* Create actual link to the object.  Pass in NULL for the path, since this
     * function shouldn't change an object's user path. */
    if (H5L__create_real(link_loc, link_name, NULL, link_file, &lnk, NULL, lcpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link to object")

done:
    /* Free the object header location */
    if (loc_valid)
        if (H5G_loc_free(&obj_loc) < 0)
            HDONE_ERROR(H5E_LINK, H5E_CANTRELEASE, FAIL, "unable to free location")

    /* Free the normalized path name */
    if (norm_cur_name)
        H5MM_xfree(norm_cur_name);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__create_hard() */

/*-------------------------------------------------------------------------
 * Function:    H5L__create_soft
 *
 * Purpose:     Creates a soft link from LINK_NAME to TARGET_PATH.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Robb Matzke
 *              Monday, April  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L__create_soft(const char *target_path, const H5G_loc_t *link_loc, const char *link_name, hid_t lcpl_id)
{
    char *     norm_target = NULL;  /* Pointer to normalized current name */
    H5O_link_t lnk;                 /* Link to insert */
    herr_t     ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(link_loc);
    HDassert(target_path && *target_path);
    HDassert(link_name && *link_name);

    /* Get normalized copy of the link target */
    if ((norm_target = H5G_normalize(target_path)) == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_BADVALUE, FAIL, "can't normalize name")

    /* Set up link data specific to soft links */
    lnk.type        = H5L_TYPE_SOFT;
    lnk.u.soft.name = norm_target;

    /* Create actual link to the object */
    if (H5L__create_real(link_loc, link_name, NULL, NULL, &lnk, NULL, lcpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link to object")

done:
    /* Free the normalized target name */
    if (norm_target)
        H5MM_xfree(norm_target);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__create_soft() */

/*-------------------------------------------------------------------------
 * Function:    H5L__create_ud
 *
 * Purpose:     Creates a user-defined link. See H5Lcreate_ud for
 *              full documentation.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Friday, May 19, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L__create_ud(const H5G_loc_t *link_loc, const char *link_name, const void *ud_data, size_t ud_data_size,
               H5L_type_t type, hid_t lcpl_id)
{
    H5O_link_t lnk;                 /* Link to insert */
    herr_t     ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(type >= H5L_TYPE_UD_MIN && type <= H5L_TYPE_MAX);
    HDassert(link_loc);
    HDassert(link_name && *link_name);
    HDassert(ud_data_size == 0 || ud_data);

    /* Initialize the link struct's pointer to its udata buffer */
    lnk.u.ud.udata = NULL;

    /* Make sure that this link class is registered */
    if (H5L__find_class_idx(type) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "link class has not been registered with library")

    /* Fill in UD link-specific information in the link struct*/
    if (ud_data_size > 0) {
        lnk.u.ud.udata = H5MM_malloc((size_t)ud_data_size);
        H5MM_memcpy(lnk.u.ud.udata, ud_data, (size_t)ud_data_size);
    } /* end if */
    else
        lnk.u.ud.udata = NULL;

    lnk.u.ud.size = ud_data_size;
    lnk.type      = type;

    /* Create actual link to the object */
    if (H5L__create_real(link_loc, link_name, NULL, NULL, &lnk, NULL, lcpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to register new name for object")

done:
    /* Free the link's udata buffer if it's been allocated */
    H5MM_xfree(lnk.u.ud.udata);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__create_ud() */

/*-------------------------------------------------------------------------
 * Function:    H5L__get_val_real
 *
 * Purpose:     Retrieve link value from a link object
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, November 13 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__get_val_real(const H5O_link_t *lnk, void *buf, size_t size)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(lnk);

    /* Check for soft link */
    if (H5L_TYPE_SOFT == lnk->type) {
        /* Copy to output buffer */
        if (size > 0 && buf) {
            HDstrncpy((char *)buf, lnk->u.soft.name, size);
            if (HDstrlen(lnk->u.soft.name) >= size)
                ((char *)buf)[size - 1] = '\0';
        } /* end if */
    }     /* end if */
    /* Check for user-defined link */
    else if (lnk->type >= H5L_TYPE_UD_MIN) {
        const H5L_class_t *link_class; /* User-defined link class */

        /* Get the link class for this type of link.  It's okay if the class
         * isn't registered, though--we just can't give any more information
         * about it
         */
        link_class = H5L_find_class(lnk->type);

        if (link_class != NULL && link_class->query_func != NULL) {
            if ((link_class->query_func)(lnk->name, lnk->u.ud.udata, lnk->u.ud.size, buf, size) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CALLBACK, FAIL, "query callback returned failure")
        } /* end if */
        else if (buf && size > 0)
            ((char *)buf)[0] = '\0';
    } /* end if */
    else
        HGOTO_ERROR(H5E_LINK, H5E_BADTYPE, FAIL, "object is not a symbolic or user-defined link")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__get_val_real() */

/*-------------------------------------------------------------------------
 * Function:    H5L__get_val_cb
 *
 * Purpose:     Callback for retrieving link value or udata.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, September 20, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__get_val_cb(H5G_loc_t H5_ATTR_UNUSED *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
                H5G_loc_t H5_ATTR_UNUSED *obj_loc, void *_udata /*in,out*/, H5G_own_loc_t *own_loc /*out*/)
{
    H5L_trav_gv_t *udata     = (H5L_trav_gv_t *)_udata; /* User data passed in */
    herr_t         ret_value = SUCCEED;                 /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the name in this group resolved to a valid link */
    if (lnk == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "'%s' doesn't exist", name)

    /* Retrieve the value for the link */
    if (H5L__get_val_real(lnk, udata->buf, udata->size) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't retrieve link value")

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__get_val_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L__get_val
 *
 * Purpose:     Returns the value of a symbolic link or the udata for a
 *              user-defined link.
 *
 * Return:      Success:    Non-negative, with at most SIZE bytes of the
 *              link value copied into the BUF buffer.  If the
 *              link value is larger than SIZE characters
 *              counting the null terminator then the BUF
 *              result will not be null terminated.
 *
 *              Failure:    Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, April 13, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L__get_val(const H5G_loc_t *loc, const char *name, void *buf /*out*/, size_t size)
{
    H5L_trav_gv_t udata;               /* User data for callback */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(loc);
    HDassert(name && *name);

    /* Set up user data for retrieving information */
    udata.size = size;
    udata.buf  = buf;

    /* Traverse the group hierarchy to locate the object to get info about */
    if (H5G_traverse(loc, name, H5G_TARGET_SLINK | H5G_TARGET_UDLINK, H5L__get_val_cb, &udata) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "name doesn't exist")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5L__get_val() */

/*-------------------------------------------------------------------------
 * Function:    H5L__get_val_by_idx_cb
 *
 * Purpose:     Callback for retrieving a link's value according to an
 *              index's order.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, November 13 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__get_val_by_idx_cb(H5G_loc_t H5_ATTR_UNUSED *grp_loc /*in*/, const char H5_ATTR_UNUSED *name,
                       const H5O_link_t H5_ATTR_UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata /*in,out*/,
                       H5G_own_loc_t *own_loc /*out*/)
{
    H5L_trav_gvbi_t *udata = (H5L_trav_gvbi_t *)_udata; /* User data passed in */
    H5O_link_t       fnd_lnk;                           /* Link within group */
    hbool_t          lnk_copied = FALSE;                /* Whether the link was copied */
    herr_t           ret_value  = SUCCEED;              /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the name of the group resolved to a valid object */
    if (obj_loc == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "group doesn't exist")

    /* Query link */
    if (H5G_obj_lookup_by_idx(obj_loc->oloc, udata->idx_type, udata->order, udata->n, &fnd_lnk) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "link not found")
    lnk_copied = TRUE;

    /* Retrieve the value for the link */
    if (H5L__get_val_real(&fnd_lnk, udata->buf, udata->size) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't retrieve link value")

done:
    /* Reset the link information, if we have a copy */
    if (lnk_copied)
        H5O_msg_reset(H5O_LINK_ID, &fnd_lnk);

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__get_val_by_idx_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L__get_val_by_idx
 *
 * Purpose:     Internal routine to query a link value according to the
 *              index within a group
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		December 27, 2017
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L__get_val_by_idx(const H5G_loc_t *loc, const char *name, H5_index_t idx_type, H5_iter_order_t order,
                    hsize_t n, void *buf /*out*/, size_t size)
{
    H5L_trav_gvbi_t udata;               /* User data for callback */
    herr_t          ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    HDassert(loc);
    HDassert(name && *name);

    /* Set up user data for retrieving information */
    udata.idx_type = idx_type;
    udata.order    = order;
    udata.n        = n;
    udata.buf      = buf;
    udata.size     = size;

    /* Traverse the group hierarchy to locate the object to get info about */
    if (H5G_traverse(loc, name, H5G_TARGET_SLINK | H5G_TARGET_UDLINK, H5L__get_val_by_idx_cb, &udata) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't get link info for index: %llu", (unsigned long long)n)

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__get_val_by_idx() */

/*-------------------------------------------------------------------------
 * Function:    H5L__delete_cb
 *
 * Purpose:     Callback for deleting a link.  This routine
 *              actually deletes the link
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, September 19, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__delete_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk,
               H5G_loc_t H5_ATTR_UNUSED *obj_loc, void H5_ATTR_UNUSED *_udata /*in,out*/,
               H5G_own_loc_t *own_loc /*out*/)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Check if the group resolved to a valid link */
    if (grp_loc == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "group doesn't exist")

    /* Check if the name in this group resolved to a valid link */
    if (name == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "name doesn't exist")

    /* Check for non-existent (NULL) link.
     * Note that this can also occur when attempting to remove '.'
     */
    if (lnk == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_CANTDELETE, FAIL,
                    "callback link pointer is NULL (specified link may be '.' or not exist)")

    /* Remove the link from the group */
    if (H5G_obj_remove(grp_loc->oloc, grp_loc->path->full_path_r, name) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTDELETE, FAIL, "unable to remove link from group")

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__delete_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L__delete
 *
 * Purpose:     Delete a link from a group.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, September 17, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L__delete(const H5G_loc_t *loc, const char *name)
{
    char * norm_name = NULL;    /* Pointer to normalized name */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(loc);
    HDassert(name && *name);

    /* Get normalized copy of the name */
    if ((norm_name = H5G_normalize(name)) == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_BADVALUE, FAIL, "can't normalize name")

    /* Set up user data for unlink operation */
    if (H5G_traverse(loc, norm_name, H5G_TARGET_SLINK | H5G_TARGET_UDLINK | H5G_TARGET_MOUNT, H5L__delete_cb,
                     NULL) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTREMOVE, FAIL, "can't unlink object")

done:
    /* Free the normalized path name */
    if (norm_name)
        H5MM_xfree(norm_name);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__delete() */

/*-------------------------------------------------------------------------
 * Function:    H5L__delete_by_idx_cb
 *
 * Purpose:     Callback for removing a link according to an index's order.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, November 13 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__delete_by_idx_cb(H5G_loc_t H5_ATTR_UNUSED *grp_loc /*in*/, const char H5_ATTR_UNUSED *name,
                      const H5O_link_t H5_ATTR_UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata /*in,out*/,
                      H5G_own_loc_t *own_loc /*out*/)
{
    H5L_trav_gvbi_t *udata     = (H5L_trav_gvbi_t *)_udata; /* User data passed in */
    herr_t           ret_value = SUCCEED;                   /* Return value */

    FUNC_ENTER_STATIC_TAG((obj_loc) ? (obj_loc->oloc->addr) : HADDR_UNDEF)

    /* Check if the name of the group resolved to a valid object */
    if (obj_loc == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "group doesn't exist")

    /* Delete link */
    if (H5G_obj_remove_by_idx(obj_loc->oloc, obj_loc->path->full_path_r, udata->idx_type, udata->order,
                              udata->n) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "link not found")

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI_TAG(ret_value)
} /* end H5L__delete_by_idx_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L__delete_by_idx
 *
 * Purpose:     Internal routine to delete a link according to its index
 *              within a group.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *		December 27, 2017
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L__delete_by_idx(const H5G_loc_t *loc, const char *name, H5_index_t idx_type, H5_iter_order_t order,
                   hsize_t n)
{
    H5L_trav_rmbi_t udata;               /* User data for callback */
    herr_t          ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(loc);
    HDassert(name && *name);

    /* Set up user data for unlink operation */
    udata.idx_type = idx_type;
    udata.order    = order;
    udata.n        = n;

    /* Traverse the group hierarchy to remove the link */
    if (H5G_traverse(loc, name, H5G_TARGET_SLINK | H5G_TARGET_UDLINK | H5G_TARGET_MOUNT,
                     H5L__delete_by_idx_cb, &udata) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTDELETE, FAIL, "link doesn't exist")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__delete_by_idx() */

/*-------------------------------------------------------------------------
 * Function:    H5L__move_dest_cb
 *
 * Purpose:     Second callback for moving and renaming an object.  This routine
 *              inserts a new link into the group returned by the traversal.
 *              It is called by H5L__move_cb.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Monday, April 3, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__move_dest_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t H5_ATTR_UNUSED *lnk,
                  H5G_loc_t *obj_loc, void *_udata /*in,out*/, H5G_own_loc_t *own_loc /*out*/)
{
    H5L_trav_mv2_t *udata  = (H5L_trav_mv2_t *)_udata; /* User data passed in */
    H5G_t *         grp    = NULL; /* H5G_t for this group, opened to pass to user callback */
    hid_t           grp_id = FAIL; /* ID for this group (passed to user callback */
    H5G_loc_t       temp_loc;      /* For UD callback */
    hbool_t         temp_loc_init = FALSE;
    herr_t          ret_value     = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Make sure an object with this name doesn't already exist */
    if (obj_loc != NULL)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "an object with that name already exists")

    /* Check for crossing file boundaries with a new hard link */
    if (udata->lnk->type == H5L_TYPE_HARD)
        /* Check that both objects are in same file */
        if (!H5F_SAME_SHARED(grp_loc->oloc->file, udata->file))
            HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "moving a link across files is not allowed")

    /* Give the object its new name */
    /* Casting away const okay -JML */
    HDassert(udata->lnk->name == NULL);
    udata->lnk->name = (char *)name;

    /* Insert the link into the group */
    if (H5G_obj_insert(grp_loc->oloc, name, udata->lnk, TRUE, H5O_TYPE_UNKNOWN, NULL) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link to object")

    /* If the link was a user-defined link, call its move callback if it has one */
    if (udata->lnk->type >= H5L_TYPE_UD_MIN) {
        const H5L_class_t *link_class; /* User-defined link class */

        /* Get the link class for this type of link. */
        if (NULL == (link_class = H5L_find_class(udata->lnk->type)))
            HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "link class is not registered")

        if ((!udata->copy && link_class->move_func) || (udata->copy && link_class->copy_func)) {
            H5O_loc_t  temp_oloc;
            H5G_name_t temp_path;

            /* Create a temporary location (or else H5G_open will do a shallow
             * copy and wipe out grp_loc)
             */
            H5G_name_reset(&temp_path);
            if (H5O_loc_copy_deep(&temp_oloc, grp_loc->oloc) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTCOPY, FAIL, "unable to copy object location")

            temp_loc.oloc = &temp_oloc;
            temp_loc.path = &temp_path;
            temp_loc_init = TRUE;

            /* Set up location for user-defined callback */
            if (NULL == (grp = H5G_open(&temp_loc)))
                HGOTO_ERROR(H5E_LINK, H5E_CANTOPENOBJ, FAIL, "unable to open group")
            if ((grp_id = H5VL_wrap_register(H5I_GROUP, grp, TRUE)) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTREGISTER, FAIL, "unable to register group ID")

            if (udata->copy) {
                if ((link_class->copy_func)(udata->lnk->name, grp_id, udata->lnk->u.ud.udata,
                                            udata->lnk->u.ud.size) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_CALLBACK, FAIL, "UD copy callback returned error")
            } /* end if */
            else {
                if ((link_class->move_func)(udata->lnk->name, grp_id, udata->lnk->u.ud.udata,
                                            udata->lnk->u.ud.size) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_CALLBACK, FAIL, "UD move callback returned error")
            } /* end else */
        }     /* end if */
    }         /* end if */

done:
    /* Close the location given to the user callback if it was created */
    if (grp_id >= 0) {
        if (H5I_dec_app_ref(grp_id) < 0)
            HDONE_ERROR(H5E_LINK, H5E_CANTRELEASE, FAIL, "unable to close atom from UD callback")
    } /* end if */
    else if (grp != NULL) {
        if (H5G_close(grp) < 0)
            HDONE_ERROR(H5E_LINK, H5E_CANTRELEASE, FAIL, "unable to close group given to UD callback")
    } /* end if */
    else if (temp_loc_init)
        H5G_loc_free(&temp_loc);

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    /* Reset the "name" field in udata->lnk because it is owned by traverse()
     * and must not be manipulated after traverse closes */
    udata->lnk->name = NULL;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__move_dest_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L__move_cb
 *
 * Purpose:     Callback for moving and renaming an object.  This routine
 *              replaces the names of open objects with the moved object
 *              in the path
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Friday, April 3, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__move_cb(H5G_loc_t *grp_loc /*in*/, const char *name, const H5O_link_t *lnk, H5G_loc_t *obj_loc,
             void *_udata /*in,out*/, H5G_own_loc_t *own_loc /*out*/)
{
    H5L_trav_mv_t *udata = (H5L_trav_mv_t *)_udata; /* User data passed in */
    H5L_trav_mv2_t udata_out;                       /* User data for H5L__move_dest_cb traversal */
    char *         orig_name   = NULL;              /* The name of the link in this group */
    hbool_t        link_copied = FALSE;             /* Has udata_out.lnk been allocated? */
    herr_t         ret_value   = SUCCEED;           /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the name in this group resolved to a valid link */
    if (obj_loc == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "name doesn't exist")

    /* Check for operations on '.' */
    if (lnk == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "the name of a link must be supplied to move or copy")

    /* Set up user data for move_dest_cb */
    if (NULL == (udata_out.lnk = (H5O_link_t *)H5O_msg_copy(H5O_LINK_ID, lnk, NULL)))
        HGOTO_ERROR(H5E_LINK, H5E_CANTCOPY, FAIL, "unable to copy link to be moved")

    /* In this special case, the link's name is going to be replaced at its
     * destination, so we should free it here.
     */
    udata_out.lnk->name = (char *)H5MM_xfree(udata_out.lnk->name);
    link_copied         = TRUE;

    udata_out.lnk->cset = udata->cset;
    udata_out.file      = grp_loc->oloc->file;
    udata_out.copy      = udata->copy;

    /* Keep a copy of link's name (it's "owned" by the H5G_traverse() routine) */
    orig_name = H5MM_xstrdup(name);

    /* Reset the # of soft / UD links that can be traversed, so that the second
     * (destination) traversal has the correct value
     */
    if (H5CX_set_nlinks(udata->orig_nlinks) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTSET, FAIL, "can't reset # of soft / UD links to traverse")

    /* Insert the link into its new location */
    if (H5G_traverse(udata->dst_loc, udata->dst_name, udata->dst_target_flags, H5L__move_dest_cb,
                     &udata_out) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "unable to follow symbolic link")

    /* If this is a move and not a copy operation, change the object's name and remove the old link */
    if (!udata->copy) {
        H5RS_str_t *dst_name_r; /* Ref-counted version of dest name */

        /* Make certain that the destination name is a full (not relative) path */
        if (*(udata->dst_name) != '/') {
            HDassert(udata->dst_loc->path->full_path_r);

            /* Create reference counted string for full dst path */
            if ((dst_name_r = H5G_build_fullpath_refstr_str(udata->dst_loc->path->full_path_r,
                                                            udata->dst_name)) == NULL)
                HGOTO_ERROR(H5E_LINK, H5E_PATH, FAIL, "can't build destination path name")
        } /* end if */
        else
            dst_name_r = H5RS_wrap(udata->dst_name);
        HDassert(dst_name_r);

        /* Fix names up */
        if (H5G_name_replace(lnk, H5G_NAME_MOVE, obj_loc->oloc->file, obj_loc->path->full_path_r,
                             udata->dst_loc->oloc->file, dst_name_r) < 0) {
            H5RS_decr(dst_name_r);
            HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to replace name")
        } /* end if */

        /* Remove the old link */
        if (H5G_obj_remove(grp_loc->oloc, grp_loc->path->full_path_r, orig_name) < 0) {
            H5RS_decr(dst_name_r);
            HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "unable to remove old name")
        } /* end if */

        H5RS_decr(dst_name_r);
    } /* end if */

done:
    /* Cleanup */
    if (orig_name)
        H5MM_xfree(orig_name);

    /* If udata_out.lnk was copied, free any memory allocated
     * In this special case, the H5L__move_dest_cb callback resets the name
     * so H5O_msg_free shouldn't try to free it
     */
    if (link_copied)
        H5O_msg_free(H5O_LINK_ID, udata_out.lnk);

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__move_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L__move
 *
 * Purpose:     Atomically move or copy a link.
 *
 *              Creates a copy of a link in a new destination with a new name.
 *              SRC_LOC and SRC_NAME together define the link's original
 *              location, while DST_LOC and DST_NAME together define its
 *              final location.
 *
 *              If copy_flag is FALSE, the original link is removed
 *              (effectively moving the link).
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Monday, May 1, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L__move(const H5G_loc_t *src_loc, const char *src_name, const H5G_loc_t *dst_loc, const char *dst_name,
          hbool_t copy_flag, hid_t lcpl_id)
{
    unsigned        dst_target_flags = H5G_TARGET_NORMAL;
    H5T_cset_t      char_encoding    = H5F_DEFAULT_CSET; /* Character encoding for link */
    H5P_genplist_t *lc_plist;                            /* Link creation property list */
    H5L_trav_mv_t   udata;                               /* User data for traversal */
    herr_t          ret_value = SUCCEED;                 /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(src_loc);
    HDassert(dst_loc);
    HDassert(src_name && *src_name);
    HDassert(dst_name && *dst_name);

    /* Check for flags present in creation property list */
    if (lcpl_id != H5P_DEFAULT) {
        unsigned crt_intmd_group;

        if (NULL == (lc_plist = (H5P_genplist_t *)H5I_object(lcpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

        /* Get intermediate group creation property */
        if (H5CX_get_intermediate_group(&crt_intmd_group) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get property value for creating missing groups")

        /* Set target flags for source and destination */
        if (crt_intmd_group > 0)
            dst_target_flags |= H5G_CRT_INTMD_GROUP;

        /* Get character encoding property */
        if (H5CX_get_encoding(&char_encoding) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get property value for character encoding")
    } /* end if */

    /* Set up user data */
    udata.dst_loc          = dst_loc;
    udata.dst_name         = dst_name;
    udata.dst_target_flags = dst_target_flags;
    udata.cset             = char_encoding;
    udata.copy             = copy_flag;

    /* Retrieve the original # of soft / UD links that can be traversed, so
     *  that the countdown can be reset after the first path is traversed.
     */
    if (H5CX_get_nlinks(&udata.orig_nlinks) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to retrieve # of soft / UD links to traverse")

    /* Do the move */
    if (H5G_traverse(src_loc, src_name, H5G_TARGET_MOUNT | H5G_TARGET_SLINK | H5G_TARGET_UDLINK, H5L__move_cb,
                     &udata) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "unable to find link")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__move() */

/*-------------------------------------------------------------------------
 * Function:    H5L__exists_final_cb
 *
 * Purpose:     Callback for checking whether a link exists, as the final
 *              component of a path
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, March 16 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__exists_final_cb(H5G_loc_t H5_ATTR_UNUSED *grp_loc /*in*/, const char H5_ATTR_UNUSED *name,
                     const H5O_link_t *lnk, H5G_loc_t H5_ATTR_UNUSED *obj_loc, void *_udata /*in,out*/,
                     H5G_own_loc_t *own_loc /*out*/)
{
    H5L_trav_le_t *udata = (H5L_trav_le_t *)_udata; /* User data passed in */

    FUNC_ENTER_STATIC_NOERR

    /* Check if the name in this group resolved to a valid link */
    udata->exists = (hbool_t)(lnk != NULL);

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5L__exists_final_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L__exists_inter_cb
 *
 * Purpose:     Callback for checking whether a link exists, as an intermediate
 *              component of a path
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, December 31 2015
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__exists_inter_cb(H5G_loc_t H5_ATTR_UNUSED *grp_loc /*in*/, const char H5_ATTR_UNUSED *name,
                     const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata /*in,out*/,
                     H5G_own_loc_t *own_loc /*out*/)
{
    H5L_trav_le_t *udata     = (H5L_trav_le_t *)_udata; /* User data passed in */
    herr_t         ret_value = SUCCEED;                 /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the name in this group resolved to a valid link */
    if (lnk != NULL) {
        /* Check for more components to the path */
        if (udata->sep) {
            H5G_traverse_t cb_func; /* Callback function for tranversal */
            char *         next;    /* Pointer to next component name */

            /* Look for another separator */
            next = udata->sep;
            if (NULL == (udata->sep = HDstrchr(udata->sep, '/')))
                cb_func = H5L__exists_final_cb;
            else {
                /* Chew through adjacent separators, if present */
                do {
                    *udata->sep = '\0';
                    udata->sep++;
                } while ('/' == *udata->sep);
                cb_func = H5L__exists_inter_cb;
            } /* end else */
            if (H5G_traverse(obj_loc, next, H5G_TARGET_SLINK | H5G_TARGET_UDLINK, cb_func, udata) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't determine if link exists")
        } /* end if */
        else
            udata->exists = TRUE;
    } /* end if */
    else
        udata->exists = FALSE;

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__exists_inter_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L_exists_tolerant
 *
 * Purpose:     Returns whether a link exists in a group
 *
 * Note:        Same as H5L__exists, except that missing links are reported
 *              as 'FALSE' instead of causing failures
 *
 * Return:      Non-negative (TRUE/FALSE) on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Thursday, December 31 2015
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5L_exists_tolerant(const H5G_loc_t *loc, const char *name)
{
    H5L_trav_le_t  udata;            /* User data for traversal */
    H5G_traverse_t cb_func;          /* Callback function for tranversal */
    char *         name_copy = NULL; /* Duplicate of name */
    char *         name_trav;        /* Name to traverse */
    htri_t         ret_value = FAIL; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(loc);
    HDassert(name);

    /* Copy the name and skip leading '/'s */
    name_trav = name_copy = H5MM_strdup(name);
    while ('/' == *name_trav)
        name_trav++;

    /* A path of "/" will always exist in a file */
    if ('\0' == *name_trav)
        HGOTO_DONE(TRUE)

    /* Set up user data & correct callback */
    udata.exists = FALSE;
    if (NULL == (udata.sep = HDstrchr(name_trav, '/')))
        cb_func = H5L__exists_final_cb;
    else {
        /* Chew through adjacent separators, if present */
        do {
            *udata.sep = '\0';
            udata.sep++;
        } while ('/' == *udata.sep);
        cb_func = H5L__exists_inter_cb;
    } /* end else */

    /* Traverse the group hierarchy to locate the link to check */
    if (H5G_traverse(loc, name_trav, H5G_TARGET_SLINK | H5G_TARGET_UDLINK, cb_func, &udata) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't determine if link exists")

    /* Set return value */
    ret_value = (htri_t)udata.exists;

done:
    /* Release duplicated string */
    H5MM_xfree(name_copy);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5L_exists_tolerant() */

/*-------------------------------------------------------------------------
 * Function:    H5L__exists
 *
 * Purpose:     Returns whether a link exists in a group
 *
 * Note:        Same as H5L_exists_tolerant, except that missing links are reported
 *              as failures
 *
 * Return:      Non-negative (TRUE/FALSE) on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, March 16 2007
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5L__exists(const H5G_loc_t *loc, const char *name)
{
    H5L_trav_le_t udata;            /* User data for traversal */
    htri_t        ret_value = FAIL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* A path of "/" will always exist in a file */
    if (0 == HDstrcmp(name, "/"))
        HGOTO_DONE(TRUE)

    /* Traverse the group hierarchy to locate the object to get info about */
    udata.exists = FALSE;
    if (H5G_traverse(loc, name, H5G_TARGET_SLINK | H5G_TARGET_UDLINK, H5L__exists_final_cb, &udata) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_EXISTS, FAIL, "path doesn't exist")

    /* Set return value */
    ret_value = (htri_t)udata.exists;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5L__exists() */

/*-------------------------------------------------------------------------
 * Function:    H5L__get_info_cb
 *
 * Purpose:     Callback for retrieving a link's metadata
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  James Laird
 *              Monday, April 17 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__get_info_cb(H5G_loc_t *grp_loc /*in*/, const char H5_ATTR_UNUSED *name, const H5O_link_t *lnk,
                 H5G_loc_t H5_ATTR_UNUSED *obj_loc, void *_udata /*in,out*/, H5G_own_loc_t *own_loc /*out*/)
{
    H5L_trav_gi_t *udata     = (H5L_trav_gi_t *)_udata; /* User data passed in */
    herr_t         ret_value = SUCCEED;                 /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the name in this group resolved to a valid link */
    if (lnk == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "name doesn't exist")

    /* Get information from the link */
    if (H5G_link_to_info(grp_loc->oloc, lnk, udata->linfo) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't get link info")

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__get_info_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L_get_info
 *
 * Purpose:     Returns metadata about a link.
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  James Laird
 *              Monday, April 17 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_get_info(const H5G_loc_t *loc, const char *name, H5L_info2_t *linfo /*out*/)
{
    H5L_trav_gi_t udata;               /* User data for callback */
    herr_t        ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    udata.linfo = linfo;

    /* Traverse the group hierarchy to locate the object to get info about */
    if (H5G_traverse(loc, name, H5G_TARGET_SLINK | H5G_TARGET_UDLINK, H5L__get_info_cb, &udata) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_EXISTS, FAIL, "name doesn't exist")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5L_get_info() */

/*-------------------------------------------------------------------------
 * Function:    H5L__get_info_by_idx_cb
 *
 * Purpose:     Callback for retrieving a link's metadata according to an
 *              index's order.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Monday, November  6 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__get_info_by_idx_cb(H5G_loc_t H5_ATTR_UNUSED *grp_loc /*in*/, const char H5_ATTR_UNUSED *name,
                        const H5O_link_t H5_ATTR_UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata /*in,out*/,
                        H5G_own_loc_t *own_loc /*out*/)
{
    H5L_trav_gibi_t *udata = (H5L_trav_gibi_t *)_udata; /* User data passed in */
    H5O_link_t       fnd_lnk;                           /* Link within group */
    hbool_t          lnk_copied = FALSE;                /* Whether the link was copied */
    herr_t           ret_value  = SUCCEED;              /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the name of the group resolved to a valid object */
    if (obj_loc == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "group doesn't exist")

    /* Query link */
    if (H5G_obj_lookup_by_idx(obj_loc->oloc, udata->idx_type, udata->order, udata->n, &fnd_lnk) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "link not found")
    lnk_copied = TRUE;

    /* Get information from the link */
    if (H5G_link_to_info(obj_loc->oloc, &fnd_lnk, udata->linfo) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't get link info")

done:
    /* Reset the link information, if we have a copy */
    if (lnk_copied)
        H5O_msg_reset(H5O_LINK_ID, &fnd_lnk);

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__get_info_by_idx_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L__get_info_by_idx
 *
 * Purpose:     Internal routine to retrieve link info according to an
 *              index's order.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L__get_info_by_idx(const H5G_loc_t *loc, const char *name, H5_index_t idx_type, H5_iter_order_t order,
                     hsize_t n, H5L_info2_t *linfo /*out*/)
{
    H5L_trav_gibi_t udata;               /* User data for callback */
    herr_t          ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    HDassert(loc);
    HDassert(name && *name);
    HDassert(linfo);

    /* Set up user data for callback */
    udata.idx_type = idx_type;
    udata.order    = order;
    udata.n        = n;
    udata.linfo    = linfo;

    /* Traverse the group hierarchy to locate the object to get info about */
    if (H5G_traverse(loc, name, H5G_TARGET_SLINK | H5G_TARGET_UDLINK, H5L__get_info_by_idx_cb, &udata) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to get link info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__get_info_by_idx() */

/*-------------------------------------------------------------------------
 * Function:    H5L__get_name_by_idx_cb
 *
 * Purpose:     Callback for retrieving a link's name according to an
 *              index's order.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Saturday, November 11 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L__get_name_by_idx_cb(H5G_loc_t H5_ATTR_UNUSED *grp_loc /*in*/, const char H5_ATTR_UNUSED *name,
                        const H5O_link_t H5_ATTR_UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata /*in,out*/,
                        H5G_own_loc_t *own_loc /*out*/)
{
    H5L_trav_gnbi_t *udata     = (H5L_trav_gnbi_t *)_udata; /* User data passed in */
    herr_t           ret_value = SUCCEED;                   /* Return value */

    FUNC_ENTER_STATIC

    /* Check if the name of the group resolved to a valid object */
    if (obj_loc == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "group doesn't exist")

    /* Query link */
    if ((udata->name_len = H5G_obj_get_name_by_idx(obj_loc->oloc, udata->idx_type, udata->order, udata->n,
                                                   udata->name, udata->size)) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTFOUND, FAIL, "link not found")

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__get_name_by_idx_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5L__get_name_by_idx
 *
 * Purpose:     Internal routine to retrieve link name according to an
 *              index's order.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5L__get_name_by_idx(const H5G_loc_t *loc, const char *group_name, H5_index_t idx_type, H5_iter_order_t order,
                     hsize_t n, char *name /*out*/, size_t size)
{
    H5L_trav_gnbi_t udata;            /* User data for callback */
    ssize_t         ret_value = FAIL; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    HDassert(loc);
    HDassert(group_name && *group_name);

    /* Set up user data for callback */
    udata.idx_type = idx_type;
    udata.order    = order;
    udata.n        = n;
    udata.name     = name;
    udata.size     = size;
    udata.name_len = -1;

    /* Traverse the group hierarchy to locate the link to get name of */
    if (H5G_traverse(loc, group_name, H5G_TARGET_SLINK | H5G_TARGET_UDLINK, H5L__get_name_by_idx_cb, &udata) <
        0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't get name")

    /* Set the return value */
    ret_value = udata.name_len;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__get_name_by_idx() */

/*-------------------------------------------------------------------------
 * Function:    H5L__link_copy_file
 *
 * Purpose:     Copy a link and the object it points to from one file to
 *              another.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Sep 29 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L__link_copy_file(H5F_t *dst_file, const H5O_link_t *_src_lnk, const H5O_loc_t *src_oloc,
                    H5O_link_t *dst_lnk, H5O_copy_t *cpy_info)
{
    H5O_link_t        tmp_src_lnk;                   /* Temporary copy of src link, when needed */
    const H5O_link_t *src_lnk            = _src_lnk; /* Source link */
    hbool_t           dst_lnk_init       = FALSE;    /* Whether the destination link is initialized */
    hbool_t           expanded_link_open = FALSE;    /* Whether the target location has been opened */
    H5G_loc_t         tmp_src_loc;                   /* Group location holding target object */
    H5G_name_t        tmp_src_path;                  /* Path for target object */
    H5O_loc_t         tmp_src_oloc;                  /* Object location for target object */
    herr_t            ret_value = SUCCEED;           /* Return value */

    FUNC_ENTER_PACKAGE

    /* check arguments */
    HDassert(dst_file);
    HDassert(src_lnk);
    HDassert(dst_lnk);
    HDassert(cpy_info);

    /* Expand soft or external link, if requested */
    if ((H5L_TYPE_SOFT == src_lnk->type && cpy_info->expand_soft_link) ||
        (H5L_TYPE_EXTERNAL == src_lnk->type && cpy_info->expand_ext_link)) {
        H5G_loc_t  lnk_grp_loc;  /* Group location holding link */
        H5G_name_t lnk_grp_path; /* Path for link */
        htri_t     tar_exists;   /* Whether the target object exists */

        /* Set up group location for link */
        H5G_name_reset(&lnk_grp_path);
        lnk_grp_loc.path = &lnk_grp_path;
        lnk_grp_loc.oloc = (H5O_loc_t *)src_oloc; /* Casting away const OK -QAK */

        /* Check if the target object exists */
        if ((tar_exists = H5G_loc_exists(&lnk_grp_loc, src_lnk->name)) < 0)
            HGOTO_ERROR(H5E_LINK, H5E_CANTCOPY, FAIL, "unable to check if target object exists")

        if (tar_exists) {
            /* Make a temporary copy of the link, so that it will not change the
             * info in the cache when we change it to a hard link */
            if (NULL == H5O_msg_copy(H5O_LINK_ID, src_lnk, &tmp_src_lnk))
                HGOTO_ERROR(H5E_LINK, H5E_CANTCOPY, FAIL, "unable to copy message")

            /* Set up group location for target object.  Let H5G_traverse expand
             * the link. */
            tmp_src_loc.path = &tmp_src_path;
            tmp_src_loc.oloc = &tmp_src_oloc;
            if (H5G_loc_reset(&tmp_src_loc) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTCOPY, FAIL, "unable to reset location")

            /* Find the target object */
            if (H5G_loc_find(&lnk_grp_loc, src_lnk->name, &tmp_src_loc) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CANTCOPY, FAIL, "unable to find target object")
            expanded_link_open = TRUE;

            /* Convert symbolic link to hard link */
            if (tmp_src_lnk.type == H5L_TYPE_SOFT)
                tmp_src_lnk.u.soft.name = (char *)H5MM_xfree(tmp_src_lnk.u.soft.name);
            else if (tmp_src_lnk.u.ud.size > 0)
                tmp_src_lnk.u.ud.udata = H5MM_xfree(tmp_src_lnk.u.ud.udata);
            tmp_src_lnk.type        = H5L_TYPE_HARD;
            tmp_src_lnk.u.hard.addr = tmp_src_oloc.addr;
            src_lnk                 = &tmp_src_lnk;
        } /* end if */
    }     /* end if */

    /* Copy src link information to dst link information */
    if (NULL == H5O_msg_copy(H5O_LINK_ID, src_lnk, dst_lnk))
        HGOTO_ERROR(H5E_LINK, H5E_CANTCOPY, FAIL, "unable to copy message")
    dst_lnk_init = TRUE;

    /* Check if object in source group is a hard link & copy it */
    if (H5L_TYPE_HARD == src_lnk->type) {
        H5O_loc_t new_dst_oloc; /* Copied object location in destination */

        /* Set up copied object location to fill in */
        H5O_loc_reset(&new_dst_oloc);
        new_dst_oloc.file = dst_file;

        if (!expanded_link_open) {
            /* Build temporary object location for source */
            H5O_loc_reset(&tmp_src_oloc);
            tmp_src_oloc.file = src_oloc->file;
            tmp_src_oloc.addr = src_lnk->u.hard.addr;
        } /* end if */
        HDassert(H5F_addr_defined(tmp_src_oloc.addr));

        /* Copy the shared object from source to destination */
        /* Don't care about obj_type or udata because those are only important
         * for old style groups */
        if (H5O_copy_header_map(&tmp_src_oloc, &new_dst_oloc, cpy_info, TRUE, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_LINK, H5E_CANTCOPY, FAIL, "unable to copy object")

        /* Copy new destination object's information for eventual insertion */
        dst_lnk->u.hard.addr = new_dst_oloc.addr;
    } /* end if */

done:
    /* Check if we used a temporary src link */
    if (src_lnk != _src_lnk) {
        HDassert(src_lnk == &tmp_src_lnk);
        H5O_msg_reset(H5O_LINK_ID, &tmp_src_lnk);
    } /* end if */
    if (ret_value < 0)
        if (dst_lnk_init)
            H5O_msg_reset(H5O_LINK_ID, dst_lnk);
    /* Check if we need to free the temp source oloc */
    if (expanded_link_open)
        if (H5G_loc_free(&tmp_src_loc) < 0)
            HDONE_ERROR(H5E_LINK, H5E_CANTFREE, FAIL, "unable to free object")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L__link_copy_file() */

/*-------------------------------------------------------------------------
 * Function:    H5L_iterate
 *
 * Purpose:     Iterates through links in a group
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_iterate(H5G_loc_t *loc, const char *group_name, H5_index_t idx_type, H5_iter_order_t order,
            hsize_t *idx_p, H5L_iterate2_t op, void *op_data)
{
    H5G_link_iterate_t lnk_op;           /* Link operator                    */
    hsize_t            last_lnk;         /* Index of last object looked at   */
    hsize_t            idx;              /* Internal location to hold index  */
    herr_t             ret_value = FAIL; /* Return value                     */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity checks */
    HDassert(loc);
    HDassert(group_name);
    HDassert(op);

    /* Set up iteration beginning/end info */
    idx      = (idx_p == NULL ? 0 : *idx_p);
    last_lnk = 0;

    /* Build link operator info */
    lnk_op.op_type        = H5G_LINK_OP_NEW;
    lnk_op.op_func.op_new = op;

    /* Iterate over the links */
    if ((ret_value = H5G_iterate(loc, group_name, idx_type, order, idx, &last_lnk, &lnk_op, op_data)) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_BADITER, FAIL, "link iteration failed")

    /* Set the index we stopped at */
    if (idx_p)
        *idx_p = last_lnk;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_iterate() */
