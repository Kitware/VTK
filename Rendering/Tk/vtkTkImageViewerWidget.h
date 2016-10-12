/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTkImageViewerWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTkImageViewerWidget
 * @brief   a Tk Widget for viewing vtk images
 *
 *
 * vtkTkImageViewerWidget is a Tk widget that you can render into. It has a
 * GetImageViewer method that returns a vtkImageViewer. You can also
 * specify a vtkImageViewer to be used when creating the widget by using
 * the -iv option. It also takes -width and -height options.
 * Events can be bound on this widget just like any other Tk widget.
 *
 * @sa
 * vtkImageViewer
*/

#ifndef vtkTkImageViewerWidget_h
#define vtkTkImageViewerWidget_h

#include "vtkImageViewer.h"
#include "vtkTcl.h"
#include "vtkTk.h"
#ifndef VTK_PYTHON_BUILD
#include "vtkTclUtil.h"
#endif
#include "vtkWindows.h"

struct vtkTkImageViewerWidget
{
  Tk_Window  TkWin;             /* Tk window structure */
  Tcl_Interp *Interp;           /* Tcl interpreter */
  int Width;
  int Height;
  vtkImageViewer *ImageViewer;
  char *IV;
#ifdef _WIN32
  WNDPROC OldProc;
#endif
};

#endif

