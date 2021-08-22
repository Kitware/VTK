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

/*-------------------------------------------------------------------------
 *
 * Created:	H5G.c
 *
 * Purpose:	Symbol table functions.	 The functions that begin with
 *		'H5G_stab_' don't understand the naming system; they operate
 * 		on a single symbol table at a time.
 *
 *		The functions that begin with 'H5G_node_' operate on the leaf
 *		nodes of a symbol table B-tree.  They should be defined in
 *		the H5Gnode.c file.
 *
 *		The remaining functions know how to traverse the group
 *		directed graph.
 *
 * Names:	Object names are a slash-separated list of components.  If
 *		the name begins with a slash then it's absolute, otherwise
 *		it's relative ("/foo/bar" is absolute while "foo/bar" is
 *		relative).  Multiple consecutive slashes are treated as
 *		single slashes and trailing slashes are ignored.  The special
 *		case `/' is the root group.  Every file has a root group.
 *
 *		API functions that look up names take a location ID and a
 *		name.  The location ID can be a file ID or a group ID and the
 *		name can be relative or absolute.
 *
 *              +--------------+----------- +--------------------------------+
 * 		| Location ID  | Name       | Meaning                        |
 *              +--------------+------------+--------------------------------+
 * 		| File ID      | "/foo/bar" | Find 'foo' within 'bar' within |
 *		|              |            | the root group of the specified|
 *		|              |            | file.                          |
 *              +--------------+------------+--------------------------------+
 * 		| File ID      | "foo/bar"  | Find 'foo' within 'bar' within |
 *		|              |            | the root group of the specified|
 *		|              |            | file.                          |
 *              +--------------+------------+--------------------------------+
 * 		| File ID      | "/"        | The root group of the specified|
 *		|              |            | file.                          |
 *              +--------------+------------+--------------------------------+
 * 		| File ID      | "."        | The root group of the specified|
 *		|              |            | the specified file.            |
 *              +--------------+------------+--------------------------------+
 * 		| Group ID     | "/foo/bar" | Find 'foo' within 'bar' within |
 *		|              |            | the root group of the file     |
 *		|              |            | containing the specified group.|
 *              +--------------+------------+--------------------------------+
 * 		| Group ID     | "foo/bar"  | File 'foo' within 'bar' within |
 *		|              |            | the specified group.           |
 *              +--------------+------------+--------------------------------+
 * 		| Group ID     | "/"        | The root group of the file     |
 *		|              |            | containing the specified group.|
 *              +--------------+------------+--------------------------------+
 * 		| Group ID     | "."        | The specified group.           |
 *              +--------------+------------+--------------------------------+
 *
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Gmodule.h" /* This source code file is part of the H5G module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                        */
#include "H5CXprivate.h" /* API Contexts                             */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5Gpkg.h"      /* Groups                                   */
#include "H5Iprivate.h"  /* IDs                                      */
#include "H5Pprivate.h"  /* Property lists                           */
#include "H5VLprivate.h" /* Virtual Object Layer                     */

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

/* Group close callback */
static herr_t H5G__close_cb(H5VL_object_t *grp_vol_obj);

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

/* Group ID class */
static const H5I_class_t H5I_GROUP_CLS[1] = {{
    H5I_GROUP,                /* ID class value */
    0,                        /* Class flags */
    0,                        /* # of reserved IDs for class */
    (H5I_free_t)H5G__close_cb /* Callback routine for closing objects of this class */
}};

/* Flag indicating "top" of interface has been initialized */
static hbool_t H5G_top_package_initialize_s = FALSE;

/*-------------------------------------------------------------------------
 * Function: H5G_init
 *
 * Purpose:  Initialize the interface from some other layer.
 *
 * Return:   Success:    non-negative
 *
 *           Failure:    negative
 *-------------------------------------------------------------------------
 */
