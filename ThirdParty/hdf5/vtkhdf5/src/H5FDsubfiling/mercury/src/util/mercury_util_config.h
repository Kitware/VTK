/**
 * Copyright (c) 2013-2022 UChicago Argonne, LLC and The HDF Group.
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MERCURY_UTIL_CONFIG_H
#define MERCURY_UTIL_CONFIG_H

/*************************************/
/* Public Type and Struct Definition */
/*************************************/

#include "H5private.h"

/* Type definitions */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*****************/
/* Public Macros */
/*****************/

/* Reflects any major or incompatible public API changes */
#define HG_UTIL_VERSION_MAJOR 4
/* Reflects any minor backwards compatible API or functionality addition */
#define HG_UTIL_VERSION_MINOR 0
/* Reflects any backwards compatible bug fixes */
#define HG_UTIL_VERSION_PATCH 0

/* Return codes */
#define HG_UTIL_SUCCESS 0
#define HG_UTIL_FAIL    -1

#include <mercury_compiler_attributes.h>

/* Inline macro */
#ifdef _WIN32
#define HG_UTIL_INLINE __inline
#else
#define HG_UTIL_INLINE __inline__
#endif

#define HG_UTIL_PUBLIC
#define HG_UTIL_PRIVATE
#define HG_UTIL_PLUGIN

#endif /* MERCURY_UTIL_CONFIG_H */
