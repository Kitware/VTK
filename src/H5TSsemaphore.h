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
 *        semaphore's capabilities.  The implementation is based on the
 *	  "lightweight semaphore" describend here:
 *https://preshing.com/20150316/semaphores-are-surprisingly-versatile/ and implemented here:
 *https://github.com/preshing/cpp11-on-multicore/blob/master/common/sema.h
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

#if defined(_WIN32)
/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_signal
 *
 * Purpose:  Increments (unlocks) the semaphore.  If the semaphore's value
 *           becomes greater than zero, then another thread blocked in a wait
 *           call will be woken up and proceed to lock the semaphore.
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_semaphore_signal(H5TS_semaphore_t *sem)
{
    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    if (H5_UNLIKELY(0 == ReleaseSemaphore(*sem, 1, NULL)))
        return FAIL;

    return SUCCEED;
} /* end H5TS_semaphore_signal() */

/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_wait
 *
 * Purpose:  Decrements (locks) the semaphore.  If the semaphore's value is
 *           greater than zero, then the decrement proceeds, and the function
 *           returns immediately.  If the semaphore currently has a value of
 *           zero or less, then the call blocks until it becomes possible to
 *           perform the decrement (i.e. the semaphore value rises above zero).
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_semaphore_wait(H5TS_semaphore_t *sem)
{
    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    if (H5_UNLIKELY(WAIT_OBJECT_0 != WaitForSingleObject(*sem, INFINITE)))
        return FAIL;

    return SUCCEED;
} /* end H5TS_semaphore_wait() */

#elif defined(__unix__) && !defined(__MACH__)
/*
 * POSIX semaphores
 */

/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_signal
 *
 * Purpose:  Increments (unlocks) the semaphore.  If the semaphore's value
 *           becomes greater than zero, then another thread blocked in a wait
 *           call will be woken up and proceed to lock the semaphore.
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_semaphore_signal(H5TS_semaphore_t *sem)
{
    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    if (H5_UNLIKELY(0 != sem_post(sem)))
        return FAIL;

    return SUCCEED;
} /* end H5TS_semaphore_signal() */

/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_wait
 *
 * Purpose:  Decrements (locks) the semaphore.  If the semaphore's value is
 *           greater than zero, then the decrement proceeds, and the function
 *           returns immediately.  If the semaphore currently has a value of
 *           zero or less, then the call blocks until it becomes possible to
 *           perform the decrement (i.e. the semaphore value rises above zero).
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_semaphore_wait(H5TS_semaphore_t *sem)
{
    int rc;

    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    /* Loop because of:
     *  http://stackoverflow.com/questions/2013181/gdb-causes-sem-wait-to-fail-with-eintr-error
     */
    do {
        rc = sem_wait(sem);
    } while (rc == -1 && errno == EINTR);

    if (H5_UNLIKELY(0 != rc))
        return FAIL;

    return SUCCEED;
} /* end H5TS_semaphore_wait() */
#else
/*
 * Emulate semaphore w/mutex & condition variable
 */

/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_signal
 *
 * Purpose:  Increments (unlocks) the semaphore.  If the semaphore's value
 *           becomes greater than zero, then another thread blocked in a wait
 *           call will proceed to lock the semaphore.
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_semaphore_signal(H5TS_semaphore_t *sem)
{
    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    /* Acquire the mutex for the semaphore */
    if (H5_UNLIKELY(H5TS_mutex_lock(&sem->mutex) < 0))
        return FAIL;

    /* Wake a thread up, if any are waiting */
    if (sem->waiters)
        if (H5_UNLIKELY(H5TS_cond_signal(&sem->cond) < 0)) {
            H5TS_mutex_unlock(&sem->mutex);
            return FAIL;
        }

    /* Increment the semaphore's value */
    sem->counter++;

    /* Release the mutex for the semaphore */
    if (H5_UNLIKELY(H5TS_mutex_unlock(&sem->mutex) < 0))
        return FAIL;

    return SUCCEED;
} /* end H5TS_semaphore_signal() */

/*-------------------------------------------------------------------------
 * Function: H5TS_semaphore_wait
 *
 * Purpose:  Decrements (locks) the semaphore.  If the semaphore's value is
 *           greater than zero, then the decrement proceeds, and the function
 *           returns immediately.  If the semaphore currently has a value of
 *           zero or less, then the call blocks until it becomes possible to
 *           perform the decrement (i.e. the semaphore value rises above zero).
 *
 * Return:   Non-negative on success / Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static inline herr_t
H5TS_semaphore_wait(H5TS_semaphore_t *sem)
{
    /* Check argument */
    if (H5_UNLIKELY(NULL == sem))
        return FAIL;

    /* Acquire the mutex for the semaphore */
    if (H5_UNLIKELY(H5TS_mutex_lock(&sem->mutex) < 0))
        return FAIL;

    /* Wait for semaphore value > 0 */
    while (0 == sem->counter) {
        herr_t ret;

        /* Wait for signal that semaphore count has been incremented */
        sem->waiters++;
        ret = H5TS_cond_wait(&sem->cond, &sem->mutex);
        sem->waiters--;

        /* Check for error */
        if (H5_UNLIKELY(ret < 0)) {
            H5TS_mutex_unlock(&sem->mutex);
            return FAIL;
        }
    }

    /* Decrement the semaphore's value */
    sem->counter--;

    /* Release the mutex for the semaphore */
    if (H5_UNLIKELY(H5TS_mutex_unlock(&sem->mutex) < 0))
        return FAIL;

    return SUCCEED;
} /* end H5TS_semaphore_wait() */
#endif
