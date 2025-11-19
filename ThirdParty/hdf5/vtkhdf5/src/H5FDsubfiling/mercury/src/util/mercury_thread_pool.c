/**
 * Copyright (c) 2013-2022 UChicago Argonne, LLC and The HDF Group.
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mercury_thread_pool.h"

#include "mercury_util_error.h"

#include <stdlib.h>

/****************/
/* Local Macros */
/****************/

/************************************/
/* Local Type and Struct Definition */
/************************************/

struct hg_thread_pool_private {
    struct hg_thread_pool pool;
    unsigned int          thread_count;
    hg_thread_t          *threads;
};

/********************/
/* Local Prototypes */
/********************/

/**
 * Worker thread run by the thread pool
 */
static HG_THREAD_RETURN_TYPE hg_thread_pool_worker(void *args);

/*******************/
/* Local Variables */
/*******************/

/*---------------------------------------------------------------------------*/
static HG_THREAD_RETURN_TYPE
hg_thread_pool_worker(void *args)
{
    hg_thread_ret_t        ret  = 0;
    hg_thread_pool_t      *pool = (hg_thread_pool_t *)args;
    struct hg_thread_work *work;

    while (1) {
        hg_thread_mutex_lock(&pool->mutex);

        /* If not shutting down and nothing to do, worker sleeps */
        while (!pool->shutdown && HG_QUEUE_IS_EMPTY(&pool->queue)) {
            int rc;

            pool->sleeping_worker_count++;

            rc = hg_thread_cond_wait(&pool->cond, &pool->mutex);
            HG_UTIL_CHECK_ERROR_NORET(rc != HG_UTIL_SUCCESS, unlock,
                                      "Thread cannot wait on condition variable");

            pool->sleeping_worker_count--;
        }

        if (pool->shutdown && HG_QUEUE_IS_EMPTY(&pool->queue))
            goto unlock;

        /* Grab our task */
        work = HG_QUEUE_FIRST(&pool->queue);
        HG_QUEUE_POP_HEAD(&pool->queue, entry);

        /* Unlock */
        hg_thread_mutex_unlock(&pool->mutex);

        /* Get to work */
        (*work->func)(work->args);
    }

unlock:
    hg_thread_mutex_unlock(&pool->mutex);

    return ret;
}

/*---------------------------------------------------------------------------*/
int
hg_thread_pool_init(unsigned int thread_count, hg_thread_pool_t **pool_ptr)
{
    int                            ret       = HG_UTIL_SUCCESS, rc;
    struct hg_thread_pool_private *priv_pool = NULL;
    unsigned int                   i;

    HG_UTIL_CHECK_ERROR(pool_ptr == NULL, error, ret, HG_UTIL_FAIL, "NULL pointer");

    priv_pool = (struct hg_thread_pool_private *)malloc(sizeof(struct hg_thread_pool_private));
    HG_UTIL_CHECK_ERROR(priv_pool == NULL, error, ret, HG_UTIL_FAIL, "Could not allocate thread pool");

    priv_pool->pool.sleeping_worker_count = 0;
    priv_pool->thread_count               = thread_count;
    priv_pool->threads                    = NULL;
    HG_QUEUE_INIT(&priv_pool->pool.queue);
    priv_pool->pool.shutdown = 0;

    rc = hg_thread_mutex_init(&priv_pool->pool.mutex);
    HG_UTIL_CHECK_ERROR(rc != HG_UTIL_SUCCESS, error, ret, HG_UTIL_FAIL, "Could not initialize mutex");

    rc = hg_thread_cond_init(&priv_pool->pool.cond);
    HG_UTIL_CHECK_ERROR(rc != HG_UTIL_SUCCESS, error, ret, HG_UTIL_FAIL,
                        "Could not initialize thread condition");

    priv_pool->threads = (hg_thread_t *)malloc(thread_count * sizeof(hg_thread_t));
    HG_UTIL_CHECK_ERROR(!priv_pool->threads, error, ret, HG_UTIL_FAIL,
                        "Could not allocate thread pool array");

    /* Start worker threads */
    for (i = 0; i < thread_count; i++) {
        rc = hg_thread_create(&priv_pool->threads[i], hg_thread_pool_worker, (void *)priv_pool);
        HG_UTIL_CHECK_ERROR(rc != HG_UTIL_SUCCESS, error, ret, HG_UTIL_FAIL, "Could not create thread");
    }

    *pool_ptr = (struct hg_thread_pool *)priv_pool;

    return ret;

error:
    if (priv_pool)
        hg_thread_pool_destroy((struct hg_thread_pool *)priv_pool);

    return ret;
}

/*---------------------------------------------------------------------------*/
int
hg_thread_pool_destroy(hg_thread_pool_t *pool)
{
    struct hg_thread_pool_private *priv_pool = (struct hg_thread_pool_private *)pool;
    int                            ret       = HG_UTIL_SUCCESS, rc;
    unsigned int                   i;

    if (!priv_pool)
        goto done;

    if (priv_pool->threads) {
        hg_thread_mutex_lock(&priv_pool->pool.mutex);

        priv_pool->pool.shutdown = 1;

        rc = hg_thread_cond_broadcast(&priv_pool->pool.cond);
        HG_UTIL_CHECK_ERROR(rc != HG_UTIL_SUCCESS, error, ret, HG_UTIL_FAIL,
                            "Could not broadcast condition signal");

        hg_thread_mutex_unlock(&priv_pool->pool.mutex);

        for (i = 0; i < priv_pool->thread_count; i++) {
            rc = hg_thread_join(priv_pool->threads[i]);
            HG_UTIL_CHECK_ERROR(rc != HG_UTIL_SUCCESS, done, ret, HG_UTIL_FAIL, "Could not join thread");
        }
    }

    rc = hg_thread_mutex_destroy(&priv_pool->pool.mutex);
    HG_UTIL_CHECK_ERROR(rc != HG_UTIL_SUCCESS, done, ret, HG_UTIL_FAIL, "Could not destroy mutex");

    rc = hg_thread_cond_destroy(&priv_pool->pool.cond);
    HG_UTIL_CHECK_ERROR(rc != HG_UTIL_SUCCESS, done, ret, HG_UTIL_FAIL, "Could not destroy thread condition");

    free(priv_pool->threads);
    free(priv_pool);

done:
    return ret;

error:
    hg_thread_mutex_unlock(&priv_pool->pool.mutex);

    return ret;
}
