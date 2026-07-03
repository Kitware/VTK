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
 * Purpose: This file contains support for mutex locks, equivalent to the
 *        pthread 'pthread_mutex_t' type and capabilities.
 *
 * Note:  Because this threadsafety framework operates outside the library,
 *        it does not use the error stack (although it does use error macros
 *        that don't push errors on a stack) and only uses the "namecheck only"
 *        FUNC_ENTER_* / FUNC_LEAVE_* macros.
 */

/****************/
/* Module Setup */
/****************/

/***********/
/* Headers */
/***********/

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

#ifdef H5_HAVE_C11_THREADS

#define H5TS_mutex_lock(mutex)   (H5_UNLIKELY(mtx_lock(mutex) != thrd_success) ? FAIL : SUCCEED)
#define H5TS_mutex_unlock(mutex) (H5_UNLIKELY(mtx_unlock(mutex) != thrd_success) ? FAIL : SUCCEED)

#else
#ifdef H5_HAVE_WIN_THREADS
/*-------------------------------------------------------------------------
 * Function: H5TS_mutex_lock
 *
 * Purpose:  Lock a H5TS_mutex_t
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_mutex_lock(H5TS_mutex_t *mutex)
{
    EnterCriticalSection(mutex);

    return SUCCEED;
} /* end H5TS_mutex_lock() */

/*-------------------------------------------------------------------------
 * Function: H5TS_mutex_unlock
 *
 * Purpose:  Unlock a H5TS_mutex_t
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_mutex_unlock(H5TS_mutex_t *mutex)
{
    LeaveCriticalSection(mutex);

    return SUCCEED;
} /* end H5TS_mutex_unlock() */
#else

#define H5TS_mutex_lock(mutex)   (H5_UNLIKELY(0 != pthread_mutex_lock(mutex)) ? FAIL : SUCCEED)
#define H5TS_mutex_unlock(mutex) (H5_UNLIKELY(0 != pthread_mutex_unlock(mutex)) ? FAIL : SUCCEED)

#endif
#endif
