/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSystemIncludes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSystemIncludes - transition VTK to ANSI C++, centralize
// inclusion of system files
// .SECTION Description
// The vtkSystemIncludes centralizes the inclusion of system include
// files. (This is particularly important as VTK moves towards ANSI
// C++.)  For example, this include file enables user's to build VTK
// with STL (i.e., use std::ostream and other standard ANSI C++
// functionality).  A configured flag in vtkConfigure.h
// (VTK_USE_ANSI_STDLIB) must be set to enable ANSI C++ compliance.

#ifndef __vtkSystemIncludes_h
#define __vtkSystemIncludes_h

/* first include the local configuration for this machine */
#define __VTK_SYSTEM_INCLUDES__INSIDE
#include "vtkWin32Header.h"
#undef __VTK_SYSTEM_INCLUDES__INSIDE

// The language wrapper files do not need the real streams.  They
// define VTK_STREAMS_FWD_ONLY so that the streams are only
// forward-declared.  This significantly improves compile time on some
// platforms.
#if defined(VTK_STREAMS_FWD_ONLY)
# include "vtkIOStreamFwd.h" // Forward-declare the C++ streams.
#else
# include "vtkIOStream.h"    // Include the real C++ streams.
#endif

#define __VTK_SYSTEM_INCLUDES__INSIDE
#include "vtkIdType.h"            // Define vtkIdType and its stream operators.
#include "vtkOStreamWrapper.h"    // Include the ostream wrapper.
#include "vtkOStrStreamWrapper.h" // Include the ostrstream wrapper.
#undef __VTK_SYSTEM_INCLUDES__INSIDE

// Include generic stuff.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Some constants used throughout the code
#define VTK_LARGE_FLOAT 1.0e+38F
#ifdef VTK_USE_64BIT_IDS
#  ifdef _WIN32
#    define VTK_LARGE_ID 9223372036854775807i64 // 2^63 - 1
#  else
#    define VTK_LARGE_ID 9223372036854775807LL // 2^63 - 1
#  endif
#else
#  define VTK_LARGE_ID 2147483647 // 2^31 - 1
#endif

#define VTK_LARGE_INTEGER 2147483647 // 2^31 - 1

// These types are returned by GetDataType to indicate pixel type.
#define VTK_VOID            0
#define VTK_BIT             1 
#define VTK_CHAR            2
#define VTK_UNSIGNED_CHAR   3
#define VTK_SHORT           4
#define VTK_UNSIGNED_SHORT  5
#define VTK_INT             6
#define VTK_UNSIGNED_INT    7
#define VTK_LONG            8
#define VTK_UNSIGNED_LONG   9
#define VTK_FLOAT          10
#define VTK_DOUBLE         11 
#define VTK_ID_TYPE        12

// These types are not currently supported by GetDataType, but are 
// for completeness.
#define VTK_STRING         13
#define VTK_OPAQUE         14

// Some constant required for correct template performance
#define VTK_BIT_MIN            0
#define VTK_BIT_MAX            1
#define VTK_CHAR_MIN          -128
#define VTK_CHAR_MAX           127
#define VTK_UNSIGNED_CHAR_MIN  0
#define VTK_UNSIGNED_CHAR_MAX  255
#define VTK_SHORT_MIN         -32768
#define VTK_SHORT_MAX          32767
#define VTK_UNSIGNED_SHORT_MIN 0
#define VTK_UNSIGNED_SHORT_MAX 65535
#define VTK_INT_MIN          (-VTK_LARGE_INTEGER-1)
#define VTK_INT_MAX            VTK_LARGE_INTEGER
#define VTK_UNSIGNED_INT_MIN   0
#define VTK_UNSIGNED_INT_MAX   4294967295UL
#define VTK_LONG_MIN         (-VTK_LARGE_INTEGER-1)
#define VTK_LONG_MAX           VTK_LARGE_INTEGER
#define VTK_UNSIGNED_LONG_MIN  0
#define VTK_UNSIGNED_LONG_MAX  4294967295UL
#define VTK_FLOAT_MIN         -VTK_LARGE_FLOAT
#define VTK_FLOAT_MAX          VTK_LARGE_FLOAT
#define VTK_DOUBLE_MIN        -1.0e+299L
#define VTK_DOUBLE_MAX         1.0e+299L

