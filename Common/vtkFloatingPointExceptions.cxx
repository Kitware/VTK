/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatingPointExceptions.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFloatingPointExceptions.h"

#ifdef _MSC_VER
#include <float.h>
#endif

//-----------------------------------------------------------------------------
// Description:
// Enable floating point exceptions.
void vtkFloatingPointExceptions::Enable()
{
#ifdef _MSC_VER
  // enable floating point exceptions on MSVC
  _controlfp(_EM_DENORMAL | _EM_UNDERFLOW | _EM_INEXACT, _MCW_EM);
#endif  //_MSC_VER
#if defined(__linux__) && defined(__i386__)
  // This only works on linux x86
  unsigned int fpucw= 0x1372;
  __asm__ ("fldcw %0" : : "m" (fpucw));
#endif  //__linux__
}
