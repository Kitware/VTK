/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SbrProp.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "SbrProp.hh"
#include "SbrRen.hh"
#include "SbrPrim.hh"
#include "starbase.c.h"

// Description:
// Implement base class method.
void vtkSbrProperty::Render(vtkProperty *prop, vtkRenderer *ren)
{
  this->Render(prop, (vtkSbrRenderer *)ren);
}

// Description:
// Actual property render method.
void vtkSbrProperty::Render(vtkProperty *prop, vtkSbrRenderer *ren)
{
  int fd;
  int     style;
  int     shininess;
  int     trans_level;
  static int pattern[16] = {0,10,8,2,5,15,13,7,4,14,12,6,1,11,9,3};
  gescape_arg esc_arg1,esc_arg2;
  int l;
  float *DiffuseColor, *EdgeColor, *SpecularColor;
  fd = ren->GetFd();
  
  EdgeColor = prop->GetEdgeColor();
  DiffuseColor = prop->GetDiffuseColor();
  SpecularColor = prop->GetSpecularColor();

  if (!prop->GetEdgeVisibility()) 
    {
    EdgeColor = DiffuseColor;
    }

  line_color(fd, DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]);
  fill_color(fd, DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]);
  perimeter_color(fd, EdgeColor[0], EdgeColor[1], EdgeColor[2]);
  text_color(fd, DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]);
  marker_color(fd,DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]);

  bf_fill_color(fd, DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]);
  bf_perimeter_color(fd, EdgeColor[0], EdgeColor[1], EdgeColor[2]);

  // Tell the geometry primitives about the default properties 
  vtkSbrPrimitive::SetProperty(prop);

  switch (prop->GetRepresentation()) 
    {
  case VL_POINTS:
    style = INT_POINT;
    break;
  case VL_WIREFRAME:
    style = INT_OUTLINE;
    break;
  case VL_SURFACE:
    style = INT_SOLID;
    break;
  default:
    style = INT_SOLID;
    break;
    }

  interior_style(fd, style, prop->GetEdgeVisibility());
  bf_interior_style(fd, style, prop->GetEdgeVisibility());
  surface_coefficients(fd, prop->GetAmbient(), prop->GetDiffuse(), 
		       prop->GetSpecular());
  bf_surface_coefficients(fd, prop->GetAmbient(), 
			  prop->GetDiffuse(), prop->GetSpecular());

  shininess = (int)prop->GetSpecularPower();
  if (shininess < 1) shininess = 1;
  if (shininess > 16383) shininess = 16383;
  
  surface_model(fd, TRUE, shininess, 
		SpecularColor[0], SpecularColor[1], SpecularColor[2]);
  bf_surface_model(fd, TRUE, shininess, 
		   SpecularColor[0], SpecularColor[1],
		   SpecularColor[2]);

  trans_level = (int)(16.0*(1.0 - prop->GetTransparency()));
  esc_arg1.i[0] = 0x0000;
  for (l = 0; l < trans_level; l++)
    esc_arg1.i[0] |= (0x0001 << pattern[l]);
  esc_arg1.i[0] = ~esc_arg1.i[0];
  gescape(fd, TRANSPARENCY, &esc_arg1, &esc_arg2);
}
