/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTkImageWindowWidget.h
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
// .NAME vtkTkImageWindowWidget - a Tk Widget for viewing vtk images

// .SECTION Description
// vtkTkImageWindowWidget is a Tk widget that you can render into. It has a 
// GetImageWindow method that returns a vtkImageWindow. You can also 
// specify a vtkImageWindow to be used when creating the widget by using
// the -iw option. It also takes -width and -height options.
// Events can be bound on this widget just like any other Tk widget.

// .SECTION See Also
// vtkImageWindow


#ifndef __vtkTkImageWindowWidget_h
#define __vtkTkImageWindowWidget_h

#include "vtkImageWindow.h"
#include <tcl.h>
#include <tk.h>
#ifndef VTK_PYTHON_BUILD
#include "vtkTclUtil.h"
#endif
struct vtkTkImageWindowWidget
{
  Tk_Window  TkWin;             /* Tk window structure */
  Tcl_Interp *Interp;           /* Tcl interpreter */
  int Width;
  int Height;
  vtkImageWindow *ImageWindow;
  char *IW;
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








