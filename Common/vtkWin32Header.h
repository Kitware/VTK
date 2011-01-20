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
// .NAME vtkWin32Header - manage Windows system differences
// .SECTION Description
// The vtkWin32Header captures some system differences between Unix and
// Windows operating systems.

#ifndef __vtkWIN32Header_h
#define __vtkWIN32Header_h

#ifndef __VTK_SYSTEM_INCLUDES__INSIDE
Do_not_include_vtkWin32Header_directly__vtkSystemIncludes_includes_it;
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
#if defined(_WIN32) || defined(WIN32)

// define strict header for windows
#ifndef STRICT
#define STRICT
#endif

#ifdef VTK_USE_ANSI_STDLIB
#ifndef NOMINMAX
#define NOMINMAX
#endif
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
#if !defined(__WRAP__)
# define VTK_WORKAROUND_WINDOWS_MANGLE
#endif

#if ( _MSC_VER >= 1300 ) // Visual studio .NET
#pragma warning ( disable : 4311 )
#pragma warning ( disable : 4312 )
#  define vtkGetWindowLong GetWindowLongPtr
#  define vtkSetWindowLong SetWindowLongPtr
#  define vtkLONG LONG_PTR
#  define vtkGWL_WNDPROC GWLP_WNDPROC
#  define vtkGWL_HINSTANCE GWLP_HINSTANCE
#  define vtkGWL_USERDATA GWLP_USERDATA
#else // older or non-Visual studio
#  define vtkGetWindowLong GetWindowLong
#  define vtkSetWindowLong SetWindowLong
#  define vtkLONG LONG
#  define vtkGWL_WNDPROC GWL_WNDPROC
#  define vtkGWL_HINSTANCE GWL_HINSTANCE
#  define vtkGWL_USERDATA GWL_USERDATA
#endif //

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

// MSVC 6.0 in release mode will warn about code it produces with its
// optimizer.  Disable the warnings specifically for this
// configuration.  Real warnings will be revealed by a debug build or
// by other compilers.
#if defined(_MSC_VER) && (_MSC_VER < 1300) && defined(NDEBUG)
# pragma warning ( disable : 4701 ) /* Variable may be used uninitialized.  */
# pragma warning ( disable : 4702 ) /* Unreachable code.  */
#endif

#if defined(__BORLANDC__)
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

// Now set up all of the export macros for the VTK kits
#if defined(VTK_BUILD_SHARED_LIBS)

 #define VTK_EXPORT VTK_ABI_EXPORT

 #if defined(vtkCommon_EXPORTS)
  #define VTK_COMMON_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_COMMON_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkFiltering_EXPORTS)
  #define VTK_FILTERING_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_FILTERING_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkImaging_EXPORTS)
  #define VTK_IMAGING_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_IMAGING_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkGenericFiltering_EXPORTS)
  #define VTK_GENERIC_FILTERING_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_GENERIC_FILTERING_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkGeovis_EXPORTS)
  #define VTK_GEOVIS_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_GEOVIS_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkGraphics_EXPORTS)
  #define VTK_GRAPHICS_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_GRAPHICS_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkGraphicsJava_EXPORTS)
  #define VTK_GRAPHICS_JAVA_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_GRAPHICS_JAVA_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkInfovis_EXPORTS)
  #define VTK_INFOVIS_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_INFOVIS_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkIO_EXPORTS)
  #define VTK_IO_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_IO_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkRendering_EXPORTS)
  #define VTK_RENDERING_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_RENDERING_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkTextAnalysis_EXPORTS)
  #define VTK_TEXT_ANALYSIS_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_TEXT_ANALYSIS_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkVolumeRendering_EXPORTS)
  #define VTK_VOLUMERENDERING_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_VOLUMERENDERING_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkHybrid_EXPORTS)
  #define VTK_HYBRID_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_HYBRID_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkWidgets_EXPORTS)
  #define VTK_WIDGETS_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_WIDGETS_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkParallel_EXPORTS)
  #define VTK_PARALLEL_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_PARALLEL_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkViews_EXPORTS)
  #define VTK_VIEWS_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_VIEWS_EXPORT VTK_ABI_IMPORT
 #endif

 #if defined(vtkCharts_EXPORTS)
  #define VTK_CHARTS_EXPORT VTK_ABI_EXPORT
 #else
  #define VTK_CHARTS_EXPORT VTK_ABI_IMPORT
 #endif

#else
 #define VTK_COMMON_EXPORT
 #define VTK_FILTERING_EXPORT
 #define VTK_GENERIC_FILTERING_EXPORT
 #define VTK_GEOVIS_EXPORT
 #define VTK_GRAPHICS_EXPORT
 #define VTK_GRAPHICS_JAVA_EXPORT
 #define VTK_IMAGING_EXPORT
 #define VTK_INFOVIS_EXPORT
 #define VTK_IO_EXPORT
 #define VTK_RENDERING_EXPORT
 #define VTK_TEXT_ANALYSIS_EXPORT
 #define VTK_VOLUMERENDERING_EXPORT
 #define VTK_HYBRID_EXPORT
 #define VTK_WIDGETS_EXPORT
 #define VTK_PARALLEL_EXPORT
 #define VTK_VIEWS_EXPORT
 #define VTK_CHARTS_EXPORT
 #define VTK_EXPORT
#endif

// this is exclusively for the tcl Init functions
#define VTK_TK_EXPORT VTK_ABI_EXPORT

#endif
