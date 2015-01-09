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
 * Created:		H5O.c
 *			Aug  5 1997
 *			Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:		Object header routines.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5O_init_interface

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5FLprivate.h"	/* Free lists                           */
#include "H5FOprivate.h"        /* File objects                         */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Lprivate.h"		/* Links				*/
#include "H5MFprivate.h"	/* File memory management		*/
#include "H5Opkg.h"             /* Object headers			*/
#include "H5SMprivate.h"        /* Shared object header messages        */


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* User data for recursive traversal over objects from a group */
typedef struct {
    hid_t       obj_id;         /* The ID for the starting group */
    H5G_loc_t	*start_loc;     /* Location of starting group */
    hid_t       lapl_id;        /* LAPL for walking across links */
    hid_t       dxpl_id; 	/* DXPL for operations */
    H5SL_t     *visited;        /* Skip list for tracking visited nodes */
    H5O_iterate_t op;           /* Application callback */
    void       *op_data;        /* Application's op data */
} H5O_iter_visit_ud_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

static herr_t H5O_delete_oh(H5F_t *f, hid_t dxpl_id, H5O_t *oh);
static herr_t H5O_obj_type_real(H5O_t *oh, H5O_type_t *obj_type);
static herr_t H5O_visit(hid_t loc_id, const char *obj_name, H5_index_t idx_type,
    H5_iter_order_t order, H5O_iterate_t op, void *op_data, hid_t lapl_id,
    hid_t dxpl_id);
static herr_t H5O_get_hdr_info_real(const H5O_t *oh, H5O_hdr_info_t *hdr);
static const H5O_obj_class_t *H5O_obj_class_real(H5O_t *oh);


/*********************/
/* Package Variables */
/*********************/

/* Header message ID to class mapping */

/* Remember to increment H5O_MSG_TYPES in H5Opkg.h when adding a new
 * message.
 */

const H5O_msg_class_t *const H5O_msg_class_g[] = {
    H5O_MSG_NULL,		/*0x0000 Null				*/
    H5O_MSG_SDSPACE,		/*0x0001 Dataspace			*/
    H5O_MSG_LINFO,		/*0x0002 Link information		*/
    H5O_MSG_DTYPE,		/*0x0003 Datatype			*/
    H5O_MSG_FILL,       	/*0x0004 Old data storage -- fill value */
    H5O_MSG_FILL_NEW,		/*0x0005 New data storage -- fill value */
    H5O_MSG_LINK,		/*0x0006 Link 				*/
    H5O_MSG_EFL,		/*0x0007 Data storage -- external data files */
    H5O_MSG_LAYOUT,		/*0x0008 Data Layout			*/
#ifdef H5O_ENABLE_BOGUS
    H5O_MSG_BOGUS,		/*0x0009 "Bogus" (for testing)		*/
#else /* H5O_ENABLE_BOGUS */
    NULL,			/*0x0009 "Bogus" (for testing)		*/
#endif /* H5O_ENABLE_BOGUS */
    H5O_MSG_GINFO,		/*0x000A Group information		*/
    H5O_MSG_PLINE,		/*0x000B Data storage -- filter pipeline */
    H5O_MSG_ATTR,		/*0x000C Attribute			*/
    H5O_MSG_NAME,		/*0x000D Object name			*/
    H5O_MSG_MTIME,		/*0x000E Object modification date and time */
    H5O_MSG_SHMESG,		/*0x000F File-wide shared message table */
    H5O_MSG_CONT,		/*0x0010 Object header continuation	*/
    H5O_MSG_STAB,		/*0x0011 Symbol table			*/
    H5O_MSG_MTIME_NEW,		/*0x0012 New Object modification date and time */
    H5O_MSG_BTREEK,		/*0x0013 Non-default v1 B-tree 'K' values */
    H5O_MSG_DRVINFO,		/*0x0014 Driver info settings		*/
    H5O_MSG_AINFO,		/*0x0015 Attribute information		*/
    H5O_MSG_REFCOUNT,		/*0x0016 Object's ref. count		*/
    H5O_MSG_UNKNOWN		/*0x0017 Placeholder for unknown message */
};

/* Declare a free list to manage the H5O_t struct */
H5FL_DEFINE(H5O_t);

/* Declare a free list to manage the H5O_mesg_t sequence information */
H5FL_SEQ_DEFINE(H5O_mesg_t);

/* Declare a free list to manage the H5O_chunk_t sequence information */
H5FL_SEQ_DEFINE(H5O_chunk_t);

/* Declare a free list to manage the chunk image information */
H5FL_BLK_DEFINE(chunk_image);

/* Declare external the free list for H5O_cont_t sequences */
H5FL_SEQ_EXTERN(H5O_cont_t);


/*****************************/
/* Library Private Variables */
/*****************************/

/* Declare external the free list for time_t's */
H5FL_EXTERN(time_t);

/* Declare external the free list for H5_obj_t's */
H5FL_EXTERN(H5_obj_t);


/*******************/
/* Local Variables */
/*******************/

/* Header object ID to class mapping */
/*
 * Initialize the object class info table.  Begin with the most general types
 * and end with the most specific. For instance, any object that has a
 * datatype message is a datatype but only some of them are datasets.
 */
static const H5O_obj_class_t *const H5O_obj_class_g[] = {
    H5O_OBJ_DATATYPE,		/* Datatype object (H5O_TYPE_NAMED_DATATYPE - 2) */
    H5O_OBJ_DATASET,		/* Dataset object (H5O_TYPE_DATASET - 1) */
    H5O_OBJ_GROUP,		/* Group object (H5O_TYPE_GROUP - 0) */
};



