/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GlrRen.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkGlrRenderer - SGI gl renderer
// .SECTION Description
// vtkGlrRenderer is a concrete implementation of the abstract class
// vtkRenderer. vtkGlrRenderer interfaces to the Silicon Graphics gl
// graphics library.

#ifndef __vtkGlrRenderer_hh
#define __vtkGlrRenderer_hh

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Renderer.hh"
#include "gl.h"

class vtkGlrRenderer : public vtkRenderer
{
 protected:
  int NumberOfLightsBound;

 public:
  vtkGlrRenderer();

  void Render(void); // overides base 
  char *GetClassName() {return "vtkGlrRenderer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGeometryPrimitive *GetPrimitive(char *);
  void ClearLights(void);
  int UpdateActors(void);
  int UpdateCameras(void);
  int UpdateLights(void);

  // stereo related stuff
  virtual float *GetCenter();
  virtual void DisplayToView(); 
  virtual void ViewToDisplay(); 
  virtual int  IsInViewport(int x,int y); 
};

#endif
