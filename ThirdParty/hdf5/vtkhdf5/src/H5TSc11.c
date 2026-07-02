/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose: C11 threads threadsafety routines
 *
 * Note:  Because this threadsafety framework operates outside the library,
 *        it does not use the error stack (although it does use error macros
 *        that don't push errors on a stack) and only uses the "namecheck only"
 *        FUNC_ENTER_* / FUNC_LEAVE_* macros.
 */

/****************/
/* Module Setup */
/****************/

#include "H5TSmodule.h" /* This source code file is part of the H5TS module */

/***********/
/* Headers */
/***********/
#include "H5private.h"  /* Generic Functions                   */
#include "H5Eprivate.h" /* Error handling                      */
#include "H5TSpkg.h"    /* Threadsafety                        */

#ifdef H5_HAVE_C11_THREADS

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

#ifdef H5_HAVE_THREADSAFE_API
/*--------------------------------------------------------------------------
 * Function:    H5TS__c11_first_thread_init
 *
 * Purpose:     Initialize global API lock, keys for per-thread error stacks
 *              and cancallability information. Called by the first thread
 *              that enters the library.
 *
 * Return:      None
 *
 *--------------------------------------------------------------------------
 */
void
H5TS__c11_first_thread_init(void)
{
    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    /* Initialize H5TS package */
    H5TS__init_package();

    FUNC_LEAVE_NOAPI_VOID_NAMECHECK_ONLY
} /* end H5TS__c11_first_thread_init() */
#endif /* H5_HAVE_THREADSAFE_API */

#endif /* H5_HAVE_C11_THREADS */
