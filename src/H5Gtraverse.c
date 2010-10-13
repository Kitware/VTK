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
 * Created:		H5Gtraverse.c
 *			Sep 13 2005
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Functions for traversing group hierarchy
 *
 *-------------------------------------------------------------------------
 */
#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */
#define H5G_PACKAGE		/*suppress error about including H5Gpkg	  */


/* Packages needed by this file... */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"         /* Datasets                             */
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"		/* File access				*/
#include "H5Gpkg.h"		/* Groups		  		*/
#include "H5HLprivate.h"	/* Local Heaps				*/
#include "H5Iprivate.h"		/* IDs					*/
#include "H5Lprivate.h"		/* Links				*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Ppublic.h"		/* Property Lists			*/

/* Private typedefs */

/* User data for path traversal routine */
typedef struct {
    /* down */
    hbool_t chk_exists;         /* Flag to indicate we are checking if object exists */

    /* up */
    H5G_loc_t *obj_loc;         /* Object location */
    hbool_t exists;             /* Indicate if object exists */
} H5G_trav_slink_t;

/* Private macros */

/* Local variables */
static char *H5G_comp_g = NULL;                 /*component buffer      */
static size_t H5G_comp_alloc_g = 0;             /*sizeof component buffer */

/* PRIVATE PROTOTYPES */
static herr_t H5G_traverse_slink_cb(H5G_loc_t *grp_loc, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);
static herr_t H5G_traverse_ud(const H5G_loc_t *grp_loc, const H5O_link_t *lnk,
    H5G_loc_t *obj_loc/*in,out*/, unsigned target, size_t *nlinks/*in,out*/,
    hbool_t *obj_exists, hid_t lapl_id, hid_t dxpl_id);
static herr_t H5G_traverse_slink(const H5G_loc_t *grp_loc, const H5O_link_t *lnk,
    H5G_loc_t *obj_loc/*in,out*/, unsigned target, size_t *nlinks/*in,out*/,
    hbool_t *obj_exists, hid_t lapl_id, hid_t dxpl_id);
static herr_t H5G_traverse_mount(H5G_loc_t *loc/*in,out*/);
static herr_t H5G_traverse_real(const H5G_loc_t *loc, const char *name,
    unsigned target, size_t *nlinks, H5G_traverse_t op, void *op_data,
    hid_t lapl_id, hid_t dxpl_id);


