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
 * Purpose: This file contains support for condition variables, equivalent to
 *        the pthread 'pthread_cond_t' type and capabilities.
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

#define H5TS_cond_wait(cond, mutex) (H5_UNLIKELY(cnd_wait((cond), (mutex)) != thrd_success) ? FAIL : SUCCEED)
#define H5TS_cond_signal(cond)      (H5_UNLIKELY(cnd_signal(cond) != thrd_success) ? FAIL : SUCCEED)
#define H5TS_cond_broadcast(cond)   (H5_UNLIKELY(cnd_broadcast(cond) != thrd_success) ? FAIL : SUCCEED)

#else
#ifdef H5_HAVE_WIN_THREADS

#define H5TS_cond_wait(cond, mutex)                                                                          \
    (H5_UNLIKELY(!SleepConditionVariableCS(cond, mutex, INFINITE)) ? FAIL : SUCCEED)

/*-------------------------------------------------------------------------
 * Function: H5TS_cond_signal
 *
 * Purpose:  Unblock a thread waiting for a condition variable
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_cond_signal(H5TS_cond_t *cond)
{
    WakeConditionVariable(cond);

    return SUCCEED;
} /* end H5TS_cond_signal() */

/*-------------------------------------------------------------------------
 * Function: H5TS_cond_broadcast
 *
 * Purpose:  Unblock all threads waiting for a condition variable
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_cond_broadcast(H5TS_cond_t *cond)
{
    WakeAllConditionVariable(cond);

    return SUCCEED;
} /* end H5TS_cond_broadcast() */
#else

#define H5TS_cond_wait(cond, mutex) (H5_UNLIKELY(pthread_cond_wait((cond), (mutex))) ? FAIL : SUCCEED)
#define H5TS_cond_signal(cond)      (H5_UNLIKELY(pthread_cond_signal(cond)) ? FAIL : SUCCEED)
#define H5TS_cond_broadcast(cond)   (H5_UNLIKELY(pthread_cond_broadcast(cond)) ? FAIL : SUCCEED)

#endif
#endif
