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
// This header includes the proper streams.

#ifndef vtkIOStream_h
#define vtkIOStream_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkConfigure.h"

#define __VTK_SYSTEM_INCLUDES__INSIDE
#include "vtkWin32Header.h" // For export macros.
#undef __VTK_SYSTEM_INCLUDES__INSIDE

#ifdef _MSC_VER
#pragma warning (push, 3)
#endif

#include <iostream>  // Include real ansi istream and ostream.
#include <fstream>   // Include real ansi ifstream and ofstream.
#include <iomanip>   // Include real ansi io manipulators.

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

#if defined(VTK_IOSTREAM_NEED_OPERATORS_LL)

# if !defined(VTK_ISTREAM_SUPPORTS_LONG_LONG)
VTKCOMMONCORE_EXPORT istream& vtkIOStreamScan(istream&, vtkIOStreamSLL&);
#  if !defined(VTK_DO_NOT_DEFINE_ISTREAM_SLL)
inline istream& operator >> (istream& is, vtkIOStreamSLL& value)
{
  return vtkIOStreamScan(is, value);
}
#  endif

VTKCOMMONCORE_EXPORT istream& vtkIOStreamScan(istream&, vtkIOStreamULL&);
#  if !defined(VTK_DO_NOT_DEFINE_ISTREAM_ULL)
inline istream& operator >> (istream& is, vtkIOStreamULL& value)
{
  return vtkIOStreamScan(is, value);
}
#  endif
# endif

# if !defined(VTK_OSTREAM_SUPPORTS_LONG_LONG)
VTKCOMMONCORE_EXPORT ostream& vtkIOStreamPrint(ostream&, vtkIOStreamSLL);
#  if !defined(VTK_DO_NOT_DEFINE_OSTREAM_SLL)
inline ostream& operator << (ostream& os, vtkIOStreamSLL value)
{
  return vtkIOStreamPrint(os, value);
}
#  endif

VTKCOMMONCORE_EXPORT ostream& vtkIOStreamPrint(ostream&, vtkIOStreamULL);
#  if !defined(VTK_DO_NOT_DEFINE_OSTREAM_ULL)
inline ostream& operator << (ostream& os, vtkIOStreamULL value)
{
  return vtkIOStreamPrint(os, value);
}
#  endif
# endif

#endif

#endif // vtkIOStream_h
// VTK-HeaderTest-Exclude: vtkIOStream.h