/*-------------------------------------------------------------------------
 * Function:	H5O_init_interface
 *
 * Purpose:	Initialize information specific to H5O interface.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, January 18, 2007
 *
 * Changes:     JRM -- 12/12/07
 *              Added santity check verifying that H5O_msg_class_g
 *              is big enough.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_init_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* H5O interface sanity checks */
    HDassert(H5O_MSG_TYPES == NELMTS(H5O_msg_class_g));
    HDassert(sizeof(H5O_fheap_id_t) == H5O_FHEAP_ID_LEN);

    HDassert(H5O_UNKNOWN_ID < H5O_MSG_TYPES);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_init_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Oopen
 *
 * Purpose:	Opens an object within an HDF5 file.
 *
 *              This function opens an object in the same way that H5Gopen2,
 *              H5Topen2, and H5Dopen2 do. However, H5Oopen doesn't require
 *              the type of object to be known beforehand. This can be
 *              useful in user-defined links, for instance, when only a
 *              path is known.
 *
 *              The opened object should be closed again with H5Oclose
 *              or H5Gclose, H5Tclose, or H5Dclose.
 *
 * Return:	Success:	An open object identifier
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *		July 14 2006
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Oopen(hid_t loc_id, const char *name, hid_t lapl_id)
{
    H5G_loc_t	loc;
    hid_t       ret_value = FAIL;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("i", "i*si", loc_id, name, lapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Open the object */
    if((ret_value = H5O_open_name(&loc, name, lapl_id, TRUE)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open object")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oopen() */


/*-------------------------------------------------------------------------
 * Function:	H5Oopen_by_idx
 *
 * Purpose:	Opens an object within an HDF5 file, according to the offset
 *              within an index.
 *
 *              This function opens an object in the same way that H5Gopen,
 *              H5Topen, and H5Dopen do. However, H5Oopen doesn't require
 *              the type of object to be known beforehand. This can be
 *              useful in user-defined links, for instance, when only a
 *              path is known.
 *
 *              The opened object should be closed again with H5Oclose
 *              or H5Gclose, H5Tclose, or H5Dclose.
 *
 * Return:	Success:	An open object identifier
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		November 20 2006
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Oopen_by_idx(hid_t loc_id, const char *group_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, hid_t lapl_id)
{
    H5G_loc_t	loc;
    H5G_loc_t   obj_loc;                /* Location used to open group */
    H5G_name_t  obj_path;            	/* Opened object group hier. path */
    H5O_loc_t   obj_oloc;            	/* Opened object object location */
    hbool_t     loc_found = FALSE;      /* Entry at 'name' found */
    hid_t       ret_value = FAIL;

    FUNC_ENTER_API(FAIL)
    H5TRACE6("i", "i*sIiIohi", loc_id, group_name, idx_type, order, n, lapl_id);

    /* Check args */
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

    /* Set up opened group location to fill in */
    obj_loc.oloc = &obj_oloc;
    obj_loc.path = &obj_path;
    H5G_loc_reset(&obj_loc);

    /* Find the object's location, according to the order in the index */
    if(H5G_loc_find_by_idx(&loc, group_name, idx_type, order, n, &obj_loc/*out*/, lapl_id, H5AC_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "group not found")
    loc_found = TRUE;

    /* Open the object */
    if((ret_value = H5O_open_by_loc(&obj_loc, lapl_id, H5AC_dxpl_id, TRUE)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open object")

done:
    /* Release the object location if we failed after copying it */
    if(ret_value < 0 && loc_found)
        if(H5G_loc_free(&obj_loc) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "can't free location")

    FUNC_LEAVE_API(ret_value)
} /* end H5Oopen_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5Oopen_by_addr
 *
 * Purpose:	Warning! This function is EXTREMELY DANGEROUS!
 *              Improper use can lead to FILE CORRUPTION, INACCESSIBLE DATA,
 *              and other VERY BAD THINGS!
 *
 *              This function opens an object using its address within the
 *              HDF5 file, similar to an HDF5 hard link. The open object
 *              is identical to an object opened with H5Oopen() and should
 *              be closed with H5Oclose() or a type-specific closing
 *              function (such as H5Gclose() ).
 *
 *              This function is very dangerous if called on an invalid
 *              address. For this reason, H5Oincr_refcount() should be
 *              used to prevent HDF5 from deleting any object that is
 *              referenced by address (e.g. by a user-defined link).
 *              H5Odecr_refcount() should be used when the object is
 *              no longer being referenced by address (e.g. when the UD link
 *              is deleted).
 *
 *              The address of the HDF5 file on disk has no effect on
 *              H5Oopen_by_addr(), nor does the use of any unusual file
 *              drivers. The "address" is really the offset within the
 *              HDF5 file, and HDF5's file drivers will transparently
 *              map this to an address on disk for the filesystem.
 *
 * Return:	Success:	An open object identifier
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *		July 14 2006
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Oopen_by_addr(hid_t loc_id, haddr_t addr)
{
    H5G_loc_t	loc;
    H5G_loc_t   obj_loc;                /* Location used to open group */
    H5G_name_t  obj_path;            	/* Opened object group hier. path */
    H5O_loc_t   obj_oloc;            	/* Opened object object location */
    hid_t       lapl_id = H5P_LINK_ACCESS_DEFAULT; /* lapl to use to open this object */
    hid_t       ret_value = FAIL;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("i", "ia", loc_id, addr);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!H5F_addr_defined(addr))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no address supplied")

    /* Set up opened group location to fill in */
    obj_loc.oloc = &obj_oloc;
    obj_loc.path = &obj_path;
    H5G_loc_reset(&obj_loc);
    obj_loc.oloc->addr = addr;
    obj_loc.oloc->file = loc.oloc->file;
    H5G_name_reset(obj_loc.path);       /* objects opened through this routine don't have a path name */

    /* Open the object */
    if((ret_value = H5O_open_by_loc(&obj_loc, lapl_id, H5AC_dxpl_id, TRUE)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open object")

done:

    FUNC_LEAVE_API(ret_value)
} /* end H5Oopen_by_addr() */


/*-------------------------------------------------------------------------
 * Function:	H5Olink
 *
 * Purpose:	Creates a hard link from NEW_NAME to the object specified
 *		by OBJ_ID using properties defined in the Link Creation
 *              Property List LCPL.
 *
 *		This function should be used to link objects that have just
 *              been created.
 *
 *		NEW_NAME is interpreted relative to
 *		NEW_LOC_ID, which is either a file ID or a
 *		group ID.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *              Tuesday, December 13, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Olink(hid_t obj_id, hid_t new_loc_id, const char *new_name, hid_t lcpl_id,
    hid_t lapl_id)
{
    H5G_loc_t	new_loc;
    H5G_loc_t	obj_loc;
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "ii*sii", obj_id, new_loc_id, new_name, lcpl_id, lapl_id);

    /* Check arguments */
    if(H5G_loc(obj_id, &obj_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(new_loc_id == H5L_SAME_LOC)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "cannot use H5L_SAME_LOC when only one location is specified")
    if(H5G_loc(new_loc_id, &new_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!new_name || !*new_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified")
/* Avoid compiler warning on 32-bit machines */
#if H5_SIZEOF_SIZE_T > H5_SIZEOF_INT32_T
    if(HDstrlen(new_name) > H5L_MAX_LINK_NAME_LEN)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "name too long")
#endif /* H5_SIZEOF_SIZE_T > H5_SIZEOF_INT32_T */
    if(lcpl_id != H5P_DEFAULT && (TRUE != H5P_isa_class(lcpl_id, H5P_LINK_CREATE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a link creation property list")

    /* Link to the object */
    if(H5L_link(&new_loc, new_name, &obj_loc, lcpl_id, lapl_id, H5AC_dxpl_id) < 0)
        HGOTO_ERROR(H5E_LINK, H5E_CANTINIT, FAIL, "unable to create link")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Olink() */


/*-------------------------------------------------------------------------
 * Function:	H5Oincr_refcount
 *
 * Purpose:	Warning! This function is EXTREMELY DANGEROUS!
 *              Improper use can lead to FILE CORRUPTION, INACCESSIBLE DATA,
 *              and other VERY BAD THINGS!
 *
 *              This function increments the "hard link" reference count
 *              for an object. It should be used when a user-defined link
 *              that references an object by address is created. When the
 *              link is deleted, H5Odecr_refcount should be used.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *		July 14 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oincr_refcount(hid_t object_id)
{
    H5O_loc_t  *oloc;
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", object_id);

    /* Get the object's oloc so we can adjust its link count */
    if((oloc = H5O_get_loc(object_id)) == NULL)
        HGOTO_ERROR(H5E_ATOM, H5E_BADVALUE, FAIL, "unable to get object location from ID")

    if(H5O_link(oloc, 1, H5AC_dxpl_id) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_LINKCOUNT, FAIL, "modifying object link count failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5O_incr_refcount() */


/*-------------------------------------------------------------------------
 * Function:	H5Odecr_refcount
 *
 * Purpose:	Warning! This function is EXTREMELY DANGEROUS!
 *              Improper use can lead to FILE CORRUPTION, INACCESSIBLE DATA,
 *              and other VERY BAD THINGS!
 *
 *              This function decrements the "hard link" reference count
 *              for an object. It should be used when user-defined links
 *              that reference an object by address are deleted, and only
 *              after H5Oincr_refcount has already been used.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *		July 14 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Odecr_refcount(hid_t object_id)
{
    H5O_loc_t  *oloc;
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", object_id);

    /* Get the object's oloc so we can adjust its link count */
    if((oloc = H5O_get_loc(object_id)) == NULL)
        HGOTO_ERROR(H5E_ATOM, H5E_BADVALUE, FAIL, "unable to get object location from ID")

    if(H5O_link(oloc, -1, H5AC_dxpl_id) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_LINKCOUNT, FAIL, "modifying object link count failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Odecr_refcount() */


/*-------------------------------------------------------------------------
 * Function:	H5Oexists_by_name
 *
 * Purpose:	Determine if a linked-to object exists
 *
 * Return:	Success:	TRUE/FALSE
 * 		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		February  2 2010
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Oexists_by_name(hid_t loc_id, const char *name, hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Location info */
    hid_t       ret_value = FAIL;       /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("t", "i*si", loc_id, name, lapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Check if the object exists */
    if((ret_value = H5G_loc_exists(&loc, name, lapl_id, H5AC_dxpl_id)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to determine if '%s' exists", name)

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oexists_by_name() */


/*-------------------------------------------------------------------------
 * Function:	H5Oget_info
 *
 * Purpose:	Retrieve information about an object.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		November 21 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oget_info(hid_t loc_id, H5O_info_t *oinfo)
{
    H5G_loc_t	loc;                    /* Location of group */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", loc_id, oinfo);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")

    /* Retrieve the object's information */
    if(H5G_loc_info(&loc, ".", TRUE, oinfo/*out*/, H5P_LINK_ACCESS_DEFAULT, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_info() */


/*-------------------------------------------------------------------------
 * Function:	H5Oget_info_by_name
 *
 * Purpose:	Retrieve information about an object.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		November 21 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oget_info_by_name(hid_t loc_id, const char *name, H5O_info_t *oinfo, hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Location of group */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "i*s*xi", loc_id, name, oinfo, lapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Retrieve the object's information */
    if(H5G_loc_info(&loc, name, TRUE, oinfo/*out*/, lapl_id, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_info_by_name() */


/*-------------------------------------------------------------------------
 * Function:	H5Oget_info_by_idx
 *
 * Purpose:	Retrieve information about an object, according to the order
 *              of an index.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		November 26 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oget_info_by_idx(hid_t loc_id, const char *group_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, H5O_info_t *oinfo, hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Location of group */
    H5G_loc_t   obj_loc;                /* Location used to open group */
    H5G_name_t  obj_path;            	/* Opened object group hier. path */
    H5O_loc_t   obj_oloc;            	/* Opened object object location */
    hbool_t     loc_found = FALSE;      /* Entry at 'name' found */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "i*sIiIoh*xi", loc_id, group_name, idx_type, order, n, oinfo,
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
    if(!oinfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no info struct")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Set up opened group location to fill in */
    obj_loc.oloc = &obj_oloc;
    obj_loc.path = &obj_path;
    H5G_loc_reset(&obj_loc);

    /* Find the object's location, according to the order in the index */
    if(H5G_loc_find_by_idx(&loc, group_name, idx_type, order, n, &obj_loc/*out*/, lapl_id, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "group not found")
    loc_found = TRUE;

    /* Retrieve the object's information */
    if(H5O_get_info(obj_loc.oloc, H5AC_ind_dxpl_id, TRUE, oinfo) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't retrieve object info")

done:
    /* Release the object location */
    if(loc_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "can't free location")

    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_info_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5Oset_comment
 *
 * Purpose:     Gives the specified object a comment.  The COMMENT string
 *		should be a null terminated string.  An object can have only
 *		one comment at a time.  Passing NULL for the COMMENT argument
 *		will remove the comment property from the object.
 *
 * Note:	Deprecated in favor of using attributes on objects
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		August 30 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oset_comment(hid_t obj_id, const char *comment)
{
    H5G_loc_t	loc;                    /* Location of group */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*s", obj_id, comment);

    /* Check args */
    if(H5G_loc(obj_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

    /* (Re)set the object's comment */
    if(H5G_loc_set_comment(&loc, ".", comment, H5P_LINK_ACCESS_DEFAULT, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oset_comment() */


/*-------------------------------------------------------------------------
 * Function:	H5Oset_comment_by_name
 *
 * Purpose:     Gives the specified object a comment.  The COMMENT string
 *		should be a null terminated string.  An object can have only
 *		one comment at a time.  Passing NULL for the COMMENT argument
 *		will remove the comment property from the object.
 *
 * Note:	Deprecated in favor of using attributes on objects
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		August 30 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oset_comment_by_name(hid_t loc_id, const char *name, const char *comment,
    hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Location of group */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "i*s*si", loc_id, name, comment, lapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* (Re)set the object's comment */
    if(H5G_loc_set_comment(&loc, name, comment, lapl_id, H5AC_ind_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oset_comment_by_name() */


/*-------------------------------------------------------------------------
 * Function:	H5Oget_comment
 *
 * Purpose:	Retrieve comment for an object.
 *
 * Return:	Success:	Number of bytes in the comment excluding the
 *				null terminator.  Zero if the object has no
 *				comment.
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		August 30 2007
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Oget_comment(hid_t obj_id, char *comment, size_t bufsize)
{
    H5G_loc_t	loc;                    /* Location of group */
    ssize_t     ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("Zs", "i*sz", obj_id, comment, bufsize);

    /* Check args */
    if(H5G_loc(obj_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

    /* Retrieve the object's comment */
    if((ret_value = H5G_loc_get_comment(&loc, ".", comment/*out*/, bufsize, H5P_LINK_ACCESS_DEFAULT, H5AC_ind_dxpl_id)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_comment() */


/*-------------------------------------------------------------------------
 * Function:	H5Oget_comment_by_name
 *
 * Purpose:	Retrieve comment for an object.
 *
 * Return:	Success:	Number of bytes in the comment excluding the
 *				null terminator.  Zero if the object has no
 *				comment.
 *
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		August 30 2007
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Oget_comment_by_name(hid_t loc_id, const char *name, char *comment, size_t bufsize,
    hid_t lapl_id)
{
    H5G_loc_t	loc;                    /* Location of group */
    ssize_t     ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("Zs", "i*s*szi", loc_id, name, comment, bufsize, lapl_id);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Retrieve the object's comment */
    if((ret_value = H5G_loc_get_comment(&loc, name, comment/*out*/, bufsize, lapl_id, H5AC_ind_dxpl_id)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oget_comment_by_name() */


/*-------------------------------------------------------------------------
 * Function:	H5Ovisit
 *
 * Purpose:	Recursively visit an object and all the objects reachable
 *              from it.  If the starting object is a group, all the objects
 *              linked to from that group will be visited.  Links within
 *              each group are visited according to the order within the
 *              specified index (unless the specified index does not exist for
 *              a particular group, then the "name" index is used).
 *
 *              NOTE: Soft links and user-defined links are ignored during
 *              this operation.
 *
 *              NOTE: Each _object_ reachable from the initial group will only
 *              be visited once.  If multiple hard links point to the same
 *              object, the first link to the object's path (according to the
 *              iteration index and iteration order given) will be used to in
 *              the callback about the object.
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
 *		November 25 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ovisit(hid_t obj_id, H5_index_t idx_type, H5_iter_order_t order,
    H5O_iterate_t op, void *op_data)
{
    herr_t      ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "iIiIox*x", obj_id, idx_type, order, op, op_data);

    /* Check args */
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(!op)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no callback operator specified")

    /* Call internal object visitation routine */
    if((ret_value = H5O_visit(obj_id, ".", idx_type, order, op, op_data, H5P_LINK_ACCESS_DEFAULT, H5AC_ind_dxpl_id)) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "object visitation failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ovisit() */


/*-------------------------------------------------------------------------
 * Function:	H5Ovisit_by_name
 *
 * Purpose:	Recursively visit an object and all the objects reachable
 *              from it.  If the starting object is a group, all the objects
 *              linked to from that group will be visited.  Links within
 *              each group are visited according to the order within the
 *              specified index (unless the specified index does not exist for
 *              a particular group, then the "name" index is used).
 *
 *              NOTE: Soft links and user-defined links are ignored during
 *              this operation.
 *
 *              NOTE: Each _object_ reachable from the initial group will only
 *              be visited once.  If multiple hard links point to the same
 *              object, the first link to the object's path (according to the
 *              iteration index and iteration order given) will be used to in
 *              the callback about the object.
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
H5Ovisit_by_name(hid_t loc_id, const char *obj_name, H5_index_t idx_type,
    H5_iter_order_t order, H5O_iterate_t op, void *op_data, hid_t lapl_id)
{
    herr_t      ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "i*sIiIox*xi", loc_id, obj_name, idx_type, order, op, op_data,
             lapl_id);

    /* Check args */
    if(!obj_name || !*obj_name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
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

    /* Call internal object visitation routine */
    if((ret_value = H5O_visit(loc_id, obj_name, idx_type, order, op, op_data, lapl_id, H5AC_ind_dxpl_id)) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "object visitation failed")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ovisit_by_name() */


/*-------------------------------------------------------------------------
 * Function:	H5Oclose
 *
 * Purpose:	Close an open file object.
 *
 *              This is the companion to H5Oopen. It is used to close any
 *              open object in an HDF5 file (but not IDs are that not file
 *              objects, such as property lists and dataspaces). It has
 *              the same effect as calling H5Gclose, H5Dclose, or H5Tclose.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *		July 14 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Oclose(hid_t object_id)
{
    herr_t       ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", object_id);

    /* Get the type of the object and close it in the correct way */
    switch(H5I_get_type(object_id)) {
        case H5I_GROUP:
        case H5I_DATATYPE:
        case H5I_DATASET:
            if(H5I_object(object_id) == NULL)
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a valid object")
            if(H5I_dec_app_ref(object_id) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "unable to close object")
            break;

        case H5I_UNINIT:
        case H5I_BADID:
        case H5I_FILE:
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
            HGOTO_ERROR(H5E_ARGS, H5E_CANTRELEASE, FAIL, "not a valid file object ID (dataset, group, or datatype)")
        break;
    } /* end switch */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Oclose() */


/*-------------------------------------------------------------------------
 * Function:	H5O_create
 *
 * Purpose:	Creates a new object header. Allocates space for it and
 *              then calls an initialization function. The object header
 *              is opened for write access and should eventually be
 *              closed by calling H5O_close().
 *
 * Return:	Success:	Non-negative, the ENT argument contains
 *				information about the object header,
 *				including its address.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  5 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_create(H5F_t *f, hid_t dxpl_id, size_t size_hint, size_t initial_rc,
    hid_t ocpl_id, H5O_loc_t *loc/*out*/)
{
    H5P_genplist_t  *oc_plist;          /* Object creation property list */
    H5O_t      *oh = NULL;              /* Object header created */
    haddr_t     oh_addr;                /* Address of initial object header */
    size_t      oh_size;                /* Size of initial object header */
    uint8_t	oh_flags;		/* Object header's initial status flags */
    unsigned    insert_flags = H5AC__NO_FLAGS_SET; /* Flags for inserting object header into cache */
    hbool_t     store_msg_crt_idx;      /* Whether to always store message creation indices for this file */
    herr_t      ret_value = SUCCEED;    /* return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(loc);
    HDassert(TRUE == H5P_isa_class(ocpl_id, H5P_OBJECT_CREATE));

    /* Check for invalid access request */
    if(0 == (H5F_INTENT(f) & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, FAIL, "no write intent on file")

    /* Make certain we allocate at least a reasonable size for the object header */
    size_hint = H5O_ALIGN_F(f, MAX(H5O_MIN_SIZE, size_hint));

    /* Get the property list */
    if(NULL == (oc_plist = (H5P_genplist_t *)H5I_object(ocpl_id)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADTYPE, FAIL, "not a property list")

    /* Get any object header status flags set by properties */
    if(H5P_get(oc_plist, H5O_CRT_OHDR_FLAGS_NAME, &oh_flags) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get object header flags")

    /* Allocate the object header and zero out header fields */
    if(NULL == (oh = H5FL_CALLOC(H5O_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Initialize file-specific information for object header */
    store_msg_crt_idx = H5F_STORE_MSG_CRT_IDX(f);
    if(H5F_USE_LATEST_FORMAT(f) || store_msg_crt_idx || (oh_flags & H5O_HDR_ATTR_CRT_ORDER_TRACKED))
        oh->version = H5O_VERSION_LATEST;
    else
        oh->version = H5O_VERSION_1;
    oh->sizeof_size = H5F_SIZEOF_SIZE(f);
    oh->sizeof_addr = H5F_SIZEOF_ADDR(f);
#ifdef H5O_ENABLE_BAD_MESG_COUNT
    /* Check whether the "bad message count" property is set */
    if(H5P_exist_plist(oc_plist, H5O_BAD_MESG_COUNT_NAME) > 0) {
        /* Retrieve bad message count flag */
        if(H5P_get(oc_plist, H5O_BAD_MESG_COUNT_NAME, &oh->store_bad_mesg_count) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't get bad message count flag")
    } /* end if */
#endif /* H5O_ENABLE_BAD_MESG_COUNT */

    /* Set initial status flags */
    oh->flags = oh_flags;

    /* Initialize version-specific fields */
    if(oh->version > H5O_VERSION_1) {
        /* Initialize all time fields with current time, if we are storing them */
        if(oh->flags & H5O_HDR_STORE_TIMES)
            oh->atime = oh->mtime = oh->ctime = oh->btime = H5_now();
        else
            oh->atime = oh->mtime = oh->ctime = oh->btime = 0;

        /* Make certain attribute creation order tracking is enabled if
         *      attributes can be shared in this file.
         */
        if(store_msg_crt_idx)
            oh->flags |= H5O_HDR_ATTR_CRT_ORDER_TRACKED;

        /* Retrieve attribute storage phase change values from property list */
        if(H5P_get(oc_plist, H5O_CRT_ATTR_MAX_COMPACT_NAME, &oh->max_compact) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get max. # of compact attributes")
        if(H5P_get(oc_plist, H5O_CRT_ATTR_MIN_DENSE_NAME, &oh->min_dense) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get min. # of dense attributes")

        /* Check for non-default attribute storage phase change values */
        if(oh->max_compact != H5O_CRT_ATTR_MAX_COMPACT_DEF || oh->min_dense != H5O_CRT_ATTR_MIN_DENSE_DEF)
            oh->flags |= H5O_HDR_ATTR_STORE_PHASE_CHANGE;

        /* Determine correct value for chunk #0 size bits */
/* Avoid compiler warning on 32-bit machines */
#if H5_SIZEOF_SIZE_T > H5_SIZEOF_INT32_T
        if(size_hint > 4294967295UL)
            oh->flags |= H5O_HDR_CHUNK0_8;
        else
#endif /* H5_SIZEOF_SIZE_T > H5_SIZEOF_INT32_T */
        if(size_hint > 65535)
            oh->flags |= H5O_HDR_CHUNK0_4;
        else if(size_hint > 255)
            oh->flags |= H5O_HDR_CHUNK0_2;
    } /* end if */
    else {
        /* Reset unused time fields */
        oh->atime = oh->mtime = oh->ctime = oh->btime = 0;
    } /* end else */

    /* Compute total size of initial object header */
    /* (i.e. object header prefix and first chunk) */
    oh_size = (size_t)H5O_SIZEOF_HDR(oh) + size_hint;

    /* Allocate disk space for header and first chunk */
    if(HADDR_UNDEF == (oh_addr = H5MF_alloc(f, H5FD_MEM_OHDR, dxpl_id, (hsize_t)oh_size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "file allocation failed for object header")

    /* Create the chunk list */
    oh->nchunks = oh->alloc_nchunks = 1;
    if(NULL == (oh->chunk = H5FL_SEQ_MALLOC(H5O_chunk_t, (size_t)oh->alloc_nchunks)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Initialize the first chunk */
    oh->chunk[0].addr = oh_addr;
    oh->chunk[0].size = oh_size;
    oh->chunk[0].gap = 0;

    /* Allocate enough space for the first chunk */
    /* (including space for serializing the object header prefix */
    if(NULL == (oh->chunk[0].image = H5FL_BLK_CALLOC(chunk_image, oh_size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Put magic # for object header in first chunk */
    if(oh->version > H5O_VERSION_1)
        HDmemcpy(oh->chunk[0].image, H5O_HDR_MAGIC, (size_t)H5_SIZEOF_MAGIC);

    /* Create the message list */
    oh->nmesgs = 1;
    oh->alloc_nmesgs = H5O_NMESGS;
    if(NULL == (oh->mesg = H5FL_SEQ_CALLOC(H5O_mesg_t, oh->alloc_nmesgs)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    /* Initialize the initial "null" message, covering the entire first chunk */
    oh->mesg[0].type = H5O_MSG_NULL;
    oh->mesg[0].dirty = TRUE;
    oh->mesg[0].native = NULL;
    oh->mesg[0].raw = oh->chunk[0].image + (H5O_SIZEOF_HDR(oh) - H5O_SIZEOF_CHKSUM_OH(oh)) + H5O_SIZEOF_MSGHDR_OH(oh);
    oh->mesg[0].raw_size = size_hint - (size_t)H5O_SIZEOF_MSGHDR_OH(oh);
    oh->mesg[0].chunkno = 0;

    /* Check for non-zero initial refcount on the object header */
    if(initial_rc > 0) {
        /* Set the initial refcount & pin the header when its inserted */
        oh->rc = initial_rc;
        insert_flags |= H5AC__PIN_ENTRY_FLAG;
    } /* end if */

    /* Cache object header */
    if(H5AC_insert_entry(f, dxpl_id, H5AC_OHDR, oh_addr, oh, insert_flags) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "unable to cache object header")
    oh = NULL;

    /* Set up object location */
    loc->file = f;
    loc->addr = oh_addr;

    /* Open it */
    if(H5O_open(loc) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, FAIL, "unable to open object header")

done:
    if(ret_value < 0 && oh)
        if(H5O_free(oh) < 0)
	    HDONE_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to destroy object header data")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_create() */


/*-------------------------------------------------------------------------
 * Function:	H5O_open
 *
 * Purpose:	Opens an object header which is described by the symbol table
 *		entry OBJ_ENT.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Monday, January	 5, 1998
 *
 * Modification:
 *              Raymond Lu
 *              5 November 2007
 *              Turn off the holding file variable if it's on.  When it's
 *              needed, the caller will turn it on again.
 *-------------------------------------------------------------------------
 */
herr_t
H5O_open(H5O_loc_t *loc)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(loc);
    HDassert(loc->file);

#ifdef H5O_DEBUG
    if(H5DEBUG(O))
	HDfprintf(H5DEBUG(O), "> %a\n", loc->addr);
#endif

    /* Turn off the variable for holding file or increment open-lock counters */
    if(loc->holding_file)
     	loc->holding_file = FALSE;
    else
        H5F_INCR_NOPEN_OBJS(loc->file);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_open() */


/*-------------------------------------------------------------------------
 * Function:	H5O_open_name
 *
 * Purpose:	Opens an object within an HDF5 file.
 *
 * Return:	Success:	An open object identifier
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		March  5 2007
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5O_open_name(H5G_loc_t *loc, const char *name, hid_t lapl_id, hbool_t app_ref)
{
    H5G_loc_t   obj_loc;                /* Location used to open group */
    H5G_name_t  obj_path;            	/* Opened object group hier. path */
    H5O_loc_t   obj_oloc;            	/* Opened object object location */
    hbool_t     loc_found = FALSE;      /* Entry at 'name' found */
    hid_t       ret_value = FAIL;

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(loc);
    HDassert(name && *name);

    /* Set up opened group location to fill in */
    obj_loc.oloc = &obj_oloc;
    obj_loc.path = &obj_path;
    H5G_loc_reset(&obj_loc);

    /* Find the object's location */
    if(H5G_loc_find(loc, name, &obj_loc/*out*/, lapl_id, H5AC_dxpl_id) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found")
    loc_found = TRUE;

    /* Open the object */
    if((ret_value = H5O_open_by_loc(&obj_loc, lapl_id, H5AC_ind_dxpl_id, app_ref)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open object")

done:
    if(ret_value < 0 && loc_found)
        if(H5G_loc_free(&obj_loc) < 0)
            HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, FAIL, "can't free location")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_open_name() */


/*-------------------------------------------------------------------------
 * Function:	H5O_open_by_loc
 *
 * Purpose:	Opens an object and returns an ID given its group loction.
 *
 * Return:	Success:	Open object identifier
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *		July 25 2006
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5O_open_by_loc(const H5G_loc_t *obj_loc, hid_t lapl_id, hid_t dxpl_id, hbool_t app_ref)
{
    const H5O_obj_class_t *obj_class;   /* Class of object for location */
    hid_t      ret_value;               /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(obj_loc);

    /* Get the object class for this location */
    if(NULL == (obj_class = H5O_obj_class(obj_loc->oloc, dxpl_id)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to determine object class")

    /* Call the object class's 'open' routine */
    HDassert(obj_class->open);
    if((ret_value = obj_class->open(obj_loc, lapl_id, dxpl_id, app_ref)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, FAIL, "unable to open object")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_open_by_loc() */


/*-------------------------------------------------------------------------
 * Function:	H5O_close
 *
 * Purpose:	Closes an object header that was previously open.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Monday, January	 5, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_close(H5O_loc_t *loc)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_NOPEN_OBJS(loc->file) > 0);

    /* Decrement open-lock counters */
    H5F_DECR_NOPEN_OBJS(loc->file);

#ifdef H5O_DEBUG
    if(H5DEBUG(O)) {
	if(H5F_FILE_ID(loc->file)< 0 && 1 == H5F_NREFS(loc->file))
	    HDfprintf(H5DEBUG(O), "< %a auto %lu remaining\n",
		      loc->addr,
		      (unsigned long)H5F_NOPEN_OBJS(loc->file));
	else
	    HDfprintf(H5DEBUG(O), "< %a\n", loc->addr);
    } /* end if */
#endif

    /*
     * If the file open object count has reached the number of open mount points
     * (each of which has a group open in the file) attempt to close the file.
     */
    if(H5F_NOPEN_OBJS(loc->file) == H5F_NMOUNTS(loc->file))
        /* Attempt to close down the file hierarchy */
        if(H5F_try_close(loc->file) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTCLOSEFILE, FAIL, "problem attempting file close")

    /* Release location information */
    if(H5O_loc_free(loc) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "problem attempting to free location")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_close() */


/*-------------------------------------------------------------------------
 * Function:	H5O_link_oh
 *
 * Purpose:	Adjust the link count for an open object header by adding
 *		ADJUST to the link count.
 *
 * Return:	Success:	New link count
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  5 1997
 *
 *-------------------------------------------------------------------------
 */
int
H5O_link_oh(H5F_t *f, int adjust, hid_t dxpl_id, H5O_t *oh, hbool_t *deleted)
{
    haddr_t addr = H5O_OH_GET_ADDR(oh);     /* Object header address */
    int	ret_value;                      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(f);
    HDassert(oh);
    HDassert(deleted);

    /* Check for adjusting link count */
    if(adjust) {
        if(adjust < 0) {
            /* Check for too large of an adjustment */
            if((unsigned)(-adjust) > oh->nlink)
                HGOTO_ERROR(H5E_OHDR, H5E_LINKCOUNT, FAIL, "link count would be negative")

            /* Adjust the link count for the object header */
            oh->nlink = (unsigned)((int)oh->nlink + adjust);

            /* Mark object header as dirty in cache */
            if(H5AC_mark_entry_dirty(oh) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTMARKDIRTY, FAIL, "unable to mark object header as dirty")

            /* Check if the object should be deleted */
            if(oh->nlink == 0) {
                /* Check if the object is still open by the user */
                if(H5FO_opened(f, addr) != NULL) {
                    /* Flag the object to be deleted when it's closed */
                    if(H5FO_mark(f, addr, TRUE) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "can't mark object for deletion")
                } /* end if */
                else {
                    /* Mark the object header for deletion */
                    *deleted = TRUE;
                } /* end else */
            } /* end if */
        } /* end if */
        else {
            /* A new object, or one that will be deleted */
            if(0 == oh->nlink) {
                /* Check if the object is currently open, but marked for deletion */
                if(H5FO_marked(f, addr)) {
                    /* Remove "delete me" flag on the object */
                    if(H5FO_mark(f, addr, FALSE) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "can't mark object for deletion")
                } /* end if */
            } /* end if */

            /* Adjust the link count for the object header */
            oh->nlink = (unsigned)((int)oh->nlink + adjust);

            /* Mark object header as dirty in cache */
            if(H5AC_mark_entry_dirty(oh) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTMARKDIRTY, FAIL, "unable to mark object header as dirty")
        } /* end if */

        /* Check for operations on refcount message */
        if(oh->version > H5O_VERSION_1) {
            /* Check if the object has a refcount message already */
            if(oh->has_refcount_msg) {
                /* Check for removing refcount message */
                if(oh->nlink <= 1) {
                    if(H5O_msg_remove_real(f, oh, H5O_MSG_REFCOUNT, H5O_ALL, NULL, NULL, TRUE, dxpl_id) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to delete refcount message")
                    oh->has_refcount_msg = FALSE;
                } /* end if */
                /* Update refcount message with new link count */
                else {
                    H5O_refcount_t refcount = oh->nlink;

                    if(H5O_msg_write_real(f, dxpl_id, oh, H5O_MSG_REFCOUNT, H5O_MSG_FLAG_DONTSHARE, 0, &refcount) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTUPDATE, FAIL, "unable to update refcount message")
                } /* end else */
            } /* end if */
            else {
                /* Check for adding refcount message to object */
                if(oh->nlink > 1) {
                    H5O_refcount_t refcount = oh->nlink;

                    if(H5O_msg_append_real(f, dxpl_id, oh, H5O_MSG_REFCOUNT, H5O_MSG_FLAG_DONTSHARE, 0, &refcount) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "unable to create new refcount message")
                    oh->has_refcount_msg = TRUE;
                } /* end if */
            } /* end else */
        } /* end if */
    } /* end if */

    /* Set return value */
    ret_value = (int)oh->nlink;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_link_oh() */


/*-------------------------------------------------------------------------
 * Function:	H5O_link
 *
 * Purpose:	Adjust the link count for an object header by adding
 *		ADJUST to the link count.
 *
 * Return:	Success:	New link count
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug  5 1997
 *
 *-------------------------------------------------------------------------
 */
int
H5O_link(const H5O_loc_t *loc, int adjust, hid_t dxpl_id)
{
    H5O_t	*oh = NULL;
    hbool_t deleted = FALSE;            /* Whether the object was deleted */
    int	ret_value;                      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);
    HDassert(loc->file);
    HDassert(H5F_addr_defined(loc->addr));

    /* Pin the object header */
    if(NULL == (oh = H5O_pin(loc, dxpl_id)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPIN, FAIL, "unable to pin object header")

    /* Call the "real" link routine */
    if((ret_value = H5O_link_oh(loc->file, adjust, dxpl_id, oh, &deleted)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_LINKCOUNT, FAIL, "unable to adjust object link count")

done:
    if(oh && H5O_unpin(oh) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPIN, FAIL, "unable to unpin object header")
    if(ret_value >= 0 && deleted && H5O_delete(loc->file, dxpl_id, loc->addr) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "can't delete object from file")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_link() */


/*-------------------------------------------------------------------------
 * Function:	H5O_protect
 *
 * Purpose:	Wrapper around H5AC_protect for use during a H5O_protect->
 *              H5O_msg_append->...->H5O_msg_append->H5O_unprotect sequence of calls
 *              during an object's creation.
 *
 * Return:	Success:	Pointer to the object header structure for the
 *                              object.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Dec 31 2002
 *
 *-------------------------------------------------------------------------
 */
H5O_t *
H5O_protect(const H5O_loc_t *loc, hid_t dxpl_id, H5AC_protect_t prot)
{
    H5O_t *oh = NULL;           /* Object header protected */
    H5O_cache_ud_t udata;       /* User data for protecting object header */
    H5O_cont_msgs_t cont_msg_info;      /* Continuation message info */
    unsigned file_intent;       /* R/W intent on file */
    H5O_t *ret_value;           /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* check args */
    HDassert(loc);
    HDassert(loc->file);

    /* Check for valid address */
    if(!H5F_addr_defined(loc->addr))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "address undefined")

    /* Check for write access on the file */
    file_intent = H5F_INTENT(loc->file);
    if((prot == H5AC_WRITE) && (0 == (file_intent & H5F_ACC_RDWR)))
	HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "no write intent on file")

    /* Construct the user data for protect callback */
    udata.made_attempt = FALSE;
    udata.v1_pfx_nmesgs = 0;
    udata.common.f = loc->file;
    udata.common.dxpl_id = dxpl_id;
    udata.common.file_intent = file_intent;
    udata.common.merged_null_msgs = 0;
    udata.common.mesgs_modified = FALSE;
    HDmemset(&cont_msg_info, 0, sizeof(cont_msg_info));
    udata.common.cont_msg_info = &cont_msg_info;
    udata.common.addr = loc->addr;

    /* Lock the object header into the cache */
    if(NULL == (oh = (H5O_t *)H5AC_protect(loc->file, dxpl_id, H5AC_OHDR, loc->addr, &udata, prot)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, NULL, "unable to load object header")

    /* Check if there are any continuation messages to process */
    if(cont_msg_info.nmsgs > 0) {
        size_t curr_msg;        /* Current continuation message to process */
        H5O_chk_cache_ud_t chk_udata;   /* User data for loading chunk */

        /* Sanity check - we should only have continuation messages to process
         *      when the object header is actually loaded from the file.
         */
        HDassert(udata.made_attempt == TRUE);
        HDassert(cont_msg_info.msgs);

        /* Construct the user data for protecting chunks */
        chk_udata.decoding = TRUE;
        chk_udata.oh = oh;
        chk_udata.chunkno = UINT_MAX;   /* Set to invalid value, for better error detection */
        chk_udata.common.f = loc->file;
        chk_udata.common.dxpl_id = dxpl_id;
        chk_udata.common.file_intent = file_intent;
        chk_udata.common.merged_null_msgs = udata.common.merged_null_msgs;
        chk_udata.common.mesgs_modified = udata.common.mesgs_modified;
        chk_udata.common.cont_msg_info = &cont_msg_info;

        /* Read in continuation messages, until there are no more */
        curr_msg = 0;
        while(curr_msg < cont_msg_info.nmsgs) {
            H5O_chunk_proxy_t *chk_proxy;       /* Proxy for chunk, to bring it into memory */
#ifndef NDEBUG
            size_t chkcnt = oh->nchunks;      /* Count of chunks (for sanity checking) */
#endif /* NDEBUG */

            /* Bring the chunk into the cache */
            /* (which adds to the object header) */
            chk_udata.common.addr = cont_msg_info.msgs[curr_msg].addr;
            chk_udata.size = cont_msg_info.msgs[curr_msg].size;
            if(NULL == (chk_proxy = (H5O_chunk_proxy_t *)H5AC_protect(loc->file, dxpl_id, H5AC_OHDR_CHK, cont_msg_info.msgs[curr_msg].addr, &chk_udata, prot)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, NULL, "unable to load object header chunk")

            /* Sanity check */
            HDassert(chk_proxy->oh == oh);
            HDassert(chk_proxy->chunkno == chkcnt);
            HDassert(oh->nchunks == (chkcnt + 1));

            /* Release the chunk from the cache */
            if(H5AC_unprotect(loc->file, dxpl_id, H5AC_OHDR_CHK, cont_msg_info.msgs[curr_msg].addr, chk_proxy, H5AC__NO_FLAGS_SET) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, NULL, "unable to release object header chunk")

            /* Advance to next continuation message */
            curr_msg++;
        } /* end while */

        /* Release any continuation messages built up */
        cont_msg_info.msgs = (H5O_cont_t *)H5FL_SEQ_FREE(H5O_cont_t, cont_msg_info.msgs);

        /* Pass back out some of the chunk's user data */
        udata.common.merged_null_msgs = chk_udata.common.merged_null_msgs;
        udata.common.mesgs_modified = chk_udata.common.mesgs_modified;
    } /* end if */

    /* Check for incorrect # of object header messages, if we've just loaded
     *  this object header from the file
     */
    if(udata.made_attempt) {
        /* Check for incorrect # of messages in v1 object header */
        if(oh->version == H5O_VERSION_1 &&
                (oh->nmesgs + udata.common.merged_null_msgs) != udata.v1_pfx_nmesgs) {
/* Don't enforce the error on an incorrect # of object header messages bug
 *      unless strict format checking is enabled.  This allows for older
 *      files, created with a version of the library that had a bug in tracking
 *      the correct # of header messages to be read in without the library
 *      erroring out here. -QAK
 */
#ifdef H5_STRICT_FORMAT_CHECKS
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "corrupt object header - incorrect # of messages")
#else /* H5_STRICT_FORMAT_CHECKS */
            /* Mark object header prefix dirty later if we don't have write access */
            /* (object header will have been marked dirty during protect, if we
             *  have write access -QAK)
             */
            if(prot != H5AC_WRITE)
                oh->prefix_modified = TRUE;
#ifndef NDEBUG
            else {
                unsigned oh_status = 0;         /* Object header entry cache status */

                /* Check the object header's status in the metadata cache */
                if(H5AC_get_entry_status(loc->file, loc->addr, &oh_status) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, NULL, "unable to check metadata cache status for object header")

                /* Make certain that object header is not dirty */
                HDassert(!(oh_status & H5AC_ES__IS_DIRTY));
            } /* end else */
#endif /* NDEBUG */
#endif /* H5_STRICT_FORMAT_CHECKS */
        } /* end if */

        /* Check for any messages that were modified while being read in */
        if(udata.common.mesgs_modified && prot != H5AC_WRITE)
            oh->mesgs_modified = TRUE;

        /* Reset the field that contained chunk 0's size during speculative load */
        oh->chunk0_size = 0;
    } /* end if */

    /* Take care of loose ends for modifications made while bringing in the
     *      object header & chunks.
     */
    if(prot == H5AC_WRITE) {
        /* Check for the object header prefix being modified somehow */
        /* (usually through updating the # of object header messages) */
        if(oh->prefix_modified) {
            /* Mark the header as dirty now */
            if(H5AC_mark_entry_dirty(oh) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTMARKDIRTY, NULL, "unable to mark object header as dirty")

            /* Reset flag */
            oh->prefix_modified = FALSE;
        } /* end if */

        /* Check for deferred dirty messages */
        if(oh->mesgs_modified) {
            unsigned u;         /* Local index variable */

            /* Loop through all messages, marking their chunks as dirty */
            /* (slightly inefficient, since we don't know exactly which messages
             *  were modified when the object header & chunks were brought in
             *  from the file, but this only can happen once per load -QAK)
             */
            for(u = 0; u < oh->nmesgs; u++) {
                /* Mark each chunk with a dirty message as dirty also */
                if(oh->mesg[u].dirty) {
                    H5O_chunk_proxy_t *chk_proxy;        /* Chunk that message is in */

                    /* Protect chunk */
                    if(NULL == (chk_proxy = H5O_chunk_protect(loc->file, dxpl_id, oh, oh->mesg[u].chunkno)))
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, NULL, "unable to load object header chunk")

                    /* Unprotect chunk, marking it dirty */
                    if(H5O_chunk_unprotect(loc->file, dxpl_id, chk_proxy, TRUE) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, NULL, "unable to unprotect object header chunk")
                } /* end if */
            } /* end for */

            /* Reset flag */
            oh->mesgs_modified = FALSE;
        } /* end if */
    } /* end if */

#ifdef H5O_DEBUG
H5O_assert(oh);
#endif /* H5O_DEBUG */

    /* Set return value */
    ret_value = oh;

done:
    if(ret_value == NULL && oh)
        if(H5AC_unprotect(loc->file, dxpl_id, H5AC_OHDR, loc->addr, oh, H5AC__NO_FLAGS_SET) < 0)
            HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, NULL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_protect() */


/*-------------------------------------------------------------------------
 * Function:	H5O_pin
 *
 * Purpose:	Pin an object header down for use during a sequence of message
 *              operations, which prevents the object header from being
 *              evicted from the cache.
 *
 * Return:	Success:	Pointer to the object header structure for the
 *                              object.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 13 2008
 *
 *-------------------------------------------------------------------------
 */
H5O_t *
H5O_pin(const H5O_loc_t *loc, hid_t dxpl_id)
{
    H5O_t       *oh = NULL;     /* Object header */
    H5O_t       *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* check args */
    HDassert(loc);

    /* Get header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_WRITE)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, NULL, "unable to protect object header")

    /* Increment the reference count on the object header */
    /* (which will pin it, if appropriate) */
    if(H5O_inc_rc(oh) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINC, NULL, "unable to increment reference count on object header")

    /* Set the return value */
    ret_value = oh;

done:
    /* Release the object header from the cache */
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, NULL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_pin() */


/*-------------------------------------------------------------------------
 * Function:	H5O_unpin
 *
 * Purpose:	Unpin an object header, allowing it to be evicted from the
 *              metadata cache.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 13 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_unpin(H5O_t *oh)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(oh);

    /* Decrement the reference count on the object header */
    /* (which will unpin it, if appropriate) */
    if(H5O_dec_rc(oh) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTDEC, FAIL, "unable to decrement reference count on object header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_unpin() */


/*-------------------------------------------------------------------------
 * Function:	H5O_unprotect
 *
 * Purpose:	Wrapper around H5AC_unprotect for use during a H5O_protect->
 *              H5O_msg_append->...->H5O_msg_append->H5O_unprotect sequence of calls
 *              during an object's creation.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Dec 31 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_unprotect(const H5O_loc_t *loc, hid_t dxpl_id, H5O_t *oh, unsigned oh_flags)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);
    HDassert(oh);

    /* Unprotect the object header */
    if(H5AC_unprotect(loc->file, dxpl_id, H5AC_OHDR, oh->chunk[0].addr, oh, oh_flags) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_unprotect() */


/*-------------------------------------------------------------------------
 * Function:	H5O_touch_oh
 *
 * Purpose:	If FORCE is non-zero then create a modification time message
 *		unless one already exists.  Then update any existing
 *		modification time message with the current time.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, July 27, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_touch_oh(H5F_t *f, hid_t dxpl_id, H5O_t *oh, hbool_t force)
{
    H5O_chunk_proxy_t *chk_proxy = NULL;        /* Chunk that message is in */
    hbool_t chk_dirtied = FALSE;        /* Flag for unprotecting chunk */
    time_t	now;                    /* Current time */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(f);
    HDassert(oh);

    /* Check if this object header is tracking times */
    if(oh->flags & H5O_HDR_STORE_TIMES) {
        /* Get current time */
        now = H5_now();

        /* Check version, to determine how to store time information */
        if(oh->version == H5O_VERSION_1) {
            size_t	idx;                    /* Index of modification time message to update */

            /* Look for existing message */
            for(idx = 0; idx < oh->nmesgs; idx++)
                if(H5O_MSG_MTIME == oh->mesg[idx].type || H5O_MSG_MTIME_NEW == oh->mesg[idx].type)
                    break;

            /* Create a new message, if necessary */
            if(idx == oh->nmesgs) {
                unsigned mesg_flags = 0;        /* Flags for message in object header */

                /* If we would have to create a new message, but we aren't 'forcing' it, get out now */
                if(!force)
                    HGOTO_DONE(SUCCEED);        /*nothing to do*/

                /* Allocate space for the modification time message */
                if(H5O_msg_alloc(f, dxpl_id, oh, H5O_MSG_MTIME_NEW, &mesg_flags, &now, &idx) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to allocate space for modification time message")

                /* Set the message's flags if appropriate */
                oh->mesg[idx].flags = (uint8_t)mesg_flags;
            } /* end if */

            /* Protect chunk */
            if(NULL == (chk_proxy = H5O_chunk_protect(f, dxpl_id, oh, oh->mesg[idx].chunkno)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header chunk")

            /* Allocate 'native' space, if necessary */
            if(NULL == oh->mesg[idx].native) {
                if(NULL == (oh->mesg[idx].native = H5FL_MALLOC(time_t)))
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "memory allocation failed for modification time message")
            } /* end if */

            /* Update the message */
            *((time_t *)(oh->mesg[idx].native)) = now;

            /* Mark the message as dirty */
            oh->mesg[idx].dirty = TRUE;
            chk_dirtied = TRUE;
        } /* end if */
        else {
            /* XXX: For now, update access time & change fields in the object header */
            /* (will need to add some code to update modification time appropriately) */
            oh->atime = oh->ctime = now;

            /* Mark object header as dirty in cache */
            if(H5AC_mark_entry_dirty(oh) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTMARKDIRTY, FAIL, "unable to mark object header as dirty")
        } /* end else */
    } /* end if */

done:
    /* Release chunk */
    if(chk_proxy && H5O_chunk_unprotect(f, dxpl_id, chk_proxy, chk_dirtied) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to unprotect object header chunk")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_touch_oh() */


/*-------------------------------------------------------------------------
 * Function:	H5O_touch
 *
 * Purpose:	Touch an object by setting the modification time to the
 *		current time and marking the object as dirty.  Unless FORCE
 *		is non-zero, nothing happens if there is no MTIME message in
 *		the object header.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, July 27, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_touch(const H5O_loc_t *loc, hbool_t force, hid_t dxpl_id)
{
    H5O_t	*oh = NULL;             /* Object header to modify */
    unsigned 	oh_flags = H5AC__NO_FLAGS_SET; /* Flags for unprotecting object header */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);

    /* Get the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_WRITE)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header")

    /* Create/Update the modification time message */
    if(H5O_touch_oh(loc->file, dxpl_id, oh, force) < 0)
	HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "unable to update object modificaton time")

    /* Mark object header as changed */
    oh_flags |= H5AC__DIRTIED_FLAG;

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, oh_flags) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_touch() */

#ifdef H5O_ENABLE_BOGUS

/*-------------------------------------------------------------------------
 * Function:	H5O_bogus_oh
 *
 * Purpose:	Create a "bogus" message unless one already exists.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              <koziol@ncsa.uiuc.edu>
 *              Tuesday, January 21, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_bogus_oh(H5F_t *f, hid_t dxpl_id, H5O_t *oh, unsigned mesg_flags)
{
    size_t	idx;                /* Local index variable */
    herr_t ret_value = SUCCEED;     /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(f);
    HDassert(oh);

    /* Look for existing message */
    for(idx = 0; idx < oh->nmesgs; idx++)
	if(H5O_MSG_BOGUS == oh->mesg[idx].type)
            break;

    /* Create a new message */
    if(idx == oh->nmesgs) {
        H5O_bogus_t *bogus;             /* Pointer to the bogus information */

        /* Allocate the native message in memory */
	if(NULL == (bogus = H5MM_malloc(sizeof(H5O_bogus_t))))
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "memory allocation failed for 'bogus' message")

        /* Update the native value */
        bogus->u = H5O_BOGUS_VALUE;

        /* Allocate space in the object header for bogus message */
	if(H5O_msg_alloc(f, dxpl_id, oh, H5O_MSG_BOGUS, &mesg_flags, bogus, &idx) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to allocate space for 'bogus' message")

        /* Point to "bogus" information (take it over) */
	oh->mesg[idx].native = bogus;

        /* Set the appropriate flags for the message */
        oh->mesg[idx].flags = mesg_flags;

        /* Mark the message and object header as dirty */
        oh->mesg[idx].dirty = TRUE;
        oh->cache_info.is_dirty = TRUE;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_bogus_oh() */
#endif /* H5O_ENABLE_BOGUS */


/*-------------------------------------------------------------------------
 * Function:	H5O_delete
 *
 * Purpose:	Delete an object header from a file.  This frees the file
 *              space used for the object header (and it's continuation blocks)
 *              and also walks through each header message and asks it to
 *              remove all the pieces of the file referenced by the header.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar 19 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_delete(H5F_t *f, hid_t dxpl_id, haddr_t addr)
{
    H5O_t *oh = NULL;           /* Object header information */
    H5O_loc_t loc;              /* Object location for object to delete */
    unsigned oh_flags = H5AC__NO_FLAGS_SET; /* Flags for unprotecting object header */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));

    /* Set up the object location */
    loc.file = f;
    loc.addr = addr;
    loc.holding_file = FALSE;

    /* Get the object header information */
    if(NULL == (oh = H5O_protect(&loc, dxpl_id, H5AC_WRITE)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header")

    /* Delete object */
    if(H5O_delete_oh(f, dxpl_id, oh) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "can't delete object from file")

    /* Mark object header as deleted */
    oh_flags = H5AC__DIRTIED_FLAG | H5AC__DELETED_FLAG | H5AC__FREE_FILE_SPACE_FLAG;

done:
    if(oh && H5O_unprotect(&loc, dxpl_id, oh, oh_flags) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_PROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_delete() */


/*-------------------------------------------------------------------------
 * Function:	H5O_delete_oh
 *
 * Purpose:	Internal function to:
 *              Delete an object header from a file.  This frees the file
 *              space used for the object header (and it's continuation blocks)
 *              and also walks through each header message and asks it to
 *              remove all the pieces of the file referenced by the header.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Mar 19 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_delete_oh(H5F_t *f, hid_t dxpl_id, H5O_t *oh)
{
    H5O_mesg_t *curr_msg;       /* Pointer to current message being operated on */
    unsigned	u;
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check args */
    HDassert(f);
    HDassert(oh);

    /* Walk through the list of object header messages, asking each one to
     * delete any file space used
     */
    for(u = 0, curr_msg = &oh->mesg[0]; u < oh->nmesgs; u++, curr_msg++) {
        /* Free any space referred to in the file from this message */
        if(H5O_delete_mesg(f, dxpl_id, oh, curr_msg) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTDELETE, FAIL, "unable to delete file space for object header message")
    } /* end for */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_delete_oh() */


/*-------------------------------------------------------------------------
 * Function:	H5O_obj_type
 *
 * Purpose:	Retrieves the type of object pointed to by `loc'.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, November  4, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_obj_type(const H5O_loc_t *loc, H5O_type_t *obj_type, hid_t dxpl_id)
{
    H5O_t	*oh = NULL;             /* Object header for location */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Load the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header")

    /* Retrieve the type of the object */
    if(H5O_obj_type_real(oh, obj_type) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to determine object type")

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_obj_type() */


/*-------------------------------------------------------------------------
 * Function:	H5O_obj_type_real
 *
 * Purpose:	Returns the type of object pointed to by `oh'.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 21, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_obj_type_real(H5O_t *oh, H5O_type_t *obj_type)
{
    const H5O_obj_class_t *obj_class;           /* Class of object for header */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity check */
    HDassert(oh);
    HDassert(obj_type);

    /* Look up class for object header */
    if(NULL == (obj_class = H5O_obj_class_real(oh))) {
        /* Clear error stack from "failed" class lookup */
        H5E_clear_stack(NULL);

        /* Set type to "unknown" */
        *obj_type = H5O_TYPE_UNKNOWN;
    } /* end if */
    else {
        /* Set object type */
        *obj_type = obj_class->type;
    } /* end else */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_obj_type_real() */


/*-------------------------------------------------------------------------
 * Function:	H5O_obj_class
 *
 * Purpose:	Returns the class of object pointed to by `loc'.
 *
 * Return:	Success:	An object class
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Monday, November  6, 2006
 *
 *-------------------------------------------------------------------------
 */
const H5O_obj_class_t *
H5O_obj_class(const H5O_loc_t *loc, hid_t dxpl_id)
{
    H5O_t	*oh = NULL;                     /* Object header for location */
    const H5O_obj_class_t *ret_value;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Load the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, NULL, "unable to load object header")

    /* Test whether entry qualifies as a particular type of object */
    if(NULL == (ret_value = H5O_obj_class_real(oh)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, NULL, "unable to determine object type")

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, NULL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_obj_class() */


/*-------------------------------------------------------------------------
 * Function:	H5O_obj_class_real
 *
 * Purpose:	Returns the class of object pointed to by `oh'.
 *
 * Return:	Success:	An object class
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 21, 2005
 *
 *-------------------------------------------------------------------------
 */
static const H5O_obj_class_t *
H5O_obj_class_real(H5O_t *oh)
{
    size_t	i;                      /* Local index variable */
    const H5O_obj_class_t *ret_value;   /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Sanity check */
    HDassert(oh);

    /* Test whether entry qualifies as a particular type of object */
    /* (Note: loop is in reverse order, to test specific objects first) */
    for(i = NELMTS(H5O_obj_class_g); i > 0; --i) {
        htri_t	isa;            /* Is entry a particular type? */

	if((isa = (H5O_obj_class_g[i - 1]->isa)(oh)) < 0)
	    HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "unable to determine object type")
	else if(isa)
	    HGOTO_DONE(H5O_obj_class_g[i - 1])
    } /* end for */

    if(0 == i)
	HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "unable to determine object type")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_obj_class_real() */


/*-------------------------------------------------------------------------
 * Function:	H5O_get_loc
 *
 * Purpose:	Gets the object location for an object given its ID.
 *
 * Return:	Success:	Pointer to H5O_loc_t
 *		Failure:	NULL
 *
 * Programmer:	James Laird
 *		July 25 2006
 *
 *-------------------------------------------------------------------------
 */
H5O_loc_t *
H5O_get_loc(hid_t object_id)
{
    H5O_loc_t   *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    switch(H5I_get_type(object_id)) {
        case H5I_GROUP:
            if(NULL == (ret_value = H5O_OBJ_GROUP->get_oloc(object_id)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, NULL, "unable to get object location from group ID")
            break;

        case H5I_DATASET:
            if(NULL == (ret_value = H5O_OBJ_DATASET->get_oloc(object_id)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, NULL, "unable to get object location from dataset ID")
            break;

        case H5I_DATATYPE:
            if(NULL == (ret_value = H5O_OBJ_DATATYPE->get_oloc(object_id)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, NULL, "unable to get object location from datatype ID")
            break;

        case H5I_UNINIT:
        case H5I_BADID:
        case H5I_FILE:
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
            HGOTO_ERROR(H5E_OHDR, H5E_BADTYPE, NULL, "invalid object type")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_get_loc() */


/*-------------------------------------------------------------------------
 * Function:	H5O_loc_reset
 *
 * Purpose:	Reset a object location to an empty state
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 19, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_loc_reset(H5O_loc_t *loc)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check arguments */
    HDassert(loc);

    /* Clear the object location to an empty state */
    HDmemset(loc, 0, sizeof(H5O_loc_t));
    loc->addr = HADDR_UNDEF;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_loc_reset() */


/*-------------------------------------------------------------------------
 * Function:    H5O_loc_copy
 *
 * Purpose:     Copy object location information
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Monday, September 19, 2005
 *
 * Notes:       'depth' parameter determines how much of the group entry
 *              structure we want to copy.  The values are:
 *                  H5_COPY_SHALLOW - Copy all the field values from the source
 *                      to the destination, but not copying objects pointed to.
 *                      (Destination "takes ownership" of objects pointed to)
 *                  H5_COPY_DEEP - Copy all the fields from the source to
 *                      the destination, deep copying objects pointed to.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_loc_copy(H5O_loc_t *dst, H5O_loc_t *src, H5_copy_depth_t depth)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check arguments */
    HDassert(src);
    HDassert(dst);
    HDassert(depth == H5_COPY_SHALLOW || depth == H5_COPY_DEEP);

    /* Copy the top level information */
    HDmemcpy(dst, src, sizeof(H5O_loc_t));

    /* Deep copy the names */
    if(depth == H5_COPY_DEEP) {
        /* If the original entry was holding open the file, this one should
         * hold it open, too.
         */
        if(src->holding_file)
            H5F_INCR_NOPEN_OBJS(dst->file);
    } else if(depth == H5_COPY_SHALLOW) {
        H5O_loc_reset(src);
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_loc_copy() */


/*-------------------------------------------------------------------------
 * Function:	H5O_loc_hold_file
 *
 * Purpose:	Have this object header hold a file open until it is
 *              released.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *              Wednesday, August 16, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_loc_hold_file(H5O_loc_t *loc)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check arguments */
    HDassert(loc);
    HDassert(loc->file);

    /* If this location is not already holding its file open, do so. */
    if(!loc->holding_file) {
        H5F_INCR_NOPEN_OBJS(loc->file);
        loc->holding_file = TRUE;
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_loc_hold_file() */


/*-------------------------------------------------------------------------
 * Function:	H5O_loc_free
 *
 * Purpose:	Release resources used by this object header location.
 *              Not to be confused with H5O_close; this is used on
 *              locations that don't correspond to open objects.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	James Laird
 *              Wednesday, August 16, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_loc_free(H5O_loc_t *loc)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments */
    HDassert(loc);

    /* If this location is holding its file open try to close the file. */
    if(loc->holding_file) {
        H5F_DECR_NOPEN_OBJS(loc->file);
        loc->holding_file = FALSE;
        if(H5F_NOPEN_OBJS(loc->file) <= 0) {
            if(H5F_try_close(loc->file) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, FAIL, "can't close file")
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_loc_free() */


/*-------------------------------------------------------------------------
 * Function:	H5O_get_hdr_info
 *
 * Purpose:	Retrieve the object header information for an object
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		September 22 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_get_hdr_info(const H5O_loc_t *loc, hid_t dxpl_id, H5O_hdr_info_t *hdr)
{
    H5O_t *oh = NULL;                   /* Object header */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(loc);
    HDassert(hdr);

    /* Reset the object header info structure */
    HDmemset(hdr, 0, sizeof(*hdr));

    /* Get the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, FAIL, "unable to load object header")

    /* Get the information for the object header */
    if(H5O_get_hdr_info_real(oh, hdr) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't retrieve object header info")

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_PROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_get_hdr_info() */


/*-------------------------------------------------------------------------
 * Function:	H5O_get_hdr_info_real
 *
 * Purpose:	Internal routine to retrieve the object header information for an object
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		September 22 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_get_hdr_info_real(const H5O_t *oh, H5O_hdr_info_t *hdr)
{
    const H5O_mesg_t *curr_msg;         /* Pointer to current message being operated on */
    const H5O_chunk_t *curr_chunk;	/* Pointer to current message being operated on */
    unsigned u;                         /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(oh);
    HDassert(hdr);

    /* Set the version for the object header */
    hdr->version = oh->version;

    /* Set the number of messages & chunks */
    H5_ASSIGN_OVERFLOW(hdr->nmesgs, oh->nmesgs, size_t, unsigned);
    H5_ASSIGN_OVERFLOW(hdr->nchunks, oh->nchunks, size_t, unsigned);

    /* Set the status flags */
    hdr->flags = oh->flags;

    /* Iterate over all the messages, accumulating message size & type information */
    hdr->space.meta = (hsize_t)H5O_SIZEOF_HDR(oh) + (hsize_t)(H5O_SIZEOF_CHKHDR_OH(oh) * (oh->nchunks - 1));
    hdr->space.mesg = 0;
    hdr->space.free = 0;
    hdr->mesg.present = 0;
    hdr->mesg.shared = 0;
    for(u = 0, curr_msg = &oh->mesg[0]; u < oh->nmesgs; u++, curr_msg++) {
        uint64_t type_flag;             /* Flag for message type */

        /* Accumulate space usage information, based on the type of message */
	if(H5O_NULL_ID == curr_msg->type->id)
            hdr->space.free += (hsize_t)((size_t)H5O_SIZEOF_MSGHDR_OH(oh) + curr_msg->raw_size);
        else if(H5O_CONT_ID == curr_msg->type->id)
            hdr->space.meta += (hsize_t)((size_t)H5O_SIZEOF_MSGHDR_OH(oh) + curr_msg->raw_size);
        else {
            hdr->space.meta += (hsize_t)H5O_SIZEOF_MSGHDR_OH(oh);
            hdr->space.mesg += curr_msg->raw_size;
        } /* end else */

        /* Set flag to indicate presence of message type */
        type_flag = ((uint64_t)1) << curr_msg->type->id;
        hdr->mesg.present |= type_flag;

        /* Set flag if the message is shared in some way */
        if(curr_msg->flags & H5O_MSG_FLAG_SHARED)                                   \
            hdr->mesg.shared |= type_flag;
    } /* end for */

    /* Iterate over all the chunks, adding any gaps to the free space */
    hdr->space.total = 0;
    for(u = 0, curr_chunk = &oh->chunk[0]; u < oh->nchunks; u++, curr_chunk++) {
        /* Accumulate the size of the header on disk */
        hdr->space.total += curr_chunk->size;

        /* If the chunk has a gap, add it to the free space */
        hdr->space.free += curr_chunk->gap;
    } /* end for */

    /* Sanity check that all the bytes are accounted for */
    HDassert(hdr->space.total == (hdr->space.free + hdr->space.meta + hdr->space.mesg));

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_get_hdr_info_real() */


/*-------------------------------------------------------------------------
 * Function:	H5O_get_info
 *
 * Purpose:	Retrieve the information for an object
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		November 21 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_get_info(const H5O_loc_t *loc, hid_t dxpl_id, hbool_t want_ih_info,
    H5O_info_t *oinfo)
{
    const H5O_obj_class_t *obj_class;   /* Class of object for header */
    H5O_t *oh = NULL;                   /* Object header */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(loc);
    HDassert(oinfo);

    /* Get the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header")

    /* Reset the object info structure */
    HDmemset(oinfo, 0, sizeof(*oinfo));

    /* Retrieve the file's fileno */
    H5F_GET_FILENO(loc->file, oinfo->fileno);

    /* Set the object's address */
    oinfo->addr = loc->addr;

    /* Get class for object */
    if(NULL == (obj_class = H5O_obj_class_real(oh)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to determine object class")

    /* Retrieve the type of the object */
    oinfo->type = obj_class->type;

    /* Set the object's reference count */
    oinfo->rc = oh->nlink;

    /* Get modification time for object */
    if(oh->version > H5O_VERSION_1) {
        oinfo->atime = oh->atime;
        oinfo->mtime = oh->mtime;
        oinfo->ctime = oh->ctime;
        oinfo->btime = oh->btime;
    } /* end if */
    else {
        htri_t	exists;                 /* Flag if header message of interest exists */

        /* No information for access & modification fields */
        /* (we stopped updating the "modification time" header message for
         *      raw data changes, so the "modification time" header message
         *      is closest to the 'change time', in POSIX terms - QAK)
         */
        oinfo->atime = 0;
        oinfo->mtime = 0;
        oinfo->btime = 0;

        /* Might be information for modification time */
        if((exists = H5O_msg_exists_oh(oh, H5O_MTIME_ID)) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "unable to check for MTIME message")
        if(exists > 0) {
            /* Get "old style" modification time info */
            if(NULL == H5O_msg_read_oh(loc->file, dxpl_id, oh, H5O_MTIME_ID, &oinfo->ctime))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't read MTIME message")
        } /* end if */
        else {
            /* Check for "new style" modification time info */
            if((exists = H5O_msg_exists_oh(oh, H5O_MTIME_NEW_ID)) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "unable to check for MTIME_NEW message")
            if(exists > 0) {
                /* Get "new style" modification time info */
                if(NULL == H5O_msg_read_oh(loc->file, dxpl_id, oh, H5O_MTIME_NEW_ID, &oinfo->ctime))
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't read MTIME_NEW message")
            } /* end if */
            else
                oinfo->ctime = 0;
        } /* end else */
    } /* end else */

    /* Get the information for the object header */
    if(H5O_get_hdr_info_real(oh, &oinfo->hdr) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't retrieve object header info")

    /* Retrieve # of attributes */
    if(H5O_attr_count_real(loc->file, dxpl_id, oh, &oinfo->num_attrs) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't retrieve attribute count")

    /* Get B-tree & heap metadata storage size, if requested */
    if(want_ih_info) {
        /* Check for 'bh_info' callback for this type of object */
        if(obj_class->bh_info) {
            /* Call the object's class 'bh_info' routine */
            if((obj_class->bh_info)(loc->file, dxpl_id, oh, &oinfo->meta_size.obj) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't retrieve object's btree & heap info")
        } /* end if */

        /* Get B-tree & heap info for any attributes */
        if(oinfo->num_attrs > 0) {
            if(H5O_attr_bh_info(loc->file, dxpl_id, oh, &oinfo->meta_size.attr) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't retrieve attribute btree & heap info")
        } /* end if */
    } /* end if */

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_get_info() */


/*-------------------------------------------------------------------------
 * Function:	H5O_get_create_plist
 *
 * Purpose:	Retrieve the object creation properties for an object
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		November 28 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_get_create_plist(const H5O_loc_t *loc, hid_t dxpl_id, H5P_genplist_t *oc_plist)
{
    H5O_t *oh = NULL;                   /* Object header */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(loc);
    HDassert(oc_plist);

    /* Get the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header")

    /* Set property values, if they were used for the object */
    if(oh->version > H5O_VERSION_1) {
        uint8_t ohdr_flags;             /* "User-visible" object header status flags */

        /* Set attribute storage values */
        if(H5P_set(oc_plist, H5O_CRT_ATTR_MAX_COMPACT_NAME, &oh->max_compact) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't set max. # of compact attributes in property list")
        if(H5P_set(oc_plist, H5O_CRT_ATTR_MIN_DENSE_NAME, &oh->min_dense) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTSET, FAIL, "can't set min. # of dense attributes in property list")

        /* Mask off non-"user visible" flags */
        ohdr_flags = oh->flags & (H5O_HDR_ATTR_CRT_ORDER_TRACKED | H5O_HDR_ATTR_CRT_ORDER_INDEXED | H5O_HDR_STORE_TIMES);

        /* Set object header flags */
        if(H5P_set(oc_plist, H5O_CRT_OHDR_FLAGS_NAME, &ohdr_flags) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set object header flags")
    } /* end if */

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_get_create_plist() */


/*-------------------------------------------------------------------------
 * Function:	H5O_get_nlinks
 *
 * Purpose:	Retrieve the number of link messages read in from the file
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		March 11 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_get_nlinks(const H5O_loc_t *loc, hid_t dxpl_id, hsize_t *nlinks)
{
    H5O_t *oh = NULL;                   /* Object header */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(loc);
    HDassert(nlinks);

    /* Get the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header")

    /* Retrieve the # of link messages seen when the object header was loaded */
    *nlinks = oh->link_msgs_seen;

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_get_nlinks() */


/*-------------------------------------------------------------------------
 * Function:	H5O_obj_create
 *
 * Purpose:	Creates an object, in an abstract manner.
 *
 * Return:	Success:	Pointer to object opened
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *		April 9 2007
 *
 *-------------------------------------------------------------------------
 */
void *
H5O_obj_create(H5F_t *f, H5O_type_t obj_type, void *crt_info, H5G_loc_t *obj_loc,
    hid_t dxpl_id)
{
    size_t u;                           /* Local index variable */
    void *ret_value = NULL;             /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Sanity checks */
    HDassert(f);
    HDassert(obj_type >= H5O_TYPE_GROUP && obj_type <= H5O_TYPE_NAMED_DATATYPE);
    HDassert(crt_info);
    HDassert(obj_loc);

    /* Iterate through the object classes */
    for(u = 0; u < NELMTS(H5O_obj_class_g); u++) {
        /* Check for correct type of object to create */
	if(H5O_obj_class_g[u]->type == obj_type) {
            /* Call the object class's 'create' routine */
            HDassert(H5O_obj_class_g[u]->create);
            if(NULL == (ret_value = H5O_obj_class_g[u]->create(f, crt_info, obj_loc, dxpl_id)))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, NULL, "unable to open object")

            /* Break out of loop */
            break;
        } /* end if */
    } /* end for */
    HDassert(ret_value);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_obj_create() */


/*-------------------------------------------------------------------------
 * Function:	H5O_get_oh_addr
 *
 * Purpose:	Retrieve the address of the object header
 *
 * Note:	This routine participates in the "Inlining C struct access"
 *		pattern, don't call it directly, use the appropriate macro
 *		defined in H5Oprivate.h.
 *
 * Return:	Success:	Valid haddr_t
 *		Failure:	HADDR_UNDEF
 *
 * Programmer:	Quincey Koziol
 *		March 15 2007
 *
 *-------------------------------------------------------------------------
 */
haddr_t
H5O_get_oh_addr(const H5O_t *oh)
{
    /* Use FUNC_ENTER_NOAPI_NOINIT_NOERR here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(oh);
    HDassert(oh->chunk);

    FUNC_LEAVE_NOAPI(oh->chunk[0].addr)
} /* end H5O_get_oh_addr() */


/*-------------------------------------------------------------------------
 * Function:	H5O_get_rc_and_type
 *
 * Purpose:	Retrieve an object's reference count and type
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *		November  4 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_get_rc_and_type(const H5O_loc_t *loc, hid_t dxpl_id, unsigned *rc, H5O_type_t *otype)
{
    H5O_t *oh = NULL;                   /* Object header */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    HDassert(loc);

    /* Get the object header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
	HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to load object header")

    /* Set the object's reference count */
    if(rc)
        *rc = oh->nlink;

    /* Retrieve the type of the object */
    if(otype)
        if(H5O_obj_type_real(oh, otype) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL, "unable to determine object type")

done:
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
	HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_get_rc_and_type() */


/*-------------------------------------------------------------------------
 * Function:    H5O_free_visit_visited
 *
 * Purpose:     Free the key for an object visited during a group traversal
 *
 * Return:      Non-negative on success, negative on failure
 *
 * Programmer:  Quincey Koziol
 *	        Nov 25, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_free_visit_visited(void *item, void UNUSED *key, void UNUSED *operator_data/*in,out*/)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    item = H5FL_FREE(H5_obj_t, item);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_free_visit_visited() */


/*-------------------------------------------------------------------------
 * Function:	H5O_visit_cb
 *
 * Purpose:     Callback function for recursively visiting objects from a group
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Nov 25, 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_visit_cb(hid_t UNUSED group, const char *name, const H5L_info_t *linfo,
    void *_udata)
{
    H5O_iter_visit_ud_t *udata = (H5O_iter_visit_ud_t *)_udata;     /* User data for callback */
    H5G_loc_t   obj_loc;                /* Location of object */
    H5G_name_t  obj_path;            	/* Object's group hier. path */
    H5O_loc_t   obj_oloc;            	/* Object's object location */
    hbool_t     obj_found = FALSE;      /* Object at 'name' found */
    herr_t ret_value = H5_ITER_CONT;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(name);
    HDassert(linfo);
    HDassert(udata);

    /* Check if this is a hard link */
    if(linfo->type == H5L_TYPE_HARD) {
        H5_obj_t obj_pos;       /* Object "position" for this object */

        /* Set up opened group location to fill in */
        obj_loc.oloc = &obj_oloc;
        obj_loc.path = &obj_path;
        H5G_loc_reset(&obj_loc);

        /* Find the object using the LAPL passed in */
        /* (Correctly handles mounted files) */
        if(H5G_loc_find(udata->start_loc, name, &obj_loc/*out*/, udata->lapl_id, udata->dxpl_id) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, H5_ITER_ERROR, "object not found")
        obj_found = TRUE;

        /* Construct unique "position" for this object */
        H5F_GET_FILENO(obj_oloc.file, obj_pos.fileno);
        obj_pos.addr = obj_oloc.addr;

        /* Check if we've seen the object the link references before */
        if(NULL == H5SL_search(udata->visited, &obj_pos)) {
            H5O_info_t oinfo;           /* Object info */

            /* Get the object's info */
            if(H5O_get_info(&obj_oloc, udata->dxpl_id, TRUE, &oinfo) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, H5_ITER_ERROR, "unable to get object info")

            /* Make the application callback */
            ret_value = (udata->op)(udata->obj_id, name, &oinfo, udata->op_data);

            /* Check for continuing to visit objects */
            if(ret_value == H5_ITER_CONT) {
                /* If its ref count is > 1, we add it to the list of visited objects */
                /* (because it could come up again during traversal) */
                if(oinfo.rc > 1) {
                    H5_obj_t *new_node;                  /* New object node for visited list */

                    /* Allocate new object "position" node */
                    if((new_node = H5FL_MALLOC(H5_obj_t)) == NULL)
                        HGOTO_ERROR(H5E_OHDR, H5E_NOSPACE, H5_ITER_ERROR, "can't allocate object node")

                    /* Set node information */
                    *new_node = obj_pos;

                    /* Add to list of visited objects */
                    if(H5SL_insert(udata->visited, new_node, new_node) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, H5_ITER_ERROR, "can't insert object node into visited list")
                } /* end if */
            } /* end if */
        } /* end if */
    } /* end if */

done:
    /* Release resources */
    if(obj_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTRELEASE, H5_ITER_ERROR, "can't free location")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_visit_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5O_visit
 *
 * Purpose:	Recursively visit an object and all the objects reachable
 *              from it.  If the starting object is a group, all the objects
 *              linked to from that group will be visited.  Links within
 *              each group are visited according to the order within the
 *              specified index (unless the specified index does not exist for
 *              a particular group, then the "name" index is used).
 *
 *              NOTE: Soft links and user-defined links are ignored during
 *              this operation.
 *
 *              NOTE: Each _object_ reachable from the initial group will only
 *              be visited once.  If multiple hard links point to the same
 *              object, the first link to the object's path (according to the
 *              iteration index and iteration order given) will be used to in
 *              the callback about the object.
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
static herr_t
H5O_visit(hid_t loc_id, const char *obj_name, H5_index_t idx_type,
    H5_iter_order_t order, H5O_iterate_t op, void *op_data, hid_t lapl_id,
    hid_t dxpl_id)
{
    H5O_iter_visit_ud_t udata;  /* User data for callback */
    H5G_loc_t	loc;            /* Location of reference object */
    H5G_loc_t   obj_loc;        /* Location used to open object */
    H5G_name_t  obj_path;       /* Opened object group hier. path */
    H5O_loc_t   obj_oloc;       /* Opened object object location */
    hbool_t     loc_found = FALSE;      /* Entry at 'name' found */
    H5O_info_t  oinfo;          /* Object info struct */
    hid_t       obj_id = (-1);  /* ID of object */
    herr_t      ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Portably initialize user data struct to zeros */
    HDmemset(&udata, 0, sizeof(udata));

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

    /* Set up opened group location to fill in */
    obj_loc.oloc = &obj_oloc;
    obj_loc.path = &obj_path;
    H5G_loc_reset(&obj_loc);

    /* Find the object's location */
    if(H5G_loc_find(&loc, obj_name, &obj_loc/*out*/, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
    loc_found = TRUE;

    /* Get the object's info */
    if(H5O_get_info(&obj_oloc, dxpl_id, TRUE, &oinfo) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to get object info")

    /* Open the object */
    /* (Takes ownership of the obj_loc information) */
    if((obj_id = H5O_open_by_loc(&obj_loc, lapl_id, dxpl_id, TRUE)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, FAIL, "unable to open object")

    /* Make callback for starting object */
    if((ret_value = op(obj_id, ".", &oinfo, op_data)) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "can't visit objects")

    /* Check return value of first callback */
    if(ret_value != H5_ITER_CONT)
        HGOTO_DONE(ret_value);

    /* Check for object being a group */
    if(oinfo.type == H5O_TYPE_GROUP) {
        H5G_loc_t	start_loc;          /* Location of starting group */

        /* Get the location of the starting group */
        if(H5G_loc(obj_id, &start_loc) < 0)
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

        /* Set up user data for visiting links */
        udata.obj_id = obj_id;
        udata.start_loc = &start_loc;
        udata.lapl_id = lapl_id;
        udata.dxpl_id = dxpl_id;
        udata.op = op;
        udata.op_data = op_data;

        /* Create skip list to store visited object information */
        if((udata.visited = H5SL_create(H5SL_TYPE_OBJ, NULL)) == NULL)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTCREATE, FAIL, "can't create skip list for visited objects")

        /* If its ref count is > 1, we add it to the list of visited objects */
        /* (because it could come up again during traversal) */
        if(oinfo.rc > 1) {
            H5_obj_t *obj_pos;                  /* New object node for visited list */

            /* Allocate new object "position" node */
            if((obj_pos = H5FL_MALLOC(H5_obj_t)) == NULL)
                HGOTO_ERROR(H5E_OHDR, H5E_NOSPACE, FAIL, "can't allocate object node")

            /* Construct unique "position" for this object */
            obj_pos->fileno = oinfo.fileno;
            obj_pos->addr = oinfo.addr;

            /* Add to list of visited objects */
            if(H5SL_insert(udata.visited, obj_pos, obj_pos) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTINSERT, FAIL, "can't insert object node into visited list")
        } /* end if */

        /* Call internal group visitation routine */
        if((ret_value = H5G_visit(obj_id, ".", idx_type, order, H5O_visit_cb, &udata, lapl_id, dxpl_id)) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "object visitation failed")
    } /* end if */

done:
    if(obj_id > 0) {
        if(H5I_dec_app_ref(obj_id) < 0)
            HDONE_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "unable to close object")
    } /* end if */
    else if(loc_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "can't free location")
    if(udata.visited)
        H5SL_destroy(udata.visited, H5O_free_visit_visited, NULL);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_visit() */


/*-------------------------------------------------------------------------
 * Function:	H5O_inc_rc
 *
 * Purpose:	Increments the reference count on an object header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 13 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_inc_rc(H5O_t *oh)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(oh);

    /* Pin the object header when the reference count goes above 0 */
    if(oh->rc == 0)
        if(H5AC_pin_protected_entry(oh) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTPIN, FAIL, "unable to pin object header")

    /* Increment reference count */
    oh->rc++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_inc_rc() */


/*-------------------------------------------------------------------------
 * Function:	H5O_dec_rc
 *
 * Purpose:	Decrements the reference count on an object header
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jul 13 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_dec_rc(H5O_t *oh)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(oh);

    /* Decrement reference count */
    oh->rc--;

    /* Unpin the object header when the reference count goes back to 0 */
    if(oh->rc == 0)
        if(H5AC_unpin_entry(oh) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTUNPIN, FAIL, "unable to unpin object header")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dec_rc() */


/*-------------------------------------------------------------------------
 * Function:   H5O_dec_rc_by_loc
 *
 * Purpose:    Decrement the refcount of an object header, using its
 *              object location information.
 *
 * Return:     Non-negative on success/Negative on failure
 *
 * Programmer: Quincey Koziol
 *             koziol@hdfgroup.org
 *             Oct 08 2010
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_dec_rc_by_loc(const H5O_loc_t *loc, hid_t dxpl_id)
{
    H5O_t       *oh = NULL;             /* Object header */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* check args */
    HDassert(loc);

    /* Get header */
    if(NULL == (oh = H5O_protect(loc, dxpl_id, H5AC_READ)))
       HGOTO_ERROR(H5E_OHDR, H5E_CANTPROTECT, FAIL, "unable to protect object header")

    /* Decrement the reference count on the object header */
    /* (which will unpin it, if appropriate) */
    if(H5O_dec_rc(oh) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTDEC, FAIL, "unable to decrement reference count on object header")

done:
    /* Release the object header from the cache */
    if(oh && H5O_unprotect(loc, dxpl_id, oh, H5AC__NO_FLAGS_SET) < 0)
        HDONE_ERROR(H5E_OHDR, H5E_CANTUNPROTECT, FAIL, "unable to release object header")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dec_rc_by_loc() */


/*-------------------------------------------------------------------------
 * Function:	H5O_free
 *
 * Purpose:	Destroys an object header.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Jan 15 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5O_free(H5O_t *oh)
{
    unsigned	u;                      /* Local index variable */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(oh);

    /* Destroy chunks */
    if(oh->chunk) {
        for(u = 0; u < oh->nchunks; u++)
            oh->chunk[u].image = H5FL_BLK_FREE(chunk_image, oh->chunk[u].image);

        oh->chunk = (H5O_chunk_t *)H5FL_SEQ_FREE(H5O_chunk_t, oh->chunk);
    } /* end if */

    /* Destroy messages */
    if(oh->mesg) {
        for(u = 0; u < oh->nmesgs; u++) {
#ifndef NDEBUG
            /* Verify that message is clean, unless it could have been marked
             * dirty by decoding */
            if(oh->ndecode_dirtied && oh->mesg[u].dirty)
                oh->ndecode_dirtied--;
            else
                HDassert(oh->mesg[u].dirty == 0);
#endif /* NDEBUG */

            H5O_msg_free_mesg(&oh->mesg[u]);
        } /* end for */

        /* Make sure we accounted for all the messages dirtied by decoding */
        HDassert(!oh->ndecode_dirtied);

        oh->mesg = (H5O_mesg_t *)H5FL_SEQ_FREE(H5O_mesg_t, oh->mesg);
    } /* end if */

    /* destroy object header */
    oh = H5FL_FREE(H5O_t, oh);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_free() */

