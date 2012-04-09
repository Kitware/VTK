/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatingPointExceptions.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFloatingPointExceptions - Deal with floating-point exceptions
// .SECTION Description
// Right now it is really basic and it only provides a function to enable
// floating point exceptions on some compilers.
// Note that Borland C++ has floating-point exceptions by default, not
// Visual studio nor gcc. It is mainly use to optionally enable floating
// point exceptions in the C++ tests.

#ifndef __vtkFloatingPointExceptions_h
#define __vtkFloatingPointExceptions_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h" // For VTKCOMMONCORE_EXPORT

class VTKCOMMONCORE_EXPORT vtkFloatingPointExceptions
{
public:
  // Description:
  // Enable floating point exceptions.
  static void Enable();

  // Description:
  // Disable floating point exceptions.
  static void Disable();

private:
  vtkFloatingPointExceptions(); // Not implemented.
  vtkFloatingPointExceptions(const vtkFloatingPointExceptions&);  // Not implemented.
  void operator=(const vtkFloatingPointExceptions&);  // Not implemented.
};

#endif
// VTK-HeaderTest-Exclude: vtkFloatingPointExceptions.h
