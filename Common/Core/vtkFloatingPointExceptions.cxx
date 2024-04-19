// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFloatingPointExceptions.h"

#include "vtkFloatingPointExceptionsConfigure.h"

#if defined(VTK_USE_FENV)
#include <csignal>
#include <cstdio>
#include <fenv.h>
#endif

// for _controlfp
#ifdef _MSC_VER
#include <float.h>
#endif

#if defined(VTK_USE_FENV)

#define signal_handler VTK_ABI_NAMESPACE_MANGLE(signal_handler)
extern "C" void signal_handler(int signal)
{
  // NOLINTNEXTLINE(bugprone-signal-handler)
  fprintf(stderr, "Error: Floating point exception detected. Signal %d\n", signal);
  // Call `abort()` so that a backtrace is created. We already broke signal
  // handler rules by calling `fprintf`, so any kind of "recovery" is
  // ill-advised.
  abort();
}

#endif

//------------------------------------------------------------------------------
// Description:
// Enable floating point exceptions.
VTK_ABI_NAMESPACE_BEGIN
void vtkFloatingPointExceptions::Enable()
{
#ifdef _MSC_VER
  // enable floating point exceptions on MSVC
  _controlfp(_EM_DENORMAL | _EM_UNDERFLOW | _EM_INEXACT, _MCW_EM);
#endif //_MSC_VER
#if defined(VTK_USE_FENV) && FE_ALL_EXCEPT != 0
  // This should work on all platforms
  feenableexcept(FE_DIVBYZERO | FE_INVALID);
  // Set the signal handler
  signal(SIGFPE, signal_handler);
#endif
}

//------------------------------------------------------------------------------
// Description:
// Disable floating point exceptions.
void vtkFloatingPointExceptions::Disable()
{
#ifdef _MSC_VER
  // disable floating point exceptions on MSVC
  _controlfp(
    _EM_INVALID | _EM_DENORMAL | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW | _EM_INEXACT,
    _MCW_EM);
#endif //_MSC_VER
#if defined(VTK_USE_FENV) && FE_ALL_EXCEPT != 0
  fedisableexcept(FE_DIVBYZERO | FE_INVALID);
#endif
}
VTK_ABI_NAMESPACE_END
