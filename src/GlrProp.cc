/*=========================================================================

  Program:   Visualization Library
  Module:    KglrProp.cc
  Language:  C++
  Date:      2/8/94
  Version:   1.5

This file is part of the Visualization Library. No part of this file or its
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
void vlGlrProperty::Render(vlRenderer *ren)
{
  this->Render((vlGlrRenderer *)ren);
}

// Description:
// Actual property render method.
void vlGlrProperty::Render(vlGlrRenderer *ren)
{
  int i, method;

  // unbind any textures for starters
  texbind(TX_TEXTURE_0,0);

  lmcolor (LMC_NULL);
  mat[1] = this->Transparency;
  mat[15] = this->SpecularPower;

  for (i=0; i < 3; i++) 
    {
    mat[i+3] = this->Ambient*this->AmbientColor[i];
    mat[i+7] = this->Diffuse*this->DiffuseColor[i];
    mat[i+11] = this->Specular*this->SpecularColor[i];
    }
  
  lmdef(DEFMATERIAL, 1, 0, mat);
  lmbind(MATERIAL, 1);
  lmbind (BACKMATERIAL, 0);  

  // Tell the geometry primitives about the default properties 
  vlGlrPrimitive::SetProperty(this);

  // set interpolation 
  switch (this->Interpolation) 
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
