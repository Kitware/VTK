/*=========================================================================

  Program:   Visualization Library
  Module:    SbrLgt.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "SbrRen.hh"
#include "SbrLgt.hh"

// Description:
// Implement base class method.
void vlSbrLight::Render(vlRenderer *ren,int light_index)
{
  this->Render((vlSbrRenderer *)ren,light_index);
}

// Description:
// Actual light render method.
void vlSbrLight::Render(vlSbrRenderer *ren,int light_index)
{
  float	dx, dy, dz;
  float	color[3];
  int light_flag;
  int fd;

  light_flag = ren->GetLightSwitch();
  fd = ren->GetFd();
  
  // get required info from light
  color[0] = this->Intensity * this->Color[0];
  color[1] = this->Intensity * this->Color[1];
  color[2] = this->Intensity * this->Color[2];
  
  dx = this->FocalPoint[0] - this->Position[0];
  dy = this->FocalPoint[1] - this->Position[1];
  dz = this->FocalPoint[2] - this->Position[2];
  
  // define the light source
  if (!this->Positional)
    {
    light_source(fd, light_index, DIRECTIONAL,
		 color[0], color[1], color[2],
		 -dx, -dy, -dz);
    }
  else
    {
    light_source(fd, light_index, POSITIONAL,
		 color[0], color[1], color[2],
		 this->Position[0], 
		 this->Position[1], 
		 this->Position[2]);
    light_model(fd, light_index, ATTEN_LIGHT | SPOT_LIGHT | CONE_LIGHT,
		(int)this->Exponent, 1.0, this->ConeAngle,
		dx, dy, dz);
    light_attenuation(fd, light_index, 1, 
		      this->AttenuationValues[0],
		      this->AttenuationValues[1],
		      this->AttenuationValues[2]);
    }
  
  light_flag |= (0x0001 << light_index);
  vlDebugMacro(<< "Defining front light\n");
  
  // define another mirror light if backlit is on and not positional
  if (ren->GetBackLight() && !this->Positional) 
    {
    light_index++;
    light_source(fd, light_index, DIRECTIONAL,
		 color[0], color[1], color[2],
		 dx, dy, dz);
    vlDebugMacro(<< "Defining back light\n");
    light_flag |= (0x0001 << light_index);
    }

  // update the light switch
  light_switch(fd, light_flag);
 
  vlDebugMacro(<< "SB_light_switch: " << light_flag << "\n");
}

