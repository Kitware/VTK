/**
 * Copyright (c) 2013-2022 UChicago Argonne, LLC and The HDF Group.
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MERCURY_THREAD_MUTEX_H
#define MERCURY_THREAD_MUTEX_H

#include "mercury_util_config.h"

#include "mercury_thread_annotation.h"

#ifdef _WIN32
#define _WINSOCKAPI_
#include <windows.h>
/* clang-format off */
#    define HG_THREAD_MUTEX_INITIALIZER {NULL}
/* clang-format on */
typedef CRITICAL_SECTION hg_thread_mutex_t;
#else
#include <pthread.h>
#define HG_THREAD_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
typedef pthread_mutex_t HG_LOCK_CAPABILITY("mutex") hg_thread_mutex_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the mutex.
 *
 * \param mutex [IN/OUT]        pointer to mutex object
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC int hg_thread_mutex_init(hg_thread_mutex_t *mutex);

/**
 * Initialize the mutex, asking for "fast" mutex.
 *
 * \param mutex [IN/OUT]        pointer to mutex object
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC int hg_thread_mutex_init_fast(hg_thread_mutex_t *mutex);

/**
 * Destroy the mutex.
 *
 * \param mutex [IN/OUT]        pointer to mutex object
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC int hg_thread_mutex_destroy(hg_thread_mutex_t *mutex);

/**
 * Lock the mutex.
 *
 * \param mutex [IN/OUT]        pointer to mutex object
 */
static HG_UTIL_INLINE void hg_thread_mutex_lock(hg_thread_mutex_t *mutex) HG_LOCK_ACQUIRE(*mutex);

/**
 * Try locking the mutex.
 *
 * \param mutex [IN/OUT]        pointer to mutex object
 *
 * \return Non-negative on success or negative on failure
 */
static HG_UTIL_INLINE int hg_thread_mutex_try_lock(hg_thread_mutex_t *mutex)
    HG_LOCK_TRY_ACQUIRE(HG_UTIL_SUCCESS, *mutex);

/**
 * Unlock the mutex.
 *
 * \param mutex [IN/OUT]        pointer to mutex object
 */
static HG_UTIL_INLINE void hg_thread_mutex_unlock(hg_thread_mutex_t *mutex) HG_LOCK_RELEASE(*mutex);

/*---------------------------------------------------------------------------*/
static HG_UTIL_INLINE void
hg_thread_mutex_lock(hg_thread_mutex_t *mutex) HG_LOCK_NO_THREAD_SAFETY_ANALYSIS
{
#ifdef _WIN32
    EnterCriticalSection(mutex);
#else
    (void)pthread_mutex_lock(mutex);
#endif
}

/*---------------------------------------------------------------------------*/
static HG_UTIL_INLINE int
hg_thread_mutex_try_lock(hg_thread_mutex_t *mutex) HG_LOCK_NO_THREAD_SAFETY_ANALYSIS
{
#ifdef _WIN32
    if (!TryEnterCriticalSection(mutex))
        return HG_UTIL_FAIL;
#else
    if (pthread_mutex_trylock(mutex))
        return HG_UTIL_FAIL;
#endif

    return HG_UTIL_SUCCESS;
}

/*---------------------------------------------------------------------------*/
static HG_UTIL_INLINE void
hg_thread_mutex_unlock(hg_thread_mutex_t *mutex) HG_LOCK_NO_THREAD_SAFETY_ANALYSIS
{
#ifdef _WIN32
    LeaveCriticalSection(mutex);
#else
    (void)pthread_mutex_unlock(mutex);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* MERCURY_THREAD_MUTEX_H */
