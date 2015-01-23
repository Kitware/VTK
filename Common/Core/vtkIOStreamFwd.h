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
// This header forward-declares the proper streams.

#ifndef vtkIOStreamFwd_h
#define vtkIOStreamFwd_h

#include "vtkConfigure.h"

#ifdef _MSC_VER
#pragma warning (push, 3)
#endif

// Forward-declare ansi streams.
#include <iosfwd>
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

#endif // vtkIOStreamFwd_h
// VTK-HeaderTest-Exclude: vtkIOStreamFwd.h
