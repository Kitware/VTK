/**
 * Copyright (c) 2013-2022 UChicago Argonne, LLC and The HDF Group.
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MERCURY_THREAD_H
#define MERCURY_THREAD_H

#if !defined(_WIN32) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include "mercury_util_config.h"

#ifdef _WIN32
#define _WINSOCKAPI_
#include <windows.h>
typedef HANDLE                 hg_thread_t;
typedef LPTHREAD_START_ROUTINE hg_thread_func_t;
typedef DWORD                  hg_thread_ret_t;
#define HG_THREAD_RETURN_TYPE hg_thread_ret_t WINAPI
typedef DWORD     hg_thread_key_t;
typedef DWORD_PTR hg_cpu_set_t;
#else
#include <pthread.h>
typedef pthread_t hg_thread_t;
typedef void *(*hg_thread_func_t)(void *);
typedef void         *hg_thread_ret_t;
#define HG_THREAD_RETURN_TYPE hg_thread_ret_t
typedef pthread_key_t hg_thread_key_t;
#ifdef __APPLE__
/* Size definition for CPU sets.  */
#define HG_CPU_SETSIZE 1024
#define HG_NCPUBITS    (8 * sizeof(hg_cpu_mask_t))
/* Type for array elements in 'cpu_set_t'.  */
typedef uint64_t hg_cpu_mask_t;
typedef struct {
    hg_cpu_mask_t bits[HG_CPU_SETSIZE / HG_NCPUBITS];
} hg_cpu_set_t;
#else
typedef cpu_set_t hg_cpu_set_t;
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the thread.
 *
 * \param thread [IN/OUT]       pointer to thread object
 */
HG_UTIL_PUBLIC void hg_thread_init(hg_thread_t *thread);

/**
 * Create a new thread for the given function.
 *
 * \param thread [IN/OUT]       pointer to thread object
 * \param f [IN]                pointer to function
 * \param data [IN]             pointer to data than be passed to function f
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC int hg_thread_create(hg_thread_t *thread, hg_thread_func_t f, void *data);

/**
 * Ends the calling thread.
 *
 * \param ret [IN]              exit code for the thread
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC void hg_thread_exit(hg_thread_ret_t ret);

/**
 * Wait for thread completion.
 *
 * \param thread [IN]           thread object
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC int hg_thread_join(hg_thread_t thread);

/**
 * Terminate the thread.
 *
 * \param thread [IN]           thread object
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC int hg_thread_cancel(hg_thread_t thread);

/**
 * Yield the processor.
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC int hg_thread_yield(void);

/**
 * Obtain handle of the calling thread.
 *
 * \return
 */
static HG_UTIL_INLINE hg_thread_t hg_thread_self(void);

/**
 * Compare thread IDs.
 *
 * \return Non-zero if equal, zero if not equal
 */
static HG_UTIL_INLINE int hg_thread_equal(hg_thread_t t1, hg_thread_t t2);

/**
 * Create a thread-specific data key visible to all threads in the process.
 *
 * \param key [OUT]             pointer to thread key object
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC int hg_thread_key_create(hg_thread_key_t *key);

/**
 * Delete a thread-specific data key previously returned by
 * hg_thread_key_create().
 *
 * \param key [IN]              thread key object
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC int hg_thread_key_delete(hg_thread_key_t key);

/**
 * Get value from specified key.
 *
 * \param key [IN]              thread key object
 *
 * \return Pointer to data associated to the key
 */
static HG_UTIL_INLINE void *hg_thread_getspecific(hg_thread_key_t key);

/**
 * Set value to specified key.
 *
 * \param key [IN]              thread key object
 * \param value [IN]            pointer to data that will be associated
 *
 * \return Non-negative on success or negative on failure
 */
static HG_UTIL_INLINE int hg_thread_setspecific(hg_thread_key_t key, const void *value);

/**
 * Get affinity mask.
 *
 * \param thread [IN]           thread object
 * \param cpu_mask [IN/OUT]     cpu mask
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC int hg_thread_getaffinity(hg_thread_t thread, hg_cpu_set_t *cpu_mask);

/**
 * Set affinity mask.
 *
 * \param thread [IN]           thread object
 * \param cpu_mask [IN]         cpu mask
 *
 * \return Non-negative on success or negative on failure
 */
HG_UTIL_PUBLIC int hg_thread_setaffinity(hg_thread_t thread, const hg_cpu_set_t *cpu_mask);

/*---------------------------------------------------------------------------*/
static HG_UTIL_INLINE hg_thread_t
hg_thread_self(void)
{
#ifdef _WIN32
    return GetCurrentThread();
#else
    return pthread_self();
#endif
}

/*---------------------------------------------------------------------------*/
static HG_UTIL_INLINE int
hg_thread_equal(hg_thread_t t1, hg_thread_t t2)
{
#ifdef _WIN32
    return GetThreadId(t1) == GetThreadId(t2);
#else
    return pthread_equal(t1, t2);
#endif
}

/*---------------------------------------------------------------------------*/
static HG_UTIL_INLINE void *
hg_thread_getspecific(hg_thread_key_t key)
{
#ifdef _WIN32
    return TlsGetValue(key);
#else
    return pthread_getspecific(key);
#endif
}

/*---------------------------------------------------------------------------*/
static HG_UTIL_INLINE int
hg_thread_setspecific(hg_thread_key_t key, const void *value)
{
#ifdef _WIN32
    if (!TlsSetValue(key, (LPVOID)value))
        return HG_UTIL_FAIL;
#else
    if (pthread_setspecific(key, value))
        return HG_UTIL_FAIL;
#endif

    return HG_UTIL_SUCCESS;
}

#ifdef __cplusplus
}
#endif

#endif /* MERCURY_THREAD_H */
