/*=========================================================================

  Program:   Visualization Library
  Module:    XRenWin.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlSbrRenderWindow - HP starbase rendering window
// .SECTION Description
// vlSbrRenderWindow is a concrete implementation of the abstract class
// vlRenderWindow. vlSbrRenderer interfaces to the Hewlett-Packard starbase
// graphics library.

#ifndef __vlXRenderWindow_hh
#define __vlXRenderWindow_hh

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "RenderW.hh"

class vlXRenderWindow : public vlRenderWindow
{
public:
  vlXRenderWindow();
  char *GetClassName() {return "vlXRenderWindow";};
  void PrintSelf(ostream& os, vlIndent indent);
  
  // Xwindow get set functions
  int     *GetSize();
  int     *GetScreenSize();
  int     *GetPosition();
  Display *GetDisplayId();
  void     SetDisplayId(Display *);
  Window   GetWindowId();
  void     SetWindowId(Window);
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