herr_t
H5G_init(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_init() */

/*-------------------------------------------------------------------------
 * Function:	H5G__init_package
 *
 * Purpose:	Initializes the H5G interface.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Monday, January	 5, 1998
 *
 * Notes:       The group creation properties are registered in the property
 *              list interface initialization routine (H5P_init_package)
 *              so that the file creation property class can inherit from it
 *              correctly. (Which allows the file creation property list to
 *              control the group creation properties of the root group of
 *              a file) QAK - 24/10/2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G__init_package(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Initialize the ID group for the group IDs */
    if (H5I_register_type(H5I_GROUP_CLS) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to initialize interface")

    /* Mark "top" of interface as initialized, too */
    H5G_top_package_initialize_s = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G__init_package() */

/*-------------------------------------------------------------------------
 * Function:	H5G_top_term_package
 *
 * Purpose:	Close the "top" of the interface, releasing IDs, etc.
 *
 * Return:	Success:	Positive if anything is done that might
 *				affect other interfaces; zero otherwise.
 * 		Failure:	Negative.
 *
 * Programmer:	Quincey Koziol
 *		Sunday, September	13, 2015
 *
 *-------------------------------------------------------------------------
 */
int
H5G_top_term_package(void)
{
    int n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if (H5G_top_package_initialize_s) {
        if (H5I_nmembers(H5I_GROUP) > 0) {
            (void)H5I_clear_type(H5I_GROUP, FALSE, FALSE);
            n++; /*H5I*/
        }        /* end if */

        /* Mark closed */
        if (0 == n)
            H5G_top_package_initialize_s = FALSE;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5G_top_term_package() */

/*-------------------------------------------------------------------------
 * Function:	H5G_term_package
 *
 * Purpose:	Terminates the H5G interface
 *
 * Note:	Finishes shutting down the interface, after
 *		H5G_top_term_package() is called
 *
 * Return:	Success:	Positive if anything is done that might
 *				affect other interfaces; zero otherwise.
 * 		Failure:	Negative.
 *
 * Programmer:	Robb Matzke
 *		Monday, January	 5, 1998
 *
 *-------------------------------------------------------------------------
 */
int
H5G_term_package(void)
{
    int n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if (H5_PKG_INIT_VAR) {
        /* Sanity checks */
        HDassert(0 == H5I_nmembers(H5I_GROUP));
        HDassert(FALSE == H5G_top_package_initialize_s);

        /* Destroy the group object id group */
        n += (H5I_dec_type_ref(H5I_GROUP) > 0);

        /* Mark closed */
        if (0 == n)
            H5_PKG_INIT_VAR = FALSE;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5G_term_package() */

/*-------------------------------------------------------------------------
 * Function:    H5G__close_cb
 *
 * Purpose:     Called when the ref count reaches zero on the group's ID
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G__close_cb(H5VL_object_t *grp_vol_obj)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity check */
    HDassert(grp_vol_obj);

    /* Close the group */
    if (H5VL_group_close(grp_vol_obj, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CLOSEERROR, FAIL, "unable to close group")

    /* Free the VOL object */
    if (H5VL_free_object(grp_vol_obj) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTDEC, FAIL, "unable to free VOL object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G__close_cb() */

/*-------------------------------------------------------------------------
 * Function:    H5Gcreate2
 *
 * Purpose:     Creates a new group relative to LOC_ID, giving it the
 *              specified creation property list GCPL_ID and access
 *              property list GAPL_ID.  The link to the new group is
 *              created with the LCPL_ID.
 *
 * Usage:       H5Gcreate2(loc_id, char *name, lcpl_id, gcpl_id, gapl_id)
 *                  hid_t loc_id;	  IN: File or group identifier
 *                  const char *name; IN: Absolute or relative name of the new group
 *                  hid_t lcpl_id;	  IN: Property list for link creation
 *                  hid_t gcpl_id;	  IN: Property list for group creation
 *                  hid_t gapl_id;	  IN: Property list for group access
 *
 * Return:      Success:    The object ID of a new, empty group open for
 *                          writing.  Call H5Gclose() when finished with
 *                          the group.
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gcreate2(hid_t loc_id, const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id)
{
    void *            grp     = NULL; /* Structure for new group */
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    hid_t             ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE5("i", "i*siii", loc_id, name, lcpl_id, gcpl_id, gapl_id);

    /* Check arguments */
    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be NULL")
    if (!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be an empty string")

    /* Check link creation property list */
    if (H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;
    else if (TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a link creation property list")

    /* Check group creation property list */
    if (H5P_DEFAULT == gcpl_id)
        gcpl_id = H5P_GROUP_CREATE_DEFAULT;
    else if (TRUE != H5P_isa_class(gcpl_id, H5P_GROUP_CREATE))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a group creation property list")

    /* Set the LCPL for the API context */
    H5CX_set_lcpl(lcpl_id);

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&gapl_id, H5P_CLS_GACC, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Set the location parameters */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* Create the group */
    if (NULL == (grp = H5VL_group_create(vol_obj, &loc_params, name, lcpl_id, gcpl_id, gapl_id,
                                         H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, H5I_INVALID_HID, "unable to create group")

    /* Get an ID for the group */
    if ((ret_value = H5VL_register(H5I_GROUP, grp, vol_obj->connector, TRUE)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to get ID for group handle")

done:
    if (H5I_INVALID_HID == ret_value)
        if (grp && H5VL_group_close(vol_obj, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CLOSEERROR, H5I_INVALID_HID, "unable to release group")

    FUNC_LEAVE_API(ret_value)
} /* end H5Gcreate2() */

/*-------------------------------------------------------------------------
 * Function:    H5Gcreate_anon
 *
 * Purpose:     Creates a new group relative to LOC_ID, giving it the
 *              specified creation property list GCPL_ID and access
 *              property list GAPL_ID.
 *
 *              The resulting ID should be linked into the file with
 *              H5Olink or it will be deleted when closed.
 *
 *              Given the default setting, H5Gcreate_anon() followed by
 *              H5Olink() will have the same function as H5Gcreate2().
 *
 * Usage:       H5Gcreate_anon(loc_id, char *name, gcpl_id, gapl_id)
 *                  hid_t loc_id;	  IN: File or group identifier
 *                  const char *name; IN: Absolute or relative name of the new group
 *                  hid_t gcpl_id;	  IN: Property list for group creation
 *                  hid_t gapl_id;	  IN: Property list for group access
 *
 * Example:     To create missing groups "A" and "B01" along the given path "/A/B01/grp"
 *              hid_t create_id = H5Pcreate(H5P_GROUP_CREATE);
 *              int   status = H5Pset_create_intermediate_group(create_id, TRUE);
 *              hid_t gid = H5Gcreate_anon(file_id, "/A/B01/grp", create_id, H5P_DEFAULT);
 *
 * Return:      Success:    The object ID of a new, empty group open for
 *                          writing. Call H5Gclose() when finished with
 *                          the group.
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gcreate_anon(hid_t loc_id, hid_t gcpl_id, hid_t gapl_id)
{
    void *            grp     = NULL;              /* Structure for new group */
    H5VL_object_t *   vol_obj = NULL;              /* Object for loc_id */
    H5VL_loc_params_t loc_params;                  /* Location parameters for object access */
    hid_t             ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE3("i", "iii", loc_id, gcpl_id, gapl_id);

    /* Check group creation property list */
    if (H5P_DEFAULT == gcpl_id)
        gcpl_id = H5P_GROUP_CREATE_DEFAULT;
    else if (TRUE != H5P_isa_class(gcpl_id, H5P_GROUP_CREATE))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not group create property list")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&gapl_id, H5P_CLS_GACC, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Set location struct fields */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Create the group */
    if (NULL == (grp = H5VL_group_create(vol_obj, &loc_params, NULL, H5P_LINK_CREATE_DEFAULT, gcpl_id,
                                         gapl_id, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, H5I_INVALID_HID, "unable to create group")

    /* Get an ID for the group */
    if ((ret_value = H5VL_register(H5I_GROUP, grp, vol_obj->connector, TRUE)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to get ID for group handle")

done:
    /* Cleanup on failure */
    if (H5I_INVALID_HID == ret_value)
        if (grp && H5VL_group_close(vol_obj, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CLOSEERROR, H5I_INVALID_HID, "unable to release group")

    FUNC_LEAVE_API(ret_value)
} /* end H5Gcreate_anon() */

/*-------------------------------------------------------------------------
 * Function:    H5Gopen2
 *
 * Purpose:     Opens an existing group for modification.  When finished,
 *              call H5Gclose() to close it and release resources.
 *
 *              This function allows the user the pass in a Group Access
 *              Property List, which H5Gopen1() does not.
 *
 * Return:      Success:    Object ID of the group
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gopen2(hid_t loc_id, const char *name, hid_t gapl_id)
{
    void *            grp     = NULL; /* Group opened */
    H5VL_object_t *   vol_obj = NULL; /* object of loc_id */
    H5VL_loc_params_t loc_params;
    hid_t             ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE3("i", "i*si", loc_id, name, gapl_id);

    /* Check args */
    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be NULL")
    if (!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be an empty string")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&gapl_id, H5P_CLS_GACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Open the group */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    if (NULL == (grp = H5VL_group_open(vol_obj, &loc_params, name, gapl_id, H5P_DATASET_XFER_DEFAULT,
                                       H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, H5I_INVALID_HID, "unable to open group")

    /* Register an ID for the group */
    if ((ret_value = H5VL_register(H5I_GROUP, grp, vol_obj->connector, TRUE)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register group")

done:
    if (H5I_INVALID_HID == ret_value)
        if (grp && H5VL_group_close(vol_obj, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CLOSEERROR, H5I_INVALID_HID, "unable to release group")

    FUNC_LEAVE_API(ret_value)
} /* end H5Gopen2() */

/*-------------------------------------------------------------------------
 * Function:    H5Gget_create_plist
 *
 * Purpose:     Returns a copy of the group creation property list.
 *
 * Return:      Success:    ID for a copy of the group creation
 *                          property list.  The property list ID should be
 *                          released by calling H5Pclose().
 *
 *              Failure:    H5I_INVALID_HID
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gget_create_plist(hid_t group_id)
{
    H5VL_object_t *vol_obj   = NULL;
    hid_t          ret_value = H5I_INVALID_HID;

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE1("i", "i", group_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(group_id, H5I_GROUP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a group ID")

    /* Get the group creation property list for the group */
    if (H5VL_group_get(vol_obj, H5VL_GROUP_GET_GCPL, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, &ret_value) <
        0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, H5I_INVALID_HID, "can't get group's creation property list")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Gget_create_plist() */

/*-------------------------------------------------------------------------
 * Function:    H5Gget_info
 *
 * Purpose:     Retrieve information about a group.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gget_info(hid_t loc_id, H5G_info_t *group_info)
{
    H5VL_object_t *   vol_obj;
    H5I_type_t        id_type; /* Type of ID */
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", loc_id, group_info);

    /* Check args */
    id_type = H5I_get_type(loc_id);
    if (!(H5I_GROUP == id_type || H5I_FILE == id_type))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid group (or file) ID")
    if (!group_info)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "group_info parameter cannot be NULL")

    /* Get group location */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Retrieve the group's information */
    loc_params.type     = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = id_type;
    if (H5VL_group_get(vol_obj, H5VL_GROUP_GET_INFO, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, &loc_params,
                       group_info) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "unable to get group info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Gget_info() */

/*-------------------------------------------------------------------------
 * Function:    H5Gget_info_by_name
 *
 * Purpose:     Retrieve information about a group, where the group is
 *              identified by name instead of ID.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gget_info_by_name(hid_t loc_id, const char *name, H5G_info_t *group_info, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj;
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "i*s*xi", loc_id, name, group_info, lapl_id);

    /* Check args */
    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be NULL")
    if (!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "name parameter cannot be an empty string")
    if (!group_info)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "group_info parameter cannot be NULL")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Set up location parameters */
    loc_params.type                         = H5VL_OBJECT_BY_NAME;
    loc_params.loc_data.loc_by_name.name    = name;
    loc_params.loc_data.loc_by_name.lapl_id = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* Retrieve the group's information */
    if (H5VL_group_get(vol_obj, H5VL_GROUP_GET_INFO, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, &loc_params,
                       group_info) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve group info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Gget_info_by_name() */

/*-------------------------------------------------------------------------
 * Function:    H5Gget_info_by_idx
 *
 * Purpose:     Retrieve information about a group, according to the order
 *              of an index.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gget_info_by_idx(hid_t loc_id, const char *group_name, H5_index_t idx_type, H5_iter_order_t order,
                   hsize_t n, H5G_info_t *group_info, hid_t lapl_id)
{
    H5VL_object_t *   vol_obj;
    H5VL_loc_params_t loc_params;
    herr_t            ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "i*sIiIoh*xi", loc_id, group_name, idx_type, order, n, group_info, lapl_id);

    /* Check args */
    if (!group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "group_name parameter cannot be NULL")
    if (!*group_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "group_name parameter cannot be an empty string")
    if (idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if (order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if (!group_info)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "group_info parameter cannot be NULL")

    /* Verify access property list and set up collective metadata if appropriate */
    if (H5CX_set_apl(&lapl_id, H5P_CLS_LACC, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Get the location object */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid location identifier")

    /* Set location parameters */
    loc_params.type                         = H5VL_OBJECT_BY_IDX;
    loc_params.loc_data.loc_by_idx.name     = group_name;
    loc_params.loc_data.loc_by_idx.idx_type = idx_type;
    loc_params.loc_data.loc_by_idx.order    = order;
    loc_params.loc_data.loc_by_idx.n        = n;
    loc_params.loc_data.loc_by_idx.lapl_id  = lapl_id;
    loc_params.obj_type                     = H5I_get_type(loc_id);

    /* Retrieve the group's information */
    if (H5VL_group_get(vol_obj, H5VL_GROUP_GET_INFO, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, &loc_params,
                       group_info) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve group info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Gget_info_by_idx() */

/*-------------------------------------------------------------------------
 * Function:    H5Gclose
 *
 * Purpose:     Closes the specified group. The group ID will no longer be
 *              valid for accessing the group.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gclose(hid_t group_id)
{
    herr_t ret_value = SUCCEED; /* Return value                     */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", group_id);

    /* Check arguments */
    if (H5I_GROUP != H5I_get_type(group_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a group ID")

    /* Decrement the counter on the group ID. It will be freed if the count
     * reaches zero.
     */
    if (H5I_dec_app_ref(group_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTDEC, FAIL, "decrementing group ID failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Gclose() */

/*-------------------------------------------------------------------------
 * Function:    H5Gflush
 *
 * Purpose:     Flushes all buffers associated with a group to disk.
 *
 * Return:      Non-negative on success, negative on failure
 *
 * Programmer:  Mike McGreevy
 *              May 19, 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gflush(hid_t group_id)
{
    H5VL_object_t *vol_obj;             /* Group for this operation     */
    herr_t         ret_value = SUCCEED; /* Return value                 */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", group_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(group_id, H5I_GROUP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a group ID")

    /* Set up collective metadata if appropriate */
    if (H5CX_set_loc(group_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Flush group's metadata to file */
    if (H5VL_group_specific(vol_obj, H5VL_GROUP_FLUSH, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, group_id) <
        0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTFLUSH, FAIL, "unable to flush group")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Gflush */

/*-------------------------------------------------------------------------
 * Function:    H5Grefresh
 *
 * Purpose:     Refreshes all buffers associated with a group.
 *
 * Return:      Non-negative on success, negative on failure
 *
 * Programmer:  Mike McGreevy
 *              July 21, 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Grefresh(hid_t group_id)
{
    H5VL_object_t *vol_obj;             /* Group for this operation     */
    herr_t         ret_value = SUCCEED; /* Return value                 */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", group_id);

    /* Check args */
    if (NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(group_id, H5I_GROUP)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a group ID")

    /* Set up collective metadata if appropriate */
    if (H5CX_set_loc(group_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Refresh group's metadata */
    if (H5VL_group_specific(vol_obj, H5VL_GROUP_REFRESH, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL,
                            group_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTLOAD, FAIL, "unable to refresh group")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Grefresh */
