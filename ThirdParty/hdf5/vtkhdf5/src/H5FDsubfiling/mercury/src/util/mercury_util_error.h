/**
 * Copyright (c) 2013-2022 UChicago Argonne, LLC and The HDF Group.
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MERCURY_UTIL_ERROR_H
#define MERCURY_UTIL_ERROR_H

#include "mercury_util_config.h"

/* Branch predictor hints */
#ifndef _WIN32
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif

/* Error macros */
#define HG_UTIL_GOTO_DONE(label, ret, ret_val)                                                               \
    do {                                                                                                     \
        ret = ret_val;                                                                                       \
        goto label;                                                                                          \
    } while (0)

#define HG_UTIL_GOTO_ERROR(label, ret, err_val, ...)                                                         \
    do {                                                                                                     \
        ret = err_val;                                                                                       \
        goto label;                                                                                          \
    } while (0)

/* Check for cond, set ret to err_val and goto label */
#define HG_UTIL_CHECK_ERROR(cond, label, ret, err_val, ...)                                                  \
    do {                                                                                                     \
        if (unlikely(cond)) {                                                                                \
            ret = err_val;                                                                                   \
            goto label;                                                                                      \
        }                                                                                                    \
    } while (0)

#define HG_UTIL_CHECK_ERROR_NORET(cond, label, ...)                                                          \
    do {                                                                                                     \
        if (unlikely(cond)) {                                                                                \
            goto label;                                                                                      \
        }                                                                                                    \
    } while (0)

#endif /* MERCURY_UTIL_ERROR_H */