// These types are returned to distinguish data object types
#define VTK_POLY_DATA                       0
#define VTK_STRUCTURED_POINTS               1
#define VTK_STRUCTURED_GRID                 2
#define VTK_RECTILINEAR_GRID                3
#define VTK_UNSTRUCTURED_GRID               4
#define VTK_PIECEWISE_FUNCTION              5
#define VTK_IMAGE_DATA                      6
#define VTK_DATA_OBJECT                     7
#define VTK_DATA_SET                        8
#define VTK_POINT_SET                       9
#define VTK_UNIFORM_GRID                   10
#define VTK_COMPOSITE_DATA_SET             11
#define VTK_HIERARCHICAL_DATA_SET          12
#define VTK_MULTI_BLOCK_DATA_SET           13
#define VTK_HIERARCHICAL_BOX_DATA_SET      14

// These types define error codes for vtk functions
#define VTK_OK                 1
#define VTK_ERROR              2

// These types define different text properties
#define VTK_ARIAL     0
#define VTK_COURIER   1
#define VTK_TIMES     2

#define VTK_TEXT_LEFT     0
#define VTK_TEXT_CENTERED 1
#define VTK_TEXT_RIGHT    2

#define VTK_TEXT_BOTTOM 0
#define VTK_TEXT_TOP    2

#define VTK_TEXT_GLOBAL_ANTIALIASING_SOME 0
#define VTK_TEXT_GLOBAL_ANTIALIASING_NONE 1
#define VTK_TEXT_GLOBAL_ANTIALIASING_ALL 2

#define VTK_LUMINANCE       1
#define VTK_LUMINANCE_ALPHA 2
#define VTK_RGB             3
#define VTK_RGBA            4

#define VTK_COLOR_MODE_DEFAULT 0
#define VTK_COLOR_MODE_MAP_SCALARS 1

// Constants for InterpolationType
#define VTK_NEAREST_INTERPOLATION       0
#define VTK_LINEAR_INTERPOLATION        1

// For volume rendering
#define VTK_MAX_VRCOMP                  4

// For multithreading

// The maximum number of threads allowed
#ifdef VTK_USE_SPROC
#define VTK_MAX_THREADS              32
#endif

#ifdef VTK_USE_PTHREADS
#define VTK_MAX_THREADS              32
#endif

#ifdef VTK_USE_WIN32_THREADS
#define VTK_MAX_THREADS              8
#endif

#ifndef VTK_USE_WIN32_THREADS
#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
#define VTK_MAX_THREADS              1
#endif
#endif
#endif

// If VTK_USE_PTHREADS is defined, then the multithreaded
// function is of type void *, and returns NULL
// Otherwise the type is void which is correct for WIN32
// and SPROC
#ifdef VTK_USE_PTHREADS
#define VTK_THREAD_RETURN_VALUE  NULL
#define VTK_THREAD_RETURN_TYPE   void *
#endif

#ifdef VTK_USE_WIN32_THREADS
#define VTK_THREAD_RETURN_VALUE 0
#define VTK_THREAD_RETURN_TYPE DWORD __stdcall
#endif

#if !defined(VTK_USE_PTHREADS) && !defined(VTK_USE_WIN32_THREADS)
#define VTK_THREAD_RETURN_VALUE
#define VTK_THREAD_RETURN_TYPE void
#endif

// For encoding

#define VTK_ENCODING_NONE         0 // to specify that no encoding should occur
#define VTK_ENCODING_US_ASCII     1
#define VTK_ENCODING_UNICODE      2
#define VTK_ENCODING_UTF_8        3
#define VTK_ENCODING_ISO_8859_1   4
#define VTK_ENCODING_ISO_8859_2   5
#define VTK_ENCODING_ISO_8859_3   6
#define VTK_ENCODING_ISO_8859_4   7
#define VTK_ENCODING_ISO_8859_5   8
#define VTK_ENCODING_ISO_8859_6   9
#define VTK_ENCODING_ISO_8859_7   10
#define VTK_ENCODING_ISO_8859_8   11
#define VTK_ENCODING_ISO_8859_9   12
#define VTK_ENCODING_ISO_8859_10  13
#define VTK_ENCODING_ISO_8859_11  14
#define VTK_ENCODING_ISO_8859_12  15
#define VTK_ENCODING_ISO_8859_13  16
#define VTK_ENCODING_ISO_8859_14  17
#define VTK_ENCODING_ISO_8859_15  18
#define VTK_ENCODING_ISO_8859_16  19
#define VTK_ENCODING_UNKNOWN      20  // leave this one at the end

#endif
