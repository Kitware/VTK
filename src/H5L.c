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

/****************/
/* Module Setup */
/****************/

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */
#define H5G_PACKAGE		/*suppress error about including H5Gpkg   */
#define H5L_PACKAGE		/*suppress error about including H5Lpkg   */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5L_init_interface

/***********/
/* Headers */
/***********/
#include "H5private.h"          /* Generic Functions                    */
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5Dprivate.h"         /* Datasets                             */
#include "H5Eprivate.h"         /* Error handling                       */
#include "H5Gpkg.h"             /* Groups                               */
#include "H5Fpkg.h"             /* File access                          */
#include "H5Iprivate.h"         /* IDs                                  */
#include "H5Lpkg.h"             /* Links                                */
#include "H5MMprivate.h"        /* Memory management                    */
#include "H5Oprivate.h"         /* File objects                         */
#include "H5Pprivate.h"         /* Property lists                       */

/****************/
/* Local Macros */
/****************/

#define H5L_MIN_TABLE_SIZE 32 /* Minimum size of the user-defined link type table if it is allocated */


/******************/
/* Local Typedefs */
/******************/

/* User data for path traversal routine for getting link info by name */
typedef struct {
    H5L_info_t      *linfo;             /* Buffer to return to user */
    hid_t           dxpl_id;            /* DXPL to use in callback */
} H5L_trav_gi_t;

/* User data for path traversal routine for getting link info by index */
typedef struct {
    /* In */
    H5_index_t idx_type;               /* Index to use */
    H5_iter_order_t order;              /* Order to iterate in index */
    hsize_t n;                          /* Offset of link within index */
    hid_t dxpl_id;                      /* DXPL to use in callback */

    /* Out */
    H5L_info_t      *linfo;             /* Buffer to return to user */
} H5L_trav_gibi_t;

/* User data for path traversal callback to creating a link */
typedef struct {
    H5F_t *file;                        /* Pointer to the file */
    H5P_genplist_t *lc_plist;           /* Link creation property list */
    hid_t dxpl_id;                      /* Dataset transfer property list */
    H5G_name_t *path;                   /* Path to object being linked */
    H5O_obj_create_t *ocrt_info;        /* Pointer to object creation info */
    H5O_link_t *lnk;                    /* Pointer to link information to insert */
} H5L_trav_cr_t;

/* User data for path traversal routine for moving and renaming a link */
typedef struct {
    const char *dst_name;               /* Destination name for moving object */
    H5T_cset_t cset;                    /* Char set for new name */
    H5G_loc_t  *dst_loc;		/* Destination location for moving object */
    unsigned dst_target_flags;          /* Target flags for destination object */
    hbool_t copy;                       /* TRUE if this is a copy operation */
    hid_t lapl_id;                      /* LAPL to use in callback */
    hid_t dxpl_id;                      /* DXPL to use in callback */
} H5L_trav_mv_t;

/* User data for path traversal routine for moving and renaming an object */
typedef struct {
    H5F_t *file;                        /* Pointer to the file */
    H5O_link_t *lnk;                    /* Pointer to link information to insert */
    hbool_t copy;                       /* TRUE if this is a copy operation */
    hid_t dxpl_id;                      /* Dataset transfer property list */
} H5L_trav_mv2_t;

/* User data for path traversal routine for getting link value */
typedef struct {
    size_t size;                        /* Size of user buffer */
    void *buf;                          /* User buffer */
} H5L_trav_gv_t;

/* User data for path traversal routine for getting link value by index */
typedef struct {
    /* In */
    H5_index_t idx_type;               /* Index to use */
    H5_iter_order_t order;              /* Order to iterate in index */
    hsize_t n;                          /* Offset of link within index */
    hid_t dxpl_id;                      /* DXPL to use in callback */
    size_t size;                        /* Size of user buffer */

    /* Out */
    void *buf;                          /* User buffer */
} H5L_trav_gvbi_t;

/* User data for path traversal routine for removing link */
typedef struct {
    hid_t dxpl_id;                      /* DXPL to use in callback */
} H5L_trav_rm_t;

/* User data for path traversal routine for removing link by index */
typedef struct {
    /* In */
    H5_index_t idx_type;               /* Index to use */
    H5_iter_order_t order;              /* Order to iterate in index */
    hsize_t n;                          /* Offset of link within index */
    hid_t dxpl_id;                      /* DXPL to use in callback */
} H5L_trav_rmbi_t;

/* User data for path traversal routine for getting name by index */
typedef struct {
    /* In */
    H5_index_t idx_type;                /* Index to use */
    H5_iter_order_t order;              /* Order to iterate in index */
    hsize_t n;                          /* Offset of link within index */
    size_t size;                        /* Size of name buffer */
    hid_t dxpl_id;                      /* DXPL to use in callback */

    /* Out */
    char *name;                         /* Buffer to return name to user */
    ssize_t name_len;                   /* Length of full name */
} H5L_trav_gnbi_t;

/********************/
/* Local Prototypes */
/********************/

static int H5L_find_class_idx(H5L_type_t id);
static herr_t H5L_link_cb(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);
static herr_t H5L_create_real(const H5G_loc_t *link_loc, const char *link_name,
    H5G_name_t *obj_path, H5F_t *obj_file, H5O_link_t *lnk, H5O_obj_create_t *ocrt_info,
    hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id);
static herr_t H5L_get_val_real(const H5O_link_t *lnk, void *buf, size_t size);
static herr_t H5L_get_val_cb(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);
static herr_t H5L_get_val_by_idx_cb(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);
static herr_t H5L_delete_cb(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);
static herr_t H5L_delete_by_idx_cb(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);
static herr_t H5L_move_cb(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);
static herr_t H5L_move_dest_cb(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);
static herr_t H5L_exists_cb(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);
static htri_t H5L_exists(const H5G_loc_t *loc, const char *name, hid_t lapl_id,
    hid_t dxpl_id);
static herr_t H5L_get_info_cb(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);
static herr_t H5L_get_info_by_idx_cb(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);
static herr_t H5L_get_name_by_idx_cb(H5G_loc_t *grp_loc/*in*/,
    const char *name, const H5O_link_t *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/);

/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Information about user-defined links */
static size_t           H5L_table_alloc_g = 0;
static size_t           H5L_table_used_g = 0;
static H5L_class_t      *H5L_table_g = NULL;


/*-------------------------------------------------------------------------
 * Function:	H5L_init
 *
 * Purpose:	Initialize the interface from some other package.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	James Laird
 *              Thursday, July 13, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_init(void)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5L_init, FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_init() */


