/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XglrRen.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkXglrRenderer - Suns XGL renderer
// .SECTION Description
// vtkXglrRenderer is a concrete implementation of the abstract class
// vtkRenderer. vtkXglrRenderer interfaces to Suns XGL graphics library.

#ifndef __vtkXglrRenderer_hh
#define __vtkXglrRenderer_hh

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Renderer.hh"
#include <xgl/xgl.h>

#define MAX_LIGHTS 12

class vtkXglrRenderer : public vtkRenderer
{
protected:
  Xgl_light XglrLights[MAX_LIGHTS];
  int NumberOfLightsBound;
  Xgl_3d_ctx Context;

public:
  vtkXglrRenderer();
  char *GetClassName() {return "vtkXglrRenderer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Render(void);
  vtkGeometryPrimitive *GetPrimitive(char *);
  int UpdateActors(void);
  int UpdateCameras(void);
  int UpdateLights(void);
  Xgl_3d_ctx *GetContext() {return &(this->Context);};
  Xgl_win_ras  *GetRaster() 
  {return ((vtkXglrRenderWindow *)(this->GetRenderWindow()))->GetRaster();};
  Xgl_light *GetLightArray() {return this->XglrLights;};
};

#endif
