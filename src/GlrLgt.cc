/*=========================================================================

  Program:   Visualization Library
  Module:    GlrLgt.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "GlrRen.hh"
#include "GlrLgt.hh"

// Use directional light instead so mirror lights behave nicer 
static float light_info[] = {
  LCOLOR, 0.0, 0.0, 0.0,
  POSITION, 0.0, 0.0, 0.0, 0.0,
  LMNULL
  };


// Description:
// Implement base class method.
void vlGlrLight::Render(vlRenderer *ren,int light_index)
{
  this->Render((vlGlrRenderer *)ren,light_index);
}

// Description:
// Actual light render method.
void vlGlrLight::Render(vlGlrRenderer *ren,int light_index)
{
  float	dx, dy, dz;
  float	color[3];

  // get required info from light
  color[0] = this->Intensity * this->Color[0];
  color[1] = this->Intensity * this->Color[1];
  color[2] = this->Intensity * this->Color[2];
  
  dx = this->FocalPoint[0] - this->Position[0];
  dy = this->FocalPoint[1] - this->Position[1];
  dz = this->FocalPoint[2] - this->Position[2];
  
  // define the light source
  light_info[1] = color[0];
  light_info[2] = color[1];
  light_info[3] = color[2];
  light_info[5] = -dx;
  light_info[6] = -dy;
  light_info[7] = -dz;
  
  vlDebugMacro(<< "Defining front light\n");
  lmdef(DEFLIGHT, light_index, 0, light_info);
  
  // define another mirror light if backlit is on
  if (ren->GetBackLight()) 
    {
    light_info[5] = dx;
    light_info[6] = dy;
    light_info[7] = dz;
    vlDebugMacro(<< "Defining back light\n");
    lmdef(DEFLIGHT, light_index + 1, 0, light_info);
    }

}

