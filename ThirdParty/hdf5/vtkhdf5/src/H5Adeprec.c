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

/*-------------------------------------------------------------------------
 *
 * Created:	H5Adeprec.c
 *		November 27 2006
 *		Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:	Deprecated functions from the H5A interface.  These
 *              functions are here for compatibility purposes and may be
 *              removed in the future.  Applications should switch to the
 *              newer APIs.
 *
 *-------------------------------------------------------------------------
 */

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
#include "H5CXprivate.h"        /* API Contexts                         */
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Opkg.h"             /* Object headers			*/


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


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/


#ifndef H5_NO_DEPRECATED_SYMBOLS

/*--------------------------------------------------------------------------
 NAME
    H5Acreate1
 PURPOSE
    Creates an attribute on an object
 USAGE
    hid_t H5Acreate1(loc_id, name, type_id, space_id, acpl_id)
        hid_t loc_id;       IN: Object (dataset or group) to be attached to
        const char *name;   IN: Name of attribute to create
        hid_t type_id;      IN: ID of datatype for attribute
        hid_t space_id;     IN: ID of dataspace for attribute
        hid_t acpl_id;      IN: ID of creation property list (currently not used)
 RETURNS
    Non-negative on success/Negative on failure

 DESCRIPTION
        This function creates an attribute which is attached to the object
    specified with 'location_id'.  The name specified with 'name' for each
    attribute for an object must be unique for that object.  The 'type_id'
    and 'space_id' are created with the H5T and H5S interfaces respectively.
    The attribute ID returned from this function must be released with H5Aclose
    or resource leaks will develop.

 NOTE
    Deprecated in favor of H5Acreate2

--------------------------------------------------------------------------*/
hid_t
H5Acreate1(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id,
    hid_t acpl_id)
{
    H5A_t	*attr = NULL;           /* Attribute created */
    H5G_loc_t   loc;                    /* Object location */
    H5T_t	*type;                  /* Datatype to use for attribute */
    H5S_t	*space;                 /* Dataspace to use for attribute */
    hid_t	ret_value;              /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("i", "i*siii", loc_id, name, type_id, space_id, acpl_id);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(0 == (H5F_INTENT(loc.oloc->file) & H5F_ACC_RDWR))
	HGOTO_ERROR(H5E_ARGS, H5E_WRITEERROR, FAIL, "no write intent on file")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a type")
    if(NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace")

    /* Set up collective metadata if appropriate */
    if(H5CX_set_loc(loc_id) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, H5I_INVALID_HID, "can't set collective metadata read")

    /* Go do the real work for attaching the attribute to the dataset */
    if(NULL == (attr = H5A__create(&loc, name, type, space, acpl_id)))
	HGOTO_ERROR(H5E_ATTR, H5E_CANTINIT, FAIL, "unable to create attribute")

    /* Register the new attribute and get an ID for it */
    if((ret_value = H5I_register(H5I_ATTR, attr, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register attribute for ID")

done:
    /* Cleanup on failure */
    if(ret_value < 0 && attr && H5A__close(attr) < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* H5Acreate1() */


/*--------------------------------------------------------------------------
 NAME
    H5Aopen_name
 PURPOSE
    Opens an attribute for an object by looking up the attribute name
 USAGE
    hid_t H5Aopen_name (loc_id, name)
        hid_t loc_id;       IN: Object (dataset or group) to be attached to
        const char *name;   IN: Name of attribute to locate and open
 RETURNS
    ID of attribute on success, negative on failure

 DESCRIPTION
        This function opens an existing attribute for access.  The attribute
    name specified is used to look up the corresponding attribute for the
    object.  The attribute ID returned from this function must be released with
    H5Aclose or resource leaks will develop.
        The location object may be either a group or a dataset, both of
    which may have any sort of attribute.
 NOTE
    Deprecated in favor of H5Aopen
--------------------------------------------------------------------------*/
hid_t
H5Aopen_name(hid_t loc_id, const char *name)
{
    H5G_loc_t   loc;            /* Object location */
    H5A_t       *attr = NULL;   /* Attribute opened */
    hid_t	ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("i", "i*s", loc_id, name);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")

    /* Open the attribute on the object header */
    if(NULL == (attr = H5A__open_by_name(&loc, ".", name)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, FAIL, "can't open attribute: '%s'", name)

    /* Register the attribute and get an ID for it */
    if((ret_value = H5I_register(H5I_ATTR, attr, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register attribute for ID")

done:
    /* Cleanup on failure */
    if(ret_value < 0)
        if(attr && H5A__close(attr) < 0)
            HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* H5Aopen_name() */


/*--------------------------------------------------------------------------
 NAME
    H5Aopen_idx
 PURPOSE
    Opens the n'th attribute for an object
 USAGE
    hid_t H5Aopen_idx (loc_id, idx)
        hid_t loc_id;       IN: Object that attribute is attached to
        unsigned idx;       IN: Index (0-based) attribute to open
 RETURNS
    ID of attribute on success, negative on failure

 DESCRIPTION
        This function opens an existing attribute for access.  The attribute
    index specified is used to look up the corresponding attribute for the
    object.  The attribute ID returned from this function must be released with
    H5Aclose or resource leaks will develop.
        The location object may be either a group or a dataset, both of
    which may have any sort of attribute.
 NOTE
    Deprecated in favor of H5Aopen_by_idx
--------------------------------------------------------------------------*/
hid_t
H5Aopen_idx(hid_t loc_id, unsigned idx)
{
    H5G_loc_t	loc;	        /* Object location */
    H5A_t       *attr = NULL;   /* Attribute opened */
    hid_t	ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE2("i", "iIu", loc_id, idx);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")

    /* Open the attribute in the object header */
    if(NULL == (attr = H5A__open_by_idx(&loc, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, (hsize_t)idx)))
        HGOTO_ERROR(H5E_ATTR, H5E_CANTOPENOBJ, H5I_INVALID_HID, "unable to open attribute")

    /* Register the attribute and get an ID for it */
    if((ret_value = H5I_register(H5I_ATTR, attr, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register attribute for ID")

done:
    /* Cleanup on failure */
    if(ret_value < 0)
        if(attr && H5A__close(attr) < 0)
            HDONE_ERROR(H5E_ATTR, H5E_CANTFREE, FAIL, "can't close attribute")

    FUNC_LEAVE_API(ret_value)
} /* H5Aopen_idx() */


/*--------------------------------------------------------------------------
 NAME
    H5Aget_num_attrs
 PURPOSE
    Determines the number of attributes attached to an object
 NOTE
    Deprecated in favor of H5Oget_info[_by_idx]
 USAGE
    int H5Aget_num_attrs (loc_id)
        hid_t loc_id;       IN: Object (dataset or group) to be queried
 RETURNS
    Number of attributes on success, negative on failure
 DESCRIPTION
        This function returns the number of attributes attached to a dataset or
    group, 'location_id'.
 NOTE
    Deprecated in favor of H5Oget_info
--------------------------------------------------------------------------*/
int
H5Aget_num_attrs(hid_t loc_id)
{
    H5O_loc_t   *loc;	/* Object location for attribute */
    void        *obj;
    int		ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE1("Is", "i", loc_id);

    /* check arguments */
    if(H5I_BADID == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "bad location ID")
    if(H5I_FILE == H5I_get_type(loc_id) || H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")
    if(NULL == (obj = H5I_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADATOM, FAIL, "illegal object atom")
    switch(H5I_get_type (loc_id)) {
        case H5I_DATASET:
            if(NULL == (loc = H5D_oloc((H5D_t*)obj)))
                HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "can't get location for object")
            break;

        case H5I_DATATYPE:
            if(NULL == (loc = H5T_oloc((H5T_t*)obj)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "target datatype is not committed")
            break;

        case H5I_GROUP:
            if(NULL == (loc = H5G_oloc((H5G_t*)obj)))
                HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "can't get location for object")
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
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "inappropriate attribute target")
    } /*lint !e788 All appropriate cases are covered */

    /* Look up the # of attributes for the object */
    if((ret_value = H5O__attr_count(loc)) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTCOUNT, FAIL, "can't get attribute count for object")

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Aget_num_attrs() */


/*--------------------------------------------------------------------------
 NAME
    H5Aiterate1
 PURPOSE
    Calls a user's function for each attribute on an object
 USAGE
    herr_t H5Aiterate1(loc_id, attr_num, op, data)
        hid_t loc_id;       IN: Object (dataset or group) to be iterated over
        unsigned *attr_num; IN/OUT: Starting (IN) & Ending (OUT) attribute number
        H5A_operator1_t op;  IN: User's function to pass each attribute to
        void *op_data;      IN/OUT: User's data to pass through to iterator operator function
 RETURNS
        Returns a negative value if something is wrong, the return value of the
    last operator if it was non-zero, or zero if all attributes were processed.

 DESCRIPTION
        This function interates over the attributes of dataset or group
    specified with 'loc_id'.  For each attribute of the object, the
    'op_data' and some additional information (specified below) are passed
    to the 'op' function.  The iteration begins with the '*attr_number'
    object in the group and the next attribute to be processed by the operator
    is returned in '*attr_number'.
        The operation receives the ID for the group or dataset being iterated
    over ('loc_id'), the name of the current attribute about the object
    ('attr_name') and the pointer to the operator data passed in to H5Aiterate
    ('op_data').  The return values from an operator are:
        A. Zero causes the iterator to continue, returning zero when all
            attributes have been processed.
        B. Positive causes the iterator to immediately return that positive
            value, indicating short-circuit success.  The iterator can be
            restarted at the next attribute.
        C. Negative causes the iterator to immediately return that value,
            indicating failure.  The iterator can be restarted at the next
            attribute.
 NOTE
    Deprecated in favor of H5Aiterate2
--------------------------------------------------------------------------*/
herr_t
H5Aiterate1(hid_t loc_id, unsigned *attr_num, H5A_operator1_t op, void *op_data)
{
    herr_t	ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "i*Iux*x", loc_id, attr_num, op, op_data);

    /* check arguments */
    if(H5I_ATTR == H5I_get_type(loc_id))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "location is not valid for an attribute")

    /* Call attribute iteration routine */
    if((ret_value = H5A__iterate_old(loc_id, attr_num, op, op_data)) < 0)
        HERROR(H5E_ATTR, H5E_BADITER, "error iterating over attributes");

done:
    FUNC_LEAVE_API(ret_value)
} /* H5Aiterate1() */
#endif /* H5_NO_DEPRECATED_SYMBOLS */

