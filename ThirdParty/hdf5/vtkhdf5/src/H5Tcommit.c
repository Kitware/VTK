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

/*
 * Module Info: This module contains the functionality for committing datatypes
 *      to a file for the H5T interface.
 */

/****************/
/* Module Setup */
/****************/

#define H5T_PACKAGE		/*suppress error about including H5Tpkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5T_init_commit_interface


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5Eprivate.h"		/* Error handling			*/
#include "H5FOprivate.h"	/* File objects				*/
#include "H5Iprivate.h"		/* IDs					*/
#include "H5Lprivate.h"		/* Links				*/
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5Tpkg.h"		/* Datatypes				*/


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
static H5T_t *H5T_open_oid(const H5G_loc_t *loc, hid_t dxpl_id);


/*********************/
/* Public Variables */
/*********************/


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*--------------------------------------------------------------------------
NAME
   H5T_init_commit_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_commit_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5T_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5T_init_commit_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_init_commit_interface)

    FUNC_LEAVE_NOAPI(H5T_init())
} /* H5T_init_commit_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Tcommit2
 *
 * Purpose:	Save a transient datatype to a file and turn the type handle
 *		into a "named", immutable type.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              April 5, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tcommit2(hid_t loc_id, const char *name, hid_t type_id, hid_t lcpl_id,
    hid_t tcpl_id, hid_t tapl_id)
{
    H5G_loc_t	loc;                    /* Location to create datatype */
    H5T_t	*type;                  /* Datatype for ID */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(H5Tcommit2, FAIL)
    H5TRACE6("e", "i*siiii", loc_id, name, type_id, lcpl_id, tcpl_id, tapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Get correct property list */
    if(H5P_DEFAULT == lcpl_id)
        lcpl_id = H5P_LINK_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link creation property list")

    /* Get correct property list */
    if(H5P_DEFAULT == tcpl_id)
        tcpl_id = H5P_DATATYPE_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(tcpl_id, H5P_DATATYPE_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not datatype creation property list")

    /* Get correct property list */
    if(H5P_DEFAULT == tapl_id)
        tapl_id = H5P_DATATYPE_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(tapl_id, H5P_DATATYPE_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not datatype access property list")

    /* Commit the type */
    if(H5T_commit_named(&loc, name, type, lcpl_id, tcpl_id, tapl_id, H5AC_dxpl_id) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to commit datatype")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tcommit2() */


/*-------------------------------------------------------------------------
 * Function:	H5T_commit_named
 *
 * Purpose:	Internal routine to save a transient datatype to a file and
 *              turn the type ID into a "named", immutable type.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              April 5, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_commit_named(const H5G_loc_t *loc, const char *name, H5T_t *dt,
    hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id, hid_t dxpl_id)
{
    H5O_obj_create_t ocrt_info;             /* Information for object creation */
    H5T_obj_create_t tcrt_info;             /* Information for named datatype creation */
    H5T_state_t old_state = H5T_STATE_TRANSIENT;        /* The state of the datatype before H5T_commit. */
    herr_t      ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI(H5T_commit_named, FAIL)

    /* Sanity checks */
    HDassert(loc);
    HDassert(name && *name);
    HDassert(dt);
    HDassert(lcpl_id != H5P_DEFAULT);
    HDassert(tcpl_id != H5P_DEFAULT);
    HDassert(tapl_id != H5P_DEFAULT);
    HDassert(dxpl_id != H5P_DEFAULT);

    /* Record the type's state so that we can revert to it if linking fails */
    old_state = dt->shared->state;

    /* Set up named datatype creation info */
    tcrt_info.dt = dt;
    tcrt_info.tcpl_id = tcpl_id;

    /* Set up object creation information */
    ocrt_info.obj_type = H5O_TYPE_NAMED_DATATYPE;
    ocrt_info.crt_info = &tcrt_info;
    ocrt_info.new_obj = NULL;

    /* Create the new named datatype and link it to its parent group */
    if(H5L_link_object(loc, name, &ocrt_info, lcpl_id, tapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to create and link to named datatype")
    HDassert(ocrt_info.new_obj);

done:
    /* If the datatype was committed but something failed after that, we need
     * to return it to the state it was in before it was committed.
     */
    if(ret_value < 0 && ocrt_info.new_obj) {
	if(dt->shared->state == H5T_STATE_OPEN && dt->sh_loc.type == H5O_SHARE_TYPE_COMMITTED) {
            /* Remove the datatype from the list of opened objects in the file */
            if(H5FO_top_decr(dt->sh_loc.file, dt->sh_loc.u.loc.oh_addr) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "can't decrement count for object")
            if(H5FO_delete(dt->sh_loc.file, dxpl_id, dt->sh_loc.u.loc.oh_addr) < 0)
                HDONE_ERROR(H5E_DATASET, H5E_CANTRELEASE, FAIL, "can't remove dataset from list of open objects")

            /* Close the datatype object */
	    if(H5O_close(&(dt->oloc)) < 0)
                HDONE_ERROR(H5E_DATATYPE, H5E_CLOSEERROR, FAIL, "unable to release object header")

            /* Remove the datatype's object header from the file */
            if(H5O_delete(dt->sh_loc.file, dxpl_id, dt->sh_loc.u.loc.oh_addr) < 0)
                HDONE_ERROR(H5E_DATATYPE, H5E_CANTDELETE, FAIL, "unable to delete object header")

            /* Mark datatype as being back in memory */
            if(H5T_set_loc(dt, dt->sh_loc.file, H5T_LOC_MEMORY))
                HDONE_ERROR(H5E_DATATYPE, H5E_CANTDELETE, FAIL, "unable to return datatype to memory")
	    dt->sh_loc.type = H5O_SHARE_TYPE_UNSHARED;
            dt->shared->state = old_state;
	} /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_commit_named() */


/*-------------------------------------------------------------------------
 * Function:	H5Tcommit_anon
 *
 * Purpose:	Save a transient datatype to a file and turn the type handle
 *		into a "named", immutable type.
 *
 *              The resulting ID should be linked into the file with
 *              H5Olink or it will be deleted when closed.
 *
 * Note:	Datatype access property list is unused currently, but is
 *		checked for sanity anyway.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:  Peter Cao
 *              May 17, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tcommit_anon(hid_t loc_id, hid_t type_id, hid_t tcpl_id, hid_t tapl_id)
{
    H5G_loc_t	loc;                    /* Group location for location */
    H5T_t	*type = NULL;           /* Datatype created */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(H5Tcommit_anon, FAIL)
    H5TRACE4("e", "iiii", loc_id, type_id, tcpl_id, tapl_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Get correct property list */
    if(H5P_DEFAULT == tcpl_id)
        tcpl_id = H5P_DATATYPE_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(tcpl_id, H5P_DATATYPE_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not datatype creation property list")

    /* Get correct property list */
    if(H5P_DEFAULT == tapl_id)
        tapl_id = H5P_DATATYPE_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(tapl_id, H5P_DATATYPE_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not datatype access property list")

    /* Commit the type */
    if(H5T_commit(loc.oloc->file, type, tcpl_id, H5AC_dxpl_id) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to commit datatype")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tcommit_anon() */


/*-------------------------------------------------------------------------
 * Function:	H5T_commit
 *
 * Purpose:	Commit a type, giving it a name and causing it to become
 *		immutable.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, June  1, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_commit(H5F_t *file, H5T_t *type, hid_t tcpl_id, hid_t dxpl_id)
{
    H5O_loc_t   temp_oloc;              /* Temporary object header location */
    H5G_name_t  temp_path;              /* Temporary path */
    hbool_t     loc_init = FALSE;       /* Have temp_oloc and temp_path been initialized? */
    size_t      dtype_size;             /* Size of the datatype message */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5T_commit)

    HDassert(file);
    HDassert(type);
    HDassert(tcpl_id != H5P_DEFAULT);

    /* Check if we are allowed to write to this file */
    if(0 == (H5F_INTENT(file) & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_DATATYPE, H5E_WRITEERROR, FAIL, "no write intent on file")

    /*
     * Check arguments.  We cannot commit an immutable type because H5Tclose()
     * normally fails on such types (try H5Tclose(H5T_NATIVE_INT)) but closing
     * a named type should always succeed.
     */
    if(H5T_STATE_NAMED == type->shared->state || H5T_STATE_OPEN == type->shared->state)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "datatype is already committed")
    if(H5T_STATE_IMMUTABLE == type->shared->state)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "datatype is immutable")

    /* Check for a "sensible" datatype to store on disk */
    if(H5T_is_sensible(type) <= 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "datatype is not sensible")

    /* Mark datatype as being on disk now.  This step changes the size of
     *  datatype as stored on disk.
     */
    if(H5T_set_loc(type, file, H5T_LOC_DISK) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "cannot mark datatype on disk")

    /* Reset datatype location and path */
    if(H5O_loc_reset(&temp_oloc) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTRESET, FAIL, "unable to initialize location")
    if(H5G_name_reset(&temp_path) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTRESET, FAIL, "unable to initialize path")
    loc_init = TRUE;

    /* Set the latest format, if requested */
    if(H5F_USE_LATEST_FORMAT(file))
        if(H5T_set_latest_version(type) < 0)
            HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set latest version of datatype")

    /* Calculate message size infomation, for creating object header */
    dtype_size = H5O_msg_size_f(file, tcpl_id, H5O_DTYPE_ID, type, (size_t)0);
    HDassert(dtype_size);

    /*
     * Create the object header and open it for write access. Insert the data
     * type message and then give the object header a name.
     */
    if(H5O_create(file, dxpl_id, dtype_size, tcpl_id, &temp_oloc) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to create datatype object header")
    if(H5O_msg_create(&temp_oloc, H5O_DTYPE_ID, H5O_MSG_FLAG_CONSTANT | H5O_MSG_FLAG_DONTSHARE, H5O_UPDATE_TIME, type, dxpl_id) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to update type header message")

    /* Copy the new object header's location into the datatype, taking ownership of it */
    if(H5O_loc_copy(&(type->oloc), &temp_oloc, H5_COPY_SHALLOW) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy datatype location")
    if(H5G_name_copy(&(type->path), &temp_path, H5_COPY_SHALLOW) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy datatype location")
    loc_init = FALSE;

    /* Set the shared info fields */
    H5T_update_shared(type);
    type->shared->state = H5T_STATE_OPEN;
    type->shared->fo_count = 1;

    /* Add datatype to the list of open objects in the file */
    if(H5FO_top_incr(type->sh_loc.file, type->sh_loc.u.loc.oh_addr) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINC, FAIL, "can't incr object ref. count")
    if(H5FO_insert(type->sh_loc.file, type->sh_loc.u.loc.oh_addr, type->shared, TRUE) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL, "can't insert datatype into list of open objects")

    /* Mark datatype as being on memory again.  Since this datatype may still be
     *  used in memory after committed to disk, change its size back as in memory.
     */
    if(H5T_set_loc(type, NULL, H5T_LOC_MEMORY) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "cannot mark datatype in memory")

done:
    if(ret_value < 0) {
        if(loc_init) {
            H5O_loc_free(&temp_oloc);
            H5G_name_free(&temp_path);
        } /* end if */
        if((type->shared->state == H5T_STATE_TRANSIENT || type->shared->state == H5T_STATE_RDONLY) && (type->sh_loc.type == H5O_SHARE_TYPE_COMMITTED)) {
	    if(H5O_close(&(type->oloc)) < 0)
                HDONE_ERROR(H5E_DATATYPE, H5E_CLOSEERROR, FAIL, "unable to release object header")
            if(H5O_delete(file, dxpl_id, type->sh_loc.u.loc.oh_addr) < 0)
                HDONE_ERROR(H5E_DATATYPE, H5E_CANTDELETE, FAIL, "unable to delete object header")
	    type->sh_loc.type = H5O_SHARE_TYPE_UNSHARED;
	} /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5T_commit() */


/*-------------------------------------------------------------------------
 * Function:	H5Tcommitted
 *
 * Purpose:	Determines if a datatype is committed or not.
 *
 * Return:	Success:	TRUE if committed, FALSE otherwise.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Thursday, June  4, 1998
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Tcommitted(hid_t type_id)
{
    H5T_t	*type;          /* Datatype to query */
    htri_t      ret_value;      /* Return value */

    FUNC_ENTER_API(H5Tcommitted, FAIL)
    H5TRACE1("t", "i", type_id);

    /* Check arguments */
    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Set return value */
    ret_value = H5T_committed(type);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tcommitted() */


/*-------------------------------------------------------------------------
 * Function:	H5T_committed
 *
 * Purpose:	Determines if a datatype is committed or not.
 *
 * Return:	Success:	TRUE if committed, FALSE otherwise.
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, September 24, 2003
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_committed(const H5T_t *type)
{
    /* Use no-init for efficiency */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_committed)

    HDassert(type);

    FUNC_LEAVE_NOAPI(H5T_STATE_OPEN == type->shared->state || H5T_STATE_NAMED == type->shared->state)
} /* end H5T_committed() */


/*-------------------------------------------------------------------------
 * Function:	H5T_link
 *
 * Purpose:	Adjust the link count for an object header by adding
 *		ADJUST to the link count.
 *
 * Return:	Success:	New link count
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Friday, September 26, 2003
 *
 *-------------------------------------------------------------------------
 */
int
H5T_link(const H5T_t *type, int adjust, hid_t dxpl_id)
{
    int ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(H5T_link, FAIL)

    HDassert(type);
    HDassert(type->sh_loc.type == H5O_SHARE_TYPE_COMMITTED);

    /* Adjust the link count on the named datatype */
    if((ret_value = H5O_link(&type->oloc, adjust, dxpl_id)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_LINKCOUNT, FAIL, "unable to adjust named datatype link count")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_link() */


/*-------------------------------------------------------------------------
 * Function:	H5Topen2
 *
 * Purpose:	Opens a named datatype using a Datatype Access Property
 *              List.
 *
 * Return:	Success:	Object ID of the named datatype.
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *              Thursday July 27, 2006
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Topen2(hid_t loc_id, const char *name, hid_t tapl_id)
{
    H5T_t      *type = NULL;           /* Datatype opened in file */
    H5G_loc_t	 loc;                   /* Group location of object to open */
    H5G_name_t   path;            	/* Datatype group hier. path */
    H5O_loc_t    oloc;            	/* Datatype object location */
    H5O_type_t   obj_type;              /* Type of object at location */
    H5G_loc_t    type_loc;              /* Group object for datatype */
    hbool_t      obj_found = FALSE;     /* Object at 'name' found */
    hid_t        dxpl_id = H5AC_dxpl_id; /* dxpl to use to open datatype */
    hid_t        ret_value = FAIL;      /* Return value */

    FUNC_ENTER_API(H5Topen2, FAIL)
    H5TRACE3("i", "i*si", loc_id, name, tapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Get correct property list */
    if(H5P_DEFAULT == tapl_id)
        tapl_id = H5P_DATATYPE_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(tapl_id, H5P_DATATYPE_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not datatype access property list")

    /* Set up datatype location to fill in */
    type_loc.oloc = &oloc;
    type_loc.path = &path;
    H5G_loc_reset(&type_loc);

    /*
     * Find the named datatype object header and read the datatype message
     * from it.
     */
    if(H5G_loc_find(&loc, name, &type_loc/*out*/, tapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, FAIL, "not found")
    obj_found = TRUE;

    /* Check that the object found is the correct type */
    if(H5O_obj_type(&oloc, &obj_type, dxpl_id) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get object type")
    if(obj_type != H5O_TYPE_NAMED_DATATYPE)
        HGOTO_ERROR(H5E_DATATYPE, H5E_BADTYPE, FAIL, "not a named datatype")

    /* Open it */
    if(NULL == (type = H5T_open(&type_loc, dxpl_id)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, FAIL, "unable to open named datatype")

    /* Register the type and return the ID */
    if((ret_value = H5I_register(H5I_DATATYPE, type, TRUE)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register named datatype")

done:
    if(ret_value < 0) {
        if(type != NULL)
            H5T_close(type);
        else {
            if(obj_found && H5F_addr_defined(type_loc.oloc->addr))
                H5G_loc_free(&type_loc);
        } /* end else */
    } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* end H5Topen2() */


/*-------------------------------------------------------------------------
 * Function:	H5Tget_create_plist
 *
 * Purpose:	Returns a copy of the datatype creation property list.
 *
 * Return:	Success:	ID for a copy of the datatype creation
 *				property list.  The property list ID should be
 *				released by calling H5Pclose().
 *
 *		Failure:	FAIL
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, November 28, 2006
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tget_create_plist(hid_t dtype_id)
{
    H5T_t	        *type;          /* Datatype object for ID */
    H5P_genplist_t      *tcpl_plist;    /* Existing datatype creation propertty list */
    hid_t		new_tcpl_id = FAIL;     /* New datatype creation property list */
    herr_t              status;         /* Generic status value */
    hid_t		ret_value;      /* Return value */

    FUNC_ENTER_API(H5Tget_create_plist, FAIL)
    H5TRACE1("i", "i", dtype_id);

    /* Check arguments */
    if(NULL == (type = (H5T_t *)H5I_object_verify(dtype_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Copy the default datatype creation property list */
    if(NULL == (tcpl_plist = (H5P_genplist_t *)H5I_object(H5P_LST_DATATYPE_CREATE_g)))
         HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get default creation property list")
    if((new_tcpl_id = H5P_copy_plist(tcpl_plist, TRUE)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "unable to copy the creation property list")

    /* Check if the datatype is committed */
    if((status = H5T_committed(type)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't check whether datatype is committed")

    /* Retrieve further information, if the datatype is committed */
    if(status > 0) {
        H5P_genplist_t  *new_plist;     /* New datatype creation property list */

        /* Get property list object for new TCPL */
        if(NULL == (new_plist = (H5P_genplist_t *)H5I_object(new_tcpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")

        /* Retrieve any object creation properties */
        if(H5O_get_create_plist(&type->oloc, H5AC_ind_dxpl_id, new_plist) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get object creation info")
    } /* end if */

    /* Set the return value */
    ret_value = new_tcpl_id;

done:
    if(ret_value < 0)
        if(new_tcpl_id > 0)
            (void)H5I_dec_ref(new_tcpl_id, TRUE);

    FUNC_LEAVE_API(ret_value)
} /* end H5Tget_create_plist() */


/*-------------------------------------------------------------------------
 * Function:	H5T_open
 *
 * Purpose:	Open a named datatype.
 *
 * Return:	Success:	Ptr to a new datatype.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Monday, June  1, 1998
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_open(const H5G_loc_t *loc, hid_t dxpl_id)
{
    H5T_shared_t   *shared_fo = NULL;
    H5T_t          *dt = NULL;
    H5T_t          *ret_value;

    FUNC_ENTER_NOAPI(H5T_open, NULL)

    HDassert(loc);

    /* Check if datatype was already open */
    if(NULL == (shared_fo = (H5T_shared_t *)H5FO_opened(loc->oloc->file, loc->oloc->addr))) {
        /* Clear any errors from H5FO_opened() */
        H5E_clear_stack(NULL);

        /* Open the datatype object */
        if(NULL == (dt = H5T_open_oid(loc, dxpl_id)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_NOTFOUND, NULL, "not found")

        /* Add the datatype to the list of opened objects in the file */
        if(H5FO_insert(dt->sh_loc.file, dt->sh_loc.u.loc.oh_addr, dt->shared, FALSE) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINSERT, NULL, "can't insert datatype into list of open objects")

        /* Increment object count for the object in the top file */
        if(H5FO_top_incr(dt->sh_loc.file, dt->sh_loc.u.loc.oh_addr) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINC, NULL, "can't increment object count")

        /* Mark any datatypes as being in memory now */
        if(H5T_set_loc(dt, NULL, H5T_LOC_MEMORY) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "invalid datatype location")

        dt->shared->fo_count = 1;
    } /* end if */
    else {
        if(NULL == (dt = H5FL_MALLOC(H5T_t)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "can't allocate space for datatype")

#if defined(H5_USING_MEMCHECKER) || !defined(NDEBUG)
        /* Clear object location */
        if(H5O_loc_reset(&(dt->oloc)) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, NULL, "unable to reset location")

        /* Clear path name */
        if(H5G_name_reset(&(dt->path)) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, NULL, "unable to reset path")
#endif /* H5_USING_MEMCHECKER */

        /* Shallow copy (take ownership) of the object location object */
        if(H5O_loc_copy(&dt->oloc, loc->oloc, H5_COPY_SHALLOW) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, NULL, "can't copy object location")

        /* Shallow copy (take ownership) of the group hier. path */
        if(H5G_name_copy(&(dt->path), loc->path, H5_COPY_SHALLOW) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, NULL, "can't copy path")

        /* Set the shared component info */
        H5T_update_shared(dt);

        /* Point to shared datatype info */
        dt->shared = shared_fo;

        /* Mark any datatypes as being in memory now */
        if(H5T_set_loc(dt, NULL, H5T_LOC_MEMORY) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "invalid datatype location")

        /* Increment ref. count on shared info */
        shared_fo->fo_count++;

        /* Check if the object has been opened through the top file yet */
        if(H5FO_top_count(dt->sh_loc.file, dt->sh_loc.u.loc.oh_addr) == 0) {
            /* Open the object through this top file */
            if(H5O_open(&(dt->oloc)) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, NULL, "unable to open object header")
        } /* end if */

        /* Increment object count for the object in the top file */
        if(H5FO_top_incr(dt->sh_loc.file, dt->sh_loc.u.loc.oh_addr) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINC, NULL, "can't increment object count")
    } /* end else */

    ret_value = dt;

done:
    if(ret_value == NULL) {
        if(dt) {
            if(shared_fo == NULL)   /* Need to free shared fo */
                dt->shared = H5FL_FREE(H5T_shared_t, dt->shared);

            H5O_loc_free(&(dt->oloc));
            H5G_name_free(&(dt->path));

            dt = H5FL_FREE(H5T_t, dt);
        } /* end if */

        if(shared_fo)
            shared_fo->fo_count--;
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_open() */


/*-------------------------------------------------------------------------
 * Function:	H5T_open_oid
 *
 * Purpose:	Open a named datatype.
 *
 * Return:	Success:	Ptr to a new datatype.
 *
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, March 17, 1999
 *
 *-------------------------------------------------------------------------
 */
static H5T_t *
H5T_open_oid(const H5G_loc_t *loc, hid_t dxpl_id)
{
    H5T_t *dt = NULL;          /* Datatype from the file */
    H5T_t *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5T_open_oid)

    HDassert(loc);

    /* Open named datatype object in file */
    if(H5O_open(loc->oloc) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, NULL, "unable to open named datatype")

    /* Deserialize the datatype message into a datatype in memory */
    if(NULL == (dt = (H5T_t *)H5O_msg_read(loc->oloc, H5O_DTYPE_ID, NULL, dxpl_id)))
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to load type message from object header")

    /* Mark the type as named and open */
    dt->shared->state = H5T_STATE_OPEN;

    /* Shallow copy (take ownership) of the object location object */
    if(H5O_loc_copy(&dt->oloc, loc->oloc, H5_COPY_SHALLOW) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, NULL, "can't copy object location")

    /* Shallow copy (take ownership) of the group hier. path */
    if(H5G_name_copy(&(dt->path), loc->path, H5_COPY_SHALLOW) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTCOPY, NULL, "can't copy path")

    /* Set the shared component info */
    H5T_update_shared(dt);

    /* Set return value */
    ret_value = dt;

done:
    if(ret_value == NULL)
        if(dt == NULL)
            H5O_close(loc->oloc);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5T_open_oid() */


/*-------------------------------------------------------------------------
 * Function:	H5T_update_shared
 *
 * Purpose:	Update the shared location information from the object location
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Friday, April 13, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_update_shared(H5T_t *dt)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_update_shared)

    HDassert(dt);

    /* Set the shared location fields from the named datatype info */
    H5O_UPDATE_SHARED(&(dt->sh_loc), H5O_SHARE_TYPE_COMMITTED, dt->oloc.file, H5O_DTYPE_ID, 0, dt->oloc.addr)

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5T_update_shared() */

