/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOStreamFwd.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIOStreamFwd - Forward-declare C++ iostreams as used by VTK.
// .SECTION Description
// VTK supports both ANSI and old-style streams.  This header
// forward-declares the proper streams according to
// VTK_USE_ANSI_STDLIB.

#ifndef __vtkIOStreamFwd_h
#define __vtkIOStreamFwd_h

#include "vtkConfigure.h"

#ifdef VTK_USE_ANSI_STDLIB

#ifdef _MSC_VER
#pragma warning (push, 3)
#endif

// Forward-declare ansi streams.
# include <iosfwd>
using std::ios;
using std::streambuf;
using std::istream;
using std::ostream;
using std::iostream;
using std::filebuf;
using std::ifstream;
using std::ofstream;
using std::fstream;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else
# ifdef __BORLANDC__
#  include <iosfwd.h>
# else

// Forward-declare non-ansi streams.
class ios;
class streambuf;
class istream;
class ostream;
class iostream;
class filebuf;
class ifstream;
class ofstream;
class fstream;
# endif

#endif


#endif // __vtkIOStreamFwd_h
