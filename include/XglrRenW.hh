/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XglrRenW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkXglrRenderWindow - Suns XGL rendering window
// .SECTION Description
// vtkXglrRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkXglrRenderer interfaces to Suns XGL graphics library.

#ifndef __vtkXglrRenderWindow_hh
#define __vtkXglrRenderWindow_hh

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "XRenWin.hh"
#include <xgl/xgl.h>

class vtkXglrRenderWindow : public vtkXRenderWindow
{
public:
  vtkXglrRenderWindow();
  ~vtkXglrRenderWindow();
  char *GetClassName() {return "vtkXglrRenderWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkRenderer  *MakeRenderer();
  vtkLightDevice     *MakeLight();
  vtkCameraDevice    *MakeCamera();
  vtkPropertyDevice  *MakeProperty();
  vtkTextureDevice   *MakeTexture();

  void Start(void);
  void Frame(void);
  void WindowInitialize(void);
  void Initialize(void);
  virtual void SetFullScreen(int);
  void WindowRemap(void);
  void PrefFullScreen(void);
  void SetSize(int,int);

  // stereo rendering stuff
  virtual void StereoUpdate();
  virtual void CopyResultFrame();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *, int front);

  // Xwindow stuff
  int      GetDesiredDepth();
  Colormap GetDesiredColormap();
  Visual  *GetDesiredVisual();
  int      CreateXWindow(Display *,int x,int y,int w,int h,int depth,
			 char name[80]);
  Xgl_3d_ctx *GetContext() {return &(this->Context);};
  Xgl_win_ras *GetRaster() {return &(this->WindowRaster);};

protected:
  Xgl_3d_ctx Context;
  Xgl_win_ras  WindowRaster;
};

#endif
