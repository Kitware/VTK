/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XRenWin.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkXRenderWindow - rendering window for X Window system
// .SECTION Description
// vtkXRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkXRenderer interfaces to the X Window system.

#ifndef __vtkXRenderWindow_hh
#define __vtkXRenderWindow_hh

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "RenderW.hh"

class vtkXRenderWindow : public vtkRenderWindow
{
public:
  vtkXRenderWindow();
  char *GetClassName() {return "vtkXRenderWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // supply base class virtual function
  vtkRenderWindowInteractor *MakeRenderWindowInteractor();

  // Xwindow get set functions
  int     *GetSize();
  int     *GetScreenSize();
  int     *GetPosition();
  Display *GetDisplayId();
  void     SetDisplayId(Display *);
  void     SetDisplayId(void *);
  Window   GetWindowId();
  void     SetWindowId(Window);
  void     SetWindowId(void *);
  void     SetNextWindowId(Window);
  virtual int      GetDesiredDepth()    = 0;
  virtual Colormap GetDesiredColormap() = 0;
  virtual Visual  *GetDesiredVisual()   = 0;

protected:
  Window   WindowId;
  Window   NextWindowId;
  Display *DisplayId;
  Colormap ColorMap;
  int      OwnWindow;
  int      ScreenSize[2];

};

#endif
