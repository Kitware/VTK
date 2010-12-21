/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:		H5Gloc.c
 *			Sep 13 2005
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Functions for working with group "locations"
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5G_PACKAGE		/*suppress error about including H5Gpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Aprivate.h"		/* Attributes				*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Gpkg.h"		/* Groups		  		*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Lprivate.h"		/* Links				*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* User data for looking up an object in a group */
typedef struct {
    /* upward */
    H5G_loc_t  *loc;            /* Group location to set */
} H5G_loc_fnd_t;

/* User data for checking if an object exists */
typedef struct {
    /* upward */
    htri_t exists;              /* Whether the object exists */
} H5G_loc_exists_t;

/* User data for looking up an object in a group by index */
typedef struct {
    /* downward */
    hid_t lapl_id;              /* LAPL to use for operation */
    hid_t dxpl_id;              /* DXPL to use for operation */
    H5_index_t idx_type;        /* Index to use */
    H5_iter_order_t order;      /* Iteration order within index */
    hsize_t n;                  /* Offset within index */

    /* upward */
    H5G_loc_t  *loc;            /* Group location to set */
} H5G_loc_fbi_t;

/* User data for getting an object's info in a group */
typedef struct {
    /* downward */
    hid_t dxpl_id;              /* DXPL to use for operation */
    hbool_t want_ih_info;       /* Whether to retrieve the index & heap info */

    /* upward */
    H5O_info_t  *oinfo;         /* Object information to retrieve */
} H5G_loc_info_t;

/* User data for setting an object's comment in a group */
typedef struct {
    /* downward */
    hid_t dxpl_id;              /* DXPL to use for operation */
    const char *comment;        /* Object comment buffer */

    /* upward */
} H5G_loc_sc_t;

/* User data for getting an object's comment in a group */
typedef struct {
    /* downward */
    hid_t dxpl_id;              /* DXPL to use for operation */
    char  *comment;             /* Object comment buffer */
    size_t bufsize;             /* Size of object comment buffer */

    /* upward */
    ssize_t comment_size;       /* Actual size of object comment */
} H5G_loc_gc_t;


/********************/
/* Local Prototypes */
/********************/

/* Group traversal callbacks */
static herr_t H5G_loc_find_cb(H5G_loc_t *grp_loc, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata,
    H5G_own_loc_t *own_loc);
static herr_t H5G_loc_find_by_idx_cb(H5G_loc_t *grp_loc, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata,
    H5G_own_loc_t *own_loc);
static herr_t H5G_loc_set_comment_cb(H5G_loc_t *grp_loc, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata,
    H5G_own_loc_t *own_loc);
