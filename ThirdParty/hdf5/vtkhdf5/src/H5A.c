/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/****************/
/* Module Setup */
/****************/

#include "H5Amodule.h"          /* This source code file is part of the H5A module */
#define H5O_FRIEND		/*suppress error about including H5Opkg	*/


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Apkg.h"		/* Attributes				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FLprivate.h"	/* Free Lists				*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Opkg.h"		/* Object headers			*/
#include "H5Sprivate.h"		/* Dataspace functions			*/
#include "H5SMprivate.h"	/* Shared Object Header Messages	*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* Object header iterator callbacks */
/* Data structure for callback for locating the index by name */
typedef struct H5A_iter_cb1 {
    const char *name;
    int idx;
} H5A_iter_cb1;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/


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

/* Declare the free lists of H5A_t */
H5FL_DEFINE(H5A_t);

/* Declare the free lists for H5A_shared_t's */
H5FL_DEFINE(H5A_shared_t);

/* Declare a free list to manage blocks of type conversion data */
H5FL_BLK_DEFINE(attr_buf);

/* Attribute ID class */
static const H5I_class_t H5I_ATTR_CLS[1] = {{
    H5I_ATTR,                   /* ID class value */
    0,                          /* Class flags */
    0,                          /* # of reserved IDs for class */
    (H5I_free_t)H5A_close       /* Callback routine for closing objects of this class */
}};

/* Flag indicating "top" of interface has been initialized */
static hbool_t H5A_top_package_initialize_s = FALSE;



/*--------------------------------------------------------------------------
NAME
   H5A__init_package -- Initialize interface-specific information
USAGE
    herr_t H5A__init_package()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
herr_t
H5A__init_package(void)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_PACKAGE

    /*
     * Create attribute ID type.
     */
    if(H5I_register_type(H5I_ATTR_CLS) < 0)
        HGOTO_ERROR(H5E_INTERNAL, H5E_CANTINIT, FAIL, "unable to initialize interface")

    /* Mark "top" of interface as initialized, too */
    H5A_top_package_initialize_s = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A__init_package() */


/*--------------------------------------------------------------------------
 NAME
    H5A_top_term_package
 PURPOSE
    Terminate various H5A objects
 USAGE
    void H5A_top_term_package()
 RETURNS
 DESCRIPTION
    Release IDs for the atom group, deferring full interface shutdown
    until later (in H5A_term_package).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5A_top_term_package(void)
{
    int	n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(H5A_top_package_initialize_s) {
	if(H5I_nmembers(H5I_ATTR) > 0) {
	    (void)H5I_clear_type(H5I_ATTR, FALSE, FALSE);
            n++; /*H5I*/
	} /* end if */

        /* Mark closed */
        if(0 == n)
            H5A_top_package_initialize_s = FALSE;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* H5A_top_term_package() */


/*--------------------------------------------------------------------------
 NAME
    H5A_term_package
 PURPOSE
    Terminate various H5A objects
 USAGE
    void H5A_term_package()
 RETURNS
 DESCRIPTION
    Release any other resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...

     Finishes shutting down the interface, after H5A_top_term_package()
     is called
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5A_term_package(void)
{
    int	n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(H5_PKG_INIT_VAR) {
        /* Sanity checks */
        HDassert(0 == H5I_nmembers(H5I_ATTR));
        HDassert(FALSE == H5A_top_package_initialize_s);

        /* Destroy the attribute object id group */
        n += (H5I_dec_type_ref(H5I_ATTR) > 0);

        /* Mark closed */
        if(0 == n)
            H5_PKG_INIT_VAR = FALSE;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* H5A_term_package() */


