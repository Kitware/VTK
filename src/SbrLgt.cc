/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SbrLgt.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "SbrRen.hh"
#include "SbrLgt.hh"
#include "Light.hh"

// Description:
// Implement base class method.
void vtkSbrLight::Render(vtkLight *lgt, vtkRenderer *ren,int light_index)
{
  this->Render(lgt, (vtkSbrRenderer *)ren,light_index);
}

// Description:
// Actual light render method.
void vtkSbrLight::Render(vtkLight *lgt, vtkSbrRenderer *ren,int light_index)
{
  float	dx, dy, dz;
  float	color[3];
  int light_flag;
  int fd;
  float *Color, *Position, *FocalPoint;
  float Intensity;

  light_flag = ren->GetLightSwitch();
  fd = ren->GetFd();
  
  // get required info from light
  Intensity = lgt->GetIntensity();
  Color = lgt->GetColor();
  color[0] = Intensity * Color[0];
  color[1] = Intensity * Color[1];
  color[2] = Intensity * Color[2];
  
  FocalPoint = lgt->GetFocalPoint();
  Position   = lgt->GetPosition();
  dx = FocalPoint[0] - Position[0];
  dy = FocalPoint[1] - Position[1];
  dz = FocalPoint[2] - Position[2];
  
  // define the light source
  if (!lgt->GetPositional())
    {
    light_source(fd, light_index, DIRECTIONAL,
		 color[0], color[1], color[2],
		 -dx, -dy, -dz);
    }
  else
    {
    float *AttenuationValues = lgt->GetAttenuationValues();

    light_source(fd, light_index, POSITIONAL,
		 color[0], color[1], color[2],
		 Position[0], Position[1], Position[2]);
    light_model(fd, light_index, ATTEN_LIGHT | SPOT_LIGHT | CONE_LIGHT,
		(int)lgt->GetExponent(), 1.0, lgt->GetConeAngle(),
		dx, dy, dz);
    light_attenuation(fd, light_index, 1, 
		      AttenuationValues[0],
		      AttenuationValues[1],
		      AttenuationValues[2]);
    }
  
  light_flag |= (0x0001 << light_index);
  vtkDebugMacro(<< "Defining front light\n");
  
  // define another mirror light if backlit is on and not positional
  if (ren->GetBackLight() && !lgt->GetPositional()) 
    {
    light_index++;
    light_source(fd, light_index, DIRECTIONAL,
		 color[0], color[1], color[2],
		 dx, dy, dz);
    vtkDebugMacro(<< "Defining back light\n");
    light_flag |= (0x0001 << light_index);
    }

  // update the light switch
  light_switch(fd, light_flag);
 
  vtkDebugMacro(<< "SB_light_switch: " << light_flag << "\n");
}

