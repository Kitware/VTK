/*=========================================================================

  Program:   Visualization Library
  Module:    GlrRen.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlGlrRenderer - SGI gl renderer
// .SECTION Description
// vlGlrRenderer is a concrete implementation of the abstract class
// vlRenderer. vlGlrRenderer interfaces to the Silicon Graphics gl
// graphics library.

#ifndef __vlGlrRenderer_hh
#define __vlGlrRenderer_hh

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Renderer.hh"
#include "gl.h"

class vlGlrRenderer : public vlRenderer
{
 protected:
  int NumberOfLightsBound;

 public:
  vlGlrRenderer();

  void Render(void); // overides base 
  char *GetClassName() {return "vlGlrRenderer";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlGeometryPrimitive *GetPrimitive(char *);
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
