/*=========================================================================

  Program:   Visualization Library
  Module:    SbrProp.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
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
void vlSbrProperty::Render(vlRenderer *ren)
{
  this->Render((vlSbrRenderer *)ren);
}

// Description:
// Actual property render method.
void vlSbrProperty::Render(vlSbrRenderer *ren)
{
  int fd;
  float *edge_color;
  int     style;
  int     shininess;
  int     trans_level;
  static int pattern[16] = {0,10,8,2,5,15,13,7,4,14,12,6,1,11,9,3};
  gescape_arg esc_arg1,esc_arg2;
  int l;

  fd = ren->GetFd();
  
  if (this->EdgeVisibility) 
    {
    edge_color = this->EdgeColor;
    } 
  else 
    {
    edge_color = this->DiffuseColor;
    }

  line_color(fd, this->DiffuseColor[0], this->DiffuseColor[1], 
	     this->DiffuseColor[2]);
  fill_color(fd, this->DiffuseColor[0], this->DiffuseColor[1], 
	     this->DiffuseColor[2]);
  perimeter_color(fd, EdgeColor[0], EdgeColor[1], 
		  EdgeColor[2]);
  text_color(fd, this->DiffuseColor[0], this->DiffuseColor[1], 
	     this->DiffuseColor[2]);
  marker_color(fd,this->DiffuseColor[0], this->DiffuseColor[1], 
	       this->DiffuseColor[2]);

  bf_fill_color(fd, this->DiffuseColor[0], this->DiffuseColor[1], 
		this->DiffuseColor[2]);
  bf_perimeter_color(fd, EdgeColor[0], EdgeColor[1], 
		     EdgeColor[2]);

  // Tell the geometry primitives about the default properties 
  vlSbrPrimitive::SetProperty(this);

  switch (this->Representation) 
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

  interior_style(fd, style, this->EdgeVisibility);
  bf_interior_style(fd, style, this->EdgeVisibility);
  surface_coefficients(fd, this->Ambient, this->Diffuse, this->Specular);
  bf_surface_coefficients(fd, this->Ambient, this->Diffuse, this->Specular);

  shininess = (int)this->SpecularPower;
  if (shininess < 1) shininess = 1;
  if (shininess > 16383) shininess = 16383;
  
  surface_model(fd, TRUE, shininess, 
    this->SpecularColor[0],this->SpecularColor[1],
    this->SpecularColor[2]);
  bf_surface_model(fd, TRUE, shininess, 
		   this->SpecularColor[0],this->SpecularColor[1],
		   this->SpecularColor[2]);

  trans_level = (int)(16.0*(1.0 - this->Transparency));
  esc_arg1.i[0] = 0x0000;
  for (l = 0; l < trans_level; l++)
    esc_arg1.i[0] |= (0x0001 << pattern[l]);
  esc_arg1.i[0] = ~esc_arg1.i[0];
  gescape(fd, TRANSPARENCY, &esc_arg1, &esc_arg2);
}
