/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OglrRen.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkOglrRenderer - SGI OpenGL renderer
// .SECTION Description
// vtkOglrRenderer is a concrete implementation of the abstract class
// vtkRenderer. vtkOglrRenderer interfaces to the Silicon Graphics OpenGL
// graphics library.

#ifndef __vtkOglrRenderer_hh
#define __vtkOglrRenderer_hh

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Renderer.hh"
#include <GL/gl.h>

class vtkOglrRenderer : public vtkRenderer
{
 protected:
  int NumberOfLightsBound;

 public:
  vtkOglrRenderer();

  void Render(void); // overides base 
  char *GetClassName() {return "vtkOglrRenderer";};
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
