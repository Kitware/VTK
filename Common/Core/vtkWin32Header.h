/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32Header.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWin32Header
 * @brief   manage Windows system differences
 *
 * The vtkWin32Header captures some system differences between Unix and
 * Windows operating systems.
*/

#ifndef vtkWin32Header_h
#define vtkWin32Header_h

#ifndef VTK_SYSTEM_INCLUDES_INSIDE
Do_not_include_vtkWin32Header_directly_vtkSystemIncludes_includes_it;
#endif

#include "vtkConfigure.h"
#include "vtkABI.h"

/*
 * This is a support for files on the disk that are larger than 2GB.
 * Since this is the first place that any include should happen, do this here.
 */
#ifdef VTK_REQUIRE_LARGE_FILE_SUPPORT
#  ifndef _LARGEFILE_SOURCE
#    define _LARGEFILE_SOURCE
#  endif
#  ifndef _LARGE_FILES
#    define _LARGE_FILES
#  endif
#  ifndef _FILE_OFFSET_BITS
#    define _FILE_OFFSET_BITS 64
#  endif
#endif

//
// Windows specific stuff------------------------------------------
#if defined(_WIN32)

// define strict header for windows
#ifndef STRICT
#define STRICT
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#endif

// Never include the windows header here when building VTK itself.
#if defined(VTK_IN_VTK)
# undef VTK_INCLUDE_WINDOWS_H
#endif

#if defined(_WIN32)
  // Include the windows header here only if requested by user code.
# if defined(VTK_INCLUDE_WINDOWS_H)
#  include <windows.h>
   // Define types from the windows header file.
   typedef DWORD vtkWindowsDWORD;
   typedef PVOID vtkWindowsPVOID;
   typedef LPVOID vtkWindowsLPVOID;
   typedef HANDLE vtkWindowsHANDLE;
   typedef LPTHREAD_START_ROUTINE vtkWindowsLPTHREAD_START_ROUTINE;
# else
   // Define types from the windows header file.
   typedef unsigned long vtkWindowsDWORD;
   typedef void* vtkWindowsPVOID;
   typedef vtkWindowsPVOID vtkWindowsLPVOID;
   typedef vtkWindowsPVOID vtkWindowsHANDLE;
   typedef vtkWindowsDWORD (__stdcall *vtkWindowsLPTHREAD_START_ROUTINE)(vtkWindowsLPVOID);
# endif
  // Enable workaround for windows header name mangling.
  // See VTK/Utilities/Upgrading/README.WindowsMangling.txt for details.
#if !defined(__VTK_WRAP__) && !defined(__WRAP_GCCXML__)
# define VTK_WORKAROUND_WINDOWS_MANGLE
#endif

#if defined(_MSC_VER) // Visual studio
#pragma warning ( disable : 4311 )
#pragma warning ( disable : 4312 )
#endif

#define vtkGetWindowLong GetWindowLongPtr
#define vtkSetWindowLong SetWindowLongPtr
#define vtkLONG LONG_PTR
#define vtkGWL_WNDPROC GWLP_WNDPROC
#define vtkGWL_HINSTANCE GWLP_HINSTANCE
#define vtkGWL_USERDATA GWLP_USERDATA

#endif

#if defined(_MSC_VER)
  // Enable MSVC compiler warning messages that are useful but off by default.
# pragma warning ( default : 4263 ) /* no override, call convention differs */
  // Disable MSVC compiler warning messages that often occur in valid code.
# if !defined(VTK_DISPLAY_WIN32_WARNINGS)
#  pragma warning ( disable : 4003 ) /* not enough actual parameters for macro */
#  pragma warning ( disable : 4097 ) /* typedef is synonym for class */
#  pragma warning ( disable : 4127 ) /* conditional expression is constant */
#  pragma warning ( disable : 4244 ) /* possible loss in conversion */
#  pragma warning ( disable : 4251 ) /* missing DLL-interface */
#  pragma warning ( disable : 4305 ) /* truncation from type1 to type2 */
#  pragma warning ( disable : 4309 ) /* truncation of constant value */
#  pragma warning ( disable : 4514 ) /* unreferenced inline function */
#  pragma warning ( disable : 4706 ) /* assignment in conditional expression */
#  pragma warning ( disable : 4710 ) /* function not inlined */
#  pragma warning ( disable : 4786 ) /* identifier truncated in debug info */
# endif
#endif

#if defined(__BORLANDC__) && (__BORLANDC__ < 0x660)
  // Disable Borland compiler warning messages that often occur in valid code.
# if !defined(VTK_DISPLAY_WIN32_WARNINGS)
#  pragma warn -8004 /* assigned a value that is never used */
#  pragma warn -8008 /* condition is always false */
#  pragma warn -8026 /* funcs w/class-by-value args not expanded inline */
#  pragma warn -8027 /* functions w/ do/for/while not expanded inline */
#  pragma warn -8060 /* possibly incorrect assignment */
#  pragma warn -8066 /* unreachable code */
#  pragma warn -8072 /* suspicious pointer arithmetic */
# endif
#endif

// Now set up the generic VTK export macro.
#if defined(VTK_BUILD_SHARED_LIBS)
# define VTK_EXPORT VTK_ABI_EXPORT
#else
# define VTK_EXPORT
#endif

#endif
// VTK-HeaderTest-Exclude: vtkWin32Header.h