static herr_t H5G_loc_get_comment_cb(H5G_loc_t *grp_loc, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata,
    H5G_own_loc_t *own_loc);


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
 * Function:	H5G_loc
 *
 * Purpose:	Given an object ID return a location for the object.
 *
 * Return:	Success:	Group pointer.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, September 13, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_loc(hid_t loc_id, H5G_loc_t *loc)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc, FAIL)

    switch(H5I_get_type(loc_id)) {
        case H5I_FILE:
            {
                H5F_t	*f;

                /* Get the file struct */
                if(NULL == (f = (H5F_t *)H5I_object(loc_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file ID")

                /* Construct a group location for root group of the file */
                if(H5G_loc_root(f, loc) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "unable to create location for file")
            } /* end case */
            break;

        case H5I_GENPROP_CLS:
        case H5I_GENPROP_LST:
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get group location of property list")

        case H5I_ERROR_CLASS:
        case H5I_ERROR_MSG:
        case H5I_ERROR_STACK:
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get group location of error class, message or stack")

        case H5I_GROUP:
            {
                H5G_t	*group;

                if(NULL == (group = (H5G_t *)H5I_object(loc_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid group ID")
                if(NULL == (loc->oloc = H5G_oloc(group)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get object location of group")
                if(NULL == (loc->path = H5G_nameof(group)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get path of group")
            } /* end case */
            break;

        case H5I_DATATYPE:
            {
                H5T_t	*dt;

                if(NULL == (dt = (H5T_t *)H5I_object(loc_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid type ID")
                if(NULL == (loc->oloc = H5T_oloc(dt)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get object location of datatype")
                if(NULL == (loc->path = H5T_nameof(dt)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get path of datatype")
            } /* end case */
            break;

        case H5I_DATASPACE:
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get group location of dataspace")

        case H5I_DATASET:
            {
                H5D_t	*dset;

                if(NULL == (dset = (H5D_t *)H5I_object(loc_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid data ID")
                if(NULL == (loc->oloc = H5D_oloc(dset)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get object location of dataset")
                if(NULL == (loc->path = H5D_nameof(dset)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get path of dataset")
            } /* end case */
            break;

        case H5I_ATTR:
            {
                H5A_t	*attr;

                if(NULL == (attr = (H5A_t *)H5I_object(loc_id)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid attribute ID")
                if(NULL == (loc->oloc = H5A_oloc(attr)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get object location of attribute")
                if(NULL == (loc->path = H5A_nameof(attr)))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get path of attribute")
            } /* end case */
            break;

        case H5I_REFERENCE:
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get group location of reference")

        case H5I_UNINIT:
        case H5I_BADID:
        case H5I_VFL:
        case H5I_NTYPES:
        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid object ID")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_root
 *
 * Purpose:	Construct a "group location" for the root group of a file
 *
 * Return:	Success:	Non-negative
 * 		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Mar  5 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_loc_root(H5F_t *f, H5G_loc_t *loc)
{
    H5G_t *root_grp;                    /* Pointer to root group's info */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc_root, FAIL)

    HDassert(f);
    HDassert(loc);

    /* Retrieve the root group for the file */
    root_grp = H5G_rootof(f);
    HDassert(root_grp);

    /* Build the group location for the root group */
    if(NULL == (loc->oloc = H5G_oloc(root_grp)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get object location for root group")
    if(NULL == (loc->path = H5G_nameof(root_grp)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get path for root group")

    /* Patch up root group's object location to reflect this file */
    /* (Since the root group info is only stored once for files which
     *  share an underlying low-level file)
     */
    /* (but only for non-mounted files) */
    if(!H5F_is_mount(f)) {
        loc->oloc->file = f;
        loc->oloc->holding_file = FALSE;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_root() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_copy
 *
 * Purpose:	Copy over information for a location
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, September 13, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_loc_copy(H5G_loc_t *dst, const H5G_loc_t *src, H5_copy_depth_t depth)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc_copy, FAIL)

    /* Check args. */
    HDassert(dst);
    HDassert(src);

    /* Copy components of the location */
    if(H5O_loc_copy(dst->oloc, src->oloc, depth) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to copy entry")
    if(H5G_name_copy(dst->path, src->path, depth) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to copy path")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_copy() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_reset
 *
 * Purpose:	Reset information for a location
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, September 13, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_loc_reset(H5G_loc_t *loc)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc_reset, FAIL)

    /* Check args. */
    HDassert(loc);

    /* Reset components of the location */
    if(H5O_loc_reset(loc->oloc) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to reset entry")
    if(H5G_name_reset(loc->path) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to reset path")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_free
 *
 * Purpose:	Free information for a location
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, September 13, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_loc_free(H5G_loc_t *loc)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc_free, FAIL)

    /* Check args. */
    HDassert(loc);

    /* Reset components of the location */
    if(H5G_name_free(loc->path) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "unable to free path")
    if(H5O_loc_free(loc->oloc) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "unable to free object header location")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_free() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_find_cb
 *
 * Purpose:	Callback for retrieving object location for an object in a group
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, October 17, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_loc_find_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char *name,
    const H5O_link_t UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/)
{
    H5G_loc_fnd_t *udata = (H5G_loc_fnd_t *)_udata;   /* User data passed in */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_loc_find_cb)

    /* Check if the name in this group resolved to a valid object */
    if(obj_loc == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object '%s' doesn't exist", name)

    /* Take ownership of the object's group location */
    /* (Group traversal callbacks are responsible for either taking ownership
     *  of the group location for the object, or freeing it. - QAK)
     */
    H5G_loc_copy(udata->loc, obj_loc, H5_COPY_SHALLOW);
    *own_loc = H5G_OWN_OBJ_LOC;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_find_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_find
 *
 * Purpose:	Find a symbol from a location
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, September 13, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_loc_find(const H5G_loc_t *loc, const char *name, H5G_loc_t *obj_loc/*out*/,
    hid_t lapl_id, hid_t dxpl_id)
{
    H5G_loc_fnd_t udata;                /* User data for traversal callback */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc_find, FAIL)

    /* Check args. */
    HDassert(loc);
    HDassert(name && *name);
    HDassert(obj_loc);

    /* Set up user data for locating object */
    udata.loc = obj_loc;

    /* Traverse group hierarchy to locate object */
    if(H5G_traverse(loc, name, H5G_TARGET_NORMAL, H5G_loc_find_cb, &udata, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "can't find object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_find() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_find_by_idx_cb
 *
 * Purpose:	Callback for retrieving object location for an object in a group
 *              according to the order within an index
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 20, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_loc_find_by_idx_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char UNUSED *name,
    const H5O_link_t UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/)
{
    H5G_loc_fbi_t *udata = (H5G_loc_fbi_t *)_udata;   /* User data passed in */
    H5O_link_t fnd_lnk;                 /* Link within group */
    hbool_t lnk_copied = FALSE;         /* Whether the link was copied */
    size_t links_left = H5L_NUM_LINKS;  /* # of links left to traverse */
    hbool_t obj_loc_valid = FALSE;      /* Flag to indicate that the object location is valid */
    hbool_t obj_exists = FALSE;         /* Whether the object exists (unused) */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_loc_find_by_idx_cb)

    /* Check if the name in this group resolved to a valid link */
    if(obj_loc == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "group doesn't exist")

    /* Query link */
    if(H5G_obj_lookup_by_idx(obj_loc->oloc, udata->idx_type, udata->order,
                udata->n, &fnd_lnk, udata->dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "link not found")
    lnk_copied = TRUE;

    /* Build the initial object location for the link */
    if(H5G_link_to_loc(obj_loc, &fnd_lnk, udata->loc) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "cannot initialize object location")
    obj_loc_valid = TRUE;

    /* Perform any special traversals that the link needs */
    /* (soft links, user-defined links, file mounting, etc.) */
    /* (may modify the object location) */
    if(H5G_traverse_special(obj_loc, &fnd_lnk, H5G_TARGET_NORMAL, &links_left, TRUE, udata->loc, &obj_exists, udata->lapl_id, udata->dxpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_TRAVERSE, FAIL, "special link traversal failed")

done:
    /* Reset the link information, if we have a copy */
    if(lnk_copied)
        H5O_msg_reset(H5O_LINK_ID, &fnd_lnk);

    /* Release the object location if we failed after copying it */
    if(ret_value < 0 && obj_loc_valid)
        if(H5G_loc_free(udata->loc) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "can't free location")

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_find_by_idx_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_find_by_idx
 *
 * Purpose:	Find a symbol from a location, according to the order in an index
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Monday, November 20, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_loc_find_by_idx(H5G_loc_t *loc, const char *group_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, H5G_loc_t *obj_loc/*out*/, hid_t lapl_id,
    hid_t dxpl_id)
{
    H5G_loc_fbi_t udata;                /* User data for traversal callback */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc_find_by_idx, FAIL)

    /* Check args. */
    HDassert(loc);
    HDassert(group_name && *group_name);
    HDassert(obj_loc);

    /* Set up user data for locating object */
    udata.dxpl_id = dxpl_id;
    udata.lapl_id = lapl_id;
    udata.idx_type = idx_type;
    udata.order = order;
    udata.n = n;
    udata.loc = obj_loc;

    /* Traverse group hierarchy to locate object */
    if(H5G_traverse(loc, group_name, H5G_TARGET_NORMAL, H5G_loc_find_by_idx_cb, &udata, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "can't find object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_find_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_insert
 *
 * Purpose:	Insert an object at a location
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, September 13, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_loc_insert(H5G_loc_t *grp_loc, const char *name, H5G_loc_t *obj_loc,
    hid_t dxpl_id)
{
    H5O_link_t  lnk;                    /* Link for object to insert */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc_insert, FAIL)

    /* Check args. */
    HDassert(grp_loc);
    HDassert(name && *name);
    HDassert(obj_loc);

    /* Create link object for the object location */
    lnk.type = H5L_TYPE_HARD;
    lnk.cset = H5F_DEFAULT_CSET;
    lnk.corder = 0;     /* Will be reset if the group is tracking creation order */
    lnk.corder_valid = FALSE;   /* Indicate that the creation order isn't valid (yet) */
    /* Casting away const OK -QAK */
    lnk.name = (char *)name;
    lnk.u.hard.addr = obj_loc->oloc->addr;

    /* Insert new group into current group's symbol table */
    if(H5G_obj_insert(grp_loc->oloc, name, &lnk, TRUE, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINSERT, FAIL, "unable to insert object")

    /* Set the name of the object location */
    if(H5G_name_set(grp_loc->path, obj_loc->path, name) < 0)
       HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "cannot set name")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_insert() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_exists_cb
 *
 * Purpose:	Callback for checking if an object exists
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, February 2, 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_loc_exists_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char UNUSED *name,
    const H5O_link_t UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/)
{
    H5G_loc_exists_t *udata = (H5G_loc_exists_t *)_udata;   /* User data passed in */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_loc_exists_cb)

    /* Check if the name in this group resolved to a valid object */
    if(obj_loc == NULL)
        if(lnk)
            udata->exists = FALSE;
        else
            udata->exists = FAIL;
    else
        udata->exists = TRUE;

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G_loc_exists_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_exists
 *
 * Purpose:	Check if an object actually exists at a location
 *
 * Return:	Success:	TRUE/FALSE
 * 		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, February 2, 2010
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5G_loc_exists(const H5G_loc_t *loc, const char *name, hid_t lapl_id, hid_t dxpl_id)
{
    H5G_loc_exists_t udata;     /* User data for traversal callback */
    htri_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc_exists, FAIL)

    /* Check args. */
    HDassert(loc);
    HDassert(name && *name);

    /* Set up user data for locating object */
    udata.exists = FALSE;

    /* Traverse group hierarchy to locate object */
    if(H5G_traverse(loc, name, H5G_TARGET_EXISTS, H5G_loc_exists_cb, &udata, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "can't check if object exists")

    /* Set return value */
    ret_value = udata.exists;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_exists() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_info_cb
 *
 * Purpose:	Callback for retrieving object info for an object in a group
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, November 23, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_loc_info_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char UNUSED *name, const H5O_link_t UNUSED *lnk,
    H5G_loc_t *obj_loc, void *_udata/*in,out*/, H5G_own_loc_t *own_loc/*out*/)
{
    H5G_loc_info_t *udata = (H5G_loc_info_t *)_udata;   /* User data passed in */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_loc_info_cb)

    /* Check if the name in this group resolved to a valid link */
    if(obj_loc == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "name doesn't exist")

    /* Query object information */
    if(H5O_get_info(obj_loc->oloc, udata->dxpl_id, udata->want_ih_info, udata->oinfo) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't get object info")

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_info_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_info
 *
 * Purpose:	Retrieve the information for an object from a group location
 *              and path to that object
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, November 23, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_loc_info(H5G_loc_t *loc, const char *name, hbool_t want_ih_info, H5O_info_t *oinfo/*out*/,
    hid_t lapl_id, hid_t dxpl_id)
{
    H5G_loc_info_t udata;               /* User data for traversal callback */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc_info, FAIL)

    /* Check args. */
    HDassert(loc);
    HDassert(name && *name);
    HDassert(oinfo);

    /* Set up user data for locating object */
    udata.dxpl_id = dxpl_id;
    udata.want_ih_info = want_ih_info;
    udata.oinfo = oinfo;

    /* Traverse group hierarchy to locate object */
    if(H5G_traverse(loc, name, H5G_TARGET_NORMAL, H5G_loc_info_cb, &udata, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "can't find object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_info() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_set_comment_cb
 *
 * Purpose:	Callback for (re)setting object comment for an object in a group
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, August 30, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_loc_set_comment_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char UNUSED *name, const H5O_link_t UNUSED *lnk,
    H5G_loc_t *obj_loc, void *_udata/*in,out*/, H5G_own_loc_t *own_loc/*out*/)
{
    H5G_loc_sc_t *udata = (H5G_loc_sc_t *)_udata;   /* User data passed in */
    H5O_name_t comment;                 /* Object header "comment" message */
    htri_t exists;                      /* Whether a "comment" message already exists */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_loc_set_comment_cb)

    /* Check if the name in this group resolved to a valid link */
    if(obj_loc == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "name doesn't exist")

    /* Check for existing comment message */
    if((exists = H5O_msg_exists(obj_loc->oloc, H5O_NAME_ID, udata->dxpl_id)) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to read object header")

    /* Remove the previous comment message if any */
    if(exists)
        if(H5O_msg_remove(obj_loc->oloc, H5O_NAME_ID, 0, TRUE, udata->dxpl_id) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTDELETE, FAIL, "unable to delete existing comment object header message")

    /* Add the new message */
    if(udata->comment && *udata->comment) {
        /* Casting away const OK -QAK */
	comment.s = (char *)udata->comment;
	if(H5O_msg_create(obj_loc->oloc, H5O_NAME_ID, 0, H5O_UPDATE_TIME, &comment, udata->dxpl_id) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to set comment object header message")
    } /* end if */

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_set_comment_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_set_comment
 *
 * Purpose:	(Re)set the information for an object from a group location
 *              and path to that object
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, August 30, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_loc_set_comment(H5G_loc_t *loc, const char *name, const char *comment,
    hid_t lapl_id, hid_t dxpl_id)
{
    H5G_loc_sc_t udata;         /* User data for traversal callback */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc_set_comment, FAIL)

    /* Check args. */
    HDassert(loc);
    HDassert(name && *name);

    /* Set up user data for locating object */
    udata.dxpl_id = dxpl_id;
    udata.comment = comment;

    /* Traverse group hierarchy to locate object */
    if(H5G_traverse(loc, name, H5G_TARGET_NORMAL, H5G_loc_set_comment_cb, &udata, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "can't find object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_set_comment() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_get_comment_cb
 *
 * Purpose:	Callback for retrieving object comment for an object in a group
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, August 30, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_loc_get_comment_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char UNUSED *name, const H5O_link_t UNUSED *lnk,
    H5G_loc_t *obj_loc, void *_udata/*in,out*/, H5G_own_loc_t *own_loc/*out*/)
{
    H5G_loc_gc_t *udata = (H5G_loc_gc_t *)_udata;   /* User data passed in */
    H5O_name_t comment;                 /* Object header "comment" message */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_loc_get_comment_cb)

    /* Check if the name in this group resolved to a valid link */
    if(obj_loc == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "name doesn't exist")

    /* Query object comment */
    comment.s = NULL;
    if(NULL == H5O_msg_read(obj_loc->oloc, H5O_NAME_ID, &comment, udata->dxpl_id)) {
	if(udata->comment && udata->bufsize > 0)
            udata->comment[0] = '\0';
	udata->comment_size = 0;
    } else {
        if(udata->comment && udata->bufsize)
	   HDstrncpy(udata->comment, comment.s, udata->bufsize);
	udata->comment_size = (ssize_t)HDstrlen(comment.s);
	H5O_msg_reset(H5O_NAME_ID, &comment);
    } /* end else */

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_get_comment_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5G_loc_get_comment
 *
 * Purpose:	Retrieve the information for an object from a group location
 *              and path to that object
 *
 * Return:	Success:	Number of bytes in the comment including the
 *				null terminator.  Zero if the object has no
 *				comment.
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		Thursday, August 30, 2007
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5G_loc_get_comment(H5G_loc_t *loc, const char *name, char *comment/*out*/,
    size_t bufsize, hid_t lapl_id, hid_t dxpl_id)
{
    H5G_loc_gc_t udata;         /* User data for traversal callback */
    ssize_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5G_loc_get_comment, FAIL)

    /* Check args. */
    HDassert(loc);
    HDassert(name && *name);

    /* Set up user data for locating object */
    udata.dxpl_id = dxpl_id;
    udata.comment = comment;
    udata.bufsize = bufsize;
    udata.comment_size = (-1);

    /* Traverse group hierarchy to locate object */
    if(H5G_traverse(loc, name, H5G_TARGET_NORMAL, H5G_loc_get_comment_cb, &udata, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "can't find object")

    /* Set the return value */
    ret_value = udata.comment_size;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_loc_get_comment() */

