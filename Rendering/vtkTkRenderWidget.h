/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTkRenderWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTkRenderWidget - a Tk Widget for vtk renderering

// .SECTION Description
// vtkTkRenderWidget is a Tk widget that you can render into. It has a 
// GetRenderWindow method that returns a vtkRenderWindow. This can then
// be used to create a vtkRenderer and etc. You can also specify a 
// vtkRenderWindow to be used when creating the widget by using
// the -rw option. It also takes -width and -height options.
// Events can be bound on this widget just like any other Tk widget.

// .SECTION See Also
// vtkRenderWindow vtkRenderer


#ifndef __vtkTkRenderWidget_h
#define __vtkTkRenderWidget_h

#include "vtkRenderWindow.h"
#include <tcl.h>
#include <tk.h>
#ifndef VTK_PYTHON_BUILD
#include "vtkTclUtil.h"
#endif

struct vtkTkRenderWidget
{
  Tk_Window  TkWin;             /* Tk window structure */
  Tcl_Interp *Interp;           /* Tcl interpreter */
  int Width;
  int Height;
  vtkRenderWindow *RenderWindow;
  char *RW;
#ifdef _WIN32
  WNDPROC OldProc;
#endif
};

// This widget requires access to structures that are normally 
// not visible to Tcl/Tk applications. For this reason you must
// have access to tkInt.h
// #include "tkInt.h"

#ifdef _WIN32
extern "C" {
#include "tkWinInt.h" 
}
#endif

#endif








