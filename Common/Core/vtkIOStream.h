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
/**
 * @class   vtkIOStream
 * @brief   Include C++ iostreams as used by VTK.
 *
 * This header includes the proper streams.
 */

#ifndef vtkIOStream_h
#define vtkIOStream_h

#include "vtkConfigure.h"

#ifdef _MSC_VER
#pragma warning(push, 3)
#endif

#include <fstream>  // Include real ansi ifstream and ofstream.
#include <iomanip>  // Include real ansi io manipulators.
#include <iostream> // Include real ansi istream and ostream.

// Need these in global namespace so the same code will work with ansi
// and old-style streams.
using std::cerr;
using std::cin;
using std::cout;
using std::dec;
using std::endl;
using std::ends;
using std::fstream;
using std::hex;
using std::ios;
using std::istream;
using std::ostream;
using std::setfill;
using std::setprecision;
using std::setw;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // vtkIOStream_h
// VTK-HeaderTest-Exclude: vtkIOStream.h
