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

/* Programmer:  Quincey Koziol <koziol@hdfgoup.org>
 *              Tuesday, July 27, 2010
 *
 * Purpose:	ID testing functions.
 */

/****************/
/* Module Setup */
/****************/

#include "H5Imodule.h"          /* This source code file is part of the H5I module */
#define H5I_TESTING		/*suppress warning about H5I testing funcs*/


/***********/
/* Headers */
/***********/
#include "H5private.h"          /* Generic Functions                        */
#include "H5ACprivate.h"        /* Metadata cache                           */
#include "H5CXprivate.h"        /* API Contexts                             */
#include "H5Eprivate.h"         /* Error handling                           */
#include "H5Gprivate.h"         /* Groups                                   */
#include "H5Ipkg.h"             /* IDs                                      */


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


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5I__get_name_test
 *
 * Purpose:     Testing version of H5Iget_name()
 *
 * Return:      Success: The length of name.
 *              Failure: -1
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, July 27, 2010
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5I__get_name_test(hid_t id, char *name/*out*/, size_t size, hbool_t *cached)
{
    H5G_loc_t   loc;                    /* Object location */
    hbool_t     api_ctx_pushed = FALSE; /* Whether API context pushed */
    ssize_t     ret_value = -1;         /* Return value */

    FUNC_ENTER_PACKAGE

    /* Get object location */
    if(H5G_loc(id, &loc) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, (-1), "can't retrieve object location")

    /* Set API context */
    if(H5CX_push() < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTSET, (-1), "can't set API context")
    api_ctx_pushed = TRUE;

    /* Call internal group routine to retrieve object's name */
    if((ret_value = H5G_get_name(&loc, name, size, cached)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, (-1), "can't retrieve object name")

done:
    if(api_ctx_pushed && H5CX_pop() < 0)
        HDONE_ERROR(H5E_SYM, H5E_CANTRESET, (-1), "can't reset API context")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I__get_name_test() */

