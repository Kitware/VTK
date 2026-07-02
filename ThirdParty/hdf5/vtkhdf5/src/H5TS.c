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
 * Purpose: This file contains the framework for ensuring that the global
 *        library lock is held when an API routine is called.  This framework
 *        works in concert with the FUNC_ENTER_API / FUNC_LEAVE_API macros
 *        defined in H5private.h.
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

#ifdef H5_HAVE_THREADSAFE_API

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

/* API threadsafety info */
H5TS_api_info_t H5TS_api_info_p;

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/*--------------------------------------------------------------------------
 * Function:    H5TSmutex_acquire
 *
 * Purpose:     Attempts to acquire the HDF5 library global lock. Should be preceded by a call to
 *              H5TSmutex_release().
 *
 * Parameters:
 *              lock_count; IN: The lock count that was held on the mutex before its release
 *              acquired; OUT: Whether the HDF5 library global lock was acquired
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TSmutex_acquire(unsigned lock_count, bool *acquired)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API_NAMECHECK_ONLY

    /* Acquire the "API" lock */
    if (H5_UNLIKELY(H5TS__api_mutex_acquire(lock_count, acquired) < 0))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_API_NAMECHECK_ONLY(ret_value)
} /* end H5TSmutex_acquire() */

/*--------------------------------------------------------------------------
 * Function:    H5TSmutex_get_attempt_count
 *
 * Purpose:     Get the current count of the global lock attempt
 *
 * Return:      Non-negative on success / Negative on failure
 *
 * Programmer:  Houjun Tang
 *              June 24, 2019
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TSmutex_get_attempt_count(unsigned *count)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API_NAMECHECK_ONLY

    *count = H5TS_atomic_load_uint(&H5TS_api_info_p.attempt_lock_count);

    FUNC_LEAVE_API_NAMECHECK_ONLY(ret_value)
} /* end H5TSmutex_get_attempt_count() */

/*--------------------------------------------------------------------------
 * Function:    H5TSmutex_release
 *
 * Purpose:     Releases the HDF5 library global lock. Should be followed by a call to H5TSmutex_acquire().
 *
 *              This should be used by applications to temporarily release the lock in order to either perform
 *              multi-threaded work of their own or yield control to another thread using HDF5. The value
 *              returned in lock_count should be provided to H5TSmutex_acquire() in order to resume a
 *              consistent library state.
 *
 * Parameters:
 *              lock_count; OUT: The current lock count for the calling thread.
 *
 * Return:      Non-negative on success / Negative on failure
 *--------------------------------------------------------------------------
 */
herr_t
H5TSmutex_release(unsigned *lock_count)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API_NAMECHECK_ONLY

    /* Release the "API" lock */
    *lock_count = 0;
    if (H5_UNLIKELY(H5TS__api_mutex_release(lock_count) < 0))
        ret_value = FAIL;

    FUNC_LEAVE_API_NAMECHECK_ONLY(ret_value)
} /* end H5TSmutex_release() */
#endif /* H5_HAVE_THREADSAFE_API */
