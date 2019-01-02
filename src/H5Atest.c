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
 * Created:		H5Atest.c
 *			Dec 18 2006
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Attribute testing routines.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Amodule.h"          /* This source code file is part of the H5A module */
#define H5A_TESTING		/*suppress warning about H5A testing funcs*/


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Apkg.h"		/* Attributes	  			*/
#include "H5ACprivate.h"	/* Metadata cache			*/
#include "H5CXprivate.h"        /* API Contexts                         */
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5SMprivate.h"        /* Shared object header messages        */


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


/*-------------------------------------------------------------------------
 * Function:	H5A__is_shared_test
 *
 * Purpose:     Check if an attribute is shared
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Dec 19, 2006
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5A__is_shared_test(hid_t attr_id)
{
    H5A_t	*attr;                  /* Attribute object for ID */
    htri_t	ret_value = FAIL;       /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    if(NULL == (attr = (H5A_t *)H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute")

    /* Check if attribute is shared */
    ret_value = H5O_msg_is_shared(H5O_ATTR_ID, attr);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A__is_shared_test() */


/*-------------------------------------------------------------------------
 * Function:	H5A__get_shared_rc_test
 *
 * Purpose:     Retrieve the refcount for a shared attribute
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Dec 19, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5A__get_shared_rc_test(hid_t attr_id, hsize_t *ref_count)
{
    H5A_t	*attr;                  /* Attribute object for ID */
    hbool_t     api_ctx_pushed = FALSE;             /* Whether API context pushed */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check arguments */
    if(NULL == (attr = (H5A_t *)H5I_object_verify(attr_id, H5I_ATTR)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an attribute")

    /* Push API context */
    if(H5CX_push() < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTSET, FAIL, "can't set API context")
    api_ctx_pushed = TRUE;

    /* Sanity check */
    HDassert(H5O_msg_is_shared(H5O_ATTR_ID, attr));

    /* Retrieve ref count for shared or shareable attribute */
    if(H5SM_get_refcount(attr->oloc.file, H5O_ATTR_ID, &attr->sh_loc, ref_count) < 0)
        HGOTO_ERROR(H5E_ATTR, H5E_CANTGET, FAIL, "can't retrieve shared message ref count")

done:
    if(api_ctx_pushed && H5CX_pop() < 0)
        HDONE_ERROR(H5E_ATTR, H5E_CANTRESET, FAIL, "can't reset API context")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5A__get_shared_rc_test() */

