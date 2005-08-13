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
#endif

#include <vtkstd/exception> // for vtkstd::exception

#endif
