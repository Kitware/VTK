/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOStream.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIOStream - Include C++ iostreams as used by VTK.
// .SECTION Description
// VTK supports both ANSI and old-style streams.  This header includes
// the proper streams according to VTK_USE_ANSI_STDLIB.

#ifndef __vtkIOStream_h
#define __vtkIOStream_h

#include "vtkConfigure.h"

#define __VTK_SYSTEM_INCLUDES__INSIDE
#include "vtkWin32Header.h" // For export macros.
#undef __VTK_SYSTEM_INCLUDES__INSIDE

#ifdef VTK_USE_ANSI_STDLIB

#ifdef _MSC_VER
#pragma warning (push, 3)
#endif

# include <iostream>  // Include real ansi istream and ostream.
# include <fstream>   // Include real ansi ifstream and ofstream.
# include <iomanip>   // Include real ansi io manipulators.

// Need these in global namespace so the same code will work with ansi
// and old-style streams.
using std::dec;
using std::hex;
using std::setw;
using std::setfill;
using std::setprecision;
using std::cerr;
using std::cout;
using std::cin;
using std::ios;
using std::endl;
using std::ends;
using std::ostream;
using std::istream;
using std::ofstream;
using std::ifstream;
using std::fstream;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else

// Include old-style streams.
# ifdef _WIN32_WCE
#  include "vtkWinCE.h"   // Include mini-streams for Windows CE.
# else
#  include <iostream.h>   // Include old-style istream and ostream.
#  include <iomanip.h>
#  include <fstream.h>    // Include old-style ifstream and ofstream.
# endif
#endif

//----------------------------------------------------------------------------
// Include deprecated strstream headers? If including file has explicitly
// defined VTK_INCLUDE_STRSTREAM_HEADERS or VTK_EXCLUDE_STRSTREAM_HEADERS,
// then honor the setting. Otherwise, use the legacy defines to figure out
// if the deprecated strstream headers should be included...
//
// To prevent VTK from including the strstream headers, define
// VTK_EXCLUDE_STRSTREAM_HEADERS before including any VTK header files
// or set the VTK_LEGACY_REMOVE CMake option to ON.
//
// Clients may include <vtksys/ios/sstream> directly in their code and should
// prefer ostringstream and istringstream over ostrstream and istrstream.
//
#if !defined(VTK_INCLUDE_STRSTREAM_HEADERS)
# if defined(VTK_LEGACY_REMOVE)
// Remove legacy code - do NOT include the strstream headers
# elif defined(VTK_LEGACY_SILENT)
// Silently still include legacy code - DO include the strstream headers
#  define VTK_INCLUDE_STRSTREAM_HEADERS
# else
// Default case - DO include the strstream headers for strict
// backwards compatibility unless client has explicitly defined
// VTK_EXCLUDE_STRSTREAM_HEADERS.
#  if !defined(VTK_EXCLUDE_STRSTREAM_HEADERS)
#   define VTK_INCLUDE_STRSTREAM_HEADERS
#  endif
# endif
#endif

#if defined(VTK_INCLUDE_STRSTREAM_HEADERS)

# ifdef VTK_USE_ANSI_STDLIB
// Include real ansi strstreams.

#  ifdef _MSC_VER
#   pragma warning(push, 3)
#  endif

#  include <strstream>

using std::ostrstream;
using std::istrstream;
using std::strstream;

#  ifdef _MSC_VER
#   pragma warning(pop)
#  endif

# else
// Include old-style streams.

# ifndef _WIN32_WCE
#  if defined(_MSC_VER)
#   include <strstrea.h>  // Include old-style strstream from MSVC.
#  else
#   include <strstream.h> // Include old-style strstream.
#  endif
# endif

# endif

#endif

//----------------------------------------------------------------------------
#if defined(VTK_IOSTREAM_NEED_OPERATORS_LL)

# if !defined(VTK_ISTREAM_SUPPORTS_LONG_LONG)
VTK_COMMON_EXPORT istream& vtkIOStreamScan(istream&, vtkIOStreamSLL&);
#  if !defined(VTK_DO_NOT_DEFINE_ISTREAM_SLL)
inline istream& operator >> (istream& is, vtkIOStreamSLL& value)
{
  return vtkIOStreamScan(is, value);
}
#  endif

VTK_COMMON_EXPORT istream& vtkIOStreamScan(istream&, vtkIOStreamULL&);
#  if !defined(VTK_DO_NOT_DEFINE_ISTREAM_ULL)
inline istream& operator >> (istream& is, vtkIOStreamULL& value)
{
  return vtkIOStreamScan(is, value);
}
#  endif
# endif

# if !defined(VTK_OSTREAM_SUPPORTS_LONG_LONG)
VTK_COMMON_EXPORT ostream& vtkIOStreamPrint(ostream&, vtkIOStreamSLL);
#  if !defined(VTK_DO_NOT_DEFINE_OSTREAM_SLL)
inline ostream& operator << (ostream& os, vtkIOStreamSLL value)
{
  return vtkIOStreamPrint(os, value);
}
#  endif

VTK_COMMON_EXPORT ostream& vtkIOStreamPrint(ostream&, vtkIOStreamULL);
#  if !defined(VTK_DO_NOT_DEFINE_OSTREAM_ULL)
inline ostream& operator << (ostream& os, vtkIOStreamULL value)
{
  return vtkIOStreamPrint(os, value);
}
#  endif
# endif

#endif

#endif // __vtkIOStream_h
