/*=========================================================================

  Program:   Visualization Toolkit
  Module:    KglrProp.cc
  Language:  C++
  Date:      2/8/94
  Version:   1.5

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "GlrRen.hh"
#include "GlrProp.hh"
#include "GlrPrim.hh"

// temporary material structure
static float mat[] = {
  ALPHA, 0.0,
  AMBIENT, 0.0, 0.0, 0.0,
  DIFFUSE, 0.0, 0.0, 0.0,
  SPECULAR, 0.0, 0.0, 0.0,
  SHININESS, 0.0,
  LMNULL
  };

// Description:
// Implement base class method.
void vtkGlrProperty::Render(vtkProperty *prop, vtkRenderer *ren)
{
  this->Render(prop, (vtkGlrRenderer *)ren);
}

// Description:
// Actual property render method.
void vtkGlrProperty::Render(vtkProperty *prop, vtkGlrRenderer *ren)
{
  int i, method;
  float Ambient, Diffuse, Specular;
  float *AmbientColor, *DiffuseColor, *SpecularColor;

  // unbind any textures for starters
  texbind(TX_TEXTURE_0,0);

  lmcolor (LMC_NULL);
  mat[1] = prop->GetTransparency();
  mat[15] = prop->GetSpecularPower();

  Ambient = prop->GetAmbient();
  Diffuse = prop->GetDiffuse();
  Specular = prop->GetSpecular();
  AmbientColor = prop->GetAmbientColor();
  DiffuseColor = prop->GetDiffuseColor();
  SpecularColor = prop->GetSpecularColor();

  for (i=0; i < 3; i++) 
    {
    mat[i+3] = Ambient*AmbientColor[i];
    mat[i+7] = Diffuse*DiffuseColor[i];
    mat[i+11] = Specular*SpecularColor[i];
    }

  lmdef(DEFMATERIAL, 1, 0, mat);
  lmbind(MATERIAL, 1);
  lmbind (BACKMATERIAL, 0);  

  // Tell the geometry primitives about the default properties 
  vtkGlrPrimitive::SetProperty(prop);

  // set interpolation 
  switch (prop->GetInterpolation()) 
    {
    case VL_FLAT:
      method = FLAT;
      break;
    case VL_GOURAUD:
    case VL_PHONG:
      method = GOURAUD;
      break;
    default:
      method = GOURAUD;
      break;
    }
  
  shademodel(method);
}