/*--------------------------------------------------------------------------
 NAME
    H5Acreate2
 PURPOSE
    Creates an attribute on an object
 USAGE
    hid_t H5Acreate2(loc_id, attr_name, type_id, space_id, acpl_id,
            aapl_id)
        hid_t loc_id;       IN: Object (dataset or group) to be attached to
        const char *attr_name;  IN: Name of attribute to locate and open
        hid_t type_id;          IN: ID of datatype for attribute
        hid_t space_id;         IN: ID of dataspace for attribute
        hid_t acpl_id;          IN: ID of creation property list (currently not used)
        hid_t aapl_id;          IN: Attribute access property list
 RETURNS
    Non-negative on success/Negative on failure

 DESCRIPTION
        This function creates an attribute which is attached to the object
    specified with 'loc_id'.  The name specified with 'attr_name' for
    each attribute for an object must be unique for that object.  The 'type_id'
    and 'space_id' are created with the H5T and H5S interfaces respectively.
    The 'aapl_id' property list is currently unused, but will be used in the
    future for optional attribute access properties.  The attribute ID returned
    from this function must be released with H5Aclose or resource leaks will
    develop.

--------------------------------------------------------------------------*/
hid_t
H5Acreate2(hid_t loc_id, const char *attr_name, hid_t type_id, hid_t space_id,
    hid_t acpl_id, hid_t aapl_id)
{
    H5A_t	        *attr = NULL;           /* Attribute created */
    H5G_loc_t           loc;                    /* Object location */
    H5T_t		*type;                  /* Datatype to use for attribute */
    H5S_t		*space;                 /* Dataspace to use for attribute */
    hid_t               dxpl_id = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    hid_t		ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("i", "i*siiii", loc_id, attr_name, type_id, space_id, acpl_id, aapl_id);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(0 == (H5F_INTENT(loc.oloc->file) & H5F_ACC_RDWR))
	HGOTO_ERROR(H5E_ARGS, H5E_WRITEERROR, FAIL, "no write intent on file")
    if(!attr_name || !*attr_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no attribute name")
    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a type")
    if(NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&aapl_id, H5P_CLS_AACC, &dxpl_id, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Go do the real work for attaching the attribute to the object */
    if(NULL == (attr = H5A_create(&loc, attr_name, type, space, acpl_id, dxpl_id)))
	HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL, "unable to create attribute")

    /* Register the new attribute and get an ID for it */
    if((ret_value = H5I_register(H5I_ATTR, attr, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register attribute for ID")

done:
    /* Cleanup on failure */
    if(ret_value < 0 && attr && H5A_close(attr) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* H5Acreate2() */


/*--------------------------------------------------------------------------
 NAME
    H5Acreate_by_name
 PURPOSE
    Creates an attribute on an object
 USAGE
    hid_t H5Acreate_by_name(loc_id, obj_name, attr_name, type_id, space_id, acpl_id,
            aapl_id, lapl_id)
        hid_t loc_id;       IN: Object (dataset or group) to be attached to
        const char *obj_name;   IN: Name of object relative to location
        const char *attr_name;  IN: Name of attribute to locate and open
        hid_t type_id;          IN: ID of datatype for attribute
        hid_t space_id;         IN: ID of dataspace for attribute
        hid_t acpl_id;          IN: ID of creation property list (currently not used)
        hid_t aapl_id;          IN: Attribute access property list
        hid_t lapl_id;          IN: Link access property list
 RETURNS
    Non-negative on success/Negative on failure

 DESCRIPTION
        This function creates an attribute which is attached to the object
    specified with 'loc_id/obj_name'.  The name specified with 'attr_name' for
    each attribute for an object must be unique for that object.  The 'type_id'
    and 'space_id' are created with the H5T and H5S interfaces respectively.
    The 'aapl_id' property list is currently unused, but will be used in the
    future for optional attribute access properties.  The attribute ID returned
    from this function must be released with H5Aclose or resource leaks will
    develop.

--------------------------------------------------------------------------*/
hid_t
H5Acreate_by_name(hid_t loc_id, const char *obj_name, const char *attr_name,
    hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id,
    hid_t lapl_id)
{
    H5A_t	        *attr = NULL;           /* Attribute created */
    H5G_loc_t           loc;                    /* Object location */
    H5G_loc_t           obj_loc;                /* Location used to open group */
    H5G_name_t          obj_path;            	/* Opened object group hier. path */
    H5O_loc_t           obj_oloc;            	/* Opened object object location */
    hbool_t             loc_found = FALSE;      /* Entry at 'obj_name' found */
    H5T_t		*type;                  /* Datatype to use for attribute */
    H5S_t		*space;                 /* Dataspace to use for attribute */
    hid_t               dxpl_id = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    hid_t		ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE8("i", "i*s*siiiii", loc_id, obj_name, attr_name, type_id, space_id,
             acpl_id, aapl_id, lapl_id);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(0 == (H5F_INTENT(loc.oloc->file) & H5F_ACC_RDWR))
	HGOTO_ERROR(H5E_ARGS, H5E_WRITEERROR, FAIL, "no write intent on file")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no object name")
    if(!attr_name || !*attr_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no attribute name")
    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a type")
    if(NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data space")

    /* Set up opened group location to fill in */
    obj_loc.oloc = &obj_oloc;
    obj_loc.path = &obj_path;
    H5G_loc_reset(&obj_loc);

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&aapl_id, H5P_CLS_AACC, &dxpl_id, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Find the object's location */
    if(H5G_loc_find(&loc, obj_name, &obj_loc/*out*/, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_NOTFOUND, FAIL, "object not found")
    loc_found = TRUE;

    /* Go do the real work for attaching the attribute to the dataset */
    if(NULL == (attr = H5A_create(&obj_loc, attr_name, type, space, acpl_id, dxpl_id)))
	HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL, "unable to create attribute")

    /* Register the new attribute and get an ID for it */
    if((ret_value = H5I_register(H5I_ATTR, attr, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register attribute for ID")
done:
    /* Release resources */
    if(loc_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTRELEASE, FAIL, "can't free location")
    if(ret_value < 0 && attr && H5A_close(attr) < 0)
            HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* H5Acreate_by_name() */


/*--------------------------------------------------------------------------
 NAME
    H5Aopen
 PURPOSE
    Opens an attribute for an object by looking up the attribute name
 USAGE
    hid_t H5Aopen(loc_id, attr_name, aapl_id)
        hid_t loc_id;           IN: Object that attribute is attached to
        const char *attr_name;  IN: Name of attribute to locate and open
        hid_t aapl_id;          IN: Attribute access property list
 RETURNS
    ID of attribute on success, negative on failure

 DESCRIPTION
        This function opens an existing attribute for access.  The attribute
    name specified is used to look up the corresponding attribute for the
    object.  The attribute ID returned from this function must be released with
    H5Aclose or resource leaks will develop.
--------------------------------------------------------------------------*/
hid_t
H5Aopen(hid_t loc_id, const char *attr_name, hid_t aapl_id)
{
    H5G_loc_t    	loc;            /* Object location */
    H5A_t               *attr = NULL;   /* Attribute opened */
    hid_t               dxpl_id  = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    hid_t		ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("i", "i*si", loc_id, attr_name, aapl_id);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!attr_name || !*attr_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no attribute name")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&aapl_id, H5P_CLS_AACC, &dxpl_id, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Read in attribute from object header */
    if(NULL == (attr = H5O_attr_open_by_name(loc.oloc, attr_name, dxpl_id)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL, "unable to load attribute info from object header for attribute: '%s'", attr_name)

    /* Finish initializing attribute */
    if(H5A__open_common(&loc, attr) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL, "unable to initialize attribute")

    /* Register the attribute and get an ID for it */
    if((ret_value = H5I_register(H5I_ATTR, attr, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register attribute for ID")

done:
    /* Cleanup on failure */
    if(ret_value < 0)
        if(attr && H5A_close(attr) < 0)
            HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* H5Aopen() */


/*--------------------------------------------------------------------------
 NAME
    H5Aopen_by_name
 PURPOSE
    Opens an attribute for an object by looking up the attribute name
 USAGE
    hid_t H5Aopen_by_name(loc_id, obj_name, attr_name, aapl_id, lapl_id)
        hid_t loc_id;           IN: Object that attribute is attached to
        const char *obj_name;   IN: Name of object relative to location
        const char *attr_name;  IN: Name of attribute to locate and open
        hid_t aapl_id;          IN: Attribute access property list
        hid_t lapl_id;          IN: Link access property list
 RETURNS
    ID of attribute on success, negative on failure

 DESCRIPTION
        This function opens an existing attribute for access.  The attribute
    name specified is used to look up the corresponding attribute for the
    object.  The attribute ID returned from this function must be released with
    H5Aclose or resource leaks will develop.
--------------------------------------------------------------------------*/
hid_t
H5Aopen_by_name(hid_t loc_id, const char *obj_name, const char *attr_name,
    hid_t aapl_id, hid_t lapl_id)
{
    H5G_loc_t    	loc;            /* Object location */
    H5A_t               *attr = NULL;   /* Attribute opened */
    hid_t               dxpl_id  = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    hid_t		ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE5("i", "i*s*sii", loc_id, obj_name, attr_name, aapl_id, lapl_id);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no object name")
    if(!attr_name || !*attr_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no attribute name")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&aapl_id, H5P_CLS_AACC, &dxpl_id, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Open the attribute on the object header */
    if(NULL == (attr = H5A_open_by_name(&loc, obj_name, attr_name, lapl_id, dxpl_id)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "can't open attribute")

    /* Register the attribute and get an ID for it */
    if((ret_value = H5I_register(H5I_ATTR, attr, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register attribute for ID")

done:
    /* Cleanup on failure */
    if(ret_value < 0)
        if(attr && H5A_close(attr) < 0)
            HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* H5Aopen_by_name() */


/*--------------------------------------------------------------------------
 NAME
    H5Aopen_by_idx
 PURPOSE
    Opens the n'th attribute for an object, according to the order within
    an index
 USAGE
    hid_t H5Aopen_by_idx(loc_id, obj_ame, idx_type, order, n, aapl_id, lapl_id)
        hid_t loc_id;           IN: Object that attribute is attached to
        const char *obj_name;   IN: Name of object relative to location
        H5_index_t idx_type;    IN: Type of index to use
        H5_iter_order_t order;  IN: Order to iterate over index
        hsize_t n;              IN: Index (0-based) attribute to open
        hid_t aapl_id;          IN: Attribute access property list
        hid_t lapl_id;          IN: Link access property list
 RETURNS
    ID of attribute on success, negative on failure

 DESCRIPTION
        This function opens an existing attribute for access.  The attribute
    index specified is used to look up the corresponding attribute for the
    object.  The attribute ID returned from this function must be released with
    H5Aclose or resource leaks will develop.
--------------------------------------------------------------------------*/
hid_t
H5Aopen_by_idx(hid_t loc_id, const char *obj_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, hid_t aapl_id, hid_t lapl_id)
{
    H5A_t       *attr = NULL;   /* Attribute opened */
    H5G_loc_t	loc;	        /* Object location */
    hid_t       dxpl_id  = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    hid_t	ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("i", "i*sIiIohii", loc_id, obj_name, idx_type, order, n, aapl_id,
             lapl_id);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no object name")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(H5P_DEFAULT == lapl_id)
        lapl_id = H5P_LINK_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(lapl_id, H5P_LINK_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not link access property list ID")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&aapl_id, H5P_CLS_AACC, &dxpl_id, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Open the attribute in the object header */
    if(NULL == (attr = H5A_open_by_idx(&loc, obj_name, idx_type, order, n, lapl_id, dxpl_id)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "unable to open attribute")

    /* Register the attribute and get an ID for it */
    if((ret_value = H5I_register(H5I_ATTR, attr, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register attribute for ID")

done:
    /* Cleanup on failure */
    if(ret_value < 0)
        if(attr && H5A_close(attr) < 0)
            HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* H5Aopen_by_idx() */


/*--------------------------------------------------------------------------
 NAME
    H5Awrite
 PURPOSE
    Write out data to an attribute
 USAGE
    herr_t H5Awrite (attr_id, dtype_id, buf)
        hid_t attr_id;       IN: Attribute to write
        hid_t dtype_id;       IN: Memory datatype of buffer
        const void *buf;     IN: Buffer of data to write
 RETURNS
    Non-negative on success/Negative on failure

 DESCRIPTION
        This function writes a complete attribute to disk.
--------------------------------------------------------------------------*/
herr_t
H5Awrite(hid_t attr_id, hid_t dtype_id, const void *buf)
{
    H5A_t *attr;                /* Attribute object for ID */
    H5T_t *mem_type;            /* Memory datatype */
    hid_t  dxpl_id = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    hid_t  aapl_id = H5P_DEFAULT;  /* temp access plist */
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "ii*x", attr_id, dtype_id, buf);

    /* check arguments */
    if(NULL == (attr = (H5A_t *)H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute")
    if(NULL == (mem_type = (H5T_t *)H5I_object_verify(dtype_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if(NULL == buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null attribute buffer")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&aapl_id, H5P_CLS_AACC, &dxpl_id, attr_id, TRUE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Go write the actual data to the attribute */
    if((ret_value = H5A__write(attr, mem_type, buf, dxpl_id)) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_WRITEERROR, FAIL, "unable to write attribute")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Awrite() */


/*--------------------------------------------------------------------------
 NAME
    H5Aread
 PURPOSE
    Read in data from an attribute
 USAGE
    herr_t H5Aread (attr_id, dtype_id, buf)
        hid_t attr_id;       IN: Attribute to read
        hid_t dtype_id;       IN: Memory datatype of buffer
        void *buf;           IN: Buffer for data to read
 RETURNS
    Non-negative on success/Negative on failure

 DESCRIPTION
        This function reads a complete attribute from disk.
--------------------------------------------------------------------------*/
herr_t
H5Aread(hid_t attr_id, hid_t dtype_id, void *buf)
{
    H5A_t *attr;                /* Attribute object for ID */
    H5T_t *mem_type;            /* Memory datatype */
    hid_t  dxpl_id = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    hid_t  aapl_id = H5P_DEFAULT;  /* temp access plist */
    herr_t ret_value;           /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "ii*x", attr_id, dtype_id, buf);

    /* check arguments */
    if(NULL == (attr = (H5A_t *)H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute")
    if(NULL == (mem_type = (H5T_t *)H5I_object_verify(dtype_id, H5I_DATATYPE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")
    if(NULL == buf)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null attribute buffer")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&aapl_id, H5P_CLS_AACC, &dxpl_id, attr_id, FALSE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Go write the actual data to the attribute */
    if((ret_value = H5A__read(attr, mem_type, buf, dxpl_id)) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_READERROR, FAIL, "unable to read attribute")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Aread() */


/*--------------------------------------------------------------------------
 NAME
    H5Aget_space
 PURPOSE
    Gets a copy of the dataspace for an attribute
 USAGE
    hid_t H5Aget_space (attr_id)
        hid_t attr_id;       IN: Attribute to get dataspace of
 RETURNS
    A dataspace ID on success, negative on failure

 DESCRIPTION
        This function retrieves a copy of the dataspace for an attribute.
    The dataspace ID returned from this function must be released with H5Sclose
    or resource leaks will develop.
--------------------------------------------------------------------------*/
hid_t
H5Aget_space(hid_t attr_id)
{
    H5A_t	*attr;                  /* Attribute object for ID */
    H5S_t      *ds = NULL;
    hid_t	ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", attr_id);

    /* check arguments */
    if(NULL == (attr = (H5A_t *)H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute")

    if(NULL == (ds = H5A_get_space(attr)))
        HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get space ID of attribute")

    /* Atomize */
    if((ret_value = H5I_register(H5I_DATASPACE, ds, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register dataspace atom")

done:
    if(ret_value < 0) {
        if(ds && (H5S_close(ds) < 0))
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release dataspace")
    } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* H5Aget_space() */


/*--------------------------------------------------------------------------
 NAME
    H5Aget_type
 PURPOSE
    Gets a copy of the datatype for an attribute
 USAGE
    hid_t H5Aget_type (attr_id)
        hid_t attr_id;       IN: Attribute to get datatype of
 RETURNS
    A datatype ID on success, negative on failure

 DESCRIPTION
        This function retrieves a copy of the datatype for an attribute.
    The datatype ID returned from this function must be released with H5Tclose
    or resource leaks will develop.
--------------------------------------------------------------------------*/
hid_t
H5Aget_type(hid_t attr_id)
{
    H5A_t	*attr;          /* Attribute object for ID */
    H5T_t      *dt = NULL;
    hid_t	 ret_value;     /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", attr_id);

    /* check arguments */
    if(NULL == (attr = (H5A_t *)H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute")

    if(NULL == (dt = H5A_get_type(attr)))
        HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get space ID of attribute")

    /* Create an atom */
    if((ret_value = H5I_register(H5I_DATATYPE, dt, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register datatype")

done:
    if(ret_value < 0) {
        if(dt && (H5T_close(dt) < 0))
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, FAIL, "unable to release datatype")
    } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* H5Aget_type() */


/*--------------------------------------------------------------------------
 NAME
    H5Aget_create_plist
 PURPOSE
    Gets a copy of the creation property list for an attribute
 USAGE
    hssize_t H5Aget_create_plist (attr_id, buf_size, buf)
        hid_t attr_id;      IN: Attribute to get name of
 RETURNS
    This function returns the ID of a copy of the attribute's creation
    property list, or negative on failure.

 ERRORS

 DESCRIPTION
        This function returns a copy of the creation property list for
    an attribute.  The resulting ID must be closed with H5Pclose() or
    resource leaks will occur.
--------------------------------------------------------------------------*/
hid_t
H5Aget_create_plist(hid_t attr_id)
{
    H5A_t		*attr;               /* Attribute object for ID */
    hid_t		ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE1("i", "i", attr_id);

    HDassert(H5P_LST_ATTRIBUTE_CREATE_ID_g != -1);

    /* Get attribute and default attribute creation property list*/
    if(NULL == (attr = (H5A_t *)H5I_object_verify(attr_id, H5I_ATTR)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute")

    if((ret_value = H5A_get_create_plist(attr)) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_CANTGET, FAIL, "can't get creation property list for attr")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Aget_create_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5Aget_name
 PURPOSE
    Gets a copy of the name for an attribute
 USAGE
    hssize_t H5Aget_name (attr_id, buf_size, buf)
        hid_t attr_id;      IN: Attribute to get name of
        size_t buf_size;    IN: The size of the buffer to store the string in.
        char *buf;          IN: Buffer to store name in
 RETURNS
    This function returns the length of the attribute's name (which may be
    longer than 'buf_size') on success or negative for failure.

 DESCRIPTION
        This function retrieves the name of an attribute for an attribute ID.
    Up to 'buf_size' characters are stored in 'buf' followed by a '\0' string
    terminator.  If the name of the attribute is longer than 'buf_size'-1,
    the string terminator is stored in the last position of the buffer to
    properly terminate the string.
--------------------------------------------------------------------------*/
ssize_t
H5Aget_name(hid_t attr_id, size_t buf_size, char *buf)
{
    H5A_t		*my_attr;               /* Attribute object for ID */
    ssize_t		ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("Zs", "iz*s", attr_id, buf_size, buf);

    /* check arguments */
    if(NULL == (my_attr = (H5A_t *)H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute")
    if(!buf && buf_size)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid buffer")

    /* Call private function in turn */
    if(0 > (ret_value = H5A__get_name(my_attr, buf_size, buf)))
	HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "can't get attribute name")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Aget_name() */


/*-------------------------------------------------------------------------
 * Function:	H5Aget_name_by_idx
 *
 * Purpose:	Retrieve name of an attribute, according to the
 *		order within an index.
 *
 *              Same pattern of behavior as H5Iget_name.
 *
 * Return:	Success:	Non-negative length of name, with information
 *				in NAME buffer
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              February  8, 2007
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Aget_name_by_idx(hid_t loc_id, const char *obj_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, char *name /*out*/, size_t size,
    hid_t lapl_id)
{
    H5G_loc_t   loc;            /* Object location */
    H5A_t	*attr = NULL;   /* Attribute object for name */
    hid_t       dxpl_id  = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    ssize_t	ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE8("Zs", "i*sIiIohxzi", loc_id, obj_name, idx_type, order, n, name, size,
             lapl_id);

    /* Check args */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&lapl_id, H5P_CLS_LACC, &dxpl_id, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Open the attribute on the object header */
    if(NULL == (attr = H5A_open_by_idx(&loc, obj_name, idx_type, order, n, lapl_id, dxpl_id)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "can't open attribute")

    /* Get the length of the name */
    ret_value = (ssize_t)HDstrlen(attr->shared->name);

    /* Copy the name into the user's buffer, if given */
    if(name) {
        HDstrncpy(name, attr->shared->name, MIN((size_t)(ret_value + 1), size));
        if((size_t)ret_value >= size)
            name[size - 1]='\0';
    } /* end if */

done:
    /* Release resources */
    if(attr && H5A_close(attr) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* end H5Aget_name_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5Aget_storage_size
 *
 * Purpose:	Returns the amount of storage size that is required for this
 *		attribute.
 *
 * Return:	Success:	The amount of storage size allocated for the
 *				attribute.  The return value may be zero
 *                              if no data has been stored.
 *
 *		Failure:	Zero
 *
 * Programmer:	Raymond Lu
 *              October 23, 2002
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5Aget_storage_size(hid_t attr_id)
{
    H5A_t	*attr;               /* Attribute object for ID */
    hsize_t	ret_value;      /* Return value */

    FUNC_ENTER_API(0)
    H5TRACE1("h", "i", attr_id);

    /* Check args */
    if(NULL == (attr = (H5A_t *)H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not an attribute")

    /* Set return value */
    ret_value = attr->shared->data_size;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Aget_storage_size() */


/*-------------------------------------------------------------------------
 * Function:	H5Aget_info
 *
 * Purpose:	Retrieve information about an attribute.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              February  6, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Aget_info(hid_t attr_id, H5A_info_t *ainfo)
{
    H5A_t	*attr;                  /* Attribute object for name */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", attr_id, ainfo);

    /* Check args */
    if(NULL == (attr = (H5A_t *)H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute")

    /* Get the attribute information */
    if(H5A__get_info(attr, ainfo) < 0)
	HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "unable to get attribute info")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Aget_info() */


/*-------------------------------------------------------------------------
 * Function:	H5Aget_info_by_name
 *
 * Purpose:	Retrieve information about an attribute by name.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              February  6, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Aget_info_by_name(hid_t loc_id, const char *obj_name, const char *attr_name,
    H5A_info_t *ainfo, hid_t lapl_id)
{
    H5G_loc_t   loc;                    /* Object location */
    H5A_t	*attr = NULL;           /* Attribute object for name */
    hid_t       dxpl_id  = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "i*s*s*xi", loc_id, obj_name, attr_name, ainfo, lapl_id);

    /* Check args */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no object name")
    if(!attr_name || !*attr_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no attribute name")
    if(NULL == ainfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid info pointer")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&lapl_id, H5P_CLS_LACC, &dxpl_id, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Open the attribute on the object header */
    if(NULL == (attr = H5A_open_by_name(&loc, obj_name, attr_name, lapl_id, dxpl_id)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "can't open attribute")

    /* Get the attribute information */
    if(H5A__get_info(attr, ainfo) < 0)
	HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "unable to get attribute info")

done:
    /* Cleanup on failure */
    if(attr && H5A_close(attr) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* end H5Aget_info_by_name() */


/*-------------------------------------------------------------------------
 * Function:	H5Aget_info_by_idx
 *
 * Purpose:	Retrieve information about an attribute, according to the
 *		order within an index.
 *
 * Return:	Success:	Non-negative with information in AINFO
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              February  8, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Aget_info_by_idx(hid_t loc_id, const char *obj_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, H5A_info_t *ainfo, hid_t lapl_id)
{
    H5G_loc_t   loc;                    /* Object location */
    H5A_t	*attr = NULL;           /* Attribute object for name */
    hid_t       dxpl_id  = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE7("e", "i*sIiIoh*xi", loc_id, obj_name, idx_type, order, n, ainfo,
             lapl_id);

    /* Check args */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")
    if(NULL == ainfo)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid info pointer")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&lapl_id, H5P_CLS_LACC, &dxpl_id, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Open the attribute on the object header */
    if(NULL == (attr = H5A_open_by_idx(&loc, obj_name, idx_type, order, n, lapl_id, dxpl_id)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "can't open attribute")

    /* Get the attribute information */
    if(H5A__get_info(attr, ainfo) < 0)
	HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "unable to get attribute info")

done:
    /* Release resources */
    if(attr && H5A_close(attr) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* end H5Aget_info_by_idx() */


/*-------------------------------------------------------------------------
 * Function:	H5Arename
 *
 * Purpose:     Rename an attribute
 *
 * Return:	Success:             Non-negative
 *		Failure:             Negative
 *
 * Programmer:	Raymond Lu
 *              October 23, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Arename(hid_t loc_id, const char *old_name, const char *new_name)
{
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*s*s", loc_id, old_name, new_name);

    /* check arguments */
    if(!old_name || !new_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "name is nil")
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")

    /* Avoid thrashing things if the names are the same */
    if(HDstrcmp(old_name, new_name)) {
        H5G_loc_t loc;                /* Object location */
  
        if(H5G_loc(loc_id, & loc) < 0)
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

        /* Call private attribute rename routine */
        if(H5O_attr_rename(loc.oloc, H5AC_ind_read_dxpl_id, old_name, new_name) < 0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTRENAME, FAIL, "can't rename attribute")
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Arename() */


/*-------------------------------------------------------------------------
 * Function:	H5Arename_by_name
 *
 * Purpose:     Rename an attribute
 *
 * Return:	Success:             Non-negative
 *		Failure:             Negative
 *
 * Programmer:	Quincey Koziol
 *              February 20, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Arename_by_name(hid_t loc_id, const char *obj_name, const char *old_attr_name,
    const char *new_attr_name, hid_t lapl_id)
{
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "i*s*s*si", loc_id, obj_name, old_attr_name, new_attr_name,
             lapl_id);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no object name")
    if(!old_attr_name || !*old_attr_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no old attribute name")
    if(!new_attr_name || !*new_attr_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no new attribute name")

    /* Avoid thrashing things if the names are the same */
    if(HDstrcmp(old_attr_name, new_attr_name)) {
        H5G_loc_t loc;                /* Object location */
        hid_t dxpl_id = H5AC_ind_read_dxpl_id; /* dxpl used by the library */

        /* Verify access property list and get correct dxpl */
        if(H5P_verify_apl_and_dxpl(&lapl_id, H5P_CLS_LACC, &dxpl_id, loc_id, TRUE) < 0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

        if(H5G_loc(loc_id, & loc) < 0)
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

        /* Call private attribute rename routine */
        if(H5A_rename_by_name(loc, obj_name, old_attr_name, new_attr_name, lapl_id, dxpl_id) < 0)
            HGOTO_ERROR(H5E_ATTR, H5E_CANTRENAME, FAIL, "can't rename attribute")
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Arename_by_name() */


/*--------------------------------------------------------------------------
 NAME
    H5Aiterate2
 PURPOSE
    Calls a user's function for each attribute on an object
 USAGE
    herr_t H5Aiterate2(loc_id, idx_type, order, idx, op, op_data)
        hid_t loc_id;           IN: Base location for object
        H5_index_t idx_type;    IN: Type of index to use
        H5_iter_order_t order;  IN: Order to iterate over index
        hsize_t *idx;           IN/OUT: Starting (IN) & Ending (OUT) attribute
                                    in index & order
        H5A_operator2_t op;     IN: User's function to pass each attribute to
        void *op_data;          IN/OUT: User's data to pass through to iterator
                                    operator function
 RETURNS
        Returns a negative value if an error occurs, the return value of the
    last operator if it was non-zero (which can be a negative value), or zero
    if all attributes were processed.

 DESCRIPTION
        This function interates over the attributes of dataset or group
    specified with 'loc_id' & 'obj_name'.  For each attribute of the object,
    the 'op_data' and some additional information (specified below) are passed
    to the 'op' function.  The iteration begins with the '*idx'
    object in the group and the next attribute to be processed by the operator
    is returned in '*idx'.
        The operation receives the ID for the group or dataset being iterated
    over ('loc_id'), the name of the current attribute about the object
    ('attr_name'), the attribute's "info" struct ('ainfo') and the pointer to
    the operator data passed in to H5Aiterate2 ('op_data').  The return values
    from an operator are:
        A. Zero causes the iterator to continue, returning zero when all
            attributes have been processed.
        B. Positive causes the iterator to immediately return that positive
            value, indicating short-circuit success.  The iterator can be
            restarted at the next attribute.
        C. Negative causes the iterator to immediately return that value,
            indicating failure.  The iterator can be restarted at the next
            attribute.
--------------------------------------------------------------------------*/
herr_t
H5Aiterate2(hid_t loc_id, H5_index_t idx_type, H5_iter_order_t order,
    hsize_t *idx, H5A_operator2_t op, void *op_data)
{
    H5A_attr_iter_op_t attr_op; /* Attribute operator */
    hsize_t	start_idx;      /* Index of attribute to start iterating at */
    hsize_t	last_attr;      /* Index of last attribute examined */
    herr_t	ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "iIiIo*hx*x", loc_id, idx_type, order, idx, op, op_data);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")

    /* Build attribute operator info */
    attr_op.op_type = H5A_ATTR_OP_APP2;
    attr_op.u.app_op2 = op;

    /* Call attribute iteration routine */
    last_attr = start_idx = (idx ? *idx : 0);
    if((ret_value = H5O_attr_iterate(loc_id, H5AC_ind_read_dxpl_id, idx_type, order, start_idx, &last_attr, &attr_op, op_data)) < 0)
        HERROR(H5E_ATTR, H5E_BADITER, "error iterating over attributes");

    /* Set the last attribute information */
    if(idx)
        *idx = last_attr;

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Aiterate2() */


/*--------------------------------------------------------------------------
 NAME
    H5Aiterate_by_name
 PURPOSE
    Calls a user's function for each attribute on an object
 USAGE
    herr_t H5Aiterate2(loc_id, obj_name, idx_type, order, idx, op, op_data, lapl_id)
        hid_t loc_id;           IN: Base location for object
        const char *obj_name;   IN: Name of object relative to location
        H5_index_t idx_type;    IN: Type of index to use
        H5_iter_order_t order;  IN: Order to iterate over index
        hsize_t *idx;           IN/OUT: Starting (IN) & Ending (OUT) attribute
                                    in index & order
        H5A_operator2_t op;     IN: User's function to pass each attribute to
        void *op_data;          IN/OUT: User's data to pass through to iterator
                                    operator function
        hid_t lapl_id;          IN: Link access property list
 RETURNS
        Returns a negative value if an error occurs, the return value of the
    last operator if it was non-zero (which can be a negative value), or zero
    if all attributes were processed.

 DESCRIPTION
        This function interates over the attributes of dataset or group
    specified with 'loc_id' & 'obj_name'.  For each attribute of the object,
    the 'op_data' and some additional information (specified below) are passed
    to the 'op' function.  The iteration begins with the '*idx'
    object in the group and the next attribute to be processed by the operator
    is returned in '*idx'.
        The operation receives the ID for the group or dataset being iterated
    over ('loc_id'), the name of the current attribute about the object
    ('attr_name'), the attribute's "info" struct ('ainfo') and the pointer to
    the operator data passed in to H5Aiterate_by_name ('op_data').  The return values
    from an operator are:
        A. Zero causes the iterator to continue, returning zero when all
            attributes have been processed.
        B. Positive causes the iterator to immediately return that positive
            value, indicating short-circuit success.  The iterator can be
            restarted at the next attribute.
        C. Negative causes the iterator to immediately return that value,
            indicating failure.  The iterator can be restarted at the next
            attribute.
--------------------------------------------------------------------------*/
herr_t
H5Aiterate_by_name(hid_t loc_id, const char *obj_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t *idx, H5A_operator2_t op, void *op_data,
    hid_t lapl_id)
{
    H5G_loc_t	loc;	        /* Object location */
    H5G_loc_t   obj_loc;        /* Location used to open group */
    H5G_name_t  obj_path;       /* Opened object group hier. path */
    H5O_loc_t   obj_oloc;       /* Opened object object location */
    hbool_t     loc_found = FALSE;      /* Entry at 'obj_name' found */
    hid_t       obj_loc_id = (-1);      /* ID for object located */
    H5A_attr_iter_op_t attr_op; /* Attribute operator */
    hsize_t	start_idx;      /* Index of attribute to start iterating at */
    hsize_t	last_attr;      /* Index of last attribute examined */
    hid_t       dxpl_id  = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    herr_t	ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE8("e", "i*sIiIo*hx*xi", loc_id, obj_name, idx_type, order, idx, op,
             op_data, lapl_id);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no object name")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&lapl_id, H5P_CLS_LACC, &dxpl_id, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Set up opened group location to fill in */
    obj_loc.oloc = &obj_oloc;
    obj_loc.path = &obj_path;
    H5G_loc_reset(&obj_loc);

    /* Find the object's location */
    if(H5G_loc_find(&loc, obj_name, &obj_loc/*out*/, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_NOTFOUND, FAIL, "object not found")
    loc_found = TRUE;

    /* Open the object */
    if((obj_loc_id = H5O_open_by_loc(&obj_loc, lapl_id, dxpl_id, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "unable to open object")

    /* Build attribute operator info */
    attr_op.op_type = H5A_ATTR_OP_APP2;
    attr_op.u.app_op2 = op;

    /* Call attribute iteration routine */
    last_attr = start_idx = (idx ? *idx : 0);
    if((ret_value = H5O_attr_iterate(obj_loc_id, dxpl_id, idx_type, order, start_idx, &last_attr, &attr_op, op_data)) < 0)
        HERROR(H5E_ATTR, H5E_BADITER, "error iterating over attributes");

    /* Set the last attribute information */
    if(idx)
        *idx = last_attr;

done:
    /* Release resources */
    if(obj_loc_id > 0) {
        if(H5I_dec_app_ref(obj_loc_id) < 0)
            HDONE_ERROR(H5E_ATTR, H5E_CANTDEC, FAIL, "unable to close temporary object")
    } /* end if */
    else if(loc_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTRELEASE, FAIL, "can't free location")

    FUNC_LEAVE_API(ret_value)
} /* H5Aiterate_by_name() */


/*--------------------------------------------------------------------------
 NAME
    H5Adelete
 PURPOSE
    Deletes an attribute from a location
 USAGE
    herr_t H5Adelete(loc_id, name)
        hid_t loc_id;       IN: Object (dataset or group) to have attribute deleted from
        const char *name;   IN: Name of attribute to delete
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function removes the named attribute from a dataset or group.
--------------------------------------------------------------------------*/
herr_t
H5Adelete(hid_t loc_id, const char *name)
{
    H5G_loc_t	loc;		        /* Object location */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*s", loc_id, name);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Delete the attribute from the location */
    if(H5O_attr_remove(loc.oloc, name, H5AC_ind_read_dxpl_id) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTDELETE, FAIL, "unable to delete attribute")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Adelete() */


/*--------------------------------------------------------------------------
 NAME
    H5Adelete_by_name
 PURPOSE
    Deletes an attribute from a location
 USAGE
    herr_t H5Adelete_by_name(loc_id, obj_name, attr_name, lapl_id)
        hid_t loc_id;           IN: Base location for object
        const char *obj_name;   IN: Name of object relative to location
        const char *attr_name;  IN: Name of attribute to delete
        hid_t lapl_id;          IN: Link access property list
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    This function removes the named attribute from an object.
--------------------------------------------------------------------------*/
herr_t
H5Adelete_by_name(hid_t loc_id, const char *obj_name, const char *attr_name,
    hid_t lapl_id)
{
    H5G_loc_t	loc;		        /* Object location */
    H5G_loc_t   obj_loc;                /* Location used to open group */
    H5G_name_t  obj_path;            	/* Opened object group hier. path */
    H5O_loc_t   obj_oloc;            	/* Opened object object location */
    hbool_t     loc_found = FALSE;      /* Entry at 'obj_name' found */
    hid_t       dxpl_id = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "i*s*si", loc_id, obj_name, attr_name, lapl_id);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no object name")
    if(!attr_name || !*attr_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no attribute name")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&lapl_id, H5P_CLS_LACC, &dxpl_id, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Set up opened group location to fill in */
    obj_loc.oloc = &obj_oloc;
    obj_loc.path = &obj_path;
    H5G_loc_reset(&obj_loc);

    /* Find the object's location */
    if(H5G_loc_find(&loc, obj_name, &obj_loc/*out*/, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_NOTFOUND, FAIL, "object not found")
    loc_found = TRUE;

    /* Delete the attribute from the location */
    if(H5O_attr_remove(obj_loc.oloc, attr_name, dxpl_id) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTDELETE, FAIL, "unable to delete attribute")

done:
    /* Release resources */
    if(loc_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTRELEASE, FAIL, "can't free location")

    FUNC_LEAVE_API(ret_value)
} /* H5Adelete_by_name() */


/*--------------------------------------------------------------------------
 NAME
    H5Adelete_by_idx
 PURPOSE
    Deletes an attribute from a location, according to the order within an index
 USAGE
    herr_t H5Adelete_by_idx(loc_id, obj_name, idx_type, order, n, lapl_id)
        hid_t loc_id;           IN: Base location for object
        const char *obj_name;   IN: Name of object relative to location
        H5_index_t idx_type;    IN: Type of index to use
        H5_iter_order_t order;  IN: Order to iterate over index
        hsize_t n;              IN: Offset within index
        hid_t lapl_id;          IN: Link access property list
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
        This function removes an attribute from an object, using the IDX_TYPE
    index to delete the N'th attribute in ORDER direction in the index.  The
    object is specified relative to the LOC_ID with the OBJ_NAME path.  To
    remove an attribute on the object specified by LOC_ID, pass in "." for
    OBJ_NAME.  The link access property list, LAPL_ID, controls aspects of
    the group hierarchy traversal when using the OBJ_NAME to locate the final
    object to operate on.
--------------------------------------------------------------------------*/
herr_t
H5Adelete_by_idx(hid_t loc_id, const char *obj_name, H5_index_t idx_type,
    H5_iter_order_t order, hsize_t n, hid_t lapl_id)
{
    H5G_loc_t	loc;		        /* Object location */
    H5G_loc_t   obj_loc;                /* Location used to open group */
    H5G_name_t  obj_path;            	/* Opened object group hier. path */
    H5O_loc_t   obj_oloc;            	/* Opened object object location */
    hbool_t     loc_found = FALSE;      /* Entry at 'obj_name' found */
    hid_t       dxpl_id = H5AC_ind_read_dxpl_id; /* dxpl used by library */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE6("e", "i*sIiIohi", loc_id, obj_name, idx_type, order, n, lapl_id);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no object name")
    if(idx_type <= H5_INDEX_UNKNOWN || idx_type >= H5_INDEX_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid index type specified")
    if(order <= H5_ITER_UNKNOWN || order >= H5_ITER_N)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration order specified")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&lapl_id, H5P_CLS_LACC, &dxpl_id, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    /* Set up opened group location to fill in */
    obj_loc.oloc = &obj_oloc;
    obj_loc.path = &obj_path;
    H5G_loc_reset(&obj_loc);

    /* Find the object's location */
    if(H5G_loc_find(&loc, obj_name, &obj_loc/*out*/, lapl_id, dxpl_id) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_NOTFOUND, FAIL, "object not found")
    loc_found = TRUE;

    /* Delete the attribute from the location */
    if(H5O_attr_remove_by_idx(obj_loc.oloc, idx_type, order, n, dxpl_id) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTDELETE, FAIL, "unable to delete attribute")

done:
    /* Release resources */
    if(loc_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTRELEASE, FAIL, "can't free location")

    FUNC_LEAVE_API(ret_value)
} /* H5Adelete_by_idx() */


/*--------------------------------------------------------------------------
 NAME
    H5Aclose
 PURPOSE
    Close an attribute ID
 USAGE
    herr_t H5Aclose (attr_id)
        hid_t attr_id;       IN: Attribute to release access to
 RETURNS
    Non-negative on success/Negative on failure

 DESCRIPTION
        This function releases an attribute from use.  Further use of the
    attribute ID will result in undefined behavior.
--------------------------------------------------------------------------*/
herr_t
H5Aclose(hid_t attr_id)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", attr_id);

    /* check arguments */
    if(NULL == H5I_object_verify(attr_id, H5I_ATTR))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute")

    /* Decrement references to that atom (and close it) */
    if(H5I_dec_app_ref(attr_id) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTDEC, FAIL, "can't close attribute")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Aclose() */


/*-------------------------------------------------------------------------
 * Function:	H5Aexists
 *
 * Purpose:	Checks if an attribute with a given name exists on an opened
 *              object.
 *
 * Return:	Success:	TRUE/FALSE
 * 		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, November 1, 2007
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Aexists(hid_t obj_id, const char *attr_name)
{
    H5G_loc_t   loc;                    /* Object location */
    htri_t	ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("t", "i*s", obj_id, attr_name);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(obj_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(obj_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!attr_name || !*attr_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no attribute name")

    /* Check if the attribute exists */
    if((ret_value = H5O_attr_exists(loc.oloc, attr_name, H5AC_ind_read_dxpl_id)) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "unable to determine if attribute exists")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Aexists() */


/*-------------------------------------------------------------------------
 * Function:	H5Aexists_by_name
 *
 * Purpose:	Checks if an attribute with a given name exists on an object.
 *
 * Return:	Success:	TRUE/FALSE
 * 		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Thursday, November 1, 2007
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Aexists_by_name(hid_t loc_id, const char *obj_name, const char *attr_name,
    hid_t lapl_id)
{
    H5G_loc_t   loc;                    /* Object location */
    hid_t       dxpl_id = H5AC_ind_read_dxpl_id;     /* dxpl used by library */
    htri_t	ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("t", "i*s*si", loc_id, obj_name, attr_name, lapl_id);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!obj_name || !*obj_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no object name")
    if(!attr_name || !*attr_name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no attribute name")

    /* Verify access property list and get correct dxpl */
    if(H5P_verify_apl_and_dxpl(&lapl_id, H5P_CLS_LACC, &dxpl_id, loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set access and transfer property lists")

    if((ret_value = H5A_exists_by_name(loc, obj_name, attr_name, lapl_id, dxpl_id)) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "unable to determine if attribute exists")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Aexists_by_name() */

