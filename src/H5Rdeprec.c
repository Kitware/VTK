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
 * Created:	H5Rdeprec.c
 *		September 13 2007
 *		Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:	Deprecated functions from the H5R interface.  These
 *              functions are here for compatibility purposes and may be
 *              removed in the future.  Applications should switch to the
 *              newer APIs.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Rmodule.h"          /* This source code file is part of the H5R module */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"	/* Metadata cache			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Gprivate.h"		/* Groups				*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Rpkg.h"		/* References				*/
#include "H5Ppublic.h"          /* for using H5P_DATASET_ACCESS_DEFAULT */


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
    H5Rget_obj_type1
 PURPOSE
    Retrieves the type of object that an object reference points to
 USAGE
    H5G_obj_t H5Rget_obj_type1(id, ref_type, ref)
        hid_t id;       IN: Dataset reference object is in or location ID of
                            object that the dataset is located within.
        H5R_type_t ref_type;    IN: Type of reference to query
        void *ref;          IN: Reference to query.

 RETURNS
    Success:	An object type defined in H5Gpublic.h
    Failure:	H5G_UNKNOWN
 DESCRIPTION
    Given a reference to some object, this function returns the type of object
    pointed to.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5G_obj_t
H5Rget_obj_type1(hid_t id, H5R_type_t ref_type, const void *ref)
{
    H5G_loc_t loc;              /* Object location */
    H5O_type_t obj_type;        /* Object type */
    H5G_obj_t ret_value;        /* Return value */

    FUNC_ENTER_API(H5G_UNKNOWN)
    H5TRACE3("Go", "iRt*x", id, ref_type, ref);

    /* Check args */
    if(H5G_loc(id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5G_UNKNOWN, "not a location")
    if(ref_type <= H5R_BADTYPE || ref_type >= H5R_MAXTYPE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5G_UNKNOWN, "invalid reference type")
    if(ref == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5G_UNKNOWN, "invalid reference pointer")

    /* Get the object information */
    if(H5R_get_obj_type(loc.oloc->file, H5AC_ind_read_dxpl_id, ref_type, ref, &obj_type) < 0)
	HGOTO_ERROR(H5E_REFERENCE, H5E_CANTINIT, H5G_UNKNOWN, "unable to determine object type")

    /* Set return value */
    ret_value = H5G_map_obj_type(obj_type);

done:
    FUNC_LEAVE_API(ret_value)
}   /* end H5Rget_obj_type1() */


/*--------------------------------------------------------------------------
 NAME
    H5Rdereference1
 PURPOSE
    Opens the HDF5 object referenced.
 USAGE
    hid_t H5Rdereference1(ref)
        hid_t id;       IN: Dataset reference object is in or location ID of
                            object that the dataset is located within.
        H5R_type_t ref_type;    IN: Type of reference to create
        void *ref;      IN: Reference to open.

 RETURNS
    Valid ID on success, Negative on failure
 DESCRIPTION
    Given a reference to some object, open that object and return an ID for
    that object.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5Rdereference1(hid_t obj_id, H5R_type_t ref_type, const void *_ref)
{
    H5G_loc_t loc;      /* Group location */
    H5F_t *file = NULL; /* File object */
    hid_t ret_value;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("i", "iRt*x", obj_id, ref_type, _ref);

    /* Check args */
    if(H5G_loc(obj_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(ref_type <= H5R_BADTYPE || ref_type >= H5R_MAXTYPE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference type")
    if(_ref == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference pointer")

    /* Get the file pointer from the entry */
    file = loc.oloc->file;

    /* Create reference */
    if((ret_value = H5R_dereference(file, H5P_DATASET_ACCESS_DEFAULT, H5AC_ind_read_dxpl_id, ref_type, _ref, TRUE)) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTINIT, FAIL, "unable dereference object")

done:
    FUNC_LEAVE_API(ret_value)
}   /* end H5Rdereference1() */

#endif /* H5_NO_DEPRECATED_SYMBOLS */

