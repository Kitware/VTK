/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OglrRenW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkOglrRenderWindow - SGI OpenGL rendering window
// .SECTION Description
// vtkOglrRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkOglrRenderer interfaces to the Silicon Graphics 
// OpenGL graphics library.

#ifndef __vtkOglrRenderWindow_hh
#define __vtkOglrRenderWindow_hh

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "XRenWin.hh"
#include "GL/glx.h"

class vtkOglrRenderWindow : public vtkXRenderWindow
{
protected:
  GLXContext ContextId;
  int MultiSamples;
  long OldMonitorSetting;

public:
  vtkOglrRenderWindow();
  char *GetClassName() {return "vtkOglrRenderWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkRenderer  *MakeRenderer();
  vtkLightDevice     *MakeLight();
  vtkCameraDevice    *MakeCamera();
  vtkTextureDevice   *MakeTexture();
  vtkPropertyDevice  *MakeProperty();

  void Start(void);
  void Frame(void);
  void Connect(void);
  void WindowConfigure(void);
  void WindowInitialize(void);
  void Initialize(void);
  virtual void SetFullScreen(int);
  void WindowRemap(void);
  void PrefFullScreen(void);
  void SetSize(int,int);

  virtual int      GetDesiredDepth();
  virtual Colormap GetDesiredColormap();
  virtual Visual  *GetDesiredVisual();
  XVisualInfo     *GetDesiredVisualInfo();

  vtkSetMacro(MultiSamples,int);
  vtkGetMacro(MultiSamples,int);

  // stereo rendering stuff
  virtual void StereoUpdate();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *,int front);
};

#endif
