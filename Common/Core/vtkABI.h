/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkABI.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkABI - manage macros for exporting symbols in the binary interface.
// .SECTION Description
// This header defines the macros for importing and exporting symbols in shared
// objects (DLLs, etc). All VTK headers should use these macros to define the
// kit specific import/export macros. So for the vtkCommon kit this might be,
//
// \code
// #include "vtkABI.h"
//
// #if defined(VTK_BUILD_SHARED_LIBS)
// # if defined(vtkCommon_EXPORTS)
// #  define VTK_COMMON_EXPORT VTK_ABI_EXPORT
// # else
// #  define VTK_COMMON_EXPORT VTK_ABI_IMPORT
// # endif
// #else
// # define VTK_COMMON_EXPORT
// #endif
// \endcode
//
// See http://gcc.gnu.org/wiki/Visibility for a discussion of the symbol
// visibility support in GCC. The project must pass extra CFLAGS/CXXFLAGS in
// order to change the default symbol visibility when using GCC. Using GCC 4.2
// or later is recommended when using visibility supoprt, the attribute was
// added in GCC 4.0. Currently hidden is not used, but it can be used to
// explicitly hide symbols from external linkage.

#ifndef __vtkABI_h
#define __vtkABI_h

#if defined(_WIN32)
# define VTK_ABI_IMPORT __declspec(dllimport)
# define VTK_ABI_EXPORT __declspec(dllexport)
# define VTK_ABI_HIDDEN
#elif __GNUC__ >= 4
# define VTK_ABI_IMPORT __attribute__ ((visibility("default")))
# define VTK_ABI_EXPORT __attribute__ ((visibility("default")))
# define VTK_ABI_HIDDEN __attribute__ ((visibility("hidden")))
#else
# define VTK_ABI_IMPORT
# define VTK_ABI_EXPORT
# define VTK_ABI_HIDDEN
#endif

#endif // __vtkABI_h
// VTK-HeaderTest-Exclude: vtkABI.h
