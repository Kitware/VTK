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
 * Purpose: This file contains an implementation of a simple thread-pool,
 *        using the H5TS package's portable objects and operations.
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
#include "H5private.h"   /* Generic Functions                   */
#include "H5Eprivate.h"  /* Error handling                      */
#include "H5FLprivate.h" /* Free Lists                          */
#include "H5TSpkg.h"     /* Threadsafety                        */

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
static herr_t                  H5TS__pool_free(H5TS_pool_t *pool);
static H5TS_THREAD_RETURN_TYPE H5TS__pool_do(void *_pool);

/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/* Declare a free list to manage H5TS_pool_t structs */
H5FL_DEFINE_STATIC(H5TS_pool_t);

/* Declare a free list to manage sequences of H5TS_thread_t structs */
H5FL_SEQ_DEFINE_STATIC(H5TS_thread_t);

/*--------------------------------------------------------------------------
 * Function:    H5TS_pool_free
 *
 * Purpose:     Free resources for a thread pool, waiting for tasks to be invoked
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
static herr_t
H5TS__pool_free(H5TS_pool_t *pool)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    /* Sanity check */
    assert(pool);

    /* Join all threads */
    for (unsigned u = 0; u < pool->num_threads; u++)
        if (H5_UNLIKELY(H5TS_thread_join(pool->threads[u], NULL) < 0))
            HGOTO_DONE(FAIL);

    /* Destroy the task queue's semaphore */
    if (H5_UNLIKELY(H5TS_semaphore_destroy(&pool->sem) < 0))
        HGOTO_DONE(FAIL);

    /* Destroy the task queue's mutex */
    if (H5_UNLIKELY(H5TS_mutex_destroy(&pool->queue_mutex) < 0))
        HGOTO_DONE(FAIL);

    /* Release memory */
    if (pool->threads)
        H5FL_SEQ_FREE(H5TS_thread_t, pool->threads);
    H5FL_FREE(H5TS_pool_t, pool);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS__pool_free() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__pool_do
 *
 * Purpose:     Routine for worker threads to service tasks
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
static H5TS_THREAD_RETURN_TYPE
H5TS__pool_do(void *_pool)
{
    H5TS_pool_t      *pool      = (H5TS_pool_t *)_pool; /* Pool for threads */
    H5TS_thread_ret_t ret_value = (H5TS_thread_ret_t)0;

    /* Acquire tasks and invoke them, until pool is shut down */
    while (1) {
        H5TS_pool_task_t *task; /* Task to invoke */

        /* Wait for task */
        if (H5_UNLIKELY(H5TS_semaphore_wait(&pool->sem) < 0))
            return ((H5TS_thread_ret_t)-1);

        /* Acquire the mutex for the task queue */
        if (H5_UNLIKELY(H5TS_mutex_lock(&pool->queue_mutex) < 0))
            return ((H5TS_thread_ret_t)-1);

        /* Check if we have a task */
        if (H5_LIKELY(pool->head)) {
            /* Grab our task */
            task = pool->head;
            if (task->next)
                pool->head = task->next;
            else
                pool->head = pool->tail = NULL;

            /* Release the task queue's mutex */
            if (H5_UNLIKELY(H5TS_mutex_unlock(&pool->queue_mutex) < 0))
                return ((H5TS_thread_ret_t)-1);

            /* Invoke function for task */
            (*task->func)(task->ctx);

            /* Free the task node */
            free(task);
        }
        else {
            assert(pool->shutdown);

            /* Release the task queue's mutex */
            if (H5_UNLIKELY(H5TS_mutex_unlock(&pool->queue_mutex) < 0))
                return ((H5TS_thread_ret_t)-1);

            break;
        }
    }

    return (ret_value);
} /* end H5TS__pool_do() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_pool_create
 *
 * Purpose:     Create a new thread pool, with the given # of threads
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_pool_create(H5TS_pool_t **pool, unsigned num_threads)
{
    H5TS_pool_t *new_pool = NULL; /* Newly created pool */
    unsigned     u;               /* Local index variable */
    herr_t       ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Sanity checks */
    if (H5_UNLIKELY(NULL == pool))
        HGOTO_DONE(FAIL);
    if (H5_UNLIKELY(0 == num_threads))
        HGOTO_DONE(FAIL);

    /* Allocate new pool */
    if (H5_UNLIKELY(NULL == (new_pool = H5FL_MALLOC(H5TS_pool_t))))
        HGOTO_DONE(FAIL);

    /* Initialize pool fields to defaults */
    memset(new_pool, 0, sizeof(*new_pool));
    if (H5_UNLIKELY(H5TS_mutex_init(&new_pool->queue_mutex, H5TS_MUTEX_TYPE_PLAIN) < 0))
        HGOTO_DONE(FAIL);

    /* Create semaphore for task queue */
    if (H5_UNLIKELY(H5TS_semaphore_init(&new_pool->sem, 0) < 0))
        HGOTO_DONE(FAIL);

    /* Allocate array of threads */
    if (H5_UNLIKELY(NULL == (new_pool->threads = H5FL_SEQ_MALLOC(H5TS_thread_t, num_threads))))
        HGOTO_DONE(FAIL);

    /* Start worker threads */
    for (u = 0; u < num_threads; u++) {
        /* Create thread, which immediately starts processing tasks */
        if (H5_UNLIKELY(H5TS_thread_create(&new_pool->threads[u], H5TS__pool_do, (void *)new_pool) < 0)) {
            /* Set # of threads successfully created (for joining them, in free routine) */
            new_pool->num_threads = u;
            HGOTO_DONE(FAIL);
        }
    }

    /* Set # of threads for pool */
    new_pool->num_threads = num_threads;

    /* Set return value */
    *pool = new_pool;

