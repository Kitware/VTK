/*=========================================================================

  Program:   Visualization Toolkit
  Module:    OglrProp.cc
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
#include "OglrProp.hh"
#include "OglrPrim.hh"

// Description:
// Implement base class method.
void vtkOglrProperty::Render(vtkProperty *prop, vtkRenderer *ren)
{
  this->Render(prop, (vtkOglrRenderer *)ren);
}

// Description:
// Actual property render method.
void vtkOglrProperty::Render(vtkProperty *prop, vtkOglrRenderer *ren)
{
  int i, method;
  float Ambient, Diffuse, Specular;
  float *AmbientColor, *DiffuseColor, *SpecularColor;
  float Info[4];
  GLenum Face;

  // unbind any textures for starters
  glDisable(GL_TEXTURE_2D);

  glDisable(GL_COLOR_MATERIAL);
  Face = GL_FRONT_AND_BACK;

  Info[3] = prop->GetTransparency();

  Ambient = prop->GetAmbient();
  Diffuse = prop->GetDiffuse();
  Specular = prop->GetSpecular();
  AmbientColor = prop->GetAmbientColor();
  DiffuseColor = prop->GetDiffuseColor();
  SpecularColor = prop->GetSpecularColor();

  for (i=0; i < 3; i++) 
    {
    Info[i] = Ambient*AmbientColor[i];
    }
  glMaterialfv( Face, GL_AMBIENT, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = Diffuse*DiffuseColor[i];
    }
  glMaterialfv( Face, GL_DIFFUSE, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = Specular*SpecularColor[i];
    }
  glMaterialfv( Face, GL_SPECULAR, Info );

  Info[0] = prop->GetSpecularPower();
  glMaterialfv( Face, GL_SHININESS, Info );

  // Tell the geometry primitives about the default properties 
  vtkOglrPrimitive::SetProperty(prop);

  // set interpolation 
  switch (prop->GetInterpolation()) 
    {
    case VTK_FLAT:
      method = GL_FLAT;
      break;
    case VTK_GOURAUD:
    case VTK_PHONG:
      method = GL_SMOOTH;
      break;
    default:
      method = GL_SMOOTH;
      break;
    }
  
  glShadeModel(method);
}
