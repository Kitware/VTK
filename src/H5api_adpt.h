/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * H5api_adpt.h
 *
 * API decorations for exported symbols
 *
 * These decorations are mostly for Windows, which has to use __declspec(dllexport)
 * when linking a library together and __declspec(dllimport) when consuming
 * a previously-built library.
 *
 * We use the same macros everywhere, both shared and static, as the shared
 * markup is ignored when building static libraries.
 *
 * Each library needs a different set of named macros or Windows will
 * incorrectly use dllexport with imported library when it should be using
 * dllimport.
 */

#ifndef H5API_ADPT_H
#define H5API_ADPT_H

#ifdef H5_BUILT_AS_DYNAMIC_LIB

/* clang-format off */

/* When building with MSVC, we need to decorate the functions with
 * __declspec()
 *
 * NOTE that _MSC_VER is also defined by clang + Visual Studio
 */
#if defined(_MSC_VER)
    #define H5_API_EXPORT        __declspec(dllexport)
    #define H5_VAR_EXPORT extern __declspec(dllexport)
    #define H5_API_IMPORT        __declspec(dllimport)
    #define H5_VAR_IMPORT extern __declspec(dllimport)

/* Compilers that support GNU extensions (gcc, clang, etc.) support API
 * visibility attributes.
 */
#elif defined(__GNUC__)
    #define H5_API_EXPORT        __attribute__((visibility("default")))
    #define H5_VAR_EXPORT extern __attribute__((visibility("default")))
    #define H5_API_IMPORT        __attribute__((visibility("default")))
    #define H5_VAR_IMPORT extern __attribute__((visibility("default")))

/* API decorations for anything not covered above */
#else
    #define H5_API_EXPORT
    #define H5_VAR_EXPORT extern
    #define H5_API_IMPORT
    #define H5_VAR_IMPORT extern
#endif

/* clang-fomat on */

/* CMake defines <project>_shared_EXPORTS when building a shared library */

/* C library */
#if defined(hdf5_shared_EXPORTS)
#define H5_DLL    H5_API_EXPORT
#define H5_DLLVAR H5_VAR_EXPORT
#else
#define H5_DLL    H5_API_IMPORT
#define H5_DLLVAR H5_VAR_IMPORT
#endif

/* Test library */
#if defined(hdf5_test_shared_EXPORTS)
#define H5TEST_DLL    H5_API_EXPORT
#define H5TEST_DLLVAR H5_VAR_EXPORT
#else
#define H5TEST_DLL    H5_API_IMPORT
#define H5TEST_DLLVAR H5_VAR_IMPORT
#endif

/* Parallel test library */
#if defined(hdf5_testpar_shared_EXPORTS)
#define H5TESTPAR_DLL    H5_API_EXPORT
#define H5TESTPAR_DLLVAR H5_VAR_EXPORT
#else
#define H5TESTPAR_DLL    H5_API_IMPORT
#define H5TESTPAR_DLLVAR H5_VAR_IMPORT
#endif

/* Tools library */
#if defined(hdf5_tools_shared_EXPORTS)
#define H5TOOLS_DLL    H5_API_EXPORT
#define H5TOOLS_DLLVAR H5_VAR_EXPORT
#else
#define H5TOOLS_DLL    H5_API_IMPORT
#define H5TOOLS_DLLVAR H5_VAR_IMPORT
#endif

/* C++ library */
#if defined(hdf5_cpp_shared_EXPORTS)
#define H5CPP_DLL    H5_API_EXPORT
#define H5CPP_DLLVAR H5_VAR_EXPORT
#else
#define H5CPP_DLL    H5_API_IMPORT
#define H5CPP_DLLVAR H5_VAR_IMPORT
#endif

/* High-level library */
#if defined(hdf5_hl_shared_EXPORTS)
#define H5HL_DLL    H5_API_EXPORT
#define H5HL_DLLVAR H5_VAR_EXPORT
#else
#define H5HL_DLL    H5_API_IMPORT
#define H5HL_DLLVAR H5_VAR_IMPORT
#endif

/* High-level C++ library */
#if defined(hdf5_hl_cpp_shared_EXPORTS)
#define H5CPP_HL_DLL    H5_API_EXPORT
#define H5CPP_HL_DLLVAR H5_VAR_EXPORT
#else
#define H5CPP_HL_DLL    H5_API_IMPORT
#define H5CPP_HL_DLLVAR H5_VAR_IMPORT
#endif

/* Fortran library C stubs */
#if defined(hdf5_f90cstub_shared_EXPORTS)
#define H5FC_DLL    H5_API_EXPORT
#define H5FC_DLLVAR H5_VAR_EXPORT
#else
#define H5FC_DLL    H5_API_IMPORT
#define H5FC_DLLVAR H5_VAR_IMPORT
#endif

/* Fortran test library C stubs */
#if defined(hdf5_test_f90cstub_shared_EXPORTS)
#define H5FC_TEST_DLL    H5_API_EXPORT
#define H5FC_TEST_DLLVAR H5_VAR_EXPORT
#else
#define H5FC_TEST_DLL    H5_API_IMPORT
#define H5FC_TEST_DLLVAR H5_VAR_IMPORT
#endif

/* High-level Fortran library C stubs */
#if defined(hdf5_hl_f90cstub_shared_EXPORTS)
#define H5FC_HL_DLL    H5_API_EXPORT
#define H5FC_HL_DLLVAR H5_VAR_EXPORT
#else
#define H5FC_HL_DLL    H5_API_IMPORT
#define H5FC_HL_DLLVAR H5_VAR_IMPORT
#endif

#else

/* Static library decorations */
#define H5_DLL
#define H5_DLLVAR extern
#define H5TEST_DLL
#define H5TEST_DLLVAR extern
#define H5TESTPAR_DLL
#define H5TESTPAR_DLLVAR extern
#define H5TOOLS_DLL
#define H5TOOLS_DLLVAR extern
#define H5CPP_DLL
#define H5CPP_DLLVAR extern
#define H5HL_DLL
#define H5HL_DLLVAR extern
#define H5CPP_HL_DLL
#define H5CPP_HL_DLLVAR extern
#define H5FC_DLL
#define H5FC_DLLVAR extern
#define H5FC_TEST_DLL
#define H5FC_TEST_DLLVAR extern
#define H5FC_HL_DLL
#define H5FC_HL_DLLVAR extern

#endif /* H5_BUILT_AS_DYNAMIC_LIB */

#endif /* H5API_ADPT_H */
