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
// files.

#ifndef vtkSystemIncludes_h
#define vtkSystemIncludes_h

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

// Setup the basic types to be used by VTK.
#include "vtkType.h"

// this should be removed at some point
#define VTK_USE_EXECUTIVES

#define __VTK_SYSTEM_INCLUDES__INSIDE
#include "vtkOStreamWrapper.h"    // Include the ostream wrapper.
#include "vtkOStrStreamWrapper.h" // Include the ostrstream wrapper.
#undef __VTK_SYSTEM_INCLUDES__INSIDE

// Include generic stuff.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Borland C++ defines several of the stdlib.h and string.h symbols in
// sub-headers search.h and mem.h.  These sub-headers have using
// declarations to pull functions from the std namespace to the global
// namespace, but they are defined only if the header was not included
// through the C++-style cstdlib or cstring header.  These outer
// headers are included by the streams library in C++-style and
// include blockers are put in place that prevent including the
// C-style versions from ever including the sub-headers.  Therefore we
// have to include the sub-headers here to get the using declarations.
#if defined(__BORLANDC__)
# include <mem.h>    /* mem... functions from string.h */
# include <search.h> /* search functions from stdlib.h */
#endif

// These types define error codes for vtk functions
#define VTK_OK                 1
#define VTK_ERROR              2

// These types define different text properties
#define VTK_ARIAL          0
#define VTK_COURIER        1
#define VTK_TIMES          2
#define VTK_UNKNOWN_FONT   3
#define VTK_FONT_FILE      4

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
#define VTK_COLOR_MODE_DIRECT_SCALARS 2

// Constants for InterpolationType
#define VTK_NEAREST_INTERPOLATION       0
#define VTK_LINEAR_INTERPOLATION        1
#define VTK_CUBIC_INTERPOLATION         2

// Constants for SlabType
#define VTK_IMAGE_SLAB_MIN  0
#define VTK_IMAGE_SLAB_MAX  1
#define VTK_IMAGE_SLAB_MEAN 2
#define VTK_IMAGE_SLAB_SUM  3

// For volume rendering
#define VTK_MAX_VRCOMP                  4

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
#define VTK_THREAD_RETURN_TYPE vtkWindowsDWORD __stdcall
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
// VTK-HeaderTest-Exclude: vtkSystemIncludes.h
