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
 * Created:	H5G.c
 *		Jul 18 1997
 *		Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:	Symbol table functions.	 The functions that begin with
 *		`H5G_stab_' don't understand the naming system; they operate
 * 		on a single symbol table at a time.
 *
 *		The functions that begin with `H5G_node_' operate on the leaf
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
 * 		| File ID      | "/foo/bar" | Find `foo' within `bar' within |
 *		|              |            | the root group of the specified|
 *		|              |            | file.                          |
 *              +--------------+------------+--------------------------------+
 * 		| File ID      | "foo/bar"  | Find `foo' within `bar' within |
 *		|              |            | the root group of the specified|
 *		|              |            | file.                          |
 *              +--------------+------------+--------------------------------+
 * 		| File ID      | "/"        | The root group of the specified|
 *		|              |            | file.                          |
 *              +--------------+------------+--------------------------------+
 * 		| File ID      | "."        | The root group of the specified|
 *		|              |            | the specified file.            |
 *              +--------------+------------+--------------------------------+
 * 		| Group ID     | "/foo/bar" | Find `foo' within `bar' within |
 *		|              |            | the root group of the file     |
 *		|              |            | containing the specified group.|
 *              +--------------+------------+--------------------------------+
 * 		| Group ID     | "foo/bar"  | File `foo' within `bar' within |
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

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */
#define H5G_PACKAGE		/*suppress error about including H5Gpkg   */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5G_init_interface

/* Packages needed by this file... */
#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"	/* Metadata cache			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"		/* File access				*/
#include "H5Gpkg.h"		/* Groups		  		*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Lprivate.h"         /* Links                                */
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Pprivate.h"         /* Property lists                       */

/* Local macros */
#define H5G_RESERVED_ATOMS	0

/* Local typedefs */

/* User data for path traversal routine for "insertion file" routine */
typedef struct {
    H5G_loc_t *loc;         /* Pointer to the location for insertion */
} H5G_trav_ins_t;

/* User data for application-style iteration over links in a group */
typedef struct {
    hid_t       gid;            /* The group ID for the application callback */
    H5G_link_iterate_t lnk_op;  /* Application callback */
    void *op_data;              /* Application's op data */
} H5G_iter_appcall_ud_t;

/* User data for recursive traversal over links from a group */
typedef struct {
    hid_t       gid;            /* The group ID for the starting group */
    H5G_loc_t	*curr_loc;      /* Location of starting group */
    hid_t       lapl_id;        /* LAPL for walking across links */
    hid_t       dxpl_id; 	/* DXPL for operations */
    H5_index_t  idx_type;       /* Index to use */
    H5_iter_order_t order;      /* Iteration order within index */
    H5SL_t     *visited;        /* Skip list for tracking visited nodes */
    char       *path;           /* Path name of the link */
    size_t      curr_path_len;  /* Current length of the path in the buffer */
    size_t      path_buf_size;  /* Size of path buffer */
    H5L_iterate_t op;           /* Application callback */
    void       *op_data;        /* Application's op data */
} H5G_iter_visit_ud_t;


/* Package variables */


/* Local variables */

/* Declare a free list to manage the H5G_t struct */
H5FL_DEFINE(H5G_t);
H5FL_DEFINE(H5G_shared_t);

/* Declare the free list to manage H5_obj_t's */
H5FL_DEFINE(H5_obj_t);


/* Private prototypes */
static herr_t H5G_open_oid(H5G_t *grp, hid_t dxpl_id);


