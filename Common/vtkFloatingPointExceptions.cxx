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

#include "vtkFloatingPointExceptionsConfigure.h"

#if defined(VTK_USE_FENV)
#include <csignal>
#include <fenv.h>
#endif

#ifdef _MSC_VER
#include <float.h>
#endif

#if defined(VTK_USE_FENV)
//-----------------------------------------------------------------------------
// Signal handler for floating point exceptions in anonymous namespace
namespace {

void signal_handler(int signal)
{
  cerr << "Error: Floating point exception detected. Signal " << signal << endl;
  // This should possibly throw an exception rather than abort, abort should
  // at least give access to the stack when it fails here.
  abort();
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
#if defined(VTK_USE_FENV)
  // This should work on all platforms
  feenableexcept(FE_DIVBYZERO | FE_INVALID);
  // Set the signal handler
  signal(SIGFPE, signal_handler);
#endif
}

//-----------------------------------------------------------------------------
// Description:
// Disable floating point exceptions.
void vtkFloatingPointExceptions::Disable()
{
#ifdef _MSC_VER
  // disable floating point exceptions on MSVC
  _controlfp(_EM_INVALID | _EM_DENORMAL | _EM_ZERODIVIDE | _EM_OVERFLOW |
             _EM_UNDERFLOW | _EM_INEXACT, _MCW_EM);
#endif  //_MSC_VER
#if defined(VTK_USE_FENV)
  fedisableexcept(FE_DIVBYZERO | FE_INVALID);
#endif
}