/*-------------------------------------------------------------------------
 * Function:	H5G_traverse_term_interface
 *
 * Purpose:	Terminates part  of the H5G interface - free the global
 *              component buffer.
 *
 * Return:	Success:	Non-negative.
 *
 * 		Failure:	Negative.
 *
 * Programmer:	Quincey Koziol
 *		Monday, September	26, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_traverse_term_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_traverse_term_interface)

    /* Free the global component buffer */
    H5G_comp_g = (char *)H5MM_xfree(H5G_comp_g);
    H5G_comp_alloc_g = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G_traverse_term_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5G_traverse_slink_cb
 *
 * Purpose:	Callback for soft link traversal.  This routine sets the
 *              correct information for the object location.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, September 13, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_traverse_slink_cb(H5G_loc_t UNUSED *grp_loc, const char UNUSED *name,
    const H5O_link_t UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/)
{
    H5G_trav_slink_t *udata = (H5G_trav_slink_t *)_udata;   /* User data passed in */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_traverse_slink_cb)

    /* Check for dangling soft link */
    if(obj_loc == NULL) {
        if(udata->chk_exists)
            udata->exists = FALSE;
        else
            HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "component not found")
    } /* end if */
    else {
        /* Copy new location information for resolved object */
        H5O_loc_copy(udata->obj_loc->oloc, obj_loc->oloc, H5_COPY_DEEP);

        /* Indicate that the object exists */
        udata->exists = TRUE;
    } /* end else */

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_traverse_slink_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5G_traverse_link_ud
 *
 * Purpose:	Callback for user-defined link traversal.  Sets up a
 *              location ID and passes it to the user traversal callback.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, September 13, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_traverse_ud(const H5G_loc_t *grp_loc/*in,out*/, const H5O_link_t *lnk,
    H5G_loc_t *obj_loc/*in,out*/, unsigned target, size_t *nlinks/*in,out*/,
    hbool_t *obj_exists, hid_t _lapl_id, hid_t dxpl_id)
{
    const H5L_class_t   *link_class;       /* User-defined link class */
    hid_t               cb_return = -1;         /* The ID the user-defined callback returned */
    H5G_loc_t           grp_loc_copy;
    H5G_name_t          grp_path_copy;
    H5O_loc_t           grp_oloc_copy;
    H5O_loc_t          *new_oloc = NULL;
    H5F_t              *temp_file = NULL;
    H5G_t              *grp;
    hid_t               lapl_id = (-1);         /* LAPL local to this routine */
    H5P_genplist_t     *lapl;                   /* LAPL with nlinks set */
    hid_t               cur_grp = (-1);
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_traverse_ud)

    /* Sanity check */
    HDassert(grp_loc);
    HDassert(lnk);
    HDassert(lnk->type >= H5L_TYPE_UD_MIN);
    HDassert(obj_loc);
    HDassert(nlinks);
    HDassert(_lapl_id >= 0);

    /* Reset the object's path information, because we can't detect any changes
     * in the "path" the user-defined callback takes */
    H5G_name_free(obj_loc->path);

    /* Get the link class for this type of link. */
    if(NULL == (link_class = H5L_find_class(lnk->type)))
        HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "unable to get UD link class")

    /* Set up location for user-defined callback.  Use a copy of our current
     * grp_loc. */
    grp_loc_copy.path = &grp_path_copy;
    grp_loc_copy.oloc = &grp_oloc_copy;
    H5G_loc_reset(&grp_loc_copy);
    if(H5G_loc_copy(&grp_loc_copy, grp_loc, H5_COPY_DEEP) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTCOPY, FAIL, "unable to copy object location")

    /* Create a group to pass to the user-defined callback */
    if((grp = H5G_open(&grp_loc_copy, dxpl_id)) == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open group")
    if((cur_grp = H5I_register(H5I_GROUP, grp, FALSE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register group")

    /* Check for generic default property list and use link access default if so */
    if(_lapl_id == H5P_DEFAULT) {
        HDassert(H5P_LINK_ACCESS_DEFAULT != -1);
        if(NULL == (lapl = (H5P_genplist_t *)H5I_object(H5P_LINK_ACCESS_DEFAULT)))
            HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "unable to get default property list")
    } /* end if */
    else {
        /* Get the underlying property list passed in */
        if(NULL == (lapl = (H5P_genplist_t *)H5I_object(_lapl_id)))
            HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "unable to get property list from ID")
    } /* end else */

    /* Copy the property list passed in */
    if((lapl_id = H5P_copy_plist(lapl, FALSE)) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "unable to copy property list")

    /* Get the underlying property list copy */
    if(NULL == (lapl = (H5P_genplist_t *)H5I_object(lapl_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "unable to get property list from ID")

    /* Record number of soft links left to traverse in the property list. */
    if(H5P_set(lapl, H5L_ACS_NLINKS_NAME, nlinks) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set nlink info")

    /* User-defined callback function */
    cb_return = (link_class->trav_func)(lnk->name, cur_grp, lnk->u.ud.udata, lnk->u.ud.size, lapl_id);

    /* Check for failing to locate the object */
    if(cb_return < 0) {
        /* Check if we just needed to know if the object exists */
        if(target & H5G_TARGET_EXISTS) {
            /* Clear any errors from the stack */
            H5E_clear_stack(NULL);

            /* Indicate that the object doesn't exist */
            *obj_exists = FALSE;

            /* Get out now */
            HGOTO_DONE(SUCCEED);
        } /* end if */
        /* else, we really needed to open the object */
        else
            HGOTO_ERROR(H5E_ARGS, H5E_BADATOM, FAIL, "traversal callback returned invalid ID")
    } /* end if */

    /* Get the oloc from the ID the user callback returned */
    switch(H5I_get_type(cb_return)) {
        case H5I_GROUP:
            if((new_oloc = H5G_oloc((H5G_t *)H5I_object(cb_return))) == NULL)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get object location from group ID")
                break;

        case H5I_DATASET:
            if((new_oloc = H5D_oloc((H5D_t *)H5I_object(cb_return))) ==NULL)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get object location from dataset ID")
                break;

        case H5I_DATATYPE:
            if((new_oloc = H5T_oloc((H5T_t *)H5I_object(cb_return))) ==NULL)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get object location from datatype ID")
                break;

        case H5I_FILE:
            if((temp_file = (H5F_t *)H5I_object(cb_return)) == NULL)
                HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "couldn't get file from ID")
            if((new_oloc = H5G_oloc(temp_file->shared->root_grp)) ==NULL)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "unable to get root group location from file ID")
                break;

        case H5I_UNINIT:
        case H5I_BADID:
        case H5I_DATASPACE:
        case H5I_ATTR:
        case H5I_REFERENCE:
        case H5I_VFL:
        case H5I_GENPROP_CLS:
        case H5I_GENPROP_LST:
        case H5I_ERROR_CLASS:
        case H5I_ERROR_MSG:
        case H5I_ERROR_STACK:
        case H5I_NTYPES:
        default:
            HGOTO_ERROR(H5E_ATOM, H5E_BADTYPE, FAIL, "not a valid location or object ID")
    } /* end switch */

    /* Copy the location the user returned to us */
    if(H5O_loc_copy(obj_loc->oloc, new_oloc, H5_COPY_DEEP) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTCOPY, FAIL, "unable to copy object location")

    /* Hold the file open until we free this object header (otherwise the
     * object location will be invalidated when the file closes).
     */
    if(H5O_loc_hold_file(obj_loc->oloc) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_LINKCOUNT, FAIL, "unable to hold file open")

    /* We have a copy of the location and we're holding the file open.
     * Close the open ID the user passed back.
     */
    if(H5I_dec_ref(cb_return, FALSE) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTRELEASE, FAIL, "unable to close atom from UD callback")
    cb_return = (-1);

done:
    /* Close location given to callback. */
    if(cur_grp > 0)
        if(H5I_dec_ref(cur_grp, FALSE) < 0)
            HDONE_ERROR(H5E_ATOM, H5E_CANTRELEASE, FAIL, "unable to close atom for current location")

    if(ret_value < 0 && cb_return > 0)
        if(H5I_dec_ref(cb_return, FALSE) < 0)
            HDONE_ERROR(H5E_ATOM, H5E_CANTRELEASE, FAIL, "unable to close atom from UD callback")

    /* Close the LAPL, if we copied one */
    if(lapl_id > 0 && H5I_dec_ref(lapl_id, FALSE) < 0)
        HDONE_ERROR(H5E_ATOM, H5E_CANTRELEASE, FAIL, "unable to close copied link access property list")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_traverse_ud() */


/*-------------------------------------------------------------------------
 * Function:	H5G_traverse_slink
 *
 * Purpose:	Traverses symbolic link.  The link head appears in the group
 *		whose entry is GRP_LOC and the link tail entry is OBJ_LOC.
 *
 * Return:	Success:	Non-negative, OBJ_LOC will contain information
 *				about the object to which the link points
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Friday, April 10, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_traverse_slink(const H5G_loc_t *grp_loc, const H5O_link_t *lnk,
    H5G_loc_t *obj_loc/*in,out*/, unsigned target, size_t *nlinks/*in,out*/,
    hbool_t *obj_exists, hid_t lapl_id, hid_t dxpl_id)
{
    H5G_trav_slink_t      udata;                  /* User data to pass to link traversal callback */
    H5G_name_t          tmp_obj_path;           /* Temporary copy of object's path */
    hbool_t             tmp_obj_path_set = FALSE;       /* Flag to indicate that tmp object path is initialized */
    H5O_loc_t           tmp_grp_oloc;           /* Temporary copy of group entry */
    H5G_name_t          tmp_grp_path;           /* Temporary copy of group's path */
    H5G_loc_t           tmp_grp_loc;            /* Temporary copy of group's location */
    hbool_t             tmp_grp_loc_set = FALSE;       /* Flag to indicate that tmp group location is initialized */
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_traverse_slink)

    /* Sanity check */
    HDassert(grp_loc);
    HDassert(lnk);
    HDassert(lnk->type == H5L_TYPE_SOFT);
    HDassert(nlinks);

    /* Set up temporary location */
    tmp_grp_loc.oloc = &tmp_grp_oloc;
    tmp_grp_loc.path = &tmp_grp_path;

    /* Portably initialize the temporary objects */
    H5G_loc_reset(&tmp_grp_loc);
    H5G_name_reset(&tmp_obj_path);

    /* Clone the group location, so we can track the names properly */
    /* ("tracking the names properly" means to ignore the effects of the
     *  link traversal on the object's & group's paths - QAK)
     */
    H5G_loc_copy(&tmp_grp_loc, grp_loc, H5_COPY_DEEP);
    tmp_grp_loc_set = TRUE;

    /* Hold the object's group hier. path to restore later */
    /* (Part of "tracking the names properly") */
    H5G_name_copy(&tmp_obj_path, obj_loc->path, H5_COPY_SHALLOW);
    tmp_obj_path_set = TRUE;

    /* Set up user data for traversal callback */
    udata.chk_exists = (target & H5G_TARGET_EXISTS) ? TRUE : FALSE;
    udata.exists = FALSE;
    udata.obj_loc = obj_loc;

    /* Traverse the link */
    if(H5G_traverse_real(&tmp_grp_loc, lnk->u.soft.name, target, nlinks, H5G_traverse_slink_cb, &udata, lapl_id, dxpl_id) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to follow symbolic link")

    /* Pass back information about whether the object exists */
    *obj_exists = udata.exists;

done:
    /* Restore object's group hier. path */
    if(tmp_obj_path_set) {
        H5G_name_free(obj_loc->path);
        H5G_name_copy(obj_loc->path, &tmp_obj_path, H5_COPY_SHALLOW);
    } /* end if */

    /* Release cloned copy of group location */
    if(tmp_grp_loc_set)
        H5G_loc_free(&tmp_grp_loc);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_traverse_slink() */


/*-------------------------------------------------------------------------
 * Function:	H5G_traverse_mount
 *
 * Purpose:	If LNK is a mount point then copy the entry for the root
 *		group of the mounted file into LNK.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Tuesday, October  6, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_traverse_mount(H5G_loc_t *obj_loc/*in,out*/)
{
    H5F_t	*parent = obj_loc->oloc->file,      /* File of object */
            *child = NULL;                      /* Child file */
    unsigned	lt, rt, md = 0;                 /* Binary search indices */
    int cmp;
    H5O_loc_t	*oloc = NULL;           /* Object location for mount points */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_traverse_mount, FAIL)

    /* Sanity check */
    HDassert(obj_loc);

    /*
     * The loop is necessary because we might have file1 mounted at the root
     * of file2, which is mounted somewhere in file3.
     */
    do {
	/*
	 * Use a binary search to find the potential mount point in the mount
	 * table for the parent
	 */
	lt = 0;
	rt = parent->shared->mtab.nmounts;
	cmp = -1;
	while(lt < rt && cmp) {
	    md = (lt + rt) / 2;
	    oloc = H5G_oloc(parent->shared->mtab.child[md].group);
	    cmp = H5F_addr_cmp(obj_loc->oloc->addr, oloc->addr);
	    if(cmp < 0)
		rt = md;
	    else
		lt = md + 1;
	} /* end while */

	/* Copy root info over to ENT */
    if(0 == cmp) {
        /* Get the child file */
        child = parent->shared->mtab.child[md].file;

        /* Get the location for the root group in the child's file */
        oloc = H5G_oloc(child->shared->root_grp);

        /* Release the mount point */
        if(H5O_loc_free(obj_loc->oloc) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "unable to free object location")

        /* Copy the entry for the root group */
        if(H5O_loc_copy(obj_loc->oloc, oloc, H5_COPY_DEEP) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTCOPY, FAIL, "unable to copy object location")

        /* In case the shared root group info points to a different file handle
         * than the child, modify obj_loc */
        obj_loc->oloc->file = child;

        /* Switch to child's file */
        parent = child;
    } /* end if */
    } while(!cmp);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_traverse_mount() */


/*-------------------------------------------------------------------------
 * Function:	H5G_traverse_special
 *
 * Purpose:	Handle traversing special link situations
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Nov 20 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_traverse_special(const H5G_loc_t *grp_loc, const H5O_link_t *lnk,
    unsigned target, size_t *nlinks, hbool_t last_comp,
    H5G_loc_t *obj_loc, hbool_t *obj_exists, hid_t lapl_id, hid_t dxpl_id)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_traverse_special, FAIL)

    /* Sanity check */
    HDassert(grp_loc);
    HDassert(lnk);
    HDassert(obj_loc);
    HDassert(nlinks);

    /*
     * If we found a symbolic link then we should follow it.  But if this
     * is the last component of the name and the H5G_TARGET_SLINK bit of
     * TARGET is set then we don't follow it.
     */
    if(H5L_TYPE_SOFT == lnk->type &&
            (0 == (target & H5G_TARGET_SLINK) || !last_comp)) {
        if((*nlinks)-- <= 0)
            HGOTO_ERROR(H5E_LINK, H5E_NLINKS, FAIL, "too many links")
        if(H5G_traverse_slink(grp_loc, lnk, obj_loc, (target & H5G_TARGET_EXISTS), nlinks, obj_exists, lapl_id, dxpl_id) < 0)
            HGOTO_ERROR(H5E_LINK, H5E_TRAVERSE, FAIL, "symbolic link traversal failed")
    } /* end if */

    /*
     * If we found a user-defined link then we should follow it.  But if this
     * is the last component of the name and the H5G_TARGET_UDLINK bit of
     * TARGET is set then we don't follow it.
     */
    if(lnk->type >= H5L_TYPE_UD_MIN &&
            (0 == (target & H5G_TARGET_UDLINK) || !last_comp) ) {
        if((*nlinks)-- <= 0)
            HGOTO_ERROR(H5E_LINK, H5E_NLINKS, FAIL, "too many links")
        if(H5G_traverse_ud(grp_loc, lnk, obj_loc, (target & H5G_TARGET_EXISTS), nlinks, obj_exists, lapl_id, dxpl_id) < 0)
            HGOTO_ERROR(H5E_LINK, H5E_TRAVERSE, FAIL, "user-defined link traversal failed")
    } /* end if */

    /*
     * Resolve mount points to the mounted group.  Do not do this step if
     * the H5G_TARGET_MOUNT bit of TARGET is set and this is the last
     * component of the name.
     *
     * (If this link is a hard link, try to perform mount point traversal)
     *
     * (Note that the soft and external link traversal above can change
     *  the status of the object (into a hard link), so don't use an 'else'
     *  statement here. -QAK)
     */
    if(H5F_addr_defined(obj_loc->oloc->addr) &&
            (0 == (target & H5G_TARGET_MOUNT) || !last_comp)) {
        if(H5G_traverse_mount(obj_loc/*in,out*/) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "mount point traversal failed")
    } /* end if */

    /* If the grp_loc is the only thing holding an external file open
     * and obj_loc is in the same file, obj_loc should also hold the
     * file open so that closing the grp_loc doesn't close the file.
     */
    if(grp_loc->oloc->holding_file && grp_loc->oloc->file == obj_loc->oloc->file)
        if(H5O_loc_hold_file(obj_loc->oloc) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_LINKCOUNT, FAIL, "unable to hold file open")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_traverse_special() */


/*-------------------------------------------------------------------------
 * Function:	H5G_traverse_real
 *
 * Purpose:	Internal version of path traversal routine
 *
 * Return:	Success:	Non-negative if name can be fully resolved.
 *
 *		Failure:	Negative if the name could not be fully
 *				resolved.
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug 11 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_traverse_real(const H5G_loc_t *_loc, const char *name, unsigned target,
    size_t *nlinks, H5G_traverse_t op, void *op_data, hid_t lapl_id, hid_t dxpl_id)
{
    H5G_loc_t           loc;            /* Location of start object     */
    H5O_loc_t           grp_oloc;	/* Object loc. for current group */
    H5G_name_t		grp_path;	/* Path for current group	*/
    H5G_loc_t           grp_loc;        /* Location of group            */
    H5O_loc_t		obj_oloc;	/* Object found			*/
    H5G_name_t		obj_path;	/* Path for object found	*/
    H5G_loc_t           obj_loc;        /* Location of object           */
    size_t		nchars;		/* component name length	*/
    H5O_link_t          lnk;            /* Link information for object  */
    hbool_t link_valid = FALSE;         /* Flag to indicate that the link information is valid */
    hbool_t obj_loc_valid = FALSE;      /* Flag to indicate that the object location is valid */
    H5G_own_loc_t own_loc=H5G_OWN_NONE; /* Enum to indicate whether callback took ownership of locations*/
    hbool_t group_copy = FALSE;         /* Flag to indicate that the group entry is copied */
    hbool_t last_comp = FALSE;          /* Flag to indicate that a component is the last component in the name */
    herr_t              ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_traverse_real)

    /* Check parameters */
    HDassert(_loc);
    HDassert(name);
    HDassert(nlinks);
    HDassert(op);

    /*
     * Where does the searching start?  For absolute names it starts at the
     * root of the file; for relative names it starts at CWG.
     */
    /* Check if we need to get the root group's entry */
    if('/' == *name) {
        H5G_t *root_grp;         /* Temporary pointer to root group of file */

        /* Look up root group for starting location */
        root_grp = H5G_rootof(_loc->oloc->file);
        HDassert(root_grp);

        /* Set the location entry to the root group's info */
        loc.oloc=&(root_grp->oloc);
        loc.path=&(root_grp->path);
    } /* end if */
    else {
        loc.oloc = _loc->oloc;
        loc.path = _loc->path;
    } /* end else */

    /* Set up group & object locations */
    grp_loc.oloc = &grp_oloc;
    grp_loc.path = &grp_path;
    obj_loc.oloc = &obj_oloc;
    obj_loc.path = &obj_path;

#if defined(H5_USING_MEMCHECKER) || !defined(NDEBUG)
    /* Clear group location */
    if(H5G_loc_reset(&grp_loc) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to reset location")
#endif /* H5_USING_MEMCHECKER */

    /* Deep copy of the starting location to group location */
    if(H5G_loc_copy(&grp_loc, &loc, H5_COPY_DEEP) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to copy location")
    group_copy = TRUE;

    /* Clear object location */
    if(H5G_loc_reset(&obj_loc) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to reset location")

    /* Check for needing a larger buffer for the individual path name components */
    if((HDstrlen(name) + 1) > H5G_comp_alloc_g) {
        char *new_comp;                 /* New component buffer */
        size_t new_alloc;               /* New component buffer size */

        new_alloc = MAX3(1024, (2 * H5G_comp_alloc_g), (HDstrlen(name) + 1));
        if(NULL == (new_comp = (char *)H5MM_realloc(H5G_comp_g, new_alloc)))
            HGOTO_ERROR(H5E_SYM, H5E_NOSPACE, FAIL, "unable to allocate component buffer")
        H5G_comp_g = new_comp;
        H5G_comp_alloc_g = new_alloc;
    } /* end if */

    /* Traverse the path */
    while((name = H5G_component(name, &nchars)) && *name) {
        const char *s;                  /* Temporary string pointer */
        htri_t lookup_status;           /* Status from object lookup */
        hbool_t obj_exists;             /* Whether the object exists */

	/*
	 * Copy the component name into a null-terminated buffer so
	 * we can pass it down to the other symbol table functions.
	 */
	HDmemcpy(H5G_comp_g, name, nchars);
	H5G_comp_g[nchars] = '\0';

	/*
	 * The special name `.' is a no-op.
	 */
	if('.' == H5G_comp_g[0] && !H5G_comp_g[1]) {
	    name += nchars;
	    continue;
	} /* end if */

        /* Check if this is the last component of the name */
        if(!((s = H5G_component(name + nchars, NULL)) && *s))
            last_comp = TRUE;

        /* If there's valid information in the link, reset it */
        if(link_valid) {
            H5O_msg_reset(H5O_LINK_ID, &lnk);
            link_valid = FALSE;
        } /* end if */

        /* Get information for object in current group */
        if((lookup_status = H5G_obj_lookup(grp_loc.oloc, H5G_comp_g, &lnk/*out*/, dxpl_id)) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "can't look up component")
        obj_exists = FALSE;

        /* If the lookup was OK, build object location and traverse special links, etc. */
        if(lookup_status) {
            /* Sanity check link and indicate it's valid */
            HDassert(lnk.type >= H5L_TYPE_HARD);
            HDassert(!HDstrcmp(H5G_comp_g, lnk.name));
            link_valid = TRUE;

            /* Build object location from the link */
            if(H5G_link_to_loc(&grp_loc, &lnk, &obj_loc) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "cannot initialize object location")
            obj_loc_valid = TRUE;

            /* Assume object exists */
            obj_exists = TRUE;

            /* Perform any special traversals that the link needs */
            /* (soft links, user-defined links, file mounting, etc.) */
            if(H5G_traverse_special(&grp_loc, &lnk, target, nlinks, last_comp, &obj_loc, &obj_exists, lapl_id, dxpl_id) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_TRAVERSE, FAIL, "special link traversal failed")
        } /* end if */

        /* Check for last component in name provided */
        if(last_comp) {
            H5O_link_t         *cb_lnk;    /* Pointer to link info for callback */
            H5G_loc_t          *cb_loc;    /* Pointer to object location for callback */

            /* Set callback parameters appropriately, based on link being found */
            if(lookup_status) {
                cb_lnk = &lnk;
                if(obj_exists)
                    cb_loc = &obj_loc;
                else
                    cb_loc = NULL;
            } /* end if */
            else {
                HDassert(!obj_loc_valid);
                cb_lnk = NULL;
                cb_loc = NULL;
            } /* end else */

            /* Call 'operator' routine */
            if((op)(&grp_loc, H5G_comp_g, cb_lnk, cb_loc, op_data, &own_loc) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CALLBACK, FAIL, "traversal operator failed")

            HGOTO_DONE(SUCCEED)
        } /* end if */

        /* Handle lookup failures now */
        if(!lookup_status) {
            /* If an intermediate group doesn't exist & flag is set, create the group */
            if(target & H5G_CRT_INTMD_GROUP) {
                const H5O_ginfo_t def_ginfo = H5G_CRT_GROUP_INFO_DEF;   /* Default group info settings */
                const H5O_linfo_t def_linfo = H5G_CRT_LINK_INFO_DEF;    /* Default link info settings */
                const H5O_pline_t def_pline = H5O_CRT_PIPELINE_DEF;     /* Default filter pipeline settings */
                H5O_ginfo_t	par_ginfo;	/* Group info settings for parent group */
                H5O_linfo_t	par_linfo;	/* Link info settings for parent group */
                H5O_pline_t	par_pline;	/* Filter pipeline settings for parent group */
                H5O_linfo_t	tmp_linfo;	/* Temporary link info settings */
                htri_t          exists;         /* Whether a group or link info message exists */
                const H5O_ginfo_t *ginfo;	/* Group info settings for new group */
                const H5O_linfo_t *linfo;	/* Link info settings for new group */
                const H5O_pline_t *pline;	/* Filter pipeline settings for new group */

                /* Check for the parent group having a group info message */
                /* (OK if not found) */
                if((exists = H5O_msg_exists(grp_loc.oloc, H5O_GINFO_ID, dxpl_id)) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "unable to read object header")
                if(exists) {
                    /* Get the group info for parent group */
                    if(NULL == H5O_msg_read(grp_loc.oloc, H5O_GINFO_ID, &par_ginfo, dxpl_id))
                        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "group info message not present")

                    /* Use parent group info settings */
                    ginfo = &par_ginfo;
                } /* end if */
                else
                    /* Use default group info settings */
                    ginfo = &def_ginfo;

                /* Check for the parent group having a link info message */
                /* (OK if not found) */
                /* Get the link info for parent group */
                if((exists = H5G_obj_get_linfo(grp_loc.oloc, &par_linfo, dxpl_id)) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "unable to read object header")
                if(exists) {
                    /* Only keep the creation order information from the parent
                     *  group's link info
                     */
                    HDmemcpy(&tmp_linfo, &def_linfo, sizeof(H5O_linfo_t));
                    tmp_linfo.track_corder = par_linfo.track_corder;
                    tmp_linfo.index_corder = par_linfo.index_corder;
                    linfo = &tmp_linfo;
                } /* end if */
                else
                    /* Use default link info settings */
                    linfo = &def_linfo;

                /* Check for the parent group having a filter pipeline message */
                /* (OK if not found) */
                if((exists = H5O_msg_exists(grp_loc.oloc, H5O_PLINE_ID, dxpl_id)) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "unable to read object header")
                if(exists) {
                    /* Get the filter pipeline for parent group */
                    if(NULL == H5O_msg_read(grp_loc.oloc, H5O_PLINE_ID, &par_pline, dxpl_id))
                        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "filter pipeline message not present")

                    /* Use parent filter pipeline settings */
                    pline = &par_pline;
                } /* end if */
                else
                    /* Use default filter pipeline settings */
                    pline = &def_pline;

                /* Create the intermediate group */
/* XXX: Should we allow user to control the group creation params here? -QAK */
                if(H5G_obj_create_real(grp_oloc.file, dxpl_id, ginfo, linfo, pline, H5P_GROUP_CREATE_DEFAULT, obj_loc.oloc/*out*/) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to create group entry")

                /* Insert new group into current group's symbol table */
                if(H5G_loc_insert(&grp_loc, H5G_comp_g, &obj_loc, dxpl_id) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTINSERT, FAIL, "unable to insert intermediate group")

                /* Close new group */
                if(H5O_close(obj_loc.oloc) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to close")

                /* If the parent group was holding the file open, the
                 * newly-created group should, as well.
                 */
                if(grp_loc.oloc->holding_file)
                    if(H5O_loc_hold_file(obj_loc.oloc) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_LINKCOUNT, FAIL, "unable to hold file open")

                /* Reset any non-default object header messages */
                if(ginfo != &def_ginfo)
                    if(H5O_msg_reset(H5O_GINFO_ID, ginfo) < 0)
                        HGOTO_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "unable to reset group info message")
                if(linfo != &def_linfo)
                    if(H5O_msg_reset(H5O_LINFO_ID, linfo) < 0)
                        HGOTO_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "unable to reset link info message")
                if(pline != &def_pline)
                    if(H5O_msg_reset(H5O_PLINE_ID, pline) < 0)
                        HGOTO_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "unable to reset I/O pipeline message")
            } /* end if */
            else
                HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "component not found")
        } /* end if */

	/*
	 * Advance to the next component of the path.
	 */

        /* Transfer "ownership" of the object's information to the group object */
        H5G_loc_free(&grp_loc);
        H5G_loc_copy(&grp_loc, &obj_loc, H5_COPY_SHALLOW);
        H5G_loc_reset(&obj_loc);
        obj_loc_valid = FALSE;

	/* Advance to next component in string */
	name += nchars;
    } /* end while */

    /* Call 'operator' routine */
    /* If we've fallen through to here, the name must be something like just '.'
     * and we should issue the callback on that. -QAK
     * Since we don't have a group location or a link to the object we pass in
     * NULL.
     */
    HDassert(group_copy);
    if((op)(NULL, ".", NULL, &grp_loc, op_data, &own_loc) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTNEXT, FAIL, "traversal operator failed")

    /* If the callback took ownership of the object location, it actually has
     * ownership of grp_loc.  It shouldn't have tried to take ownership of
     * the "group location", which was NULL. */
     HDassert(!(own_loc & H5G_OWN_GRP_LOC));
     if(own_loc & H5G_OWN_OBJ_LOC)
         own_loc |= H5G_OWN_GRP_LOC;