/*-------------------------------------------------------------------------
 * Function:	H5Gcreate2
 *
 * Purpose:	Creates a new group relative to LOC_ID, giving it the
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
 * Return:	Success:	The object ID of a new, empty group open for
 *				writing.  Call H5Gclose() when finished with
 *				the group.
 *
 *		Failure:	FAIL
 *
 * Programmer:  Quincey Koziol
 *	        April 5, 2007
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gcreate2(hid_t loc_id, const char *name, hid_t lcpl_id, hid_t gcpl_id,
    hid_t gapl_id)
{
    H5G_loc_t	    loc;                /* Location to create group */
    H5G_t	   *grp = NULL;         /* New group created */
    hid_t	    ret_value;          /* Return value */

    FUNC_ENTER_API(H5Gcreate2, FAIL)
    H5TRACE5("i", "i*siii", loc_id, name, lcpl_id, gcpl_id, gapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Get correct property list */
    if(H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link creation property list")

    /* Check group creation property list */
    if(H5P_DEFAULT == gcpl_id)
        gcpl_id = H5P_GROUP_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(gcpl_id, H5P_GROUP_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not group create property list")

    /* Check the group access property list */
    if(H5P_DEFAULT == gapl_id)
        gapl_id = H5P_GROUP_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(gapl_id, H5P_GROUP_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not group access property list")

    /* Create the new group & get its ID */
    if(NULL == (grp = H5G_create_named(&loc, name, lcpl_id, gcpl_id, gapl_id, H5AC_dxpl_id)))
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to create group")
    if((ret_value = H5I_register(H5I_GROUP, grp, TRUE)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register group")

done:
    if(ret_value < 0)
        if(grp && H5G_close(grp) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CLOSEERROR, FAIL, "unable to release group")

    FUNC_LEAVE_API(ret_value)
} /* end H5Gcreate2() */


/*-------------------------------------------------------------------------
 * Function:	H5G_create_named
 *
 * Purpose:	Internal routine to create a new "named" group.
 *
 * Return:	Success:	Non-NULL, pointer to new group object.
 *
 *		Failure:	NULL
 *
 * Programmer:  Quincey Koziol
 *	        April 5, 2007
 *
 *-------------------------------------------------------------------------
 */
H5G_t *
H5G_create_named(const H5G_loc_t *loc, const char *name, hid_t lcpl_id,
    hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id)
{
    H5O_obj_create_t ocrt_info;         /* Information for object creation */
    H5G_obj_create_t gcrt_info;         /* Information for group creation */
    H5G_t	   *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5G_create_named, NULL)

    /* Check arguments */
    HDassert(loc);
    HDassert(name && *name);
    HDassert(lcpl_id != H5P_DEFAULT);
    HDassert(gcpl_id != H5P_DEFAULT);
    HDassert(gapl_id != H5P_DEFAULT);
    HDassert(dxpl_id != H5P_DEFAULT);

    /* Set up group creation info */
    gcrt_info.gcpl_id = gcpl_id;

    /* Set up object creation information */
    ocrt_info.obj_type = H5O_TYPE_GROUP;
    ocrt_info.crt_info = &gcrt_info;
    ocrt_info.new_obj = NULL;

    /* Create the new group and link it to its parent group */
    if(H5L_link_object(loc, name, &ocrt_info, lcpl_id, gapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, NULL, "unable to create and link to group")
    HDassert(ocrt_info.new_obj);

    /* Set the return value */
    ret_value = (H5G_t *)ocrt_info.new_obj;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_create_named() */


/*-------------------------------------------------------------------------
 * Function:	H5Gcreate_anon
 *
 * Purpose:	Creates a new group relative to LOC_ID, giving it the
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
 * Example:	To create missing groups "A" and "B01" along the given path "/A/B01/grp"
 *              hid_t create_id = H5Pcreate(H5P_GROUP_CREATE);
 *              int   status = H5Pset_create_intermediate_group(create_id, TRUE);
 *              hid_t gid = H5Gcreate_anon(file_id, "/A/B01/grp", create_id, H5P_DEFAULT);
 *
 * Return:	Success:	The object ID of a new, empty group open for
 *				writing.  Call H5Gclose() when finished with
 *				the group.
 *
 *		Failure:	FAIL
 *
 * Programmer:  Peter Cao
 *	        May 08, 2005
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gcreate_anon(hid_t loc_id, hid_t gcpl_id, hid_t gapl_id)
{
    H5G_loc_t	    loc;
    H5G_t	   *grp = NULL;
    hid_t	    ret_value;

    FUNC_ENTER_API(H5Gcreate_anon, FAIL)
    H5TRACE3("i", "iii", loc_id, gcpl_id, gapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

    /* Check group creation property list */
    if(H5P_DEFAULT == gcpl_id)
        gcpl_id = H5P_GROUP_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(gcpl_id, H5P_GROUP_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not group create property list")

    /* Check the group access property list */
    if(H5P_DEFAULT == gapl_id)
        gapl_id = H5P_GROUP_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(gapl_id, H5P_GROUP_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not group access property list")

    /* Create the new group & get its ID */
    if(NULL == (grp = H5G_create(loc.oloc->file, gcpl_id, H5AC_dxpl_id)))
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to create group")
    if((ret_value = H5I_register(H5I_GROUP, grp, TRUE)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register group")

done:
    if(ret_value < 0)
        if(grp && H5G_close(grp) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CLOSEERROR, FAIL, "unable to release group")

    FUNC_LEAVE_API(ret_value)
} /* end H5Gcreate_anon() */


/*-------------------------------------------------------------------------
 * Function:	H5Gopen2
 *
 * Purpose:	Opens an existing group for modification.  When finished,
 *		call H5Gclose() to close it and release resources.
 *
 *              This function allows the user the pass in a Group Access
 *              Property List, which H5Gopen1() does not.
 *
 * Return:	Success:	Object ID of the group.
 *		Failure:	FAIL
 *
 * Programmer:	James Laird
 *		Thursday, July 27, 2006
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gopen2(hid_t loc_id, const char *name, hid_t gapl_id)
{
    H5G_t       *grp = NULL;            /* Group opened */
    H5G_loc_t	loc;                    /* Location of parent for group */
    hid_t       ret_value;              /* Return value */

    FUNC_ENTER_API(H5Gopen2, FAIL)
    H5TRACE3("i", "i*si", loc_id, name, gapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Check the group access property list */
    if(H5P_DEFAULT == gapl_id)
        gapl_id = H5P_GROUP_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(gapl_id, H5P_GROUP_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not group access property list")

    /* Open the group */
    if((grp = H5G_open_name(&loc, name, gapl_id, H5AC_dxpl_id)) == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open group")

    /* Register an ID for the group */
    if((ret_value = H5I_register(H5I_GROUP, grp, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register group")

done:
    if(ret_value < 0) {
        if(grp && H5G_close(grp) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CLOSEERROR, FAIL, "unable to release group")
    } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* end H5Gopen2() */


/*-------------------------------------------------------------------------
 * Function:	H5Gget_create_plist
 *
 * Purpose:	Returns a copy of the group creation property list.
 *
 * Return:	Success:	ID for a copy of the group creation
 *				property list.  The property list ID should be
 *				released by calling H5Pclose().
 *
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, October 25, 2005
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gget_create_plist(hid_t group_id)
{
    H5O_linfo_t         linfo;		        /* Link info message            */
    htri_t	        ginfo_exists;
    htri_t	        linfo_exists;
    htri_t              pline_exists;
    H5G_t		*grp = NULL;
    H5P_genplist_t      *gcpl_plist;
    H5P_genplist_t      *new_plist;
    hid_t		new_gcpl_id = FAIL;
    hid_t		ret_value = FAIL;

    FUNC_ENTER_API(H5Gget_create_plist, FAIL)
    H5TRACE1("i", "i", group_id);

    /* Check args */
    if(NULL == (grp = (H5G_t *)H5I_object_verify(group_id, H5I_GROUP)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a group")

    /* Copy the default group creation property list */
    if(NULL == (gcpl_plist = (H5P_genplist_t *)H5I_object(H5P_LST_GROUP_CREATE_g)))
         HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get default group creation property list")
    if((new_gcpl_id = H5P_copy_plist(gcpl_plist, TRUE)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "unable to copy the creation property list")
    if(NULL == (new_plist = (H5P_genplist_t *)H5I_object(new_gcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")

    /* Retrieve any object creation properties */
    if(H5O_get_create_plist(&grp->oloc, H5AC_ind_dxpl_id, new_plist) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't get object creation info")

    /* Check for the group having a group info message */
    if((ginfo_exists = H5O_msg_exists(&(grp->oloc), H5O_GINFO_ID, H5AC_ind_dxpl_id)) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to read object header")
    if(ginfo_exists) {
        H5O_ginfo_t ginfo;		/* Group info message            */

        /* Read the group info */
        if(NULL == H5O_msg_read(&(grp->oloc), H5O_GINFO_ID, &ginfo, H5AC_ind_dxpl_id))
            HGOTO_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "can't get group info")

        /* Set the group info for the property list */
        if(H5P_set(new_plist, H5G_CRT_GROUP_INFO_NAME, &ginfo) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set group info")
    } /* end if */

    /* Check for the group having a link info message */
    if((linfo_exists = H5G_obj_get_linfo(&(grp->oloc), &linfo, H5AC_ind_dxpl_id)) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to read object header")
    if(linfo_exists) {
        /* Set the link info for the property list */
        if(H5P_set(new_plist, H5G_CRT_LINK_INFO_NAME, &linfo) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set link info")
    } /* end if */

    /* Check for the group having a pipeline message */
    if((pline_exists = H5O_msg_exists(&(grp->oloc), H5O_PLINE_ID, H5AC_ind_dxpl_id)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "unable to read object header")
    if(pline_exists) {
        H5O_pline_t pline;      /* Pipeline message */

        /* Read the pipeline */
        if(NULL == H5O_msg_read(&(grp->oloc), H5O_PLINE_ID, &pline, H5AC_ind_dxpl_id))
            HGOTO_ERROR(H5E_SYM, H5E_BADMESG, FAIL, "can't get link pipeline")

        /* Set the pipeline for the property list */
        if(H5P_set(new_plist, H5O_CRT_PIPELINE_NAME, &pline) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set link pipeline")
    } /* end if */

    /* Set the return value */
    ret_value = new_gcpl_id;

done:
    if(ret_value < 0) {
        if(new_gcpl_id > 0)
            (void)H5I_dec_ref(new_gcpl_id, TRUE);
    } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* end H5Gget_create_plist() */


/*-------------------------------------------------------------------------
 * Function:	H5Gget_info
 *
 * Purpose:	Retrieve information about a group.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		November 27 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gget_info(hid_t grp_id, H5G_info_t *grp_info)
{
    H5I_type_t  id_type;                /* Type of ID */
    H5G_loc_t	loc;                    /* Location of group */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(H5Gget_info, FAIL)
    H5TRACE2("e", "i*x", grp_id, grp_info);

    /* Check args */
    id_type = H5I_get_type(grp_id);
    if(!(H5I_GROUP == id_type || H5I_FILE == id_type))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid argument")
    if(!grp_info)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")

    /* Get group location */
    if(H5G_loc(grp_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

    /* Retrieve the group's information */
    if(H5G_obj_info(loc.oloc, grp_info/*out*/, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve group info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Gget_info() */


/*-------------------------------------------------------------------------
 * Function:	H5Gget_info_by_name
 *
 * Purpose:	Retrieve information about a group.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		November 27 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gget_info_by_name(hid_t loc_id, const char *name, H5G_info_t *grp_info,
    hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Location of group */
    H5G_loc_t   grp_loc;                /* Location used to open group */
    H5G_name_t  grp_path;            	/* Opened object group hier. path */
    H5O_loc_t   grp_oloc;            	/* Opened object object location */
    hbool_t     loc_found = FALSE;      /* Location at 'name' found */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(H5Gget_info_by_name, FAIL)
    H5TRACE4("e", "i*s*xi", loc_id, name, grp_info, lapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(!grp_info)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Set up opened group location to fill in */
    grp_loc.oloc = &grp_oloc;
    grp_loc.path = &grp_path;
    H5G_loc_reset(&grp_loc);

    /* Find the group object */
    if(H5G_loc_find(&loc, name, &grp_loc/*out*/, lapl_id, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "group not found")
    loc_found = TRUE;

    /* Retrieve the group's information */
    if(H5G_obj_info(grp_loc.oloc, grp_info/*out*/, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve group info")

done:
    if(loc_found && H5G_loc_free(&grp_loc) < 0)
        HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "can't free location")

    FUNC_LEAVE_API(ret_value)
} /* end H5Gget_info_by_name() */


/*-------------------------------------------------------------------------
 * Function:	H5Gget_info_by_idx
 *
 * Purpose:	Retrieve information about a group, according to the order
 *              of an index.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		November 27 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gget_info_by_idx(hid_t loc_id, const char *group_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, H5G_info_t *grp_info, hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Location of group */
    H5G_loc_t   grp_loc;                /* Location used to open group */
    H5G_name_t  grp_path;            	/* Opened object group hier. path */
    H5O_loc_t   grp_oloc;            	/* Opened object object location */
    hbool_t     loc_found = FALSE;      /* Entry at 'name' found */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(H5Gget_info_by_idx, FAIL)
    H5TRACE7("e", "i*sIiIoh*xi", loc_id, group_name, idx_type, order, n, grp_info,
             lapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!group_name || !*group_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(!grp_info)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Set up opened group location to fill in */
    grp_loc.oloc = &grp_oloc;
    grp_loc.path = &grp_path;
    H5G_loc_reset(&grp_loc);

    /* Find the object's location, according to the order in the index */
    if(H5G_loc_find_by_idx(&loc, group_name, idx_type, order, n, &grp_loc/*out*/, lapl_id, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "group not found")
    loc_found = TRUE;

    /* Retrieve the group's information */
    if(H5G_obj_info(grp_loc.oloc, grp_info/*out*/, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve group info")

done:
    /* Release the object location */
    if(loc_found && H5G_loc_free(&grp_loc) < 0)
        HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "can't free location")

    FUNC_LEAVE_API(ret_value)
} /* end H5Gget_info_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5Gclose
 *
 * Purpose:	Closes the specified group.  The group ID will no longer be
 *		valid for accessing the group.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Wednesday, December 31, 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gclose(hid_t group_id)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(H5Gclose, FAIL)
    H5TRACE1("e", "i", group_id);

    /* Check args */
    if(NULL == H5I_object_verify(group_id,H5I_GROUP))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a group")

    /*
     * Decrement the counter on the group atom.	 It will be freed if the count
     * reaches zero.
     */
    if(H5I_dec_ref(group_id, TRUE) < 0)
    	HGOTO_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "unable to close group")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Gclose() */

/*
 *-------------------------------------------------------------------------
 *-------------------------------------------------------------------------
 *   N O   A P I   F U N C T I O N S   B E Y O N D   T H I S   P O I N T
 *-------------------------------------------------------------------------
 *-------------------------------------------------------------------------
 */


/*-------------------------------------------------------------------------
 * Function:	H5G_init
 *
 * Purpose:	Initialize the interface from some other package.
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, November 11, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_init(void)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5G_init, FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_init() */


/*-------------------------------------------------------------------------
 * Function:	H5G_init_interface
 *
 * Purpose:	Initializes the H5G interface.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Monday, January	 5, 1998
 *
 * Notes:       The group creation properties are registered in the property
 *              list interface initialization routine (H5P_init_interface)
 *              so that the file creation property class can inherit from it
 *              correctly. (Which allows the file creation property list to
 *              control the group creation properties of the root group of
 *              a file) QAK - 24/10/2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_init_interface(void)
{
    herr_t          ret_value = SUCCEED;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_init_interface)

    /* Initialize the atom group for the group IDs */
    if(H5I_register_type(H5I_GROUP, (size_t)H5I_GROUPID_HASHSIZE, H5G_RESERVED_ATOMS, (H5I_free_t)H5G_close) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to initialize interface")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_init_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5G_term_interface
 *
 * Purpose:	Terminates the H5G interface
 *
 * Return:	Success:	Positive if anything is done that might
 *				affect other interfaces; zero otherwise.
 *
 * 		Failure:	Negative.
 *
 * Programmer:	Robb Matzke
 *		Monday, January	 5, 1998
 *
 *-------------------------------------------------------------------------
 */
int
H5G_term_interface(void)
{
    int	n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_term_interface)

    if(H5_interface_initialize_g) {
	if((n = H5I_nmembers(H5I_GROUP)))
	    H5I_clear_type(H5I_GROUP, FALSE, FALSE);
	else {
	    /* Destroy the group object id group */
	    H5I_dec_type_ref(H5I_GROUP);

            /* Free the global component buffer */
            H5G_traverse_term_interface();

	    /* Mark closed */
	    H5_interface_initialize_g = 0;
	    n = 1; /*H5I*/
	} /* end else */
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5G_term_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5G_create
 *
 * Purpose:	Creates a new empty group with the specified name. The name
 *		is either an absolute name or is relative to LOC.
 *
 * Return:	Success:	A handle for the group.	 The group is opened
 *				and should eventually be close by calling
 *				H5G_close().
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug 11 1997
 *
 *-------------------------------------------------------------------------
 */
H5G_t *
H5G_create(H5F_t *file, hid_t gcpl_id, hid_t dxpl_id)
{
    H5G_t	*grp = NULL;	/*new group			*/
    unsigned    oloc_init = 0;  /* Flag to indicate that the group object location was created successfully */
    H5G_t	*ret_value;	/* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_create)

    /* check args */
    HDassert(file);
    HDassert(gcpl_id != H5P_DEFAULT);
    HDassert(dxpl_id != H5P_DEFAULT);

    /* create an open group */
    if(NULL == (grp = H5FL_CALLOC(H5G_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    if(NULL == (grp->shared = H5FL_CALLOC(H5G_shared_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Create the group object header */
    if(H5G_obj_create(file, dxpl_id, gcpl_id, &(grp->oloc)/*out*/) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, NULL, "unable to create group object header")
    oloc_init = 1;    /* Indicate that the object location information is valid */

    /* Add group to list of open objects in file */
    if(H5FO_top_incr(grp->oloc.file, grp->oloc.addr) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINC, NULL, "can't incr object ref. count")
    if(H5FO_insert(grp->oloc.file, grp->oloc.addr, grp->shared, TRUE) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINSERT, NULL, "can't insert group into list of open objects")

    /* Set the count of times the object is opened */
    grp->shared->fo_count = 1;

    /* Set return value */
    ret_value = grp;

done:
    if(ret_value == NULL) {
        /* Check if we need to release the file-oriented symbol table info */
        if(oloc_init) {
            if(H5O_close(&(grp->oloc)) < 0)
                HDONE_ERROR(H5E_SYM, H5E_CLOSEERROR, NULL, "unable to release object header")
            if(H5O_delete(file, dxpl_id, grp->oloc.addr) < 0)
                HDONE_ERROR(H5E_SYM, H5E_CANTDELETE, NULL, "unable to delete object header")
        } /* end if */
        if(grp != NULL) {
            if(grp->shared != NULL)
                grp->shared = H5FL_FREE(H5G_shared_t, grp->shared);
            grp = H5FL_FREE(H5G_t, grp);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_create() */


/*-------------------------------------------------------------------------
 * Function:	H5G_open_name
 *
 * Purpose:	Opens an existing group by name.
 *
 * Return:	Success:	Ptr to a new group.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		Monday, August	27, 2007
 *
 *-------------------------------------------------------------------------
 */
H5G_t *
H5G_open_name(const H5G_loc_t *loc, const char *name, hid_t gapl_id,
    hid_t dxpl_id)
{
    H5G_t      *grp = NULL;             /* Group to open */
    H5G_loc_t   grp_loc;                /* Location used to open group */
    H5G_name_t  grp_path;            	/* Opened object group hier. path */
    H5O_loc_t   grp_oloc;            	/* Opened object object location */
    hbool_t     loc_found = FALSE;      /* Location at 'name' found */
    H5O_type_t  obj_type;               /* Type of object at location */
    H5G_t      *ret_value;              /* Return value */

    FUNC_ENTER_NOAPI(H5G_open_name, NULL)

    /* Check args */
    HDassert(loc);
    HDassert(name);

    /* Set up opened group location to fill in */
    grp_loc.oloc = &grp_oloc;
    grp_loc.path = &grp_path;
    H5G_loc_reset(&grp_loc);

    /* Find the group object using the gapl passed in */
    if(H5G_loc_find(loc, name, &grp_loc/*out*/, gapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "group not found")
    loc_found = TRUE;

    /* Check that the object found is the correct type */
    if(H5O_obj_type(&grp_oloc, &obj_type, dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, NULL, "can't get object type")
    if(obj_type != H5O_TYPE_GROUP)
        HGOTO_ERROR(H5E_SYM, H5E_BADTYPE, NULL, "not a group")

    /* Open the group */
    if((grp = H5G_open(&grp_loc, dxpl_id)) == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, NULL, "unable to open group")

    /* Set return value */
    ret_value = grp;

done:
    if(!ret_value) {
        if(loc_found && H5G_loc_free(&grp_loc) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, NULL, "can't free location")
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_open_name() */


/*-------------------------------------------------------------------------
 * Function:	H5G_open
 *
 * Purpose:	Opens an existing group.  The group should eventually be
 *		closed by calling H5G_close().
 *
 * Return:	Success:	Ptr to a new group.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *		Monday, January	 5, 1998
 *
 *-------------------------------------------------------------------------
 */
H5G_t *
H5G_open(const H5G_loc_t *loc, hid_t dxpl_id)
{
    H5G_t           *grp = NULL;        /* Group opened */
    H5G_shared_t    *shared_fo;         /* Shared group object */
    H5G_t           *ret_value;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_open, NULL)

    /* Check args */
    HDassert(loc);

    /* Allocate the group structure */
    if(NULL == (grp = H5FL_CALLOC(H5G_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "can't allocate space for group")

    /* Shallow copy (take ownership) of the group location object */
    if(H5O_loc_copy(&(grp->oloc), loc->oloc, H5_COPY_SHALLOW) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTCOPY, NULL, "can't copy object location")
    if(H5G_name_copy(&(grp->path), loc->path, H5_COPY_SHALLOW) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTCOPY, NULL, "can't copy path")

    /* Check if group was already open */
    if((shared_fo = (H5G_shared_t *)H5FO_opened(grp->oloc.file, grp->oloc.addr)) == NULL) {

        /* Clear any errors from H5FO_opened() */
        H5E_clear_stack(NULL);

        /* Open the group object */
        if(H5G_open_oid(grp, dxpl_id) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "not found")

        /* Add group to list of open objects in file */
        if(H5FO_insert(grp->oloc.file, grp->oloc.addr, grp->shared, FALSE) < 0) {
            grp->shared = H5FL_FREE(H5G_shared_t, grp->shared);
            HGOTO_ERROR(H5E_SYM, H5E_CANTINSERT, NULL, "can't insert group into list of open objects")
        } /* end if */

        /* Increment object count for the object in the top file */
        if(H5FO_top_incr(grp->oloc.file, grp->oloc.addr) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINC, NULL, "can't increment object count")

        /* Set open object count */
        grp->shared->fo_count = 1;
    } /* end if */
    else {
        /* Point to shared group info */
        grp->shared = shared_fo;

        /* Increment shared reference count */
        shared_fo->fo_count++;

        /* Check if the object has been opened through the top file yet */
        if(H5FO_top_count(grp->oloc.file, grp->oloc.addr) == 0) {
            /* Open the object through this top file */
            if(H5O_open(&(grp->oloc)) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, NULL, "unable to open object header")
        } /* end if */

        /* Increment object count for the object in the top file */
        if(H5FO_top_incr(grp->oloc.file, grp->oloc.addr) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINC, NULL, "can't increment object count")
    } /* end else */

    /* Set return value */
    ret_value = grp;

done:
    if(!ret_value && grp) {
        H5O_loc_free(&(grp->oloc));
        H5G_name_free(&(grp->path));
        grp = H5FL_FREE(H5G_t, grp);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_open() */


/*-------------------------------------------------------------------------
 * Function:	H5G_open_oid
 *
 * Purpose:	Opens an existing group.  The group should eventually be
 *		closed by calling H5G_close().
 *
 * Return:	Success:	Ptr to a new group.
 *
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *	    Wednesday, March	17, 1999
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_open_oid(H5G_t *grp, hid_t dxpl_id)
{
    hbool_t             obj_opened = FALSE;
    herr_t		ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5G_open_oid)

    /* Check args */
    HDassert(grp);

    /* Allocate the shared information for the group */
    if(NULL == (grp->shared = H5FL_CALLOC(H5G_shared_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Grab the object header */
    if(H5O_open(&(grp->oloc)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open group")
    obj_opened = TRUE;

    /* Check if this object has the right message(s) to be treated as a group */
    if((H5O_msg_exists(&(grp->oloc), H5O_STAB_ID, dxpl_id) <= 0)
            && (H5O_msg_exists(&(grp->oloc), H5O_LINFO_ID, dxpl_id) <= 0))
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "not a group")

done:
    if(ret_value < 0) {
        if(obj_opened)
            H5O_close(&(grp->oloc));
        if(grp->shared)
            grp->shared = H5FL_FREE(H5G_shared_t, grp->shared);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_open_oid() */


/*-------------------------------------------------------------------------
 * Function:	H5G_close
 *
 * Purpose:	Closes the specified group.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Monday, January	 5, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_close(H5G_t *grp)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5G_close, FAIL)

    /* Check args */
    HDassert(grp && grp->shared);
    HDassert(grp->shared->fo_count > 0);

    --grp->shared->fo_count;

    if(0 == grp->shared->fo_count) {
        HDassert(grp != H5G_rootof(H5G_fileof(grp)));

        /* Remove the group from the list of opened objects in the file */
        if(H5FO_top_decr(grp->oloc.file, grp->oloc.addr) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "can't decrement count for object")
        if(H5FO_delete(grp->oloc.file, H5AC_dxpl_id, grp->oloc.addr) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "can't remove group from list of open objects")
        if(H5O_close(&(grp->oloc)) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to close")
        grp->shared = H5FL_FREE(H5G_shared_t, grp->shared);
    } else {
        /* Decrement the ref. count for this object in the top file */
        if(H5FO_top_decr(grp->oloc.file, grp->oloc.addr) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "can't decrement count for object")

        /* Check reference count for this object in the top file */
        if(H5FO_top_count(grp->oloc.file, grp->oloc.addr) == 0)
            if(H5O_close(&(grp->oloc)) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to close")

        /* If this group is a mount point and the mount point is the last open
         * reference to the group, then attempt to close down the file hierarchy
         */
        if(grp->shared->mounted && grp->shared->fo_count == 1) {
            /* Attempt to close down the file hierarchy */
            if(H5F_try_close(grp->oloc.file) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, FAIL, "problem attempting file close")
        } /* end if */
    } /* end else */

    if(H5G_name_free(&(grp->path)) < 0) {
        grp = H5FL_FREE(H5G_t, grp);
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "can't free group entry name")
    } /* end if */

    grp = H5FL_FREE(H5G_t, grp);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_close() */


/*-------------------------------------------------------------------------
 * Function:    H5G_free
 *
 * Purpose:	Free memory used by an H5G_t struct (and its H5G_shared_t).
 *          Does not close the group or decrement the reference count.
 *          Used to free memory used by the root group.
 *
 * Return:  Success:    Non-negative
 *	        Failure:    Negative
 *
 * Programmer:  James Laird
 *              Tuesday, September 7, 2004
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_free(H5G_t *grp)
{
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5G_free, FAIL)

    HDassert(grp && grp->shared);

    grp->shared = H5FL_FREE(H5G_shared_t, grp->shared);
    grp = H5FL_FREE(H5G_t, grp);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_free() */


/*-------------------------------------------------------------------------
 * Function:	H5G_oloc
 *
 * Purpose:	Returns a pointer to the object location for a group.
 *
 * Return:	Success:	Ptr to group entry
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Tuesday, March 24, 1998
 *
 *-------------------------------------------------------------------------
 */
H5O_loc_t *
H5G_oloc(H5G_t *grp)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_oloc)

    FUNC_LEAVE_NOAPI(grp ? &(grp->oloc) : NULL)
} /* end H5G_oloc() */


/*-------------------------------------------------------------------------
 * Function:	H5G_nameof
 *
 * Purpose:	Returns a pointer to the hier. name for a group.
 *
 * Return:	Success:	Ptr to hier. name
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 12, 2005
 *
 *-------------------------------------------------------------------------
 */
H5G_name_t *
H5G_nameof(H5G_t *grp)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_nameof)

    FUNC_LEAVE_NOAPI(grp ? &(grp->path) : NULL)
} /* end H5G_nameof() */


/*-------------------------------------------------------------------------
 * Function:	H5G_fileof
 *
 * Purpose:	Returns the file to which the specified group belongs.
 *
 * Return:	Success:	File pointer.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Tuesday, March 24, 1998
 *
 *-------------------------------------------------------------------------
 */
H5F_t *
H5G_fileof(H5G_t *grp)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_fileof)

    HDassert(grp);

    FUNC_LEAVE_NOAPI(grp->oloc.file)
} /* end H5G_fileof() */


/*-------------------------------------------------------------------------
 * Function:	H5G_free_grp_name
 *
 * Purpose:	Free the 'ID to name' buffers.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: August 22, 2002
 *
 * Comments: Used now only on the root group close, in H5F_close()
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_free_grp_name(H5G_t *grp)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5G_free_grp_name, FAIL)

    /* Check args */
    HDassert(grp && grp->shared);
    HDassert(grp->shared->fo_count > 0);

    /* Free the path */
    H5G_name_free(&(grp->path));

done:
     FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_free_grp_name() */


/*-------------------------------------------------------------------------
 * Function:	H5G_get_shared_count
 *
 * Purpose:	Queries the group object's "shared count"
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, July	 5, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_get_shared_count(H5G_t *grp)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_get_shared_count)

    /* Check args */
    HDassert(grp && grp->shared);

    FUNC_LEAVE_NOAPI(grp->shared->fo_count)
} /* end H5G_get_shared_count() */


/*-------------------------------------------------------------------------
 * Function:	H5G_mount
 *
 * Purpose:	Sets the 'mounted' flag for a group
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, July 19, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_mount(H5G_t *grp)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_mount)

    /* Check args */
    HDassert(grp && grp->shared);
    HDassert(grp->shared->mounted == FALSE);

    /* Set the 'mounted' flag */
    grp->shared->mounted = TRUE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G_mount() */


/*-------------------------------------------------------------------------
 * Function:	H5G_mounted
 *
 * Purpose:	Retrieves the 'mounted' flag for a group
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, July 15, 2008
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5G_mounted(H5G_t *grp)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_mounted)

    /* Check args */
    HDassert(grp && grp->shared);

    FUNC_LEAVE_NOAPI(grp->shared->mounted)
} /* end H5G_mounted() */


/*-------------------------------------------------------------------------
 * Function:	H5G_unmount
 *
 * Purpose:	Resets the 'mounted' flag for a group
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, July 19, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_unmount(H5G_t *grp)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_unmount)

    /* Check args */
    HDassert(grp && grp->shared);
    HDassert(grp->shared->mounted == TRUE);

    /* Reset the 'mounted' flag */
    grp->shared->mounted = FALSE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G_unmount() */


/*-------------------------------------------------------------------------
 * Function:	H5G_iterate_cb
 *
 * Purpose:     Callback function for iterating over links in a group
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Oct  3, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_iterate_cb(const H5O_link_t *lnk, void *_udata)
{
    H5G_iter_appcall_ud_t *udata = (H5G_iter_appcall_ud_t *)_udata;     /* User data for callback */
    herr_t ret_value = H5_ITER_ERROR;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_iterate_cb)

    /* Sanity check */
    HDassert(lnk);
    HDassert(udata);

    switch(udata->lnk_op.op_type) {
#ifndef H5_NO_DEPRECATED_SYMBOLS
        case H5G_LINK_OP_OLD:
            /* Make the old-type application callback */
            ret_value = (udata->lnk_op.op_func.op_old)(udata->gid, lnk->name, udata->op_data);
            break;
#endif /* H5_NO_DEPRECATED_SYMBOLS */

        case H5G_LINK_OP_NEW:
            {
                H5L_info_t info;    /* Link info */

                /* Retrieve the info for the link */
                if(H5G_link_to_info(lnk, &info) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTGET, H5_ITER_ERROR, "unable to get info for link")

                /* Make the application callback */
                ret_value = (udata->lnk_op.op_func.op_new)(udata->gid, lnk->name, &info, udata->op_data);
            }
            break;

        default:
            HDassert(0 && "Unknown link op type?!?");
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_iterate_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5G_iterate
 *
 * Purpose:     Private function for iterating over links in a group
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Oct  3, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_iterate(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t skip, hsize_t *last_lnk,
    const H5G_link_iterate_t *lnk_op, void *op_data, hid_t lapl_id, hid_t dxpl_id)
{
    H5G_loc_t	loc;            /* Location of parent for group */
    hid_t gid = -1;             /* ID of group to iterate over */
    H5G_t *grp = NULL;          /* Pointer to group data structure to iterate over */
    H5G_iter_appcall_ud_t udata; /* User data for callback */
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI(H5G_iterate, FAIL)

    /* Sanity check */
    HDassert(group_name);
    HDassert(last_lnk);
    HDassert(lnk_op && lnk_op->op_func.op_new);

    /*
     * Open the group on which to operate.  We also create a group ID which
     * we can pass to the application-defined operator.
     */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(NULL == (grp = H5G_open_name(&loc, group_name, lapl_id, dxpl_id)))
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open group")
    if((gid = H5I_register(H5I_GROUP, grp, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register group")

    /* Set up user data for callback */
    udata.gid = gid;
    udata.lnk_op = *lnk_op;
    udata.op_data = op_data;

    /* Call the real group iteration routine */
    if((ret_value = H5G_obj_iterate(&(grp->oloc), idx_type, order, skip, last_lnk, H5G_iterate_cb, &udata, dxpl_id)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "error iterating over links")

done:
    /* Release the group opened */
    if(gid > 0) {
        if(H5I_dec_ref(gid, TRUE) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "unable to close group")
    } /* end if */
    else if(grp && H5G_close(grp) < 0)
        HDONE_ERROR(H5E_SYM, H5E_CLOSEERROR, FAIL, "unable to release group")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_iterate() */


/*-------------------------------------------------------------------------
 * Function:    H5G_free_visit_visited
 *
 * Purpose:     Free the key for an object visited during a group traversal
 *
 * Return:      Non-negative on success, negative on failure
 *
 * Programmer:  Quincey Koziol
 *	        Nov  4, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_free_visit_visited(void *item, void UNUSED *key, void UNUSED *operator_data/*in,out*/)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_free_visit_visited)

    item = H5FL_FREE(H5_obj_t, item);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G_free_visit_visited() */


/*-------------------------------------------------------------------------
 * Function:	H5G_visit_cb
 *
 * Purpose:     Callback function for recursively visiting links from a group
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Nov  4, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_visit_cb(const H5O_link_t *lnk, void *_udata)
{
    H5G_iter_visit_ud_t *udata = (H5G_iter_visit_ud_t *)_udata;     /* User data for callback */
    H5L_info_t info;                    /* Link info */
    H5G_loc_t   obj_loc;                /* Location of object */
    H5G_name_t  obj_path;            	/* Object's group hier. path */
    H5O_loc_t   obj_oloc;            	/* Object's object location */
    hbool_t     obj_found = FALSE;      /* Object at 'name' found */
    size_t old_path_len = udata->curr_path_len; /* Length of path before appending this link's name */
    size_t link_name_len;               /* Length of link's name */
    size_t len_needed;                  /* Length of path string needed */
    herr_t ret_value = H5_ITER_CONT;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_visit_cb)

    /* Sanity check */
    HDassert(lnk);
    HDassert(udata);

    /* Check if we will need more space to store this link's relative path */
    /* ("+2" is for string terminator and possible '/' for group separator later) */
    link_name_len = HDstrlen(lnk->name);
    len_needed = udata->curr_path_len + link_name_len + 2;
    if(len_needed > udata->path_buf_size) {
        void *new_path;         /* Pointer to new path buffer */

        /* Attempt to allocate larger buffer for path */
        if(NULL == (new_path = H5MM_realloc(udata->path, len_needed)))
            HGOTO_ERROR(H5E_SYM, H5E_NOSPACE, H5_ITER_ERROR, "can't allocate path string")
        udata->path = (char *)new_path;
        udata->path_buf_size = len_needed;
    } /* end if */

    /* Build the link's relative path name */
    HDassert(udata->path[old_path_len] == '\0');
    HDstrcpy(&(udata->path[old_path_len]), lnk->name);
    udata->curr_path_len += link_name_len;

    /* Construct the link info from the link message */
    if(H5G_link_to_info(lnk, &info) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, H5_ITER_ERROR, "unable to get info for link")

    /* Make the application callback */
    ret_value = (udata->op)(udata->gid, udata->path, &info, udata->op_data);

    /* Check for doing more work */
    if(ret_value == H5_ITER_CONT && lnk->type == H5L_TYPE_HARD) {
        H5_obj_t obj_pos;       /* Object "position" for this object */

        /* Set up opened group location to fill in */
        obj_loc.oloc = &obj_oloc;
        obj_loc.path = &obj_path;
        H5G_loc_reset(&obj_loc);

        /* Find the object using the LAPL passed in */
        /* (Correctly handles mounted files) */
        if(H5G_loc_find(udata->curr_loc, lnk->name, &obj_loc/*out*/, udata->lapl_id, udata->dxpl_id) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, H5_ITER_ERROR, "object not found")
        obj_found = TRUE;

        /* Construct unique "position" for this object */
        H5F_GET_FILENO(obj_oloc.file, obj_pos.fileno);
        obj_pos.addr = obj_oloc.addr;

        /* Check if we've seen the object the link references before */
        if(NULL == H5SL_search(udata->visited, &obj_pos)) {
            H5O_type_t otype;       /* Basic object type (group, dataset, etc.) */
            unsigned rc;		/* Reference count of object    */

            /* Get the object's reference count and type */
            if(H5O_get_rc_and_type(&obj_oloc, udata->dxpl_id, &rc, &otype) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTGET, H5_ITER_ERROR, "unable to get object info")

            /* If its ref count is > 1, we add it to the list of visited objects */
            /* (because it could come up again during traversal) */
            if(rc > 1) {
                H5_obj_t *new_node;                  /* New object node for visited list */

                /* Allocate new object "position" node */
                if((new_node = H5FL_MALLOC(H5_obj_t)) == NULL)
                    HGOTO_ERROR(H5E_SYM, H5E_NOSPACE, H5_ITER_ERROR, "can't allocate object node")

                /* Set node information */
                *new_node = obj_pos;

                /* Add to list of visited objects */
                if(H5SL_insert(udata->visited, new_node, new_node) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTINSERT, H5_ITER_ERROR, "can't insert object node into visited list")
            } /* end if */

            /* If it's a group, we recurse into it */
            if(otype == H5O_TYPE_GROUP) {
                H5G_loc_t *old_loc = udata->curr_loc;       /* Pointer to previous group location info */
                H5_index_t idx_type = udata->idx_type;      /* Type of index to use */
                H5O_linfo_t	linfo;		        /* Link info message */
                htri_t linfo_exists;                    /* Whether the link info message exists */

                /* Add the path separator to the current path */
                HDassert(udata->path[udata->curr_path_len] == '\0');
                HDstrcpy(&(udata->path[udata->curr_path_len]), "/");
                udata->curr_path_len++;

                /* Attempt to get the link info for this group */
                if((linfo_exists = H5G_obj_get_linfo(&obj_oloc, &linfo, udata->dxpl_id)) < 0)
                    HGOTO_ERROR(H5E_SYM, H5E_CANTGET, H5_ITER_ERROR, "can't check for link info message")
                if(linfo_exists) {
                    /* Check for creation order tracking, if creation order index lookup requested */
                    if(idx_type == H5_INDEX_CRT_ORDER) {
                        /* Check if creation order is tracked */
                        if(!linfo.track_corder)
                            /* Switch to name order for this group */
                            idx_type = H5_INDEX_NAME;
                    } /* end if */
                    else
                        HDassert(idx_type == H5_INDEX_NAME);
                } /* end if */
                else {
                    /* Can only perform name lookups on groups with symbol tables */
                    if(idx_type != H5_INDEX_NAME)
                        /* Switch to name order for this group */
                        idx_type = H5_INDEX_NAME;
                } /* end if */

                /* Point to this group's location info */
                udata->curr_loc = &obj_loc;

                /* Iterate over links in group */
                ret_value = H5G_obj_iterate(&obj_oloc, idx_type, udata->order, (hsize_t)0, NULL, H5G_visit_cb, udata, udata->dxpl_id);

                /* Restore location */
                udata->curr_loc = old_loc;
            } /* end if */
        } /* end if */
    } /* end if */

done:
    /* Reset path back to incoming path */
    udata->path[old_path_len] = '\0';
    udata->curr_path_len = old_path_len;

    /* Release resources */
    if(obj_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, H5_ITER_ERROR, "can't free location")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_visit_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5G_visit
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
 *
 *
 * Programmer:	Quincey Koziol
 *		November 4 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_visit(hid_t loc_id, const char *group_name, H5_index_t idx_type,
    H5_iter_order_t order, H5L_iterate_t op, void *op_data, hid_t lapl_id,
    hid_t dxpl_id)
{
    H5G_iter_visit_ud_t udata;      /* User data for callback */
    H5O_linfo_t	linfo;		    /* Link info message */
    htri_t linfo_exists;            /* Whether the link info message exists */
    hid_t       gid = (-1);         /* Group ID */
    H5G_t      *grp = NULL;         /* Group opened */
    H5G_loc_t	loc;                /* Location of group passed in */
    H5G_loc_t	start_loc;          /* Location of starting group */
    unsigned    rc;		    /* Reference count of object    */
    herr_t      ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5G_visit, FAIL)

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

    /* Open the group to begin visiting within */
    if((grp = H5G_open_name(&loc, group_name, lapl_id, dxpl_id)) == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open group")

    /* Register an ID for the starting group */
    if((gid = H5I_register(H5I_GROUP, grp, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register group")

    /* Get the location of the starting group */
    if(H5G_loc(gid, &start_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

    /* Set up user data */
    udata.gid = gid;
    udata.curr_loc = &start_loc;
    udata.lapl_id = lapl_id;
    udata.dxpl_id = dxpl_id;
    udata.idx_type = idx_type;
    udata.order = order;
    udata.op = op;
    udata.op_data = op_data;

    /* Allocate space for the path name */
    if(NULL == (udata.path = H5MM_strdup("")))
        HGOTO_ERROR(H5E_SYM, H5E_NOSPACE, FAIL, "can't allocate path name buffer")
    udata.path_buf_size = 1;
    udata.curr_path_len = 0;

    /* Create skip list to store visited object information */
    if((udata.visited = H5SL_create(H5SL_TYPE_OBJ)) == NULL)
        HGOTO_ERROR(H5E_SYM, H5E_CANTCREATE, FAIL, "can't create skip list for visited objects")

    /* Get the group's reference count */
    if(H5O_get_rc_and_type(&grp->oloc, dxpl_id, &rc, NULL) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "unable to get object info")

    /* If its ref count is > 1, we add it to the list of visited objects */
    /* (because it could come up again during traversal) */
    if(rc > 1) {
        H5_obj_t *obj_pos;                  /* New object node for visited list */

        /* Allocate new object "position" node */
        if((obj_pos = H5FL_MALLOC(H5_obj_t)) == NULL)
            HGOTO_ERROR(H5E_SYM, H5E_NOSPACE, FAIL, "can't allocate object node")

        /* Construct unique "position" for this object */
        H5F_GET_FILENO(grp->oloc.file, obj_pos->fileno);
        obj_pos->addr = grp->oloc.addr;

        /* Add to list of visited objects */
        if(H5SL_insert(udata.visited, obj_pos, obj_pos) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTINSERT, FAIL, "can't insert object node into visited list")
    } /* end if */

    /* Attempt to get the link info for this group */
    if((linfo_exists = H5G_obj_get_linfo(&(grp->oloc), &linfo, dxpl_id)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't check for link info message")
    if(linfo_exists) {
        /* Check for creation order tracking, if creation order index lookup requested */
        if(idx_type == H5_INDEX_CRT_ORDER) {
            /* Check if creation order is tracked */
            if(!linfo.track_corder)
                /* Switch to name order for this group */
                idx_type = H5_INDEX_NAME;
        } /* end if */
        else
            HDassert(idx_type == H5_INDEX_NAME);
    } /* end if */
    else {
        /* Can only perform name lookups on groups with symbol tables */
        if(idx_type != H5_INDEX_NAME)
            /* Switch to name order for this group */
            idx_type = H5_INDEX_NAME;
    } /* end if */

    /* Call the link iteration routine */
    if((ret_value = H5G_obj_iterate(&(grp->oloc), idx_type, order, (hsize_t)0, NULL, H5G_visit_cb, &udata, dxpl_id)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "can't visit links")

done:
    /* Release user data resources */
    H5MM_xfree(udata.path);
    if(udata.visited)
        H5SL_destroy(udata.visited, H5G_free_visit_visited, NULL);

    /* Release the group opened */
    if(gid > 0) {
        if(H5I_dec_ref(gid, TRUE) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "unable to close group")
    } /* end if */
    else if(grp && H5G_close(grp) < 0)
        HDONE_ERROR(H5E_SYM, H5E_CLOSEERROR, FAIL, "unable to release group")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_visit() */

