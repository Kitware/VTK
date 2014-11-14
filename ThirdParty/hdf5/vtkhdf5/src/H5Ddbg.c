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

/****************/
/* Module Setup */
/****************/

#define H5D_PACKAGE		/*suppress error about including H5Dpkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5D__init_dbg_interface


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Datasets 				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/


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
   H5D__init_dbg_interface -- Initialize interface-specific information
USAGE
    herr_t H5D__init_dbg_interface()
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5D_init() currently).

--------------------------------------------------------------------------*/
static herr_t
H5D__init_dbg_interface(void)
{
    FUNC_ENTER_STATIC_NOERR

    FUNC_LEAVE_NOAPI(H5D_init())
} /* H5D__init_dbg_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5Ddebug
 *
 * Purpose:	Prints various information about a dataset.  This function is
 *		not to be documented in the API at this time.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, April 28, 1999
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Ddebug(hid_t dset_id)
{
    H5D_t	*dset;                  /* Dataset to debug */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("e", "i", dset_id);

    /* Check args */
    if(NULL == (dset = (H5D_t *)H5I_object_verify(dset_id, H5I_DATASET)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    /* Print B-tree information */
    if(H5D_CHUNKED == dset->shared->layout.type)
	(void)H5D__chunk_dump_index(dset, H5AC_dxpl_id, stdout);
    else if(H5D_CONTIGUOUS == dset->shared->layout.type)
	HDfprintf(stdout, "    %-10s %a\n", "Address:", dset->shared->layout.storage.u.contig.addr);

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Ddebug() */