done:
    /* If there's been an error, the callback doesn't really get ownership of
     * any location and we should close them both */
    if(ret_value < 0)
        own_loc = H5G_OWN_NONE;

    /* Free all open locations.  This also closes any open external files. */
    if(obj_loc_valid && !(own_loc & H5G_OWN_OBJ_LOC))
        H5G_loc_free(&obj_loc);
    if(group_copy && !(own_loc & H5G_OWN_GRP_LOC))
        H5G_loc_free(&grp_loc);

    /* If there's valid information in the link, reset it */
    if(link_valid)
        if(H5O_msg_reset(H5O_LINK_ID, &lnk) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "unable to reset link message")

   FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_traverse_real() */


/*-------------------------------------------------------------------------
 * Function:	H5G_traverse
 *
 * Purpose:	Traverse a path from a location & perform an operation when
 *              the last component of the name is reached.
 *
 * Return:	Success:	Non-negative if path can be fully traversed.
 *		Failure:	Negative if the path could not be fully
 *				traversed.
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Sep 13 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_traverse(const H5G_loc_t *loc, const char *name, unsigned target, H5G_traverse_t op,
    void *op_data, hid_t lapl_id, hid_t dxpl_id)
{
    size_t	    nlinks;                 /* Link countdown value */
    H5P_genplist_t *lapl;                   /* Property list with value for nlinks */
    herr_t          ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5G_traverse, FAIL)

    /* Check args */
    if(!name || !*name)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "no name given")
    if(!loc)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "no starting location")
    if(!op)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "no operation provided")
    HDassert(lapl_id >= 0);

    /* Set nlinks value from property list, if it exists */
    if(lapl_id == H5P_DEFAULT)
        nlinks = H5L_NUM_LINKS;
    else {
        if(NULL == (lapl = (H5P_genplist_t *)H5I_object(lapl_id)))
            HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")
        if(H5P_get(lapl, H5L_ACS_NLINKS_NAME, &nlinks) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get number of links")
    } /* end else */

    /* Go perform "real" traversal */
    if(H5G_traverse_real(loc, name, target, &nlinks, op, op_data, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "internal path traversal failed")

done:
   FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_traverse() */

