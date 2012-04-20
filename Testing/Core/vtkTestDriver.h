/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestDriver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This header is included by all the C++ test drivers in VTK.
#ifndef _vtkTestDriver_h
#define _vtkTestDriver_h

// MSVC 6.0 warns about unused inline functions from <exception> and
// <crtdbg.h>.  Unfortunately this pragma has to be in effect at the
// end of the translation unit so we cannot push/pop it around the
// include.
#if defined(_MSC_VER) && (_MSC_VER < 1300)
# pragma warning (disable: 4514) /* unreferenced inline function */

// Temporary suppression of this warning from CMake generated TestDriver
// code until it can be fixed in the generator...
# pragma warning (disable: 4701) /* local variable 'x' may be used
                                    without having been initialized */

// Warnings that show up on vc6 dashboards since floating point
// exception handling was added to the test driver...
//
# pragma warning (disable: 4018) /* signed/unsigned mismatch */
# pragma warning (disable: 4146) /* unary minus operator applied unsigned */
# pragma warning (disable: 4284) /* return type for operator-> not a UDT */
# pragma warning (disable: 4290) /* C++ exception specification ignored */

#endif

#include <exception> // for std::exception
#include "vtkFloatingPointExceptions.h"

#include <clocale> // C setlocale()
#include <locale> // C++ locale

#endif
