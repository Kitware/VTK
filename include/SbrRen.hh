/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SbrRen.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkSbrRenderer - HP starbase renderer
// .SECTION Description
// vtkSbrRenderer is a concrete implementation of the abstract class
// vtkRenderer. vtkSbrRenderer interfaces to the Hewlett-Packard starbase
// graphics library.

#ifndef __vtkSbrRenderer_hh
#define __vtkSbrRenderer_hh

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Renderer.hh"
#include "starbase.c.h"

class vtkSbrRenderer : public vtkRenderer
{
public:
  vtkSbrRenderer();
  char *GetClassName() {return "vtkSbrRenderer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Render(void);
  vtkGeometryPrimitive *GetPrimitive(char *);
  void ClearLights(void);
  int UpdateActors(void);
  int UpdateCameras(void);
  int UpdateLights(void);
  vtkGetMacro(Fd,int);
  vtkGetMacro(LightSwitch,int);
  vtkSetMacro(LightSwitch,int);
  virtual float *GetCenter();
  virtual void DisplayToView(); 
  virtual void ViewToDisplay(); 
  virtual int  IsInViewport(int x,int y); 

protected:
  int NumberOfLightsBound;
  int LightSwitch;
  int Fd;

};

#endif
