/**
 * Copyright (c) 2013-2022 UChicago Argonne, LLC and The HDF Group.
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mercury_thread_condition.h"

/*---------------------------------------------------------------------------*/
int
hg_thread_cond_init(hg_thread_cond_t *cond)
{
#ifdef _WIN32
    InitializeConditionVariable(cond);
#else
    pthread_condattr_t attr;

    pthread_condattr_init(&attr);
#if defined(H5_HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined(H5_HAVE_CLOCK_MONOTONIC_COARSE)
    /* Must set clock ID if using different clock
     * (CLOCK_MONOTONIC_COARSE not supported here) */
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
#endif
    if (pthread_cond_init(cond, &attr))
        return HG_UTIL_FAIL;
    pthread_condattr_destroy(&attr);
#endif

    return HG_UTIL_SUCCESS;
}

/*---------------------------------------------------------------------------*/
int
hg_thread_cond_destroy(hg_thread_cond_t *cond)
{
#ifndef _WIN32
    if (pthread_cond_destroy(cond))
        return HG_UTIL_FAIL;
#endif

    return HG_UTIL_SUCCESS;
}
