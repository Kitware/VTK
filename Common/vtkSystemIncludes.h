/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSystemIncludes.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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
// functionality). A compile-time flag (VTK_USE_ANSI_STDLIB) must be
// set to enable ANSI C++ compliance. You'll probably also need to set
// various compiler flags. For example, on WIndows for ANSI C++, use
// /D "VTK_USE_ANSI_STDLIB" /GX /Zm1000.

#ifndef __vtkSystemIncludes_h
#define __vtkSystemIncludes_h

/* first include the local configuration for this machine */
#define __VTK_SYSTEM_INCLUDES__INSIDE
#include "vtkWin32Header.h"
#undef __VTK_SYSTEM_INCLUDES__INSIDE

// include  generic stuff 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Handle changes for ANSI C++ ---------------------------------------------
#ifdef VTK_USE_ANSI_STDLIB
#include <iostream>
#include <strstream>
#include <fstream>
using std::cerr;
using std::cout;
using std::cin;
using std::ios;
using std::endl;
using std::ends;
using std::ostream;
using std::istream;
using std::ostrstream;
using std::istrstream;
using std::strstream;
using std::ofstream;
using std::ifstream;
using std::fstream;

// otherwise, non-ANSI -----------------------------------------------------
#else
#ifdef _WIN32_WCE
  #include "vtkWinCE.h"
#else
  #include <iostream.h>
  #if defined(_MSC_VER)
    #include <strstrea.h>
  #else
    #include <strstream.h>
  #endif
  #include <fstream.h>
#endif // Win CE
#endif 

#define VTK_HAS_ID_TYPE
#ifdef VTK_USE_64BIT_IDS
#  define VTK_ID_TYPE_IS_NOT_BASIC_TYPE
#  ifdef _WIN32
typedef __int64 vtkIdType;

/* ostream operator for __int64 */
inline ostream& __cdecl operator<<(ostream& _O, __int64 i64Val)
{
  wchar_t wchBuf[32];
  if (i64Val < 0)
  {
    _O << char("-");
    i64Val *= -1;
  }
  return (_O << _i64tow(i64Val, wchBuf, 10));
};

#  else // _WIN32
typedef long long vtkIdType;
#  endif // _WIN32
#else // VTK_USE_64BIT_IDS
typedef int vtkIdType;
#endif // VTK_USE_64BIT_IDS

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
#define VTK_DOUBLE_MIN        -1.0e+99L
#define VTK_DOUBLE_MAX         1.0e+99L

// These types are returned to distinguish data object types
#define VTK_POLY_DATA          0
#define VTK_STRUCTURED_POINTS  1
#define VTK_STRUCTURED_GRID    2
#define VTK_RECTILINEAR_GRID   3
#define VTK_UNSTRUCTURED_GRID  4
#define VTK_PIECEWISE_FUNCTION 5
#define VTK_IMAGE_DATA         6
#define VTK_DATA_OBJECT        7
#define VTK_DATA_SET           8

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

// 

#endif

