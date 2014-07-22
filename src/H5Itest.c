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

/* Programmer:  Quincey Koziol <koziol@hdfgoup.org>
 *              Tuesday, July 27, 2010
 *
 * Purpose:	ID testing functions.
 */

/****************/
/* Module Setup */
/****************/

#define H5I_PACKAGE		/*suppress error about including H5Ipkg	  */
#define H5I_TESTING		/*suppress warning about H5I testing funcs*/


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5ACprivate.h"        /* Metadata cache                       */
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Gprivate.h"		/* Groups				*/
#include "H5Ipkg.h"		/* IDs			  		*/


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
 * Function: H5I_get_name_test
 *
 * Purpose: Testing version of H5Iget_name()
 *
 * Return: Success: The length of name.
 *         Failure: -1
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, July 27, 2010
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5I_get_name_test(hid_t id, char *name/*out*/, size_t size, hbool_t *cached)
{
    H5G_loc_t     loc;          /* Object location */
    ssize_t       ret_value;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Get object location */
    if(H5G_loc(id, &loc) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't retrieve object location")

    /* Call internal group routine to retrieve object's name */
    if((ret_value = H5G_get_name(&loc, name, size, cached, H5P_DEFAULT, H5AC_ind_dxpl_id)) < 0)
	HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't retrieve object name")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5I_get_name_test() */

