// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkABI
 * @brief   manage macros for exporting symbols in the binary interface.
 *
 * This header defines the macros for importing and exporting symbols in shared
 * objects (DLLs, etc). All VTK headers should use these macros to define the
 * kit specific import/export macros. So for the vtkCommon kit this might be,
 *
 * \code
 * #include "vtkABI.h"
 *
 * #if defined(VTK_BUILD_SHARED_LIBS)
 * # if defined(vtkCommon_EXPORTS)
 * #  define VTK_COMMON_EXPORT VTK_ABI_EXPORT
 * # else
 * #  define VTK_COMMON_EXPORT VTK_ABI_IMPORT
 * # endif
 * #else
 * # define VTK_COMMON_EXPORT
 * #endif
 * \endcode
 *
 * See http://gcc.gnu.org/wiki/Visibility for a discussion of the symbol
 * visibility support in GCC. The project must pass extra CFLAGS/CXXFLAGS in
 * order to change the default symbol visibility when using GCC.
 * Currently hidden is not used, but it can be used to explicitly hide
 * symbols from external linkage.
 */

#ifndef vtkABI_h
#define vtkABI_h

#include "vtkOptions.h" // for VTK_USE_FUTURE_BOOL

#if defined(_WIN32)
#define VTK_ABI_IMPORT __declspec(dllimport)
#define VTK_ABI_EXPORT __declspec(dllexport)
#define VTK_ABI_HIDDEN
#elif __GNUC__ >= 4
#define VTK_ABI_IMPORT __attribute__((visibility("default")))
#define VTK_ABI_EXPORT __attribute__((visibility("default")))
#define VTK_ABI_HIDDEN __attribute__((visibility("hidden")))
#else
#define VTK_ABI_IMPORT
#define VTK_ABI_EXPORT
#define VTK_ABI_HIDDEN
#endif

#include "vtkABINamespace.h"

/*--------------------------------------------------------------------------*/
/* If not already defined, define vtkTypeBool. When VTK was started, some   */
/* compilers did not yet support the bool type, and so VTK often used int,  */
/* or more rarely unsigned int, where it should have used bool.             */
/* Eventually vtkTypeBool will switch to real bool.                         */
#ifndef VTK_TYPE_BOOL_TYPEDEFED
#define VTK_TYPE_BOOL_TYPEDEFED
#if VTK_USE_FUTURE_BOOL
typedef bool vtkTypeBool;
typedef bool vtkTypeUBool;
#else
typedef int vtkTypeBool;
typedef unsigned int vtkTypeUBool;
#endif
#endif

#endif // vtkABI_h
// VTK-HeaderTest-Exclude: vtkABI.h
