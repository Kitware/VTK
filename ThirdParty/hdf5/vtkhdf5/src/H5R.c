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

#include "H5Rmodule.h"          /* This source code file is part of the H5R module */


/***********/
/* Headers */
/***********/
#include "H5private.h"          /* Generic Functions                        */
#include "H5ACprivate.h"        /* Metadata cache                           */
#include "H5CXprivate.h"        /* API Contexts                             */
#include "H5Eprivate.h"         /* Error handling                           */
#include "H5Gprivate.h"         /* Groups                                   */
#include "H5Iprivate.h"         /* IDs                                      */
#include "H5Rpkg.h"             /* References                               */
#include "H5Sprivate.h"         /* Dataspaces                               */


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


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


/*--------------------------------------------------------------------------
 NAME
    H5Rcreate
 PURPOSE
    Creates a particular kind of reference for the user
 USAGE
    herr_t H5Rcreate(ref, loc_id, name, ref_type, space_id)
        void *ref;          OUT: Reference created
        hid_t loc_id;       IN: Location ID used to locate object pointed to
        const char *name;   IN: Name of object at location LOC_ID of object
                                    pointed to
        H5R_type_t ref_type;    IN: Type of reference to create
        hid_t space_id;     IN: Dataspace ID with selection, used for Dataset
                                    Region references.

 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Creates a particular type of reference specified with REF_TYPE, in the
    space pointed to by REF.  The LOC_ID and NAME are used to locate the object
    pointed to and the SPACE_ID is used to choose the region pointed to (for
    Dataset Region references).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Rcreate(void *ref, hid_t loc_id, const char *name, H5R_type_t ref_type, hid_t space_id)
{
    H5G_loc_t   loc;            /* File location */
    H5S_t      *space = NULL;   /* Pointer to dataspace containing region */
    herr_t      ret_value;      /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "*xi*sRti", ref, loc_id, name, ref_type, space_id);

    /* Check args */
    if (ref == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference pointer")
    if (H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name given")
    if (ref_type <= H5R_BADTYPE || ref_type >= H5R_MAXTYPE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference type")
    if (ref_type != H5R_OBJECT && ref_type != H5R_DATASET_REGION)
        HGOTO_ERROR(H5E_ARGS, H5E_UNSUPPORTED, FAIL, "reference type not supported")
    if (space_id == (-1) && ref_type == H5R_DATASET_REGION)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "reference region dataspace id must be valid")
    if (space_id != (-1) && (NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE))))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace")

    /* Set up collective metadata if appropriate */
    if(H5CX_set_loc(loc_id) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Create reference */
    if((ret_value = H5R__create(ref, &loc, name, ref_type, space)) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTCREATE, FAIL, "unable to create reference")

done:
    FUNC_LEAVE_API(ret_value)
}   /* end H5Rcreate() */


