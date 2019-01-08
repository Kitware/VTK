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
 * Created:	H5Tdeprec.c
 *		April 5 2007
 *		Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:	Deprecated functions from the H5T interface.  These
 *              functions are here for compatibility purposes and may be
 *              removed in the future.  Applications should switch to the
 *              newer APIs.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Tmodule.h"          /* This source code file is part of the H5T module */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5CXprivate.h"        /* API Contexts                         */
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5FOprivate.h"	/* File objects				*/
#include "H5Iprivate.h"		/* IDs					*/
#include "H5Ppublic.h"		/* Property Lists			*/
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

/*-------------------------------------------------------------------------
 * Function:	H5Tcommit1
 *
 * Purpose:	Save a transient datatype to a file and turn the type handle
 *		into a named, immutable type.
 *
 * Note:	Deprecated in favor of H5Tcommit2
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, June  1, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tcommit1(hid_t loc_id, const char *name, hid_t type_id)
{
    H5G_loc_t	loc;                    /* Location to create datatype */
    H5T_t	*type;                  /* Datatype for ID */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*si", loc_id, name, type_id);

    /* Check arguments */
    if(H5G_loc(loc_id, &loc) < 0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location")
    if(!name || !*name)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name")
    if(NULL == (type = (H5T_t *)H5I_object_verify(type_id, H5I_DATATYPE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype")

    /* Set up collective metadata if appropriate */
    if(H5CX_set_loc(loc_id) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTSET, FAIL, "can't set access property list info")

    /* Commit the datatype to the file, using default property list values */
    if(H5T__commit_named(&loc, name, type, H5P_LINK_CREATE_DEFAULT, H5P_DATATYPE_CREATE_DEFAULT) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to commit datatype")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Tcommit1() */


/*-------------------------------------------------------------------------
 * Function:	H5Topen1
 *
 * Purpose:	Opens a named datatype.
 *
 * Note:	Deprecated in favor of H5Topen2
 *
 * Return:	Success:	Object ID of the named datatype.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Monday, June  1, 1998
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Topen1(hid_t loc_id, const char *name)
{
    H5T_t       *type = NULL;
    H5G_loc_t	loc;
    hid_t       ret_value = H5I_INVALID_HID;

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE2("i", "i*s", loc_id, name);

    /* Check args */
    if(H5G_loc(loc_id, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not a location")
    if(!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "no name")

    /* Open it */
    if(NULL == (type = H5T__open_name(&loc, name)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTOPENOBJ, H5I_INVALID_HID, "unable to open named datatype")

    /* Register the type and return the ID */
    if((ret_value = H5I_register(H5I_DATATYPE, type, TRUE)) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register named datatype")

done:
    /* Cleanup on error */
    if(ret_value < 0)
        if(type && H5T_close(type) < 0)
            HDONE_ERROR(H5E_DATATYPE, H5E_CANTCLOSEOBJ, H5I_INVALID_HID, "can't close datatype")

    FUNC_LEAVE_API(ret_value)
} /* end H5Topen1() */
#endif /* H5_NO_DEPRECATED_SYMBOLS */

