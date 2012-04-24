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
 * Created:		H5Ftest.c
 *			Jan  3 2007
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		File testing routines.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5F_PACKAGE		/*suppress error about including H5Fpkg  */
#define H5F_TESTING		/*suppress warning about H5F testing funcs*/
#define H5SM_PACKAGE		/*suppress error about including H5SMpkg  */
#define H5SM_TESTING		/*suppress warning about H5SM testing funcs*/
#define H5G_PACKAGE		/*suppress error about including H5Gpkg  */
#define H5G_TESTING		/*suppress warning about H5G testing funcs*/


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"             /* File access				*/
#include "H5Gpkg.h"		/* Groups		  		*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5SMpkg.h"            /* Shared object header messages        */


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
 * Function:	H5F_get_sohm_mesg_count_test
 *
 * Purpose:     Retrieve the number of shared messages of a given type in a file
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Jan  3, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_get_sohm_mesg_count_test(hid_t file_id, unsigned type_id,
    size_t *mesg_count)
{
    H5F_t	*file;                  /* File info */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5F_get_sohm_mesg_count_test)

    /* Check arguments */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Retrieve count for message type */
    if(H5SM_get_mesg_count_test(file, H5AC_ind_dxpl_id, type_id, mesg_count) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't retrieve shared message count")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_get_sohm_mesg_count_test() */


/*-------------------------------------------------------------------------
 * Function:	H5F_check_cached_stab_test
 *
 * Purpose:     Check that a file's superblock contains a cached symbol
 *              table entry, that the entry matches that in the root
 *              group's object header, and check that the addresses are
 *              valid.
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Neil Fortner
 *	        Mar  31, 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_check_cached_stab_test(hid_t file_id)
{
    H5F_t	*file;                  /* File info */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5F_check_cached_stab_test)

    /* Check arguments */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Verify the cached stab info */
    if(H5G_verify_cached_stab_test(H5G_oloc(file->shared->root_grp), file->shared->sblock->root_ent) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to verify cached symbol table info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_check_cached_stab_test() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_maxaddr_test
 *
 * Purpose:     Retrieve the maximum address for a file
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Jun 10, 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_get_maxaddr_test(hid_t file_id, haddr_t *maxaddr)
{
    H5F_t	*file;                  /* File info */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5F_get_maxaddr_test)

    /* Check arguments */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Retrieve maxaddr for file */
    *maxaddr = file->shared->maxaddr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_get_maxaddr_test() */

