/*=========================================================================

  Program:   Visualization Library
  Module:    GlrRenW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlGlrRenderWindow - SGI gl rendering window
// .SECTION Description
// vlGlrRenderWindow is a concrete implementation of the abstract class
// vlRenderWindow. vlGlrRenderer interfaces to the Silicon Graphics gl
// graphics library.

#ifndef __vlGlrRenderWindow_hh
#define __vlGlrRenderWindow_hh

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "XRenWin.hh"

class vlGlrRenderWindow : public vlXRenderWindow
{
protected:
  int Gid;
  int MultiSamples;
  long OldMonitorSetting;

public:
  vlGlrRenderWindow();
  char *GetClassName() {return "vlGlrRenderWindow";};
  void PrintSelf(ostream& os, vlIndent indent);
  
  vlRenderer  *MakeRenderer();
  vlActor     *MakeActor();
  vlLight     *MakeLight();
  vlCamera    *MakeCamera();
  vlProperty  *MakeProperty();

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

  vlSetMacro(MultiSamples,int);
  vlGetMacro(MultiSamples,int);

  // stereo rendering stuff
  virtual void StereoUpdate();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *);
};

#endif
