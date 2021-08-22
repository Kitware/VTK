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

#include "H5Mmodule.h" /* This source code file is part of the H5M module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                        */
#include "H5CXprivate.h" /* API Contexts                             */
#include "H5Mpkg.h"      /* Maps                                     */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5VLprivate.h" /* Virtual Object Layer                     */

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Local Prototypes */
/********************/
static herr_t H5M__close_cb(H5VL_object_t *map_vol_obj);

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

/* Map ID class */
static const H5I_class_t H5I_MAP_CLS[1] = {{
    H5I_MAP,                  /* ID class value */
    0,                        /* Class flags */
    0,                        /* # of reserved IDs for class */
    (H5I_free_t)H5M__close_cb /* Callback routine for closing objects of this class */
}};

/* Flag indicating "top" of interface has been initialized */
static hbool_t H5M_top_package_initialize_s = FALSE;

/*-------------------------------------------------------------------------
 * Function: H5M_init
 *
 * Purpose:  Initialize the interface from some other layer.
 *
 * Return:   Success:    non-negative
 *
 *           Failure:    negative
 *-------------------------------------------------------------------------
 */
herr_t
H5M_init(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5M_init() */

/*-------------------------------------------------------------------------
NAME
    H5M__init_package -- Initialize interface-specific information
USAGE
    herr_t H5M__init_package()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.
---------------------------------------------------------------------------
*/
herr_t
H5M__init_package(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Initialize the atom group for the map IDs */
    if (H5I_register_type(H5I_MAP_CLS) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTINIT, FAIL, "unable to initialize interface")

    /* Mark "top" of interface as initialized, too */
    H5M_top_package_initialize_s = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5M__init_package() */

/*-------------------------------------------------------------------------
 * Function: H5M_top_term_package
 *
 * Purpose:  Close the "top" of the interface, releasing IDs, etc.
 *
 * Return:   Success:    Positive if anything was done that might
 *                affect other interfaces; zero otherwise.
 *           Failure:    Negative.
 *-------------------------------------------------------------------------
 */
int
H5M_top_term_package(void)
{
    int n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if (H5M_top_package_initialize_s) {
        if (H5I_nmembers(H5I_MAP) > 0) {
            (void)H5I_clear_type(H5I_MAP, FALSE, FALSE);
            n++; /*H5I*/
        }        /* end if */

        /* Mark closed */
        if (0 == n)
            H5M_top_package_initialize_s = FALSE;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5M_top_term_package() */

/*-------------------------------------------------------------------------
 * Function: H5M_term_package
 *
 * Purpose:  Terminate this interface.
 *
 * Note:     Finishes shutting down the interface, after
 *           H5M_top_term_package() is called
 *
 * Return:   Success:    Positive if anything was done that might
 *                affect other interfaces; zero otherwise.
 *            Failure:    Negative.
 *-------------------------------------------------------------------------
 */
int
H5M_term_package(void)
{
    int n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if (H5_PKG_INIT_VAR) {
        /* Sanity checks */
        HDassert(0 == H5I_nmembers(H5I_MAP));
        HDassert(FALSE == H5M_top_package_initialize_s);

        /* Destroy the dataset object id group */
        n += (H5I_dec_type_ref(H5I_MAP) > 0);

        /* Mark closed */
        if (0 == n)
            H5_PKG_INIT_VAR = FALSE;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5M_term_package() */

/*-------------------------------------------------------------------------
 * Function:    H5M__close_cb
 *
 * Purpose:     Called when the ref count reaches zero on the map's ID
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5M__close_cb(H5VL_object_t *map_vol_obj)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(map_vol_obj);

    /* Close the map */
    if (H5VL_optional(map_vol_obj, H5VL_MAP_CLOSE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CLOSEERROR, FAIL, "unable to close map");

    /* Free the VOL object */
    if (H5VL_free_object(map_vol_obj) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTDEC, FAIL, "unable to free VOL object");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5M__close_cb() */

#ifdef H5_HAVE_MAP_API

/*-------------------------------------------------------------------------
 * Function:    H5Mcreate
 *
 * Purpose:     Creates a new map object for storing key-value pairs.  The
 *              in-file datatype for keys is defined by KEY_TYPE_ID and
 *              the in-file datatype for values is defined by VAL_TYPE_ID.
 *              LOC_ID specifies the location to create the map object and
 *              NAME specifies the name of the link to the object
 *              (relative to LOC_ID).  Other options can be specified
 *              through the property lists LCPL_ID, MCPL_ID, and MAPL_ID.
 *
 * Return:      Success:    The object ID of the new map.
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Mcreate(hid_t loc_id, const char *name, hid_t key_type_id, hid_t val_type_id, hid_t lcpl_id, hid_t mcpl_id,
          hid_t mapl_id)
{
    void *            map     = NULL; /* New map's info */
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    hid_t             ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE7("i", "i*siiiii", loc_id, name, key_type_id, val_type_id, lcpl_id, mcpl_id, mapl_id);

    /* Check arguments */
    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be NULL")
    if (!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be an empty string")

    /* Get link creation property list */
    if (H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;
    else if (TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "lcpl_id is not a link creation property list")

    /* Get map creation property list */
    if (H5P_DEFAULT == mcpl_id)
        mcpl_id = H5P_MAP_CREATE_DEFAULT;
    else if (TRUE != H5P_isa_class(mcpl_id, H5P_MAP_CREATE))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "mcpl_id is not a map create property list ID")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&mapl_id, H5P_CLS_MACC, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Set location parameters */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* Create the map */
    if (H5VL_optional(vol_obj, H5VL_MAP_CREATE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, &loc_params, name,
                      lcpl_id, key_type_id, val_type_id, mcpl_id, mapl_id, &map) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTINIT, H5I_INVALID_HID, "unable to create map")

    /* Get an atom for the map */
    if ((ret_value = H5VL_register(H5I_MAP, map, vol_obj->connector, TRUE)) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to atomize map handle")

done:
    /* Cleanup on failure */
    if (H5I_INVALID_HID == ret_value)
        if (map && H5VL_optional(vol_obj, H5VL_MAP_CLOSE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, H5I_INVALID_HID, "unable to release map")

    FUNC_LEAVE_API(ret_value)
} /* end H5Mcreate() */

/*-------------------------------------------------------------------------
 * Function:    H5Mcreate_anon
 *
 * Purpose:     Creates a new map object for storing key-value pairs.  The
 *              in-file datatype for keys is defined by KEY_TYPE_ID and
 *              the in-file datatype for values is defined by VAL_TYPE_ID.
 *              LOC_ID specifies the file to create the map object, but no
 *              link to the object is created.  Other options can be
 *              specified through the property lists LCPL_ID, MCPL_ID, and
 *              MAPL_ID.
 *
 *              The resulting ID should be linked into the file with
 *              H5Olink or it will be deleted when closed.
 *
 * Return:      Success:    The object ID of the new map. The map should
 *                          be linked into the group hierarchy before being closed or
 *                          it will be deleted. The dataset should be
 *                          closed when the caller is no longer interested
 *                          in it.
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Mcreate_anon(hid_t loc_id, hid_t key_type_id, hid_t val_type_id, hid_t mcpl_id, hid_t mapl_id)
{
    void *            map     = NULL; /* map object from VOL connector */
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    hid_t             ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE5("i", "iiiii", loc_id, key_type_id, val_type_id, mcpl_id, mapl_id);

    /* Check arguments */
    if (H5P_DEFAULT == mcpl_id)
        mcpl_id = H5P_MAP_CREATE_DEFAULT;
    else if (TRUE != H5P_isa_class(mcpl_id, H5P_MAP_CREATE))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not map create property list ID")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&mapl_id, H5P_CLS_MACC, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Set location parameters */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* Create the map */
    if (H5VL_optional(vol_obj, H5VL_MAP_CREATE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, &loc_params, NULL,
                      H5P_LINK_CREATE_DEFAULT, key_type_id, val_type_id, mcpl_id, mapl_id, &map) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTINIT, H5I_INVALID_HID, "unable to create map")

    /* Get an atom for the map */
    if ((ret_value = H5VL_register(H5I_MAP, map, vol_obj->connector, TRUE)) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register map")

done:
    /* Cleanup on failure */
    if (H5I_INVALID_HID == ret_value)
        if (map && H5VL_optional(vol_obj, H5VL_MAP_CLOSE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, H5I_INVALID_HID, "unable to release map")

    FUNC_LEAVE_API(ret_value)
} /* end H5Mcreate_anon() */

/*------------------------------------------------------------------------
 * Function:    H5Mopen
 *
 * Purpose:     Finds a map named NAME at LOC_ID, opens it, and returns
 *              its ID. The map should be close when the caller is no
 *              longer interested in it.
 *
 *              Takes a map access property list
 *
 * Return:      Success:    Object ID of the map
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Mopen(hid_t loc_id, const char *name, hid_t mapl_id)
{
    void *            map     = NULL; /* map object from VOL connector */
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    hid_t             ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE3("i", "i*si", loc_id, name, mapl_id);

    /* Check args */
    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be NULL")
    if (!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be an empty string")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&mapl_id, H5P_CLS_MACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Set the location parameters */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* Open the map */
    if (H5VL_optional(vol_obj, H5VL_MAP_OPEN, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, &loc_params, name,
                      mapl_id, &map) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTOPENOBJ, H5I_INVALID_HID, "unable to open map")

    /* Register an atom for the map */
    if ((ret_value = H5VL_register(H5I_MAP, map, vol_obj->connector, TRUE)) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTREGISTER, H5I_INVALID_HID, "can't register map atom")

done:
    /* Cleanup on failure */
    if (H5I_INVALID_HID == ret_value)
        if (map && H5VL_optional(vol_obj, H5VL_MAP_CLOSE, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, H5I_INVALID_HID, "unable to release map")

    FUNC_LEAVE_API(ret_value)
} /* end H5Mopen() */

/*-------------------------------------------------------------------------
 * Function:    H5Mclose
 *
 * Purpose:     Closes access to a map and releases resources used by it.
 *              It is illegal to subsequently use that same map ID in
 *              calls to other map functions.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Mclose(hid_t map_id)
{
    herr_t ret_value = SUCCEED; /* Return value                     */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", map_id);

    /* Check args */
    if (H5I_MAP != H5I_get_type(map_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a map ID")

    /* Decrement the counter on the map.  It will be freed if the count
     * reaches zero.
     */
    if (H5I_dec_app_ref_always_close(map_id) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTDEC, FAIL, "can't decrement count on map ID")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Mclose() */

/*-------------------------------------------------------------------------
 * Function:    H5Mget_key_type
 *
 * Purpose:     Returns a copy of the key datatype for a map.
 *
 * Return:      Success:    ID for a copy of the datatype. The data
 *                          type should be released by calling
 *                          H5Tclose().
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Mget_key_type(hid_t map_id)
{
    H5VL_object_t *vol_obj;                     /* Map structure    */
    hid_t          ret_value = H5I_INVALID_HID; /* Return value         */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE1("i", "i", map_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(map_id, H5I_MAP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid map identifier")

    /* get the datatype */
    if (H5VL_optional(vol_obj, H5VL_MAP_GET, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, H5VL_MAP_GET_KEY_TYPE,
                      &ret_value) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTGET, H5I_INVALID_HID, "unable to get datatype")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Mget_key_type() */

/*-------------------------------------------------------------------------
 * Function:    H5Mget_val_type
 *
 * Purpose:     Returns a copy of the value datatype for a map.
 *
 * Return:      Success:    ID for a copy of the datatype. The data
 *                          type should be released by calling
 *                          H5Tclose().
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Mget_val_type(hid_t map_id)
{
    H5VL_object_t *vol_obj;                     /* Map structure    */
    hid_t          ret_value = H5I_INVALID_HID; /* Return value         */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE1("i", "i", map_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(map_id, H5I_MAP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid map identifier")

    /* get the datatype */
    if (H5VL_optional(vol_obj, H5VL_MAP_GET, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, H5VL_MAP_GET_VAL_TYPE,
                      &ret_value) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTGET, H5I_INVALID_HID, "unable to get datatype")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Mget_val_type() */

/*-------------------------------------------------------------------------
 * Function:    H5Mget_create_plist
 *
 * Purpose:     Returns a copy of the map creation property list.
 *
 * Return:      Success:    ID for a copy of the map creation
 *                          property list.  The template should be
 *                          released by calling H5P_close().
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Mget_create_plist(hid_t map_id)
{
    H5VL_object_t *vol_obj;                     /* Map structure    */
    hid_t          ret_value = H5I_INVALID_HID; /* Return value         */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE1("i", "i", map_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(map_id, H5I_MAP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid map identifier")

    /* Get the map creation property list */
    if (H5VL_optional(vol_obj, H5VL_MAP_GET, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, H5VL_MAP_GET_MCPL,
                      &ret_value) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTGET, H5I_INVALID_HID, "unable to get map creation properties")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Mget_create_plist() */

/*-------------------------------------------------------------------------
 * Function:    H5Mget_access_plist
 *
 * Purpose:     Returns a copy of the map access property list.
 *
 * Description: H5Mget_access_plist returns the map access property
 *              list identifier of the specified map.
 *
 * Return:      Success:    ID for a copy of the map access
 *                          property list.  The template should be
 *                          released by calling H5Pclose().
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Mget_access_plist(hid_t map_id)
{
    H5VL_object_t *vol_obj;                     /* Map structure    */
    hid_t          ret_value = H5I_INVALID_HID; /* Return value         */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE1("i", "i", map_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(map_id, H5I_MAP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid map identifier")

    /* Get the map access property list */
    if (H5VL_optional(vol_obj, H5VL_MAP_GET, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, H5VL_MAP_GET_MAPL,
                      &ret_value) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTGET, H5I_INVALID_HID, "unable to get map access properties")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Mget_access_plist() */

/*-------------------------------------------------------------------------
 * Function:    H5Mget_count
 *
 * Purpose:     Returns the number of key-value pairs stored in the map.
 *
 * Description: H5Mget_count returns the number of key-value pairs stored
 *              in the specified map.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Mget_count(hid_t map_id, hsize_t *count, hid_t dxpl_id)
{
    H5VL_object_t *vol_obj;             /* Map structure    */
    herr_t         ret_value = SUCCEED; /* Return value         */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE3("e", "i*hi", map_id, count, dxpl_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(map_id, H5I_MAP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid map identifier")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Get the number of key-value pairs stored in the map */
    if (H5VL_optional(vol_obj, H5VL_MAP_GET, dxpl_id, H5_REQUEST_NULL, H5VL_MAP_GET_COUNT, count) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTGET, H5I_INVALID_HID, "unable to get map access properties")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Mget_count() */

/*-------------------------------------------------------------------------
 * Function:    H5Mput
 *
 * Purpose:     H5Mput adds a key-value pair to the Map specified by
 *              MAP_ID, or updates the value for the specified key if one
 *              was set previously. KEY_MEM_TYPE_ID and VAL_MEM_TYPE_ID
 *              specify the datatypes for the provided KEY and VALUE
 *              buffers, and if different from those used to create the
 *              map object, the key and value will be internally converted
 *              to the datatypes for the map object. Any further options
 *              can be specified through the property list DXPL_ID.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Mput(hid_t map_id, hid_t key_mem_type_id, const void *key, hid_t val_mem_type_id, const void *value,
       hid_t dxpl_id)
{
    H5VL_object_t *vol_obj   = NULL;
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "ii*xi*xi", map_id, key_mem_type_id, key, val_mem_type_id, value, dxpl_id);

    /* Check arguments */
    if (key_mem_type_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid key memory datatype ID")
    if (val_mem_type_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid value memory datatype ID")

    /* Get map pointer */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(map_id, H5I_MAP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "map_id is not a map ID")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Set the key/value pair */
    if (H5VL_optional(vol_obj, H5VL_MAP_PUT, dxpl_id, H5_REQUEST_NULL, key_mem_type_id, key, val_mem_type_id,
                      value) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTSET, FAIL, "unable to put key/value pair")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Mput() */

/*-------------------------------------------------------------------------
 * Function:    H5Mget
 *
 * Purpose:     H5Mget retrieves, from the Map specified by MAP_ID, the
 *              value associated with the provided key.  KEY_MEM_TYPE_ID
 *              and VAL_MEM_TYPE_ID specify the datatypes for the provided
 *              KEY and VALUE buffers. If KEY_MEM_TYPE_ID is different
 *              from that used to create the map object, the key will be
 *              internally converted to the datatype for the map object
 *              for the query, and if VAL_MEM_TYPE_ID is different from
 *              that used to create the map object, the returned value
 *              will be converted to VAL_MEM_TYPE_ID before the function
 *              returns. Any further options can be specified through the
 *              property list DXPL_ID.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Mget(hid_t map_id, hid_t key_mem_type_id, const void *key, hid_t val_mem_type_id, void *value,
       hid_t dxpl_id)
{
    H5VL_object_t *vol_obj   = NULL;
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "ii*xi*xi", map_id, key_mem_type_id, key, val_mem_type_id, value, dxpl_id);

    /* Check arguments */
    if (key_mem_type_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid key memory datatype ID")
    if (val_mem_type_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid value memory datatype ID")

    /* Get map pointer */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(map_id, H5I_MAP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "map_id is not a map ID")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Get the value for the key */
    if (H5VL_optional(vol_obj, H5VL_MAP_GET_VAL, dxpl_id, H5_REQUEST_NULL, key_mem_type_id, key,
                      val_mem_type_id, value) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTGET, FAIL, "unable to get value from map")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Mget() */

/*-------------------------------------------------------------------------
 * Function:    H5Mexists
 *
 * Purpose:     H5Mexists checks if the provided key is stored in the map
 *              specified by MAP_ID. If KEY_MEM_TYPE_ID is different from
 *              that used to create the map object the key will be
 *              internally converted to the datatype for the map object
 *              for the query.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Mexists(hid_t map_id, hid_t key_mem_type_id, const void *key, hbool_t *exists, hid_t dxpl_id)
{
    H5VL_object_t *vol_obj   = NULL;
    herr_t         ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "ii*x*bi", map_id, key_mem_type_id, key, exists, dxpl_id);

    /* Check arguments */
    if (key_mem_type_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid key memory datatype ID")

    /* Get map pointer */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(map_id, H5I_MAP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "map_id is not a map ID")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Check if key exists */
    if ((ret_value = H5VL_optional(vol_obj, H5VL_MAP_EXISTS, dxpl_id, H5_REQUEST_NULL, key_mem_type_id, key,
                                   exists)) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTGET, ret_value, "unable to check if key exists")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Mexists() */

/*-------------------------------------------------------------------------
 * Function:    H5Miterate
 *
 * Purpose:     H5Miterate iterates over all key-value pairs stored in the
 *              map specified by MAP_ID, making the callback specified by
 *              OP for each. The IDX parameter is an in/out parameter that
 *              may be used to restart a previously interrupted iteration.
 *              At the start of iteration IDX should be set to 0, and to
 *              restart iteration at the same location on a subsequent
 *              call to H5Miterate, IDX should be the same value as
 *              returned by the previous call.
 *
 *              H5M_iterate_t is defined as:
 *              herr_t (*H5M_iterate_t)(hid_t map_id, const void *key,
 *                      void *ctx)
 *
 *              The KEY parameter is the buffer for the key for this
 *              iteration, converted to the datatype specified by
 *              KEY_MEM_TYPE_ID. The OP_DATA parameter is a simple pass
 *              through of the value passed to H5Miterate, which can be
 *              used to store application-defined data for iteration. A
 *              negative return value from this function will cause
 *              H5Miterate to issue an error, while a positive return
 *              value will cause H5Miterate to stop iterating and return
 *              this value without issuing an error. A return value of
 *              zero allows iteration to continue.
 *
 * Return:      Last value returned by op
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Miterate(hid_t map_id, hsize_t *idx, hid_t key_mem_type_id, H5M_iterate_t op, void *op_data, hid_t dxpl_id)
{
    H5VL_object_t *   vol_obj = NULL;
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "i*hix*xi", map_id, idx, key_mem_type_id, op, op_data, dxpl_id);

    /* Check arguments */
    if (key_mem_type_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid key memory datatype ID")
    if (!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no operator specified")

    /* Get map pointer */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(map_id, H5I_MAP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "map_id is not a map ID")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Set location struct fields */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(map_id);

    /* Iterate over keys */
    if ((ret_value = H5VL_optional(vol_obj, H5VL_MAP_SPECIFIC, dxpl_id, H5_REQUEST_NULL, &loc_params,
                                   H5VL_MAP_ITER, idx, key_mem_type_id, op, op_data)) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_BADITER, ret_value, "unable to iterate over keys")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Miterate() */

/*-------------------------------------------------------------------------
 * Function:    H5Miterate_by_name
 *
 * Purpose:     H5Miterate_by_name iterates over all key-value pairs
 *              stored in the map specified by MAP_ID, making the callback
 *              specified by OP for each. The IDX parameter is an in/out
 *              parameter that may be used to restart a previously
 *              interrupted iteration.  At the start of iteration IDX
 *              should be set to 0, and to restart iteration at the same
 *              location on a subsequent call to H5Miterate, IDX should be
 *              the same value as returned by the previous call.
 *
 *              H5M_iterate_t is defined as:
 *              herr_t (*H5M_iterate_t)(hid_t map_id, const void *key,
 *                      void *ctx)
 *
 *              The KEY parameter is the buffer for the key for this
 *              iteration, converted to the datatype specified by
 *              KEY_MEM_TYPE_ID. The OP_DATA parameter is a simple pass
 *              through of the value passed to H5Miterate, which can be
 *              used to store application-defined data for iteration. A
 *              negative return value from this function will cause
 *              H5Miterate to issue an error, while a positive return
 *              value will cause H5Miterate to stop iterating and return
 *              this value without issuing an error. A return value of
 *              zero allows iteration to continue.
 *
 * Return:      Last value returned by op
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Miterate_by_name(hid_t loc_id, const char *map_name, hsize_t *idx, hid_t key_mem_type_id, H5M_iterate_t op,
                   void *op_data, hid_t dxpl_id, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj = NULL;
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE8("e", "i*s*hix*xii", loc_id, map_name, idx, key_mem_type_id, op, op_data, dxpl_id, lapl_id);

    /* Check arguments */
    if (!map_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "map_name parameter cannot be NULL")
    if (!*map_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "map_name parameter cannot be an empty string")
    if (key_mem_type_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid key memory datatype ID")
    if (!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no operator specified")

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Set location struct fields */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.obj_type                     = H5I_get_type(loc_id);
    loc_params.loc_data.loc_by_name.name    = map_name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;

    /* Iterate over keys */
    if ((ret_value = H5VL_optional(vol_obj, H5VL_MAP_SPECIFIC, dxpl_id, H5_REQUEST_NULL, &loc_params,
                                   H5VL_MAP_ITER, idx, key_mem_type_id, op, op_data)) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_BADITER, ret_value, "unable to ierate over keys")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Miterate_by_name() */

/*-------------------------------------------------------------------------
 * Function:    H5Mdelete
 *
 * Purpose:     H5Mdelete deletes a key-value pair from the Map
 *              specified by MAP_ID. KEY_MEM_TYPE_ID specifies the
 *              datatype for the provided key buffers, and if different
 *              from that used to create the Map object, the key will be
 *              internally converted to the datatype for the map object.
 *              Any further options can be specified through the property
 *              list DXPL_ID.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Mdelete(hid_t map_id, hid_t key_mem_type_id, const void *key, hid_t dxpl_id)
{
    H5VL_object_t *   vol_obj = NULL;
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "ii*xi", map_id, key_mem_type_id, key, dxpl_id);

    /* Check arguments */
    if (key_mem_type_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid key memory datatype ID")

    /* Get map pointer */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(map_id, H5I_MAP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "map_id is not a map ID")

    /* Get the default dataset transfer property list if the user didn't provide one */
    if (H5P_DEFAULT == dxpl_id)
        dxpl_id = H5P_DATASET_XFER_DEFAULT;
    else if (TRUE != H5P_isa_class(dxpl_id, H5P_DATASET_XFER))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not xfer parms")

    /* Set DXPL for operation */
    H5CX_set_dxpl(dxpl_id);

    /* Set location struct fields */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(map_id);

    /* Delete the key/value pair */
    if (H5VL_optional(vol_obj, H5VL_MAP_SPECIFIC, dxpl_id, H5_REQUEST_NULL, &loc_params, H5VL_MAP_DELETE,
                      key_mem_type_id, key) < 0)
        HGOTO_ERROR(H5E_MAP, H5E_CANTSET, FAIL, "unable to delete key/value pair")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Mdelete() */

#endif /*  H5_HAVE_MAP_API */
