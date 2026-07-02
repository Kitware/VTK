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
 * Purpose: This file contains support for semaphores, equivalent to POSIX
 *        semaphore's capabilities.
 *
 *        Emulated semaphores are based off the Netscape Portable Runtime
 *        implementation:
 * https://hg.mozilla.org/projects/nspr/file/3e25d69ba6b268f2817e920a69eb2c091efe17e6/pr/src/threads/prsem.c
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

#if defined(_WIN32)
/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_init
 *
 * Purpose:  Initialize a H5TS_semaphore_t (does not allocate it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_semaphore_init(H5TS_semaphore_t *sem, unsigned initial_count)
{
    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    if (H5_UNLIKELY(NULL == (*sem = CreateSemaphore(NULL, (LONG)initial_count, LONG_MAX, NULL))))
        return FAIL;

    return SUCCEED;
} /* end H5TS_semaphore_init() */

/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_destroy
 *
 * Purpose:  Destroy a H5TS_semaphore_t (does not free it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_semaphore_destroy(H5TS_semaphore_t *sem)
{
    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    if (H5_UNLIKELY(0 == CloseHandle(*sem)))
        return FAIL;

    return SUCCEED;
} /* end H5TS_semaphore_destroy() */

#elif defined(__unix__) && !defined(__MACH__)
/*
 * POSIX semaphores
 */

/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_init
 *
 * Purpose:  Initialize a H5TS_semaphore_t (does not allocate it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_semaphore_init(H5TS_semaphore_t *sem, unsigned initial_count)
{
    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    if (H5_UNLIKELY(0 != sem_init(sem, 0, initial_count)))
        return FAIL;

    return SUCCEED;
} /* end H5TS_semaphore_init() */

/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_destroy
 *
 * Purpose:  Destroy a H5TS_semaphore_t (does not free it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_semaphore_destroy(H5TS_semaphore_t *sem)
{
    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    if (H5_UNLIKELY(0 != sem_destroy(sem)))
        return FAIL;

    return SUCCEED;
} /* end H5TS_semaphore_destroy() */
#else
/*
 * Emulate semaphore w/mutex & condition variable
 */

/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_init
 *
 * Purpose:  Initialize a H5TS_semaphore_t (does not allocate it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_semaphore_init(H5TS_semaphore_t *sem, unsigned initial_count)
{
    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    if (H5_UNLIKELY(H5TS_mutex_init(&sem->mutex, H5TS_MUTEX_TYPE_PLAIN) < 0))
        return FAIL;
    if (H5_UNLIKELY(H5TS_cond_init(&sem->cond) < 0)) {
        H5TS_mutex_destroy(&sem->mutex);
        return FAIL;
    }
    sem->waiters = 0;
    sem->counter = (int)initial_count;

    return SUCCEED;
} /* end H5TS_semaphore_init() */

/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_destroy
 *
 * Purpose:  Destroy a H5TS_semaphore_t (does not free it)
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5TS_semaphore_destroy(H5TS_semaphore_t *sem)
{
    herr_t ret_value = SUCCEED;

    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    if (H5_UNLIKELY(H5TS_mutex_destroy(&sem->mutex) < 0))
        ret_value = FAIL;
    if (H5_UNLIKELY(H5TS_cond_destroy(&sem->cond) < 0))
        return FAIL;

    return ret_value;
} /* end H5TS_semaphore_destroy() */
#endif

#endif /* H5_HAVE_THREADS */
