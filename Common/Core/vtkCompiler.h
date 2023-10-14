// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkCompiler_h
#define vtkCompiler_h

/*--------------------------------------------------------------------------*/
/* Compiler backend                                                         */
/* Be careful modifying this -- order is important.                         */
#if defined(_MSC_VER)
/* MSVC 2015+ can use a clang frontend, so we want to label it only as MSVC
 * and not MSVC and clang. */
#define VTK_COMPILER_MSVC

#elif defined(__INTEL_COMPILER)
/* Intel 14+ on OSX uses a clang frontend, so again we want to label them as
 * intel only, and not intel and clang. */
#define VTK_COMPILER_ICC

#elif defined(__PGI)
/* PGI reports as GNUC as it generates the same ABI, so we need to check for
 * it before gcc. */
#define VTK_COMPILER_PGI

#elif defined(__clang__)
/* Check for clang before GCC, as clang says it is GNUC since it has ABI
 * compliance and supports many of the same extensions. */
#define VTK_COMPILER_CLANG

#elif defined(__GNUC__)
/* Several compilers pretend to be GCC but have minor differences. To
 * compensate for that, we checked for those compilers first above. */
#define VTK_COMPILER_GCC
#define VTK_COMPILER_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

/** extern template declarations for C++11. */
#define VTK_USE_EXTERN_TEMPLATE

//----------------------------------------------------------------------------
// Provide a VTK_ALWAYS_EXPORT macro.
//
// Issue:
// Dynamic cast is not just based on the name of the class, but also the
// combined visibility of the class on macos. When building the hash_code of
// an object the symbol visibility controls of the type are taken into
// consideration (including symbol visibility of template parameters).
// Therefore, if a class has a component with private/hidden visibility then
// it cannot be passed across library boundaries.
//
// Solution:
// The solution is fairly simple, but annoying. You need to mark template
// classes intended for use in dynamic_cast with appropriate visibility
// settings.
//
// TL;DR:
// This markup is used when we want to make sure:
//  - The class can be compiled into multiple libraries and at runtime will
//    resolve to a single type instance
//  - Be a type ( or component of a types signature ) that can be passed between
//    dynamic libraries and requires RTTI support ( dynamic_cast ).
#if defined(VTK_COMPILER_MSVC)
#define VTK_ALWAYS_EXPORT
#else
#define VTK_ALWAYS_EXPORT __attribute__((visibility("default")))
#endif

#endif

// VTK-HeaderTest-Exclude: vtkCompiler.h
