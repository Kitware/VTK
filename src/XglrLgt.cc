/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XglrLgt.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "XglrRenW.hh"
#include "XglrRen.hh"
#include "XglrLgt.hh"

// Description:
// Implement base class method.
void vtkXglrLight::Render(vtkLight *lgt, vtkRenderer *ren,int light_index)
{
  this->Render(lgt, (vtkXglrRenderer *)ren,light_index);
}

// Description:
// Actual light render method.
void vtkXglrLight::Render(vtkLight *lgt, vtkXglrRenderer *ren,int light_index)
{
  Xgl_light *lights;
  Xgl_color light_color;
  Xgl_pt_f3d direction;
  float *Color, *Position, *FocalPoint;
  float Intensity;

  // get required info from light
  Intensity = lgt->GetIntensity();
  Color = lgt->GetColor();
  light_color.rgb.r = Intensity * Color[0];
  light_color.rgb.g = Intensity * Color[1];
  light_color.rgb.b = Intensity * Color[2];
  
  FocalPoint = lgt->GetFocalPoint();
  Position   = lgt->GetPosition();
  direction.x = Position[0] - FocalPoint[0];
  direction.y = Position[1] - FocalPoint[1];
  direction.z = Position[2] - FocalPoint[2];

  lights = ren->GetLightArray();
  
  // define the light source
  xgl_object_set(lights[light_index],
		 XGL_LIGHT_TYPE, XGL_LIGHT_DIRECTIONAL,
		 XGL_LIGHT_COLOR, &light_color,
		 XGL_LIGHT_DIRECTION, &direction,
		 NULL);
  
  vtkDebugMacro(<< "Defining front light\n");
  
  // define another mirror light if backlit is on
  if (ren->GetBackLight()) 
    {
    direction.x = -direction.x;
    direction.y = -direction.y;
    direction.z = -direction.z;

    /* define the light source */
    xgl_object_set(lights[light_index+1],
		   XGL_LIGHT_TYPE, XGL_LIGHT_DIRECTIONAL,
		   XGL_LIGHT_COLOR, &light_color,
		   XGL_LIGHT_DIRECTION, &direction,
		   NULL);
  
    vtkDebugMacro(<< "Defining back light\n");
    }
}