done:
    if (H5_UNLIKELY(ret_value < 0))
        if (new_pool) {
            /* Tell any existing threads that the pool is shutting down */
            new_pool->shutdown = true;

            /* Free pool */
            H5TS__pool_free(new_pool);
        }

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_pool_create() */

/*--------------------------------------------------------------------------
 * Function:    H5TS_pool_destroy
 *
 * Purpose:     Destroy a thread pool, waiting for all tasks to be invoked
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5TS_pool_destroy(H5TS_pool_t *pool)
{
    bool   have_queue_mutex = false; /* Whether we're holding the task queue mutex */
    herr_t ret_value        = SUCCEED;

    FUNC_ENTER_NOAPI_NAMECHECK_ONLY

    /* Sanity checks */
    if (H5_UNLIKELY(NULL == pool))
        HGOTO_DONE(FAIL);

    /* Acquire the mutex for the task queue */
    if (H5_UNLIKELY(H5TS_mutex_lock(&pool->queue_mutex) < 0))
        HGOTO_DONE(FAIL);
    have_queue_mutex = true;

    /* Tell any existing threads that the pool is shutting down */
    pool->shutdown = true;

    /* Release the task queue's mutex */
    if (H5_UNLIKELY(H5TS_mutex_unlock(&pool->queue_mutex) < 0))
        HGOTO_DONE(FAIL);
    have_queue_mutex = false;

    /* Add a "shutdown" task for all threads */
    for (unsigned u = 0; u < pool->num_threads; u++)
        if (H5_UNLIKELY(H5TS_semaphore_signal(&pool->sem) < 0))
            HGOTO_DONE(FAIL);

    /* Free pool */
    if (H5_UNLIKELY(H5TS__pool_free(pool) < 0))
        HGOTO_DONE(FAIL);

done:
    /* Release the task queue's mutex, if we're holding it */
    if (H5_UNLIKELY(have_queue_mutex))
        if (H5_UNLIKELY(H5TS_mutex_unlock(&pool->queue_mutex) < 0))
            ret_value = FAIL;

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* end H5TS_pool_destroy() */

#endif /* H5_HAVE_THREADS */
