/**
 * Copyright (c) 2013-2022 UChicago Argonne, LLC and The HDF Group.
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mercury_thread_mutex.h"

#include "mercury_util_error.h"

#include <string.h>

#ifndef _WIN32
static int
hg_thread_mutex_init_posix(hg_thread_mutex_t *mutex, int kind)
{
    pthread_mutexattr_t mutex_attr;
    int                 ret = HG_UTIL_SUCCESS;
    int                 rc;

    rc = pthread_mutexattr_init(&mutex_attr);
    HG_UTIL_CHECK_ERROR(rc != 0, done, ret, HG_UTIL_FAIL, "pthread_mutexattr_init() failed (%s)",
                        strerror(rc));

    /* Keep mutex mode as normal and do not expect error checking */
    rc = pthread_mutexattr_settype(&mutex_attr, kind);
    HG_UTIL_CHECK_ERROR(rc != 0, done, ret, HG_UTIL_FAIL, "pthread_mutexattr_settype() failed (%s)",
                        strerror(rc));

    rc = pthread_mutex_init(mutex, &mutex_attr);
    HG_UTIL_CHECK_ERROR(rc != 0, done, ret, HG_UTIL_FAIL, "pthread_mutex_init() failed (%s)", strerror(rc));

done:
    rc = pthread_mutexattr_destroy(&mutex_attr);

    return ret;
}
#endif

/*---------------------------------------------------------------------------*/
int
hg_thread_mutex_init(hg_thread_mutex_t *mutex)
{
    int ret = HG_UTIL_SUCCESS;

#ifdef _WIN32
    InitializeCriticalSection(mutex);
#else
    ret = hg_thread_mutex_init_posix(mutex, PTHREAD_MUTEX_NORMAL);
#endif

    return ret;
}

/*---------------------------------------------------------------------------*/
int
hg_thread_mutex_init_fast(hg_thread_mutex_t *mutex)
{
    int ret = HG_UTIL_SUCCESS;

#if defined(_WIN32)
    ret = hg_thread_mutex_init(mutex);
#elif defined(H5_HAVE_PTHREAD_MUTEX_ADAPTIVE_NP)
    /* Set type to PTHREAD_MUTEX_ADAPTIVE_NP to improve performance */
    ret = hg_thread_mutex_init_posix(mutex, PTHREAD_MUTEX_ADAPTIVE_NP);
#else
    ret = hg_thread_mutex_init_posix(mutex, PTHREAD_MUTEX_NORMAL);
#endif

    return ret;
}

/*---------------------------------------------------------------------------*/
int
hg_thread_mutex_destroy(hg_thread_mutex_t *mutex)
{
    int ret = HG_UTIL_SUCCESS;

#ifdef _WIN32
    DeleteCriticalSection(mutex);
#else
    int rc;

    rc = pthread_mutex_destroy(mutex);
    HG_UTIL_CHECK_ERROR(rc != 0, done, ret, HG_UTIL_FAIL, "pthread_mutex_destroy() failed (%s)",
                        strerror(rc));

done:
#endif
    return ret;
}
