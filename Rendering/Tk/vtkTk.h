/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTk.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#endif
// VTK-HeaderTest-Exclude: vtkTk.h
