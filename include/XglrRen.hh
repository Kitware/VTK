/*=========================================================================

  Program:   Visualization Library
  Module:    XglrRen.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlXglrRenderer - Suns XGL renderer
// .SECTION Description
// vlXglrRenderer is a concrete implementation of the abstract class
// vlRenderer. vlXglrRenderer interfaces to Suns XGL graphics library.

#ifndef __vlXglrRenderer_hh
#define __vlXglrRenderer_hh

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Renderer.hh"
#include <xgl/xgl.h>

#define MAX_LIGHTS 12

class vlXglrRenderer : public vlRenderer
{
protected:
  Xgl_light XglrLights[MAX_LIGHTS];
  int NumberOfLightsBound;
  Xgl_3d_ctx Context;

public:
  vlXglrRenderer();
  char *GetClassName() {return "vlXglrRenderer";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Render(void);
  vlGeometryPrimitive *GetPrimitive(char *);
  int UpdateActors(void);
  int UpdateCameras(void);
  int UpdateLights(void);
  Xgl_3d_ctx *GetContext() {return &(this->Context);};
  Xgl_win_ras  *GetRaster() 
  {return ((vlXglrRenderWindow *)(this->GetRenderWindow()))->GetRaster();};
  Xgl_light *GetLightArray() {return this->XglrLights;};
};

#endif
