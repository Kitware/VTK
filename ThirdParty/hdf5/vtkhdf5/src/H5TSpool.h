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

/***********/
/* Headers */
/***********/

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/* Thread pool task */
typedef struct H5TS_pool_task_t {
    H5TS_thread_start_func_t func; /* Function to invoke with thread */
    void                    *ctx;  /* Context for task's function */
    struct H5TS_pool_task_t *next; /* Pointer to next task */
} H5TS_pool_task_t;

/* Thread pool */
struct H5TS_pool_t {
    /* Semaphore */
    H5TS_semaphore_t sem;

    /* Task queue */
    H5TS_mutex_t      queue_mutex; /* Mutex to control access to task queue */
    bool              shutdown;    /* Pool is shutting down */
    H5TS_pool_task_t *head, *tail; /* Task queue */

    /* Threads */
    unsigned       num_threads; /* # of threads in pool */
    H5TS_thread_t *threads;     /* Array of worker threads in pool */
};

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

/*--------------------------------------------------------------------------
 * Function:    H5TS_pool_add_task
 *
 * Purpose:     Add a new task to a pool
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
static inline herr_t
H5TS_pool_add_task(H5TS_pool_t *pool, H5TS_thread_start_func_t func, void *ctx)
{
    H5TS_pool_task_t *task             = NULL;  /* Task for function to invoke */
    bool              have_queue_mutex = false; /* Whether we're holding the task queue mutex */
    herr_t            ret_value        = SUCCEED;

    /* Sanity checks */
    if (H5_UNLIKELY(NULL == pool || NULL == func))
        return FAIL;

    /* Allocate & initialize new task */
    if (H5_UNLIKELY(NULL == (task = (H5TS_pool_task_t *)malloc(sizeof(H5TS_pool_task_t)))))
        return FAIL;
    task->func = func;
    task->ctx  = ctx;
    task->next = NULL;

    /* Acquire the mutex for the task queue */
    if (H5_UNLIKELY(H5TS_mutex_lock(&pool->queue_mutex) < 0)) {
        ret_value = FAIL;
        goto done;
    }
    have_queue_mutex = true;

    /* Is pool shutting down? */
    if (H5_UNLIKELY(pool->shutdown)) {
        ret_value = FAIL;
        goto done;
    }

    /* Add task to pool's queue */
    if (NULL != pool->tail) {
        pool->tail->next = task;
        pool->tail       = task;
    }
    else
        pool->head = pool->tail = task;

    /* Avoid freeing the task on error, now */
    task = NULL;

    /* Release the task queue's mutex, if we're holding it */
    if (H5_UNLIKELY(H5TS_mutex_unlock(&pool->queue_mutex) < 0)) {
        ret_value = FAIL;
        goto done;
    }
    have_queue_mutex = false;

    /* Signal the semaphore */
    if (H5_UNLIKELY(H5TS_semaphore_signal(&pool->sem) < 0))
        ret_value = FAIL;

done:
    /* Free the task, on failure */
    if (H5_UNLIKELY(ret_value < 0)) {
        /* Release the task queue's mutex, if we're still holding it */
        /* (Can only happen on failure) */
        if (H5_UNLIKELY(have_queue_mutex))
            H5TS_mutex_unlock(&pool->queue_mutex);
        if (task)
            free(task);
    }

    return (ret_value);
} /* end H5TS_pool_add_task() */
