// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkTk_h
#define vtkTk_h

#include "vtkTkAppInitConfigure.h"

#ifdef VTK_TCL_TK_STATIC
#ifndef STATIC_BUILD
#define STATIC_BUILD
#endif
#endif

#include <tk.h>

// These conflict with definitions in KWsys Status.hxx
// Defined in X11/X.h in Tk Headers directory:
#ifdef Success
#undef Success
#endif
// Defined in X11/Xlib.h in Tk Headers directory:
#ifdef Status
#undef Status
#endif
// Defined in X11/Xlibint.h
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#endif
// VTK-HeaderTest-Exclude: vtkTk.h
