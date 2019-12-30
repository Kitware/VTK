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

#include "H5Fmodule.h"          /* This source code file is part of the H5F module  */
#define H5F_TESTING             /* Suppress warning about H5F testing funcs         */
#define H5G_FRIEND              /* Suppress error about including H5Gpkg.h          */
#define H5G_TESTING             /* Suppress warning about H5G testing funcs         */
#define H5SM_FRIEND             /* Suppress error about including H5SMpkg.h         */
#define H5SM_TESTING            /* Suppress warning about H5SM testing funcs        */


/***********/
/* Headers */
/***********/
#include "H5private.h"          /* Generic Functions                        */
#include "H5CXprivate.h"        /* API Contexts                             */
#include "H5Eprivate.h"         /* Error handling                           */
#include "H5Fpkg.h"             /* File access                              */
#include "H5Gpkg.h"             /* Groups                                   */
#include "H5Iprivate.h"         /* IDs                                      */
#include "H5SMpkg.h"            /* Shared object header messages            */


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
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Jan  3, 2007
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_get_sohm_mesg_count_test(hid_t file_id, unsigned type_id, size_t *mesg_count)
{
    H5F_t      *file;                       /* File info */
    hbool_t     api_ctx_pushed = FALSE;     /* Whether API context pushed */
    herr_t      ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Push API context */
    if(H5CX_push() < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set API context")
    api_ctx_pushed = TRUE;

    /* Retrieve count for message type */
    if(H5SM__get_mesg_count_test(file, type_id, mesg_count) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't retrieve shared message count")

done:
    if(api_ctx_pushed && H5CX_pop() < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTRESET, FAIL, "can't reset API context")

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
 * Return:      SUCCEED/FAIL
 *
 * Programmer:	Neil Fortner
 *	        Mar  31, 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_check_cached_stab_test(hid_t file_id)
{
    H5F_t      *file;                       /* File info */
    hbool_t     api_ctx_pushed = FALSE;     /* Whether API context pushed */
    herr_t      ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Push API context */
    if(H5CX_push() < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTSET, FAIL, "can't set API context")
    api_ctx_pushed = TRUE;

    /* Verify the cached stab info */
    if(H5G__verify_cached_stab_test(H5G_oloc(file->shared->root_grp), file->shared->sblock->root_ent) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to verify cached symbol table info")

done:
    if(api_ctx_pushed && H5CX_pop() < 0)
        HDONE_ERROR(H5E_FILE, H5E_CANTRESET, FAIL, "can't reset API context")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_check_cached_stab_test() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_maxaddr_test
 *
 * Purpose:     Retrieve the maximum address for a file
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *	        Jun 10, 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_get_maxaddr_test(hid_t file_id, haddr_t *maxaddr)
{
    H5F_t      *file;                       /* File info */
    herr_t      ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Retrieve maxaddr for file */
    *maxaddr = file->shared->maxaddr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_get_maxaddr_test() */


/*-------------------------------------------------------------------------
 * Function:	H5F_get_sbe_addr_test
 *
 * Purpose:     Retrieve the address of a superblock extension's object header
 *              for a file
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:	Quincey Koziol
 *	        Jul 10, 2016
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_get_sbe_addr_test(hid_t file_id, haddr_t *sbe_addr)
{
    H5F_t      *file;                       /* File info */
    herr_t      ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments */
    if(NULL == (file = (H5F_t *)H5I_object_verify(file_id, H5I_FILE)))
	HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file")

    /* Retrieve maxaddr for file */
    *sbe_addr = file->shared->sblock->ext_addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_get_sbe_addr_test() */

