/*=========================================================================

  Program:   Visualization Library
  Module:    SbrRenW.hh
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

#ifndef __vlSbrRenderWindow_hh
#define __vlSbrRenderWindow_hh

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "XRenWin.hh"

class vlSbrRenderWindow : public vlXRenderWindow
{
public:
  vlSbrRenderWindow();
  char *GetClassName() {return "vlSbrRenderWindow";};
  void PrintSelf(ostream& os, vlIndent indent);
  
  vlRenderer  *MakeRenderer();
  vlActor     *MakeActor();
  vlLight     *MakeLight();
  vlCamera    *MakeCamera();
  vlProperty  *MakeProperty();

  void Start(void);
  void Frame(void);
  void WindowInitialize(void);
  void Initialize(void);
  virtual void SetFullScreen(int);
  void WindowRemap(void);
  void PrefFullScreen(void);
  void SetSize(int,int);

  vlGetMacro(Fd,int);

  // stereo rendering stuff
  virtual void StereoUpdate();
  virtual void StereoRenderComplete();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *);

  // Xwindow stuff
  int      GetDesiredDepth();
  Colormap GetDesiredColormap();
  Visual  *GetDesiredVisual();
  int      CreateXWindow(Display *,int x,int y,int w,int h,int depth,
			 char name[80]);

protected:
  int      Fd;
  int      Buffer;
  int      NumPlanes;
};

#endif