/*--------------------------------------------------------------------------
 NAME
    H5Rdereference2
 PURPOSE
    Opens the HDF5 object referenced.
 USAGE
    hid_t H5Rdereference2(ref)
        hid_t id;       IN: Dataset reference object is in or location ID of
                            object that the dataset is located within.
        hid_t oapl_id;  IN: Property list of the object being referenced.
        H5R_type_t ref_type;    IN: Type of reference to create
        void *ref;      IN: Reference to open.

 RETURNS
    Valid ID on success, H5I_INVALID_HID on failure
 DESCRIPTION
    Given a reference to some object, open that object and return an ID for
    that object.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
    Raymond Lu
    13 July 2011
    I added the OAPL_ID parameter for the object being referenced.  It only
    supports dataset access property list currently.
--------------------------------------------------------------------------*/
hid_t
H5Rdereference2(hid_t obj_id, hid_t oapl_id, H5R_type_t ref_type, const void *_ref)
{
    H5G_loc_t loc;      /* Group location */
    H5F_t *file = NULL; /* File object */
    hid_t ret_value = H5I_INVALID_HID;	/* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE4("i", "iiRt*x", obj_id, oapl_id, ref_type, _ref);

    /* Check args */
    if (H5G_loc(obj_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a location")
    if (oapl_id < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a property list")
    if (ref_type <= H5R_BADTYPE || ref_type >= H5R_MAXTYPE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid reference type")
    if (_ref == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid reference pointer")

    /* Verify access property list and set up collective metadata if appropriate */
    if(H5CX_set_apl(&oapl_id, H5P_CLS_DACC, obj_id, FALSE) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Get the file pointer from the entry */
    file = loc.oloc->file;

    /* Create reference */
    if((ret_value = H5R__dereference(file, oapl_id, ref_type, _ref)) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTOPENOBJ, H5I_INVALID_HID, "unable to dereference object")

done:
    FUNC_LEAVE_API(ret_value)
}   /* end H5Rdereference2() */


/*--------------------------------------------------------------------------
 NAME
    H5Rget_region
 PURPOSE
    Retrieves a dataspace with the region pointed to selected.
 USAGE
    hid_t H5Rget_region(id, ref_type, ref)
        hid_t id;       IN: Dataset reference object is in or location ID of
                            object that the dataset is located within.
        H5R_type_t ref_type;    IN: Type of reference to get region of
        void *ref;        IN: Reference to open.

 RETURNS
    Valid ID on success, H5I_INVALID_HID on failure
 DESCRIPTION
    Given a reference to some object, creates a copy of the dataset pointed
    to's dataspace and defines a selection in the copy which is the region
    pointed to.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5Rget_region(hid_t id, H5R_type_t ref_type, const void *ref)
{
    H5G_loc_t loc;          /* Object's group location */
    H5S_t *space = NULL;    /* Dataspace object */
    hid_t ret_value = H5I_INVALID_HID;	/* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE3("i", "iRt*x", id, ref_type, ref);

    /* Check args */
    if (H5G_loc(id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a location")
    if (ref_type != H5R_DATASET_REGION)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid reference type")
    if (ref == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "invalid reference pointer")

    /* Get the dataspace with the correct region selected */
    if(NULL == (space = H5R__get_region(loc.oloc->file, ref)))
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, H5I_INVALID_HID, "unable to retrieve dataspace")

    /* Atomize */
    if ((ret_value = H5I_register(H5I_DATASPACE, space, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register dataspace atom")

done:
    FUNC_LEAVE_API(ret_value)
}   /* end H5Rget_region() */


/*--------------------------------------------------------------------------
 NAME
    H5Rget_obj_type2
 PURPOSE
    Retrieves the type of object that an object reference points to
 USAGE
    herr_t H5Rget_obj_type2(id, ref_type, ref, obj_type)
        hid_t id;       IN: Dataset reference object is in or location ID of
                            object that the dataset is located within.
        H5R_type_t ref_type;    IN: Type of reference to query
        void *ref;          IN: Reference to query.
        H5O_type_t *obj_type;   OUT: Type of object reference points to

 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Given a reference to some object, this function retrieves the type of
    object pointed to.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Rget_obj_type2(hid_t id, H5R_type_t ref_type, const void *ref,
    H5O_type_t *obj_type)
{
    H5G_loc_t loc;              /* Object location */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "iRt*x*Ot", id, ref_type, ref, obj_type);

    /* Check args */
    if (H5G_loc(id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if (ref_type <= H5R_BADTYPE || ref_type >= H5R_MAXTYPE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference type")
    if (ref == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference pointer")

    /* Get the object information */
    if(H5R__get_obj_type(loc.oloc->file, ref_type, ref, obj_type) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "unable to determine object type")

done:
    FUNC_LEAVE_API(ret_value)
}   /* end H5Rget_obj_type2() */


/*--------------------------------------------------------------------------
 NAME
    H5Rget_name
 PURPOSE
    Determines a name for the object referenced
 USAGE
    ssize_t H5Rget_name(loc_id, ref_type, ref, name, size)
        hid_t loc_id;   IN: Dataset reference object is in or location ID of
                            object that the dataset is located within.
        H5R_type_t ref_type;    IN: Type of reference
        void *ref;      IN: Reference to query.
        char *name;     OUT: Buffer to place name of object referenced. If NULL
	                     then this call will return the size in bytes of name.
        size_t size;    IN: Size of name buffer (user needs to include NULL terminator
                            when passing in the size)

 RETURNS
    Non-negative length of the path on success, -1 on failure
 DESCRIPTION
    Given a reference to some object, determine a path to the object
    referenced in the file.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This may not be the only path to that object.
 EXAMPLES
 REVISION LOG
    M. Scot Breitenfeld
    22 January 2014
    Changed the behavior for the returned value of the function when name is NULL.
    If name is NULL then size is ignored and the function returns the size 
    of the name buffer (not including the NULL terminator), it still returns
    negative on failure.
--------------------------------------------------------------------------*/
ssize_t
H5Rget_name(hid_t id, H5R_type_t ref_type, const void *_ref, char *name,
    size_t size)
{
    H5G_loc_t loc;      /* Group location */
    H5F_t *file;        /* File object */
    ssize_t ret_value;  /* Return value */

    FUNC_ENTER_API((-1))
    H5TRACE5("Zs", "iRt*x*sz", id, ref_type, _ref, name, size);

    /* Check args */
    if (H5G_loc(id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, (-1), "not a location")
    if (ref_type <= H5R_BADTYPE || ref_type >= H5R_MAXTYPE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "invalid reference type")
    if (_ref == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, (-1), "invalid reference pointer")

    /* Get the file pointer from the entry */
    file = loc.oloc->file;

    /* Get name */
    if((ret_value = H5R__get_name(file, id, ref_type, _ref, name, size)) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, (-1), "unable to determine object path")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Rget_name() */

