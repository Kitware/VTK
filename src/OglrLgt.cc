/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OglrLgt.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "OglrRen.hh"
#include "OglrLgt.hh"

// Description:
// Implement base class method.
void vtkOglrLight::Render(vtkLight *lgt, vtkRenderer *ren,int light_index)
{
  this->Render(lgt, (vtkOglrRenderer *)ren,light_index);
}

// Description:
// Actual light render method.
void vtkOglrLight::Render(vtkLight *lgt, vtkOglrRenderer *ren,int light_index)
{
  float	dx, dy, dz;
  float	color[4];
  float *Color, *Position, *FocalPoint;
  float Intensity;
  float Info[4];

  // get required info from light
  Intensity = lgt->GetIntensity();
  Color = lgt->GetColor();
  color[0] = Intensity * Color[0];
  color[1] = Intensity * Color[1];
  color[2] = Intensity * Color[2];
  color[3] = 1.0;

  FocalPoint = lgt->GetFocalPoint();
  Position   = lgt->GetPosition();
  dx = FocalPoint[0] - Position[0];
  dy = FocalPoint[1] - Position[1];
  dz = FocalPoint[2] - Position[2];

  glLightfv( light_index, GL_DIFFUSE, color);
  glLightfv( light_index, GL_SPECULAR, color);

  if( ren->GetBackLight())
  {
  glLightfv( light_index+1, GL_DIFFUSE, color);
  glLightfv( light_index+1, GL_SPECULAR, color);
  }
  
  // define the light source
  if (!lgt->GetPositional())
    {
    Info[0]  = -dx;
    Info[1]  = -dy;
    Info[2]  = -dz;
    Info[3]  = 0.0;
    glLightfv( light_index, GL_POSITION, Info );
    // define another mirror light if backlit is on
    if (ren->GetBackLight()) 
      {
      Info[0]  = dx;
      Info[1]  = dy;
      Info[2]  = dz;
      Info[3]  = 0.0;
      glLightfv( light_index + 1, GL_POSITION, Info );
      }
    }
  else
    {
    Info[0]  = Position[0];
    Info[1]  = Position[1];
    Info[2]  = Position[2];
    Info[3]  = 1.0;
    glLightfv( light_index, GL_POSITION, Info );

    Info[0] = dx;
    Info[1] = dy;
    Info[2] = dz;
    glLightfv( light_index, GL_SPOT_DIRECTION, Info );

    Info[0] = lgt->GetExponent();
    glLightfv( light_index, GL_SPOT_EXPONENT, Info );

    Info[0] = lgt->GetConeAngle();
    glLightfv( light_index, GL_SPOT_CUTOFF, Info );

    float *AttenuationValues = lgt->GetAttenuationValues();
    glLightfv( light_index, GL_CONSTANT_ATTENUATION, AttenuationValues);
    glLightfv( light_index, GL_LINEAR_ATTENUATION, AttenuationValues + 1);
    glLightfv( light_index, GL_QUADRATIC_ATTENUATION, AttenuationValues + 2);
    }

}

