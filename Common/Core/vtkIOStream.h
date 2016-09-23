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

#endif // vtkIOStream_h
// VTK-HeaderTest-Exclude: vtkIOStream.h
