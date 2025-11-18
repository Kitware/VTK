/**
 * Copyright (c) 2013-2022 UChicago Argonne, LLC and The HDF Group.
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mercury_thread.h"

#if !defined(_WIN32) && !defined(__APPLE__)
#include <sched.h>
#endif

/*---------------------------------------------------------------------------*/
void
hg_thread_init(hg_thread_t *thread)
{
#ifdef _WIN32
    *thread = NULL;
#else
    *thread = 0;
#endif
}

/*---------------------------------------------------------------------------*/
int
hg_thread_create(hg_thread_t *thread, hg_thread_func_t f, void *data)
{
#ifdef _WIN32
    *thread = CreateThread(NULL, 0, f, data, 0, NULL);
    if (*thread == NULL)
        return HG_UTIL_FAIL;
#else
    if (pthread_create(thread, NULL, f, data))
        return HG_UTIL_FAIL;
#endif

    return HG_UTIL_SUCCESS;
}

/*---------------------------------------------------------------------------*/
void
hg_thread_exit(hg_thread_ret_t ret)
{
#ifdef _WIN32
    ExitThread(ret);
#else
    pthread_exit(ret);
#endif
}

/*---------------------------------------------------------------------------*/
int
hg_thread_join(hg_thread_t thread)
{
#ifdef _WIN32
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#else
    if (pthread_join(thread, NULL))
        return HG_UTIL_FAIL;
#endif

    return HG_UTIL_SUCCESS;
}

/*---------------------------------------------------------------------------*/
int
hg_thread_cancel(hg_thread_t thread)
{
#ifdef _WIN32
    WaitForSingleObject(thread, 0);
    CloseHandle(thread);
#else
    if (pthread_cancel(thread))
        return HG_UTIL_FAIL;
#endif

    return HG_UTIL_SUCCESS;
}

/*---------------------------------------------------------------------------*/
int
hg_thread_yield(void)
{
#ifdef _WIN32
    SwitchToThread();
#elif defined(__APPLE__)
    pthread_yield_np();
#else
    sched_yield();
#endif

    return HG_UTIL_SUCCESS;
}

/*---------------------------------------------------------------------------*/
int
hg_thread_key_create(hg_thread_key_t *key)
{
    if (!key)
        return HG_UTIL_FAIL;

#ifdef _WIN32
    if ((*key = TlsAlloc()) == TLS_OUT_OF_INDEXES)
        return HG_UTIL_FAIL;
#else
    if (pthread_key_create(key, NULL))
        return HG_UTIL_FAIL;
#endif

    return HG_UTIL_SUCCESS;
}

/*---------------------------------------------------------------------------*/
int
hg_thread_key_delete(hg_thread_key_t key)
{
#ifdef _WIN32
    if (!TlsFree(key))
        return HG_UTIL_FAIL;
#else
    if (pthread_key_delete(key))
        return HG_UTIL_FAIL;
#endif

    return HG_UTIL_SUCCESS;
}

/*---------------------------------------------------------------------------*/
int
hg_thread_getaffinity(hg_thread_t thread, hg_cpu_set_t *cpu_mask)
{
#if defined(_WIN32)
    return HG_UTIL_FAIL;
#elif defined(__APPLE__)
    (void)thread;
    (void)cpu_mask;
    return HG_UTIL_FAIL;
#else
    if (pthread_getaffinity_np(thread, sizeof(hg_cpu_set_t), cpu_mask))
        return HG_UTIL_FAIL;
    return HG_UTIL_SUCCESS;
#endif
}

/*---------------------------------------------------------------------------*/
int
hg_thread_setaffinity(hg_thread_t thread, const hg_cpu_set_t *cpu_mask)
{
#if defined(_WIN32)
    if (!SetThreadAffinityMask(thread, *cpu_mask))
        return HG_UTIL_FAIL;
    return HG_UTIL_SUCCESS;
#elif defined(__APPLE__)
    (void)thread;
    (void)cpu_mask;
    return HG_UTIL_FAIL;
#else
    if (pthread_setaffinity_np(thread, sizeof(hg_cpu_set_t), cpu_mask))
        return HG_UTIL_FAIL;
    return HG_UTIL_SUCCESS;
#endif
}
