/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SbrRenW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkSbrRenderWindow - HP starbase rendering window
// .SECTION Description
// vtkSbrRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkSbrRenderer interfaces to the Hewlett-Packard starbase
// graphics library.

#ifndef __vtkSbrRenderWindow_hh
#define __vtkSbrRenderWindow_hh

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "XRenWin.hh"

class vtkSbrRenderWindow : public vtkXRenderWindow
{
public:
  vtkSbrRenderWindow();
  ~vtkSbrRenderWindow();
  char *GetClassName() {return "vtkSbrRenderWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkRenderer       *MakeRenderer();
  vtkLightDevice    *MakeLight();
  vtkCameraDevice   *MakeCamera();
  vtkPropertyDevice *MakeProperty();
  vtkTextureDevice  *MakeTexture();

  void Start(void);
  void Frame(void);
  void WindowInitialize(void);
  void Initialize(void);
  virtual void SetFullScreen(int);
  void WindowRemap(void);
  void PrefFullScreen(void);
  void SetSize(int,int);

  vtkGetMacro(Fd,int);

  // stereo rendering stuff
  virtual void StereoUpdate();
  virtual void CopyResultFrame();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2, int front);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *, int front);

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
