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
 * Purpose: This file contains support for non-recursive R/W locks, equivalent
 *        to the pthread 'pthread_rwlock_t' type and capabilities.
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

#ifdef H5_HAVE_THREADS

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
/*-------------------------------------------------------------------------
 * Function: H5TS_rwlock_init
 *
 * Purpose:  Initialize a H5TS_rwlock_t (does not allocate it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_rwlock_init(H5TS_rwlock_t *lock)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Check argument */
    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    /* Initialize synchronization primitives */
    if (H5_UNLIKELY(mtx_init(&lock->mutex, mtx_plain) != thrd_success))
        HGOTO_DONE(FAIL);
    if (H5_UNLIKELY(cnd_init(&lock->read_cv) != thrd_success))
        HGOTO_DONE(FAIL);
    if (H5_UNLIKELY(cnd_init(&lock->write_cv) != thrd_success))
        HGOTO_DONE(FAIL);

    /* Initialize scalar fields */
    lock->readers       = 0;
    lock->writers       = 0;
    lock->read_waiters  = 0;
    lock->write_waiters = 0;

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_rwlock_init() */

/*-------------------------------------------------------------------------
 * Function: H5TS_rwlock_destroy
 *
 * Purpose:  Destroy a H5TS_rwlock_t (does not free it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_rwlock_destroy(H5TS_rwlock_t *lock)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Check argument */
    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    /* Destroy synchronization primitives */
    /* NOTE: mtx_destroy() & cnd_destroy() can't fail */
    mtx_destroy(&lock->mutex);
    cnd_destroy(&lock->read_cv);
    cnd_destroy(&lock->write_cv);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_rwlock_destroy() */
#else
#ifdef H5_HAVE_WIN_THREADS
/*-------------------------------------------------------------------------
 * Function: H5TS_rwlock_init
 *
 * Purpose:  Initialize a H5TS_rwlock_t (does not allocate it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_rwlock_init(H5TS_rwlock_t *lock)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Check argument */
    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    InitializeSRWLock(lock);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_rwlock_init() */

/*-------------------------------------------------------------------------
 * Function: H5TS_rwlock_destroy
 *
 * Purpose:  Destroy a H5TS_rwlock_t (does not free it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_rwlock_destroy(H5TS_rwlock_t *lock)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Check argument */
    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    /* Destroy synchronization primitives */
    /* SRWLOCKs don't have to be destroyed */

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_rwlock_destroy() */
#elif defined(__MACH__)
/*-------------------------------------------------------------------------
 * Function: H5TS_rwlock_init
 *
 * Purpose:  Initialize a H5TS_rwlock_t (does not allocate it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_rwlock_init(H5TS_rwlock_t *lock)
{
    pthread_mutexattr_t  _attr;
    pthread_mutexattr_t *attr      = NULL;
    herr_t               ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Check argument */
    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    /* Create mutex attribute */
    if (H5_UNLIKELY(pthread_mutexattr_init(&_attr)))
        HGOTO_DONE(FAIL);
    attr = &_attr;

    /* Use "normal" mutex, without error checking */
    if (H5_UNLIKELY(pthread_mutexattr_settype(attr, PTHREAD_MUTEX_NORMAL)))
        HGOTO_DONE(FAIL);

    /* Initialize the mutex */
    if (H5_UNLIKELY(pthread_mutex_init(&lock->mutex, attr)))
        HGOTO_DONE(FAIL);

    /* Initialize the condition variables */
    if (H5_UNLIKELY(pthread_cond_init(&lock->read_cv, NULL)))
        HGOTO_DONE(FAIL);
    if (H5_UNLIKELY(pthread_cond_init(&lock->write_cv, NULL)))
        HGOTO_DONE(FAIL);

    /* Initialize scalar fields */
    lock->readers       = 0;
    lock->writers       = 0;
    lock->read_waiters  = 0;
    lock->write_waiters = 0;

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_rwlock_init() */

/*-------------------------------------------------------------------------
 * Function: H5TS_rwlock_destroy
 *
 * Purpose:  Destroy a H5TS_rwlock_t (does not free it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_rwlock_destroy(H5TS_rwlock_t *lock)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Check argument */
    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    /* Destroy synchronization primitives */
    if (H5_UNLIKELY(pthread_mutex_destroy(&lock->mutex)))
        HGOTO_DONE(FAIL);
    if (H5_UNLIKELY(pthread_cond_destroy(&lock->read_cv)))
        HGOTO_DONE(FAIL);
    if (H5_UNLIKELY(pthread_cond_destroy(&lock->write_cv)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_rwlock_destroy() */
#else
/*-------------------------------------------------------------------------
 * Function: H5TS_rwlock_init
 *
 * Purpose:  Initialize a H5TS_rwlock_t (does not allocate it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_rwlock_init(H5TS_rwlock_t *lock)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Check argument */
    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    if (H5_UNLIKELY(pthread_rwlock_init(lock, NULL)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_rwlock_init() */

/*-------------------------------------------------------------------------
 * Function: H5TS_rwlock_destroy
 *
 * Purpose:  Destroy a H5TS_rwlock_t (does not free it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_rwlock_destroy(H5TS_rwlock_t *lock)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Check argument */
    if (H5_UNLIKELY(NULL == lock))
        HGOTO_DONE(FAIL);

    if (H5_UNLIKELY(pthread_rwlock_destroy(lock)))
        HGOTO_DONE(FAIL);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_rwlock_destroy() */
#endif
#endif

#endif /* H5_HAVE_THREADS */
