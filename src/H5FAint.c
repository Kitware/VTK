/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:     H5FAint.c
 *              Fall 2012
 *              Dana Robinson
 *
 * Purpose:     Internal routines for fixed arrays.
 *
 *-------------------------------------------------------------------------
 */

/**********************/
/* Module Declaration */
/**********************/

#include "H5FAmodule.h" /* This source code file is part of the H5FA module */

/***********************/
/* Other Packages Used */
/***********************/

/***********/
/* Headers */
/***********/
#include "H5private.h"  /* Generic Functions                */
#include "H5Eprivate.h" /* Error Handling                   */
#include "H5FApkg.h"    /* Fixed Arrays                     */

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
 * Function:    H5FA__create_flush_depend
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
BEGIN_FUNC(PKG, ERR, herr_t, SUCCEED, FAIL,
           H5FA__create_flush_depend(H5AC_info_t *parent_entry, H5AC_info_t *child_entry))

    /* Sanity check */
    HDassert(parent_entry);
    HDassert(child_entry);

    /* Create a flush dependency between parent and child entry */
    if (H5AC_create_flush_dependency(parent_entry, child_entry) < 0)
        H5E_THROW(H5E_CANTDEPEND, "unable to create flush dependency")

    CATCH

END_FUNC(PKG) /* end H5FA__create_flush_depend() */

/*-------------------------------------------------------------------------
 * Function:    H5FA__destroy_flush_depend
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
BEGIN_FUNC(PKG, ERR, herr_t, SUCCEED, FAIL,
           H5FA__destroy_flush_depend(H5AC_info_t *parent_entry, H5AC_info_t *child_entry))

    /* Sanity check */
    HDassert(parent_entry);
    HDassert(child_entry);

    /* Destroy a flush dependency between parent and child entry */
    if (H5AC_destroy_flush_dependency(parent_entry, child_entry) < 0)
        H5E_THROW(H5E_CANTUNDEPEND, "unable to destroy flush dependency")

    CATCH

END_FUNC(PKG) /* end H5FA__destroy_flush_depend() */