/*-------------------------------------------------------------------------
 * Function:	H5L_init_interface
 *
 * Purpose:	Initialize information specific to H5L interface.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Tuesday, January 24, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_init_interface(void)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_init_interface)

    /* Initialize user-defined link classes */
    if(H5L_register_external() < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "unable to register external link class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_init_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5L_term_interface
 *
 * Purpose:	Terminate any resources allocated in H5L_init_interface.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Tuesday, January 24, 2006
 *
 *-------------------------------------------------------------------------
 */
int
H5L_term_interface(void)
{
    int	n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5L_term_interface)

    /* Free the table of link types */
    H5L_table_g = (H5L_class_t *)H5MM_xfree(H5L_table_g);
    H5L_table_used_g = H5L_table_alloc_g = 0;

    /* Mark the interface as uninitialized */
    H5_interface_initialize_g = 0;

    FUNC_LEAVE_NOAPI(n)
} /* H5L_term_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Lmove
 *
 * Purpose:	Renames an object within an HDF5 file and moves it to a new
 *              group.  The original name SRC is unlinked from the group graph
 *              and then inserted with the new name DST (which can specify a
 *              new path for the object) as an atomic operation. The names
 *              are interpreted relative to SRC_LOC_ID and
 *              DST_LOC_ID, which are either file IDs or group ID.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Wednesday, March 29, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lmove(hid_t src_loc_id, const char *src_name, hid_t dst_loc_id,
    const char *dst_name, hid_t lcpl_id, hid_t lapl_id)
{
    H5G_loc_t	src_loc, *src_loc_p;
    H5G_loc_t	dst_loc, *dst_loc_p;
    herr_t      ret_value=SUCCEED;              /* Return value */

    FUNC_ENTER_API(H5Lmove, FAIL)
    H5TRACE6("e", "i*si*sii", src_loc_id, src_name, dst_loc_id, dst_name, lcpl_id,
             lapl_id);

    /* Check arguments */
    if(src_loc_id == H5L_SAME_LOC && dst_loc_id == H5L_SAME_LOC)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "source and destination should not both be H5L_SAME_LOC")
    if(src_loc_id != H5L_SAME_LOC && H5G_loc(src_loc_id, &src_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(dst_loc_id != H5L_SAME_LOC && H5G_loc(dst_loc_id, &dst_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!src_name || !*src_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no current name specified")
    if(!dst_name || !*dst_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no destination name specified")
    if(lcpl_id != H5P_DEFAULT && (TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a link creation property list")

    /* Set up src & dst location pointers */
    src_loc_p = &src_loc;
    dst_loc_p = &dst_loc;
    if(src_loc_id == H5L_SAME_LOC)
        src_loc_p = dst_loc_p;
    else if(dst_loc_id == H5L_SAME_LOC)
        dst_loc_p = src_loc_p;

    /* Move the link */
    if(H5L_move(src_loc_p, src_name, dst_loc_p, dst_name, FALSE, lcpl_id,
            lapl_id, H5AC_dxpl_id) < 0)
	HGOTO_ERROR(H5E_LINK, H5E_CANTMOVE, FAIL, "unable to move link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lmove() */


/*-------------------------------------------------------------------------
 * Function:	H5Lcopy
 *
 * Purpose:	Creates an identical copy of a link with the same creation
 *              time and target.  The new link can have a different name
 *              and be in a different location than the original.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Wednesday, March 29, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lcopy(hid_t src_loc_id, const char *src_name, hid_t dst_loc_id,
    const char *dst_name, hid_t lcpl_id, hid_t lapl_id)
{
    H5G_loc_t	src_loc, *src_loc_p;
    H5G_loc_t	dst_loc, *dst_loc_p;
    herr_t      ret_value=SUCCEED;              /* Return value */

    FUNC_ENTER_API(H5Lcopy, FAIL)
    H5TRACE6("e", "i*si*sii", src_loc_id, src_name, dst_loc_id, dst_name, lcpl_id,
             lapl_id);

    /* Check arguments */
    if(src_loc_id == H5L_SAME_LOC && dst_loc_id == H5L_SAME_LOC)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "source and destination should not both be H5L_SAME_LOC")
    if(src_loc_id != H5L_SAME_LOC && H5G_loc(src_loc_id, &src_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(dst_loc_id != H5L_SAME_LOC && H5G_loc(dst_loc_id, &dst_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!src_name || !*src_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no current name specified")
    if(!dst_name || !*dst_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no destination name specified")
    if(lcpl_id != H5P_DEFAULT && (TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a link creation property list")

    /* Set up src & dst location pointers */
    src_loc_p = &src_loc;
    dst_loc_p = &dst_loc;
    if(src_loc_id == H5L_SAME_LOC)
        src_loc_p = dst_loc_p;
    else if(dst_loc_id == H5L_SAME_LOC)
        dst_loc_p = src_loc_p;

    /* Copy the link */
    if(H5L_move(src_loc_p, src_name, dst_loc_p, dst_name, TRUE, lcpl_id,
                lapl_id, H5AC_dxpl_id) < 0)
	HGOTO_ERROR(H5E_LINK, H5E_CANTMOVE, FAIL, "unable to move link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lcopy() */


/*-------------------------------------------------------------------------
 * Function:	H5Lcreate_soft
 *
 * Purpose:	Creates a soft link from LINK_NAME to LINK_TARGET.
 *
 * 		LINK_TARGET can be anything and is interpreted at lookup
 *              time relative to the group which contains the final component
 *              of LINK_NAME.  For instance, if LINK_TARGET is `./foo' and
 *              LINK_NAME is `./x/y/bar' and a request is made for `./x/y/bar'
 *              then the actual object looked up is `./x/y/./foo'.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, April  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lcreate_soft(const char *link_target,
    hid_t link_loc_id, const char *link_name, hid_t lcpl_id, hid_t lapl_id)
{
    H5G_loc_t	link_loc;               /* Group location for new link */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(H5Lcreate_soft, FAIL)
    H5TRACE5("e", "*si*sii", link_target, link_loc_id, link_name, lcpl_id, lapl_id);

    /* Check arguments */
    if(H5G_loc(link_loc_id, &link_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!link_target || !*link_target)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no target specified")
    if(!link_name || !*link_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no new name specified")
    if(lcpl_id != H5P_DEFAULT && (TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a link creation property list")

    /* Create the link */
    if(H5L_create_soft(link_target, &link_loc, link_name, lcpl_id, lapl_id, H5AC_dxpl_id) < 0)
	HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lcreate_soft() */


/*-------------------------------------------------------------------------
 * Function:	H5Lcreate_hard
 *
 * Purpose:	Creates a hard link from NEW_NAME to CUR_NAME.
 *
 *		CUR_NAME must name an existing object.  CUR_NAME and
 *              NEW_NAME are interpreted relative to CUR_LOC_ID and
 *              NEW_LOC_ID, which are either file IDs or group IDs.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, April  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lcreate_hard(hid_t cur_loc_id, const char *cur_name,
    hid_t new_loc_id, const char *new_name, hid_t lcpl_id, hid_t lapl_id)
{
    H5G_loc_t	cur_loc, *cur_loc_p;
    H5G_loc_t	new_loc, *new_loc_p;
    herr_t      ret_value = SUCCEED;            /* Return value */

    FUNC_ENTER_API(H5Lcreate_hard, FAIL)
    H5TRACE6("e", "i*si*sii", cur_loc_id, cur_name, new_loc_id, new_name, lcpl_id,
             lapl_id);

    /* Check arguments */
    if(cur_loc_id == H5L_SAME_LOC && new_loc_id == H5L_SAME_LOC)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "source and destination should not be both H5L_SAME_LOC")
    if(cur_loc_id != H5L_SAME_LOC && H5G_loc(cur_loc_id, &cur_loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(new_loc_id != H5L_SAME_LOC && H5G_loc(new_loc_id, &new_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!cur_name || !*cur_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no current name specified")
    if(!new_name || !*new_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no new name specified")
    if(lcpl_id != H5P_DEFAULT && (TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a link creation property list")

    /* Set up current & new location pointers */
    cur_loc_p = &cur_loc;
    new_loc_p = &new_loc;
    if(cur_loc_id == H5L_SAME_LOC)
        cur_loc_p = new_loc_p;
    else if(new_loc_id == H5L_SAME_LOC)
   	new_loc_p = cur_loc_p;
    else if(cur_loc_p->oloc->file != new_loc_p->oloc->file)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "source and destination should be in the same file.")

    /* Create the link */
    if(H5L_create_hard(cur_loc_p, cur_name, new_loc_p, new_name,
                lcpl_id, lapl_id, H5AC_dxpl_id) < 0)
	HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lcreate_hard() */


/*-------------------------------------------------------------------------
 * Function:	H5Lcreate_ud
 *
 * Purpose:	Creates a user-defined link of type LINK_TYPE named LINK_NAME
 *              with user-specified data UDATA.
 *
 *		The format of the information pointed to by UDATA is
 *              defined by the user. UDATA_SIZE holds the size of this buffer.
 *
 *		LINK_NAME is interpreted relative to LINK_LOC_ID.
 *
 *		The property list specified by LCPL_ID holds properties used
 *              to create the link.
 *
 *              The link class of the new link must already be registered
 *              with the library.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Tuesday, December 13, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lcreate_ud(hid_t link_loc_id, const char *link_name, H5L_type_t link_type,
    const void *udata, size_t udata_size, hid_t lcpl_id, hid_t lapl_id)
{
    H5G_loc_t	link_loc;
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Lcreate_ud, FAIL)
    H5TRACE7("e", "i*sLl*xzii", link_loc_id, link_name, link_type, udata,
             udata_size, lcpl_id, lapl_id);

    /* Check arguments */
    if(H5G_loc(link_loc_id, &link_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!link_name || !*link_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no link name specified")
    if(link_type < H5L_TYPE_UD_MIN || link_type > H5L_TYPE_MAX)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid link class")

    /* Create external link */
    if(H5L_create_ud(&link_loc, link_name, udata, udata_size, link_type, lcpl_id, lapl_id, H5AC_dxpl_id) < 0)
	HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lcreate_ud() */


/*-------------------------------------------------------------------------
 * Function:	H5Ldelete
 *
 * Purpose:	Removes the specified NAME from the group graph and
 *		decrements the link count for the object to which NAME
 *		points.  If the link count reaches zero then all file-space
 *		associated with the object will be reclaimed (but if the
 *		object is open, then the reclamation of the file space is
 *		delayed until all handles to the object are closed).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, April  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ldelete(hid_t loc_id, const char *name, hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Group's location */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(H5Ldelete, FAIL)
    H5TRACE3("e", "i*si", loc_id, name, lapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Unlink */
    if(H5L_delete(&loc, name, lapl_id, H5AC_dxpl_id) < 0)
	HGOTO_ERROR(H5E_LINK, H5E_CANTDELETE, FAIL, "unable to delete link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ldelete() */


/*-------------------------------------------------------------------------
 * Function:	H5Ldelete_by_idx
 *
 * Purpose:	Removes the specified link from the group graph and
 *		decrements the link count for the object to which it
 *		points, according to the order within an index.
 *
 *		If the link count reaches zero then all file-space
 *		associated with the object will be reclaimed (but if the
 *		object is open, then the reclamation of the file space is
 *		delayed until all handles to the object are closed).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 13, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ldelete_by_idx(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n, hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Group's location */
    H5L_trav_rmbi_t udata;              /* User data for callback */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(H5Ldelete_by_idx, FAIL)
    H5TRACE6("e", "i*sIiIohi", loc_id, group_name, idx_type, order, n, lapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!group_name || !*group_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Set up user data for unlink operation */
    udata.idx_type = idx_type;
    udata.order = order;
    udata.n = n;
    udata.dxpl_id = H5AC_dxpl_id;

    /* Traverse the group hierarchy to remove the link */
    if(H5G_traverse(&loc, group_name, H5G_TARGET_SLINK|H5G_TARGET_UDLINK|H5G_TARGET_MOUNT, H5L_delete_by_idx_cb, &udata, lapl_id, H5AC_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_EXISTS, FAIL, "name doesn't exist")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ldelete_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5Lget_val
 *
 * Purpose:	Returns the link value of a link whose name is NAME.  For
 *              symbolic links, this is the path to which the link points,
 *              including the null terminator.  For user-defined links, it
 *              is the link buffer.
 *
 *              At most SIZE bytes are copied to the BUF result buffer.
 *
 * Return:	Success:	Non-negative with the link value in BUF.
 *
 * 		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Monday, April 13, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lget_val(hid_t loc_id, const char *name, void *buf/*out*/, size_t size,
    hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Group location for location to query */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(H5Lget_val, FAIL)
    H5TRACE5("e", "i*sxzi", loc_id, name, buf, size, lapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Get the link value */
    if(H5L_get_val(&loc, name, buf, size, lapl_id, H5AC_ind_dxpl_id) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to get link value for '%s'", name)

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lget_val() */


/*-------------------------------------------------------------------------
 * Function:	H5Lget_val_by_idx
 *
 * Purpose:	Returns the link value of a link, according to the order of
 *              an index.  For symbolic links, this is the path to which the
 *              link points, including the null terminator.  For user-defined
 *              links, it is the link buffer.
 *
 *              At most SIZE bytes are copied to the BUF result buffer.
 *
 * Return:	Success:	Non-negative with the link value in BUF.
 * 		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 13, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lget_val_by_idx(hid_t loc_id, const char *group_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, void *buf/*out*/, size_t size,
    hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Group location for location to query */
    H5L_trav_gvbi_t udata;              /* User data for callback */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(H5Lget_val_by_idx, FAIL)
    H5TRACE8("e", "i*sIiIohxzi", loc_id, group_name, idx_type, order, n, buf, size,
             lapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!group_name || !*group_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Set up user data for retrieving information */
    udata.idx_type = idx_type;
    udata.order = order;
    udata.n = n;
    udata.dxpl_id = H5AC_ind_dxpl_id;
    udata.buf = buf;
    udata.size = size;

    /* Traverse the group hierarchy to locate the object to get info about */
    if(H5G_traverse(&loc, group_name, H5G_TARGET_SLINK | H5G_TARGET_UDLINK, H5L_get_val_by_idx_cb, &udata, lapl_id, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "name doesn't exist")


done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lget_val_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5Lexists
 *
 * Purpose:	Checks if a link of a given name exists in a group
 *
 * Return:	Success:	TRUE/FALSE
 * 		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Friday, March 16, 2007
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Lexists(hid_t loc_id, const char *name, hid_t lapl_id)
{
    H5G_loc_t	loc;
    htri_t ret_value;

    FUNC_ENTER_API(H5Lexists, FAIL)
    H5TRACE3("t", "i*si", loc_id, name, lapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Check for the existence of the link */
    if((ret_value = H5L_exists(&loc, name, lapl_id, H5AC_ind_dxpl_id)) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to get link info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lexists() */


/*-------------------------------------------------------------------------
 * Function:	H5Lget_info
 *
 * Purpose:	Gets metadata for a link.
 *
 * Return:	Success:	Non-negative with information in LINFO
 *
 * 		Failure:	Negative
 *
 * Programmer:	James Laird
 *              Wednesday, June 21, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lget_info(hid_t loc_id, const char *name, H5L_info_t *linfo /*out*/,
    hid_t lapl_id)
{
    H5G_loc_t	loc;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API(H5Lget_info, FAIL)
    H5TRACE4("e", "i*sxi", loc_id, name, linfo, lapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Get the link information */
    if(H5L_get_info(&loc, name, linfo, lapl_id, H5AC_ind_dxpl_id) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to get link info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lget_info() */


/*-------------------------------------------------------------------------
 * Function:	H5Lget_info_by_idx
 *
 * Purpose:	Gets metadata for a link, according to the order within an
 *              index.
 *
 * Return:	Success:	Non-negative with information in LINFO
 * 		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, November  6, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lget_info_by_idx(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
    H5L_info_t *linfo /*out*/, hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Group location for group to query */
    H5L_trav_gibi_t udata;              /* User data for callback */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(H5Lget_info_by_idx, FAIL)
    H5TRACE7("e", "i*sIiIohxi", loc_id, group_name, idx_type, order, n, linfo,
             lapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!group_name || !*group_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Set up user data for callback */
    udata.idx_type = idx_type;
    udata.order = order;
    udata.n = n;
    udata.dxpl_id = H5AC_ind_dxpl_id;
    udata.linfo = linfo;

    /* Traverse the group hierarchy to locate the object to get info about */
    if(H5G_traverse(&loc, group_name, H5G_TARGET_SLINK|H5G_TARGET_UDLINK, H5L_get_info_by_idx_cb, &udata, lapl_id, H5AC_ind_dxpl_id) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to get link info")


done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lget_info_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5Lregister
 *
 * Purpose:	Registers a class of user-defined links, or changes the
 *              behavior of an existing class.
 *
 *              The link class passed in will override any existing link
 *              class for the specified link class ID. It must at least
 *              include a H5L_class_t version (which should be
 *              H5L_LINK_CLASS_T_VERS), a link class ID, and a traversal
 *              function.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lregister(const H5L_class_t *cls)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Lregister, FAIL)
    H5TRACE1("e", "*x", cls);

    /* Check args */
    if(cls == NULL)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid link class")

    /* Check H5L_class_t version number; this is where a function to convert
     * from an outdated version should be called.
     */
    if(cls->version != H5L_LINK_CLASS_T_VERS)
      HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid H5L_class_t version number")

    if(cls->id < H5L_TYPE_UD_MIN || cls->id > H5L_TYPE_MAX)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid link identification number")
    if(cls->trav_func == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no traversal function specified")

    /* Do it */
    if(H5L_register(cls) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "unable to register link type")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lregister() */


/*-------------------------------------------------------------------------
 * Function:	H5Lunregister
 *
 * Purpose:	Unregisters a class of user-defined links, preventing them
 *              from being traversed, queried, moved, etc.
 *
 *              A link class can be re-registered using H5Lregister().
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lunregister(H5L_type_t id)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Lunregister, FAIL)
    H5TRACE1("e", "Ll", id);

    /* Check args */
    if(id < 0 || id > H5L_TYPE_MAX)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid link type")

    /* Do it */
    if(H5L_unregister(id) < 0)
	HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "unable to unregister link type")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lunregister() */


/*-------------------------------------------------------------------------
 * Function:	H5Lis_registered
 *
 * Purpose:	Tests whether a user-defined link class has been registered
 *              or not.
 *
 * Return:	Positive if the link class has been registered
 *              Zero if it is unregistered
 *              Negative on error (if the class is not a valid UD class ID)
 *
 * Programmer:	James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Lis_registered(H5L_type_t id)
{
    size_t i;                   /* Local index variable */
    htri_t ret_value = FALSE;     /* Return value */

    FUNC_ENTER_API(H5Lis_registered, FAIL)
    H5TRACE1("t", "Ll", id);

    /* Check args */
    if(id < 0 || id > H5L_TYPE_MAX)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid link type id number")

    /* Is the link class already registered? */
    for(i = 0; i < H5L_table_used_g; i++)
	if(H5L_table_g[i].id == id) {
            ret_value = TRUE;
            break;
        } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lis_registered() */


/*-------------------------------------------------------------------------
 * Function:	H5Lget_name_by_idx
 *
 * Purpose:	Gets name for a link, according to the order within an
 *              index.
 *
 *              Same pattern of behavior as H5Iget_name.
 *
 * Return:	Success:	Non-negative length of name, with information
 *				in NAME buffer
 * 		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, November 11, 2006
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Lget_name_by_idx(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
    char *name /*out*/, size_t size, hid_t lapl_id)
{
    H5G_loc_t	loc;            /* Location of group */
    H5L_trav_gnbi_t udata;      /* User data for callback */
    ssize_t ret_value;          /* Return value */

    FUNC_ENTER_API(H5Lget_name_by_idx, FAIL)
    H5TRACE8("Zs", "i*sIiIohxzi", loc_id, group_name, idx_type, order, n, name, size,
             lapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!group_name || !*group_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Set up user data for callback */
    udata.idx_type = idx_type;
    udata.order = order;
    udata.n = n;
    udata.dxpl_id = H5AC_ind_dxpl_id;
    udata.name = name;
    udata.size = size;
    udata.name_len = -1;

    /* Traverse the group hierarchy to locate the link to get name of */
    if(H5G_traverse(&loc, group_name, H5G_TARGET_SLINK|H5G_TARGET_UDLINK, H5L_get_name_by_idx_cb, &udata, lapl_id, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_EXISTS, FAIL, "name doesn't exist")

    /* Set the return value */
    ret_value = udata.name_len;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lget_name_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5Literate
 *
 * Purpose:	Iterates over links in a group, with user callback routine,
 *              according to the order within an index.
 *
 *              Same pattern of behavior as H5Giterate.
 *
 * Return:	Success:	The return value of the first operator that
 *				returns non-zero, or zero if all members were
 *				processed with no operator returning non-zero.
 *
 *		Failure:	Negative if something goes wrong within the
 *				library, or the negative value returned by one
 *				of the operators.
 *
 *
 * Programmer:	Quincey Koziol
 *              Thursday, November 16, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Literate(hid_t grp_id, H5_index_t idx_type, H5_iter_order_t order,
    hsize_t *idx_p, H5L_iterate_t op, void *op_data)
{
    H5I_type_t  id_type;        /* Type of ID */
    H5G_link_iterate_t lnk_op;  /* Link operator */
    hsize_t     last_lnk;       /* Index of last object looked at */
    hsize_t	idx;            /* Internal location to hold index */
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(H5Literate, FAIL)
    H5TRACE6("e", "iIiIo*hx*x", grp_id, idx_type, order, idx_p, op, op_data);

    /* Check arguments */
    id_type = H5I_get_type(grp_id);
    if(!(H5I_GROUP == id_type || H5I_FILE == id_type))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(!op)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no operator specified")

    /* Set up iteration beginning/end info */
    idx = (idx_p == NULL ? 0 : *idx_p);
    last_lnk = 0;

    /* Build link operator info */
    lnk_op.op_type = H5G_LINK_OP_NEW;
    lnk_op.op_func.op_new = op;

    /* Iterate over the links */
    if((ret_value = H5G_iterate(grp_id, ".", idx_type, order, idx, &last_lnk, &lnk_op, op_data, H5P_DEFAULT, H5AC_ind_dxpl_id)) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "link iteration failed")

    /* Set the index we stopped at */
    if(idx_p)
        *idx_p = last_lnk;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Literate() */


/*-------------------------------------------------------------------------
 * Function:	H5Literate_by_name
 *
 * Purpose:	Iterates over links in a group, with user callback routine,
 *              according to the order within an index.
 *
 *              Same pattern of behavior as H5Giterate.
 *
 * Return:	Success:	The return value of the first operator that
 *				returns non-zero, or zero if all members were
 *				processed with no operator returning non-zero.
 *
 *		Failure:	Negative if something goes wrong within the
 *				library, or the negative value returned by one
 *				of the operators.
 *
 *
 * Programmer:	Quincey Koziol
 *              Thursday, November 16, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Literate_by_name(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t *idx_p,
    H5L_iterate_t op, void *op_data, hid_t lapl_id)
{
    H5G_link_iterate_t lnk_op;  /* Link operator */
    hsize_t     last_lnk;       /* Index of last object looked at */
    hsize_t	idx;            /* Internal location to hold index */
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(H5Literate_by_name, FAIL)
    H5TRACE8("e", "i*sIiIo*hx*xi", loc_id, group_name, idx_type, order, idx_p, op,
             op_data, lapl_id);

    /* Check arguments */
    if(!group_name || !*group_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(!op)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no operator specified")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Set up iteration beginning/end info */
    idx = (idx_p == NULL ? 0 : *idx_p);
    last_lnk = 0;

    /* Build link operator info */
    lnk_op.op_type = H5G_LINK_OP_NEW;
    lnk_op.op_func.op_new = op;

    /* Iterate over the links */
    if((ret_value = H5G_iterate(loc_id, group_name, idx_type, order, idx, &last_lnk, &lnk_op, op_data, lapl_id, H5AC_ind_dxpl_id)) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "link iteration failed")

    /* Set the index we stopped at */
    if(idx_p)
        *idx_p = last_lnk;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Literate_by_name() */


/*-------------------------------------------------------------------------
 * Function:	H5Lvisit
 *
 * Purpose:	Recursively visit all the links in a group and all
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
 * Return:	Success:	The return value of the first operator that
 *				returns non-zero, or zero if all members were
 *				processed with no operator returning non-zero.
 *
 *		Failure:	Negative if something goes wrong within the
 *				library, or the negative value returned by one
 *				of the operators.
 *
 * Programmer:	Quincey Koziol
 *		November 24 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lvisit(hid_t grp_id, H5_index_t idx_type, H5_iter_order_t order,
    H5L_iterate_t op, void *op_data)
{
    H5I_type_t  id_type;                /* Type of ID */
    herr_t      ret_value;              /* Return value */

    FUNC_ENTER_API(H5Lvisit, FAIL)
    H5TRACE5("e", "iIiIox*x", grp_id, idx_type, order, op, op_data);

    /* Check args */
    id_type = H5I_get_type(grp_id);
    if(!(H5I_GROUP == id_type || H5I_FILE == id_type))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no callback operator specified")

    /* Call internal group visitation routine */
    if((ret_value = H5G_visit(grp_id, ".", idx_type, order, op, op_data, H5P_DEFAULT, H5AC_ind_dxpl_id)) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "link visitation failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lvisit() */


/*-------------------------------------------------------------------------
 * Function:	H5Lvisit_by_name
 *
 * Purpose:	Recursively visit all the links in a group and all
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
 * Return:	Success:	The return value of the first operator that
 *				returns non-zero, or zero if all members were
 *				processed with no operator returning non-zero.
 *
 *		Failure:	Negative if something goes wrong within the
 *				library, or the negative value returned by one
 *				of the operators.
 *
 * Programmer:	Quincey Koziol
 *		November 3 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Lvisit_by_name(hid_t loc_id, const char *group_name, H5_index_t idx_type,
    H5_iter_order_t order, H5L_iterate_t op, void *op_data, hid_t lapl_id)
{
    herr_t      ret_value;              /* Return value */

    FUNC_ENTER_API(H5Lvisit_by_name, FAIL)
    H5TRACE7("e", "i*sIiIox*xi", loc_id, group_name, idx_type, order, op, op_data,
             lapl_id);

    /* Check args */
    if(!group_name || !*group_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no callback operator specified")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Call internal group visitation routine */
    if((ret_value = H5G_visit(loc_id, group_name, idx_type, order, op, op_data, lapl_id, H5AC_ind_dxpl_id)) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "link visitation failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Lvisit_by_name() */

/*
 *-------------------------------------------------------------------------
 *-------------------------------------------------------------------------
 *   N O   A P I   F U N C T I O N S   B E Y O N D   T H I S   P O I N T
 *-------------------------------------------------------------------------
 *-------------------------------------------------------------------------
 */


/*-------------------------------------------------------------------------
 * Function:	H5L_find_class_idx
 *
 * Purpose:	Given a link class ID, return the offset in the global array
 *              that holds all the registered link classes.
 *
 * Return:	Success:	Non-negative index of entry in global
 *                              link class table.
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
static int
H5L_find_class_idx(H5L_type_t id)
{
    size_t i;                   /* Local index variable */
    int ret_value = FAIL;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5L_find_class_idx)

    for(i = 0; i < H5L_table_used_g; i++)
	if(H5L_table_g[i].id == id)
	    HGOTO_DONE((int)i)

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_find_class_idx */


/*-------------------------------------------------------------------------
 * Function:	H5L_find_class
 *
 * Purpose:	Given a link class ID return a pointer to a global struct that
 *		defines the link class.
 *
 * Return:	Success:	Ptr to entry in global link class table.
 *		Failure:	NULL
 *
 * Programmer:	James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
const H5L_class_t *
H5L_find_class(H5L_type_t id)
{
    int	idx;                            /* Filter index in global table */
    H5L_class_t *ret_value = NULL;   /* Return value */

    FUNC_ENTER_NOAPI(H5L_find_class, NULL)

    /* Get the index in the global table */
    if((idx = H5L_find_class_idx(id)) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, NULL, "unable to find link class")

    /* Set return value */
    ret_value = H5L_table_g+idx;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_find_class */


/*-------------------------------------------------------------------------
 * Function:	H5L_register
 *
 * Purpose:	Registers a class of user-defined links, or changes the
 *              behavior of an existing class.
 *
 *              See H5Lregister for full documentation.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_register(const H5L_class_t *cls)
{
    size_t      i;                      /* Local index variable */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5L_register, FAIL)

    HDassert(cls);
    HDassert(cls->id >= 0 && cls->id <= H5L_TYPE_MAX);

    /* Is the link type already registered? */
    for(i = 0; i < H5L_table_used_g; i++)
	if(H5L_table_g[i].id == cls->id)
            break;

    /* Filter not already registered */
    if(i >= H5L_table_used_g) {
	if(H5L_table_used_g >= H5L_table_alloc_g) {
	    size_t n = MAX(H5L_MIN_TABLE_SIZE, (2 * H5L_table_alloc_g));
	    H5L_class_t *table = (H5L_class_t *)H5MM_realloc(H5L_table_g, (n * sizeof(H5L_class_t)));
            if(!table)
		HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to extend link type table")
	    H5L_table_g = table;
            H5L_table_alloc_g = n;
	} /* end if */

	/* Initialize */
	i = H5L_table_used_g++;
    } /* end if */

    /* Copy link class info into table */
    HDmemcpy(H5L_table_g + i, cls, sizeof(H5L_class_t));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_register */


/*-------------------------------------------------------------------------
 * Function:	H5L_unregister
 *
 * Purpose:	Unregisters a class of user-defined links.
 *
 *              See H5Lunregister for full documentation.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, July 10, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_unregister(H5L_type_t id)
{
    size_t i;                           /* Local index variable */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5L_unregister, FAIL)

    HDassert(id >= 0 && id <= H5L_TYPE_MAX);

    /* Is the filter already registered? */
    for(i = 0; i < H5L_table_used_g; i++)
	if(H5L_table_g[i].id == id)
            break;

    /* Fail if filter not found */
    if(i >= H5L_table_used_g)
        HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "link class is not registered")

    /* Remove filter from table */
    /* Don't worry about shrinking table size (for now) */
    HDmemmove(&H5L_table_g[i], &H5L_table_g[i + 1], sizeof(H5L_class_t) * ((H5L_table_used_g - 1) - i));
    H5L_table_used_g--;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_unregister() */


/*-------------------------------------------------------------------------
 * Function:	H5L_link
 *
 * Purpose:	Creates a link from OBJ_ID to CUR_NAME.  See H5Olink() for
 *		full documentation.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Tuesday, December 13, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_link(const H5G_loc_t *new_loc, const char *new_name, H5G_loc_t *obj_loc,
    hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id)
{
    H5O_link_t lnk;                     /* Link to insert */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_link)

    /* Check args */
    HDassert(new_loc);
    HDassert(obj_loc);
    HDassert(new_name && *new_name);

    /* The link callback will check that the object isn't being hard linked
     * into a different file, so we don't need to do it here (there could be
     * external links along the path).
     */

    /* Construct link information for eventual insertion */
    lnk.type = H5L_TYPE_HARD;
    lnk.u.hard.addr = obj_loc->oloc->addr;

    /* Create the link */
    if(H5L_create_real(new_loc, new_name, obj_loc->path, obj_loc->oloc->file, &lnk, NULL, lcpl_id, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link to object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_link() */


/*-------------------------------------------------------------------------
 * Function:	H5L_link_object
 *
 * Purpose:	Creates a new object and a link to it.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, April 9, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_link_object(const H5G_loc_t *new_loc, const char *new_name,
    H5O_obj_create_t *ocrt_info, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id)
{
    H5O_link_t lnk;                     /* Link to insert */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_link_object)

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
    if(H5L_create_real(new_loc, new_name, NULL, NULL, &lnk, ocrt_info, lcpl_id, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link to object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_link_object() */


/*-------------------------------------------------------------------------
 * Function:	H5L_link_cb
 *
 * Purpose:	Callback for creating a link to an object.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 19, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_link_cb(H5G_loc_t *grp_loc/*in*/, const char *name, const H5O_link_t UNUSED *lnk,
    H5G_loc_t *obj_loc, void *_udata/*in,out*/, H5G_own_loc_t *own_loc/*out*/)
{
    H5L_trav_cr_t *udata = (H5L_trav_cr_t *)_udata;   /* User data passed in */
    H5G_t *grp = NULL;              /* H5G_t for this group, opened to pass to user callback */
    hid_t grp_id = FAIL;            /* Id for this group (passed to user callback */
    H5G_loc_t temp_loc;             /* For UD callback */
    hbool_t temp_loc_init = FALSE;
    herr_t ret_value = SUCCEED;              /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_link_cb)

    /* Check if the name in this group resolved to a valid location */
    /* (which is not what we want) */
    if(obj_loc != NULL)
        HGOTO_ERROR(H5E_SYM, H5E_EXISTS, FAIL, "name already exists")

    /* Check for crossing file boundaries with a new hard link */
    if(udata->lnk->type == H5L_TYPE_HARD) {
        /* Check for creating an object */
        /* (only for hard links) */
        if(udata->ocrt_info) {
            H5G_loc_t new_loc;          /* Group location for new object */

            /* Create new object at this location */
            if(NULL == (udata->ocrt_info->new_obj = H5O_obj_create(grp_loc->oloc->file, udata->ocrt_info->obj_type, udata->ocrt_info->crt_info, &new_loc, udata->dxpl_id)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to create object")

            /* Set address for hard link */
            udata->lnk->u.hard.addr = new_loc.oloc->addr;

            /* Set object path to use for setting object name (below) */
            udata->path = new_loc.path;
        } /* end if */
        else {
            /* Check that both objects are in same file */
            if(grp_loc->oloc->file->shared != udata->file->shared)
                HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "interfile hard links are not allowed")
        } /* end else */
    } /* end if */

    /* Set 'standard' aspects of link */
    udata->lnk->corder = 0;            /* Will be re-written during group insertion, if the group is tracking creation order */
    udata->lnk->corder_valid = FALSE;   /* Creation order not valid (yet) */

    /* Check for non-default link creation properties */
    if(udata->lc_plist) {
        /* Get character encoding property */
        if(H5P_get(udata->lc_plist, H5P_STRCRT_CHAR_ENCODING_NAME, &udata->lnk->cset) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get property value for character encoding")
    } /* end if */
    else
        udata->lnk->cset = H5F_DEFAULT_CSET;   /* Default character encoding for link */

    /* Set the link's name correctly */
    /* Casting away const OK -QAK */
    udata->lnk->name = (char *)name;

    /* Insert link into group */
    if(H5G_obj_insert(grp_loc->oloc, name, udata->lnk, TRUE, udata->dxpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link for object")

    /* Set object's path if it has been passed in and is not set */
    if(udata->path != NULL && udata->path->user_path_r == NULL)
        if(H5G_name_set(grp_loc->path, udata->path, name) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "cannot set name")

    /* If link is a user-defined link, trigger its creation callback if it has one */
    if(udata->lnk->type >= H5L_TYPE_UD_MIN) {
        const H5L_class_t   *link_class;         /* User-defined link class */

        /* Get the link class for this type of link. */
        if(NULL == (link_class = H5L_find_class(udata->lnk->type)))
            HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "unable to get class of UD link")

        if(link_class->create_func != NULL) {
            H5O_loc_t           temp_oloc;
            H5G_name_t          temp_path;

            /* Create a temporary location (or else H5G_open will do a shallow
             * copy and wipe out grp_loc)
             */
            H5G_name_reset(&temp_path);
            if(H5O_loc_copy(&temp_oloc, grp_loc->oloc, H5_COPY_DEEP) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTCOPY, FAIL, "unable to copy object location")

            temp_loc.oloc = &temp_oloc;
            temp_loc.path = &temp_path;
            temp_loc_init = TRUE;

            /* Set up location for user-defined callback */
            if((grp = H5G_open(&temp_loc, udata->dxpl_id)) == NULL)
                HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open group")
            if((grp_id = H5I_register(H5I_GROUP, grp, TRUE)) < 0)
                HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register ID for group")

            /* Make callback */
            if((link_class->create_func)(name, grp_id, udata->lnk->u.ud.udata, udata->lnk->u.ud.size, H5P_DEFAULT) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CALLBACK, FAIL, "link creation callback failed")
        } /* end if */
    } /* end if */

done:
    /* Close the location given to the user callback if it was created */
    if(grp_id >= 0) {
        if(H5I_dec_ref(grp_id, TRUE) < 0)
            HDONE_ERROR(H5E_ATOM, H5E_CANTRELEASE, FAIL, "unable to close atom from UD callback")
    } /* end if */
    else if(grp != NULL) {
        if(H5G_close(grp) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "unable to close group given to UD callback")
    } /* end if */
    else if(temp_loc_init)
        H5G_loc_free(&temp_loc);

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_link_cb() */


/*-------------------------------------------------------------------------
 * Function:    H5L_create_real
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
H5L_create_real(const H5G_loc_t *link_loc, const char *link_name,
    H5G_name_t *obj_path, H5F_t *obj_file, H5O_link_t *lnk,
    H5O_obj_create_t *ocrt_info, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id)
{
    char *norm_link_name = NULL;        /* Pointer to normalized link name */
    unsigned target_flags = H5G_TARGET_NORMAL; /* Flags to pass to group traversal function */
    H5P_genplist_t *lc_plist = NULL;   /* Link creation property list */
    H5L_trav_cr_t udata;               /* User data for callback */
    herr_t ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_create_real)

    /* Check args */
    HDassert(link_loc);
    HDassert(link_name && *link_name);
    HDassert(lnk);
    HDassert(lnk->type >= H5L_TYPE_HARD && lnk->type <= H5L_TYPE_MAX);

    /* Get normalized link name */
    if((norm_link_name = H5G_normalize(link_name)) == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "can't normalize name")

    /* Check for flags present in creation property list */
    if(lcpl_id != H5P_DEFAULT) {
        unsigned crt_intmd_group;

        /* Get link creation property list */
        if(NULL == (lc_plist = (H5P_genplist_t *)H5I_object(lcpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

        /* Get intermediate group creation property */
        if(H5P_get(lc_plist, H5L_CRT_INTERMEDIATE_GROUP_NAME, &crt_intmd_group) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get property value for creating missing groups")

        if(crt_intmd_group > 0)
            target_flags |= H5G_CRT_INTMD_GROUP;
    } /* end if */

    /* Set up user data
     * FILE is used to make sure that hard links don't cross files, and
     * should be NULL for other link types.
     * LC_PLIST is a pointer to the link creation property list.
     * DXPL_ID is the dxpl ID that needs to be used during writes and reads.
     * PATH is a pointer to the path of the object being inserted if this is
     * a hard link; this is used to set the paths to objects when they are
     * created.  For other link types, this is NULL.
     * OCRT_INFO is a pointer to the structure for object creation.
     * LNK is the link struct passed into this function.  At this point all
     * of its fields should be populated except for name, which is set when
     * inserting it in the callback.
     */
    udata.file = obj_file;
    udata.lc_plist = lc_plist;
    udata.dxpl_id = dxpl_id;
    udata.path = obj_path;
    udata.ocrt_info = ocrt_info;
    udata.lnk = lnk;

    /* Traverse the destination path & create new link */
    if(H5G_traverse(link_loc, link_name, target_flags, H5L_link_cb, &udata, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINSERT, FAIL, "can't insert link")

done:
    /* Free the normalized path name */
    if(norm_link_name)
        H5MM_xfree(norm_link_name);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_create_real() */


/*-------------------------------------------------------------------------
 * Function:	H5L_create_hard
 *
 * Purpose:	Creates a hard link from NEW_NAME to CUR_NAME.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, April  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_create_hard(H5G_loc_t *cur_loc, const char *cur_name,
    const H5G_loc_t *link_loc, const char *link_name, hid_t lcpl_id,
    hid_t lapl_id, hid_t dxpl_id)
{
    char *norm_cur_name = NULL;	        /* Pointer to normalized current name */
    H5F_t *link_file = NULL;            /* Pointer to file to link to */
    H5O_link_t lnk;                     /* Link to insert */
    H5G_loc_t obj_loc;                  /* Location of object to link to */
    H5G_name_t path;                    /* obj_loc's path*/
    H5O_loc_t oloc;                     /* obj_loc's oloc */
    hbool_t loc_valid = FALSE;
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5L_create_hard, FAIL)

    /* Check args */
    HDassert(cur_loc);
    HDassert(cur_name && *cur_name);
    HDassert(link_loc);
    HDassert(link_name && *link_name);

    /* Get normalized copy of the current name */
    if((norm_cur_name = H5G_normalize(cur_name)) == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "can't normalize name")

    /* Set up link data specific to hard links */
    lnk.type = H5L_TYPE_HARD;

    /* Get object location for object pointed to */
    obj_loc.path = &path;
    obj_loc.oloc = &oloc;
    H5G_loc_reset(&obj_loc);
    if(H5G_loc_find(cur_loc, norm_cur_name, &obj_loc, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "source object not found")
    loc_valid = TRUE;

    /* Construct link information for eventual insertion */
    lnk.u.hard.addr = obj_loc.oloc->addr;

    /* Set destination's file information */
    link_file = obj_loc.oloc->file;

    /* Create actual link to the object.  Pass in NULL for the path, since this
     * function shouldn't change an object's user path. */
    if(H5L_create_real(link_loc, link_name, NULL, link_file, &lnk, NULL, lcpl_id, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link to object")

done:
    /* Free the object header location */
    if(loc_valid)
        if(H5G_loc_free(&obj_loc) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "unable to free location")

    /* Free the normalized path name */
    if(norm_cur_name)
        H5MM_xfree(norm_cur_name);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_create_hard() */


/*-------------------------------------------------------------------------
 * Function:	H5L_create_soft
 *
 * Purpose:	Creates a soft link from LINK_NAME to TARGET_PATH.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, April  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_create_soft(const char *target_path, const H5G_loc_t *link_loc,
    const char *link_name, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id)
{
    char *norm_target = NULL;	        /* Pointer to normalized current name */
    H5O_link_t lnk;                     /* Link to insert */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5L_create_soft, FAIL)

    /* Check args */
    HDassert(link_loc);
    HDassert(target_path && *target_path);
    HDassert(link_name && *link_name);

    /* Get normalized copy of the link target */
    if((norm_target = H5G_normalize(target_path)) == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "can't normalize name")

    /* Set up link data specific to soft links */
    lnk.type = H5L_TYPE_SOFT;
    lnk.u.soft.name = norm_target;

    /* Create actual link to the object */
    if(H5L_create_real(link_loc, link_name, NULL, NULL, &lnk, NULL, lcpl_id, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link to object")

done:
    /* Free the normalized target name */
    if(norm_target)
        H5MM_xfree(norm_target);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_create_soft() */


/*-------------------------------------------------------------------------
 * Function:	H5L_create_ud
 *
 * Purpose:	Creates a user-defined link. See H5Lcreate_ud for
 *              full documentation.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Friday, May 19, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_create_ud(const H5G_loc_t *link_loc, const char *link_name,
    const void *ud_data, size_t ud_data_size, H5L_type_t type, hid_t lcpl_id,
    hid_t lapl_id, hid_t dxpl_id)
{
    H5O_link_t lnk;                     /* Link to insert */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_create_ud)

    /* Check args */
    HDassert(type >= H5L_TYPE_UD_MIN && type <= H5L_TYPE_MAX);
    HDassert(link_loc);
    HDassert(link_name && *link_name);
    HDassert(ud_data_size == 0 || ud_data);

    /* Initialize the link struct's pointer to its udata buffer */
    lnk.u.ud.udata = NULL;

    /* Make sure that this link class is registered */
    if(H5L_find_class_idx(type) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "link class has not been registered with library")

    /* Fill in UD link-specific information in the link struct*/
    if(ud_data_size > 0) {
        lnk.u.ud.udata = H5MM_malloc((size_t)ud_data_size);
        HDmemcpy(lnk.u.ud.udata, ud_data, (size_t) ud_data_size);
    } /* end if */
    else
        lnk.u.ud.udata = NULL;

    lnk.u.ud.size = ud_data_size;
    lnk.type = type;

    /* Create actual link to the object */
    if(H5L_create_real(link_loc, link_name, NULL, NULL, &lnk, NULL, lcpl_id, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to register new name for object")

done:
    /* Free the link's udata buffer if it's been allocated */
    H5MM_xfree(lnk.u.ud.udata);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_create_ud() */


/*-------------------------------------------------------------------------
 * Function:	H5L_get_val_real
 *
 * Purpose:	Retrieve link value from a link object
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 13 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_get_val_real(const H5O_link_t *lnk, void *buf, size_t size)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_get_val_real)

    /* Sanity check */
    HDassert(lnk);

    /* Check for soft link */
    if(H5L_TYPE_SOFT == lnk->type) {
        /* Copy to output buffer */
        if(size > 0 && buf) {
            HDstrncpy((char *)buf, lnk->u.soft.name, size);
            if(HDstrlen(lnk->u.soft.name) >= size)
                ((char *)buf)[size - 1] = '\0';
        } /* end if */
    } /* end if */
    /* Check for user-defined link */
    else if(lnk->type >= H5L_TYPE_UD_MIN) {
        const H5L_class_t *link_class;             /* User-defined link class */

        /* Get the link class for this type of link.  It's okay if the class
         * isn't registered, though--we just can't give any more information
         * about it
         */
        link_class = H5L_find_class(lnk->type);

        if(link_class != NULL && link_class->query_func != NULL) {
            if((link_class->query_func)(lnk->name, lnk->u.ud.udata, lnk->u.ud.size, buf, size) < 0)
                HGOTO_ERROR(H5E_LINK, H5E_CALLBACK, FAIL, "query callback returned failure")
        } /* end if */
        else if(buf && size > 0)
            ((char *)buf)[0] = '\0';
    } /* end if */
    else
        HGOTO_ERROR(H5E_LINK, H5E_BADTYPE, FAIL, "object is not a symbolic or user-defined link")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_get_val_real() */


/*-------------------------------------------------------------------------
 * Function:	H5L_get_val_cb
 *
 * Purpose:	Callback for retrieving link value or udata.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, September 20, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_get_val_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char *name, const H5O_link_t *lnk,
    H5G_loc_t UNUSED *obj_loc, void *_udata/*in,out*/, H5G_own_loc_t *own_loc/*out*/)
{
    H5L_trav_gv_t *udata = (H5L_trav_gv_t *)_udata;   /* User data passed in */
    herr_t ret_value = SUCCEED;               /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_get_val_cb)

    /* Check if the name in this group resolved to a valid link */
    if(lnk == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "'%s' doesn't exist", name)

    /* Retrieve the value for the link */
    if(H5L_get_val_real(lnk, udata->buf, udata->size) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't retrieve link value")

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_get_val_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5L_get_val
 *
 * Purpose:	Returns the value of a symbolic link or the udata for a
 *              user-defined link.
 *
 * Return:	Success:	Non-negative, with at most SIZE bytes of the
 *				link value copied into the BUF buffer.  If the
 *				link value is larger than SIZE characters
 *				counting the null terminator then the BUF
 *				result will not be null terminated.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Monday, April 13, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_get_val(H5G_loc_t *loc, const char *name, void *buf/*out*/, size_t size,
    hid_t lapl_id, hid_t dxpl_id)
{
    H5L_trav_gv_t udata;           /* User data for callback */
    herr_t ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5L_get_val, FAIL)

    /* Sanity check */
    HDassert(loc);
    HDassert(name && *name);

    /* Set up user data for retrieving information */
    udata.size = size;
    udata.buf = buf;

    /* Traverse the group hierarchy to locate the object to get info about */
    if(H5G_traverse(loc, name, H5G_TARGET_SLINK | H5G_TARGET_UDLINK, H5L_get_val_cb, &udata, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "name doesn't exist")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5L_get_val() */


/*-------------------------------------------------------------------------
 * Function:	H5L_get_val_by_idx_cb
 *
 * Purpose:	Callback for retrieving a link's value according to an
 *              index's order.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 13 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_get_val_by_idx_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char UNUSED *name,
    const H5O_link_t UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/)
{
    H5L_trav_gvbi_t *udata = (H5L_trav_gvbi_t *)_udata;   /* User data passed in */
    H5O_link_t fnd_lnk;                 /* Link within group */
    hbool_t lnk_copied = FALSE;         /* Whether the link was copied */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_get_val_by_idx_cb)

    /* Check if the name of the group resolved to a valid object */
    if(obj_loc == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "group doesn't exist")

    /* Query link */
    if(H5G_obj_lookup_by_idx(obj_loc->oloc, udata->idx_type, udata->order,
                udata->n, &fnd_lnk, udata->dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "link not found")
    lnk_copied = TRUE;

    /* Retrieve the value for the link */
    if(H5L_get_val_real(&fnd_lnk, udata->buf, udata->size) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't retrieve link value")

done:
    /* Reset the link information, if we have a copy */
    if(lnk_copied)
        H5O_msg_reset(H5O_LINK_ID, &fnd_lnk);

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_get_val_by_idx_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5L_delete_cb
 *
 * Purpose:	Callback for deleting a link.  This routine
 *              actually deletes the link
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 19, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_delete_cb(H5G_loc_t *grp_loc/*in*/, const char *name, const H5O_link_t *lnk,
    H5G_loc_t UNUSED *obj_loc, void *_udata/*in,out*/, H5G_own_loc_t *own_loc/*out*/)
{
    H5L_trav_rm_t *udata = (H5L_trav_rm_t *)_udata;   /* User data passed in */
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5L_delete_cb)

    /* Check if the group resolved to a valid link */
    if(grp_loc == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "group doesn't exist")

    /* Check if the name in this group resolved to a valid link */
    if(name == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "name doesn't exist")

    /* Check for removing '.' */
    if(lnk == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_CANTDELETE, FAIL, "can't delete self")

    /* Remove the link from the group */
    if(H5G_obj_remove(grp_loc->oloc, grp_loc->path->full_path_r, name, udata->dxpl_id) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTDELETE, FAIL, "unable to remove link from group")

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_delete_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5L_delete
 *
 * Purpose:	Delete a link from a group.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Thursday, September 17, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_delete(H5G_loc_t *loc, const char *name, hid_t lapl_id, hid_t dxpl_id)
{
    H5L_trav_rm_t      udata;                  /* User data for callback */
    char		*norm_name = NULL;	/* Pointer to normalized name */
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5L_delete, FAIL)

    /* Sanity check */
    HDassert(loc);
    HDassert(name && *name);

    /* Get normalized copy of the name */
    if((norm_name = H5G_normalize(name)) == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "can't normalize name")

    /* Set up user data for unlink operation */
    udata.dxpl_id = dxpl_id;
    if(H5G_traverse(loc, norm_name, H5G_TARGET_SLINK|H5G_TARGET_UDLINK|H5G_TARGET_MOUNT, H5L_delete_cb, &udata, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTREMOVE, FAIL, "can't unlink object")

done:
    /* Free the normalized path name */
    if(norm_name)
        H5MM_xfree(norm_name);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5L_delete_by_idx_cb
 *
 * Purpose:	Callback for removing a link according to an index's order.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 13 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_delete_by_idx_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char UNUSED *name,
    const H5O_link_t UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/)
{
    H5L_trav_gvbi_t *udata = (H5L_trav_gvbi_t *)_udata;   /* User data passed in */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_delete_by_idx_cb)

    /* Check if the name of the group resolved to a valid object */
    if(obj_loc == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "group doesn't exist")

    /* Delete link */
    if(H5G_obj_remove_by_idx(obj_loc->oloc, obj_loc->path->full_path_r,
            udata->idx_type, udata->order, udata->n, udata->dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "link not found")

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_delete_by_idx_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5L_move_dest_cb
 *
 * Purpose:	Second callback for moving and renaming an object.  This routine
 *              inserts a new link into the group returned by the traversal.
 *              It is called by H5L_move_cb.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, April 3, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_move_dest_cb(H5G_loc_t *grp_loc/*in*/, const char *name,
    const H5O_link_t UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/)
{
    H5L_trav_mv2_t *udata = (H5L_trav_mv2_t *)_udata;   /* User data passed in */
    H5G_t *grp=NULL;                    /* H5G_t for this group, opened to pass to user callback */
    hid_t grp_id = FAIL;                /* Id for this group (passed to user callback */
    H5G_loc_t temp_loc;                 /* For UD callback */
    hbool_t temp_loc_init = FALSE;
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_move_dest_cb)

    /* Make sure an object with this name doesn't already exist */
    if(obj_loc != NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "an object with that name already exists")

    /* Check for crossing file boundaries with a new hard link */
    if(udata->lnk->type == H5L_TYPE_HARD) {
        /* Check that both objects are in same file */
        if(grp_loc->oloc->file->shared != udata->file->shared)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "moving a link across files is not allowed")
    } /* end if */

    /* Give the object its new name */
    /* Casting away const okay -JML */
    HDassert(udata->lnk->name == NULL);
    udata->lnk->name = (char *)name;

    /* Insert the link into the group */
    if(H5G_obj_insert(grp_loc->oloc, name, udata->lnk, TRUE, udata->dxpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create new link to object")

    /* If the link was a user-defined link, call its move callback if it has one */
    if(udata->lnk->type >= H5L_TYPE_UD_MIN) {
        const H5L_class_t   *link_class;         /* User-defined link class */

        /* Get the link class for this type of link. */
        if(NULL == (link_class = H5L_find_class(udata->lnk->type)))
            HGOTO_ERROR(H5E_LINK, H5E_NOTREGISTERED, FAIL, "link class is not registered")

        if((!udata->copy && link_class->move_func) || (udata->copy && link_class->copy_func)) {
            H5O_loc_t           temp_oloc;
            H5G_name_t          temp_path;

            /* Create a temporary location (or else H5G_open will do a shallow
             * copy and wipe out grp_loc)
             */
            H5G_name_reset(&temp_path);
            if(H5O_loc_copy(&temp_oloc, grp_loc->oloc, H5_COPY_DEEP) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTCOPY, FAIL, "unable to copy object location")

            temp_loc.oloc = &temp_oloc;
            temp_loc.path = &temp_path;
            temp_loc_init = TRUE;

            /* Set up location for user-defined callback */
            if((grp = H5G_open(&temp_loc, udata->dxpl_id)) == NULL)
                HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open group")
            if((grp_id = H5I_register(H5I_GROUP, grp, TRUE)) < 0)
                HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register group ID")

            if(udata->copy) {
                if((link_class->copy_func)(udata->lnk->name, grp_id, udata->lnk->u.ud.udata, udata->lnk->u.ud.size) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_CALLBACK, FAIL, "UD copy callback returned error")
            } /* end if */
            else {
                if((link_class->move_func)(udata->lnk->name, grp_id, udata->lnk->u.ud.udata, udata->lnk->u.ud.size) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_CALLBACK, FAIL, "UD move callback returned error")
            } /* end else */
        } /* end if */
    } /* end if */

done:
    /* Close the location given to the user callback if it was created */
    if(grp_id >= 0) {
        if(H5I_dec_ref(grp_id, TRUE) < 0)
            HDONE_ERROR(H5E_ATOM, H5E_CANTRELEASE, FAIL, "unable to close atom from UD callback")
    } /* end if */
    else if(grp != NULL) {
        if(H5G_close(grp) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CANTRELEASE, FAIL, "unable to close group given to UD callback")
    } /* end if */
    else if(temp_loc_init)
        H5G_loc_free(&temp_loc);

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_move_dest_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5L_move_cb
 *
 * Purpose:	Callback for moving and renaming an object.  This routine
 *              replaces the names of open objects with the moved object
 *              in the path
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Friday, April 3, 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_move_cb(H5G_loc_t *grp_loc/*in*/, const char *name, const H5O_link_t *lnk,
    H5G_loc_t *obj_loc, void *_udata/*in,out*/, H5G_own_loc_t *own_loc/*out*/)
{
    H5L_trav_mv_t *udata = (H5L_trav_mv_t *)_udata;   /* User data passed in */
    H5L_trav_mv2_t udata_out;           /* User data for H5L_move_dest_cb traversal */
    char * orig_name = NULL;            /* The name of the link in this group */
    hbool_t link_copied = FALSE;        /* Has udata_out.lnk been allocated? */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_move_cb)

    /* Check if the name in this group resolved to a valid link */
    if(obj_loc == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "name doesn't exist")

    /* Check for operations on '.' */
    if(lnk == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "the name of a link must be supplied to move or copy")

    /* Set up user data for move_dest_cb */
    if((udata_out.lnk = (H5O_link_t *)H5O_msg_copy(H5O_LINK_ID, lnk, NULL)) == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_CANTCOPY, FAIL, "unable to copy link to be moved")

    /* In this special case, the link's name is going to be replaced at its
     * destination, so we should free it here.
     */
    udata_out.lnk->name = (char *)H5MM_xfree(udata_out.lnk->name);
    link_copied = TRUE;

    udata_out.lnk->cset = udata->cset;
    udata_out.file = grp_loc->oloc->file;
    udata_out.copy = udata->copy;
    udata_out.dxpl_id = udata->dxpl_id;

    /* Keep a copy of link's name (it's "owned" by the H5G_traverse() routine) */
    orig_name = H5MM_xstrdup(name);

    /* Insert the link into its new location */
    if(H5G_traverse(udata->dst_loc, udata->dst_name, udata->dst_target_flags,
            H5L_move_dest_cb, &udata_out, udata->lapl_id, udata->dxpl_id) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to follow symbolic link")

    /* If this is a move and not a copy operation, change the object's name and remove the old link */
    if(!udata->copy) {
        H5RS_str_t *dst_name_r;      /* Ref-counted version of dest name */

        /* Make certain that the destination name is a full (not relative) path */
        if(*(udata->dst_name) != '/') {
            HDassert(udata->dst_loc->path->full_path_r);

            /* Create reference counted string for full dst path */
            if((dst_name_r = H5G_build_fullpath_refstr_str(udata->dst_loc->path->full_path_r,
                    udata->dst_name)) == NULL)
                HGOTO_ERROR(H5E_SYM, H5E_PATH, FAIL, "can't build destination path name")
        } /* end if */
        else
            dst_name_r = H5RS_wrap(udata->dst_name);
        HDassert(dst_name_r);

        /* Fix names up */
        if(H5G_name_replace(lnk, H5G_NAME_MOVE, obj_loc->oloc->file, obj_loc->path->full_path_r,
                udata->dst_loc->oloc->file, dst_name_r, udata->dxpl_id) < 0) {
            H5RS_decr(dst_name_r);
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to replace name")
        } /* end if */

        /* Remove the old link */
        if(H5G_obj_remove(grp_loc->oloc, grp_loc->path->full_path_r, orig_name, udata->dxpl_id) < 0) {
            H5RS_decr(dst_name_r);
            HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to remove old name")
        } /* end if */

        H5RS_decr(dst_name_r);
    } /* end if */

done:
    /* Cleanup */
    if(orig_name)
        H5MM_xfree(orig_name);

    /* If udata_out.lnk was copied, free any memory allocated
     * In this special case, the H5L_move_dest_cb callback frees the name
     * if it succeeds
     */
    if(link_copied) {
        if(udata_out.lnk->type == H5L_TYPE_SOFT)
            udata_out.lnk->u.soft.name = (char *)H5MM_xfree(udata_out.lnk->u.soft.name);
        else if(udata_out.lnk->type >= H5L_TYPE_UD_MIN && udata_out.lnk->u.ud.size > 0)
            udata_out.lnk->u.ud.udata = H5MM_xfree(udata_out.lnk->u.ud.udata);
        H5MM_xfree(udata_out.lnk);
    } /* end if */

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_move_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5L_move
 *
 * Purpose:	Atomically move or copy a link.
 *
 *              Creates a copy of a link in a new destination with a new name.
 *              SRC_LOC and SRC_NAME together define the link's original
 *              location, while DST_LOC and DST_NAME together define its
 *              final location.
 *
 *              If copy_flag is FALSE, the original link is removed
 *              (effectively moving the link).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, May 1, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_move(H5G_loc_t *src_loc, const char *src_name, H5G_loc_t *dst_loc,
    const char *dst_name, hbool_t copy_flag, hid_t lcpl_id, hid_t lapl_id,
    hid_t dxpl_id)
{
    unsigned    dst_target_flags = H5G_TARGET_NORMAL;
    H5T_cset_t char_encoding = H5F_DEFAULT_CSET; /* Character encoding for link */
    H5P_genplist_t* lc_plist;           /* Link creation property list */
    H5P_genplist_t* la_plist;           /* Link access property list */
    H5L_trav_mv_t      udata;          /* User data for traversal */
    hid_t               lapl_copy;      /* Copy of lapl for this function */
    herr_t              ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_move)

    /* Sanity check */
    HDassert(src_loc);
    HDassert(dst_loc);
    HDassert(src_name && *src_name);
    HDassert(dst_name && *dst_name);

    /* Check for flags present in creation property list */
    if(lcpl_id != H5P_DEFAULT) {
        unsigned crt_intmd_group;

        if(NULL == (lc_plist = (H5P_genplist_t *)H5I_object(lcpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

        /* Get intermediate group creation property */
        if(H5P_get(lc_plist, H5L_CRT_INTERMEDIATE_GROUP_NAME, &crt_intmd_group) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get property value for creating missing groups")

        /* Set target flags for source and destination */
        if(crt_intmd_group > 0)
            dst_target_flags |= H5G_CRT_INTMD_GROUP;

        /* Get character encoding property */
        if(H5P_get(lc_plist, H5P_STRCRT_CHAR_ENCODING_NAME, &char_encoding) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get property value for character encoding")
    } /* end if */

    /* Copy the link access property list because traversing UD links will
     * decrease the NLINKS property. HDF5 should have NLINKS traversals to
     * get to the source and NLINKS more to get to the destination. */
    if(lapl_id == H5P_DEFAULT)
        lapl_copy = lapl_id;
    else {
        if(NULL == (la_plist = (H5P_genplist_t *)H5I_object(lapl_id)))
            HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a valid access PL")
        if((lapl_copy = H5P_copy_plist(la_plist, FALSE)) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "unable to copy access properties")
    } /* end else */

    /* Set up user data */
    udata.dst_loc = dst_loc;
    udata.dst_name= dst_name;
    udata.dst_target_flags = dst_target_flags;
    udata.cset = char_encoding;
    udata.copy = copy_flag;
    udata.lapl_id = lapl_copy;
    udata.dxpl_id = dxpl_id;

    /* Do the move */
    if(H5G_traverse(src_loc, src_name, H5G_TARGET_MOUNT | H5G_TARGET_SLINK | H5G_TARGET_UDLINK,
            H5L_move_cb, &udata, lapl_id, dxpl_id) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "unable to find link")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_move() */


/*-------------------------------------------------------------------------
 * Function:	H5L_exists_cb
 *
 * Purpose:	Callback for checking whether a link exists
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Friday, March 16 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_exists_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char UNUSED *name,
    const H5O_link_t *lnk, H5G_loc_t UNUSED *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/)
{
    hbool_t *udata = (hbool_t *)_udata;   /* User data passed in */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5L_exists_cb)

    /* Check if the name in this group resolved to a valid link */
    *udata = (hbool_t)(lnk != NULL);

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5L_exists_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5L_exists
 *
 * Purpose:	Returns whether a link exists in a group
 *
 * Return:	Non-negative (TRUE/FALSE) on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Friday, March 16 2007
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5L_exists(const H5G_loc_t *loc, const char *name, hid_t lapl_id, hid_t dxpl_id)
{
    hbool_t exists = FALSE;     /* Whether the link exists in the group */
    htri_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_exists)

    /* Traverse the group hierarchy to locate the object to get info about */
    if(H5G_traverse(loc, name, H5G_TARGET_SLINK|H5G_TARGET_UDLINK, H5L_exists_cb, &exists, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_EXISTS, FAIL, "path doesn't exist")

    /* Set return value */
    ret_value = (htri_t)exists;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5L_exists() */


/*-------------------------------------------------------------------------
 * Function:	H5L_get_info_cb
 *
 * Purpose:	Callback for retrieving a link's metadata
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, April 17 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_get_info_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char UNUSED *name,
    const H5O_link_t *lnk, H5G_loc_t UNUSED *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/)
{
    H5L_trav_gi_t *udata = (H5L_trav_gi_t *)_udata;   /* User data passed in */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_get_info_cb)

    /* Check if the name in this group resolved to a valid link */
    if(lnk == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "name doesn't exist")

    /* Get information from the link */
    if(H5G_link_to_info(lnk, udata->linfo) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't get link info")

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_get_info_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5L_get_info
 *
 * Purpose:	Returns metadata about a link.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Monday, April 17 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5L_get_info(const H5G_loc_t *loc, const char *name,
    H5L_info_t *linfo/*out*/, hid_t lapl_id, hid_t dxpl_id)
{
    H5L_trav_gi_t udata;               /* User data for callback */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5L_get_info, FAIL)

    udata.linfo = linfo;
    udata.dxpl_id = dxpl_id;

    /* Traverse the group hierarchy to locate the object to get info about */
    if(H5G_traverse(loc, name, H5G_TARGET_SLINK|H5G_TARGET_UDLINK, H5L_get_info_cb, &udata, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_EXISTS, FAIL, "name doesn't exist")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5L_get_info() */


/*-------------------------------------------------------------------------
 * Function:	H5L_get_info_by_idx_cb
 *
 * Purpose:	Callback for retrieving a link's metadata according to an
 *              index's order.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, November  6 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_get_info_by_idx_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char UNUSED *name,
    const H5O_link_t UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/)
{
    H5L_trav_gibi_t *udata = (H5L_trav_gibi_t *)_udata;   /* User data passed in */
    H5O_link_t fnd_lnk;                 /* Link within group */
    hbool_t lnk_copied = FALSE;         /* Whether the link was copied */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_get_info_by_idx_cb)

    /* Check if the name of the group resolved to a valid object */
    if(obj_loc == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "group doesn't exist")

    /* Query link */
    if(H5G_obj_lookup_by_idx(obj_loc->oloc, udata->idx_type, udata->order,
                udata->n, &fnd_lnk, udata->dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "link not found")
    lnk_copied = TRUE;

    /* Get information from the link */
    if(H5G_link_to_info(&fnd_lnk, udata->linfo) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "can't get link info")

done:
    /* Reset the link information, if we have a copy */
    if(lnk_copied)
        H5O_msg_reset(H5O_LINK_ID, &fnd_lnk);

    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_get_info_by_idx_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5L_get_default_lcpl
 *
 * Purpose:	Accessor for the default Link Creation Property List
 *
 * Return:	Success:	ID of the deafult lcpl
 *
 * 		Failure:	Negative
 *
 * Programmer:	James Laird
 *              Tuesday, July 4, 2006
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5L_get_default_lcpl(void)
{
    hid_t ret_value;            /* Return value */

    FUNC_ENTER_NOAPI(H5L_get_default_lcpl, FAIL)

    ret_value = H5P_LINK_CREATE_DEFAULT;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5L_get_default_lcpl */


/*-------------------------------------------------------------------------
 * Function:	H5L_get_name_by_idx_cb
 *
 * Purpose:	Callback for retrieving a link's name according to an
 *              index's order.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Saturday, November 11 2006
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5L_get_name_by_idx_cb(H5G_loc_t UNUSED *grp_loc/*in*/, const char UNUSED *name,
    const H5O_link_t UNUSED *lnk, H5G_loc_t *obj_loc, void *_udata/*in,out*/,
    H5G_own_loc_t *own_loc/*out*/)
{
    H5L_trav_gnbi_t *udata = (H5L_trav_gnbi_t *)_udata;   /* User data passed in */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5L_get_name_by_idx_cb)

    /* Check if the name of the group resolved to a valid object */
    if(obj_loc == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "group doesn't exist")

    /* Query link */
    if((udata->name_len = H5G_obj_get_name_by_idx(obj_loc->oloc, udata->idx_type, udata->order,
                udata->n, udata->name, udata->size, udata->dxpl_id)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "link not found")

done:
    /* Indicate that this callback didn't take ownership of the group *
     * location for the object */
    *own_loc = H5G_OWN_NONE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5L_get_name_by_idx_cb() */

