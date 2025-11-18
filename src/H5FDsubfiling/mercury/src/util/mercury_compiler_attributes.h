/**
 * Copyright (c) 2013-2022 UChicago Argonne, LLC and The HDF Group.
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MERCURY_COMPILER_ATTRIBUTES_H
#define MERCURY_COMPILER_ATTRIBUTES_H

/*************************************/
/* Public Type and Struct Definition */
/*************************************/

/*****************/
/* Public Macros */
/*****************/

/*
 * __has_attribute is supported on gcc >= 5, clang >= 2.9 and icc >= 17.
 * In the meantime, to support gcc < 5, we implement __has_attribute
 * by hand.
 */
#if !defined(__has_attribute) && defined(__GNUC__) && (__GNUC__ >= 4)
#define __has_attribute(x)                          __GCC4_has_attribute_##x
#define __GCC4_has_attribute___visibility__         1
#define __GCC4_has_attribute___warn_unused_result__ 1
#define __GCC4_has_attribute___unused__             1
#define __GCC4_has_attribute___format__             1
#define __GCC4_has_attribute___fallthrough__        0
#endif

/* Visibility of symbols */
#if defined(_WIN32)
#define HG_ATTR_ABI_IMPORT __declspec(dllimport)
#define HG_ATTR_ABI_EXPORT __declspec(dllexport)
#define HG_ATTR_ABI_HIDDEN
#elif __has_attribute(__visibility__)
#define HG_ATTR_ABI_IMPORT __attribute__((__visibility__("default")))
#define HG_ATTR_ABI_EXPORT __attribute__((__visibility__("default")))
#define HG_ATTR_ABI_HIDDEN __attribute__((__visibility__("hidden")))
#else
#define HG_ATTR_ABI_IMPORT
#define HG_ATTR_ABI_EXPORT
#define HG_ATTR_ABI_HIDDEN
#endif

#endif /* MERCURY_COMPILER_ATTRIBUTES_H */
