/*=========================================================================

  Program:   Visualization Library
  Module:    GlrCam.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#ifndef __vlGlrCamera_hh
#define __vlGlrCamera_hh

#include "Camera.hh"
#include "gl.h"

class vlGlrRenderer;

class vlGlrCamera : public vlCamera
{
 public:
  virtual char *GetClassName() {return "vlGlrCamera";};
  void Render(vlRenderer *ren); // overides base 
  void Render(vlGlrRenderer *ren); // real function 

};

#endif
