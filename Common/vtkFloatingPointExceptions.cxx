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

#if defined(__linux__)
#include <csignal>
#include <fenv.h>
#endif

#ifdef _MSC_VER
#include <float.h>
#endif

#if defined(__linux__)
//-----------------------------------------------------------------------------
// Signal handler for floating point exceptions in anonymous namespace
namespace {

void signal_handler(int signal)
{
  cerr << "Error: Floating point error detected.\n";
  // This should possibly throw an exception rather than exit.
  exit(1);
}

} // End anonymous namespace
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
#if defined(__linux__)
  // This should work on all platforms
  feenableexcept(FE_DIVBYZERO | FE_INVALID);
  // Set the signal handler
  signal(SIGFPE, signal_handler);
  // This only works on linux x86
//  unsigned int fpucw= 0x1372;
//  __asm__ ("fldcw %0" : : "m" (fpucw));
#endif  //__linux__
}

