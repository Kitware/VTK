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
 * Created:     H5FSint.c
 *              Fall 2012
 *              Dana Robinson <derobins@hdfgroup.org>
 *
 * Purpose:     Internal routines for free space managers.
 *
 *-------------------------------------------------------------------------
 */

/**********************/
/* Module Declaration */
/**********************/

#include "H5FSmodule.h"         /* This source code file is part of the H5FS module */


/***********************/
/* Other Packages Used */
/***********************/


/***********/
/* Headers */
/***********/
#include "H5private.h"              /* Generic Functions                */
#include "H5Eprivate.h"             /* Error Handling                   */
#include "H5FSpkg.h"                /* Free Space Managers              */


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
 * Function:    H5FS_init
 *
 * Purpose:     Initialize the interface in case it is unable to initialize
 *              itself soon enough.
 *
 * Return:      Success:    non-negative
 *              Failure:    negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 4, 2000
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS_init(void)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOERR
    /* FUNC_ENTER() does all the work */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FS_init() */


/*-------------------------------------------------------------------------
 * Function:    H5FS__create_flush_depend
 *
 * Purpose:     Create a flush dependency between two data structure components
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Dana Robinson
 *              Fall 2012
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS__create_flush_depend(H5AC_info_t *parent_entry, H5AC_info_t *child_entry)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT
    
    /* Sanity check */
    HDassert(parent_entry);
    HDassert(child_entry);

    /* Create a flush dependency between parent and child entry */
    if(H5AC_create_flush_dependency(parent_entry, child_entry) < 0)
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTDEPEND, FAIL, "unable to create flush dependency")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5FS__create_flush_depend() */


/*-------------------------------------------------------------------------
 * Function:    H5FS__destroy_flush_depend
 *
 * Purpose:     Destroy a flush dependency between two data structure components
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Dana Robinson
 *              Fall 2012
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FS__destroy_flush_depend(H5AC_info_t *parent_entry, H5AC_info_t *child_entry)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT
    
    /* Sanity check */
    HDassert(parent_entry);
    HDassert(child_entry);

    /* Destroy a flush dependency between parent and child entry */
    if(H5AC_destroy_flush_dependency(parent_entry, child_entry) < 0)
        HGOTO_ERROR(H5E_FSPACE, H5E_CANTUNDEPEND, FAIL, "unable to destroy flush dependency")
        
done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* end H5FS__destroy_flush_depend() */

