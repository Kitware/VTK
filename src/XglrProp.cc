/*=========================================================================

  Program:   Visualization Library
  Module:    XglrProp.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <math.h>
#include "XglrRenW.hh"
#include "XglrRen.hh"
#include "XglrProp.hh"
#include "XglrPrim.hh"

// Description:
// Implement base class method.
void vlXglrProperty::Render(vlProperty *prop, vlRenderer *ren)
{
  this->Render(prop, (vlXglrRenderer *)ren);
}

// Description:
// Actual property render method.
void vlXglrProperty::Render(vlProperty *prop, vlXglrRenderer *ren)
{
  int i, method, line_method;
  Xgl_ctx *context;
  Xgl_color_rgb diffuseColor;
  Xgl_color_rgb specularColor;
  float Ambient, Diffuse, Specular;
  float *AmbientColor, *DiffuseColor, *SpecularColor;

  Ambient = prop->GetAmbient();
  Diffuse = prop->GetDiffuse();
  Specular = prop->GetSpecular();
  AmbientColor = prop->GetAmbientColor();
  DiffuseColor = prop->GetDiffuseColor();
  SpecularColor = prop->GetSpecularColor();

  // get the context for this renderer 
  context = ren->GetContext();
  diffuseColor.r = DiffuseColor[0];
  diffuseColor.g = DiffuseColor[1];
  diffuseColor.b = DiffuseColor[2];
  specularColor.r = SpecularColor[0];
  specularColor.g = SpecularColor[1];
  specularColor.b = SpecularColor[2];


  // see if this is a frontface or backface property 
  if (prop->GetBackface() == 0.0) 
    {
    xgl_object_set(*context,
		   XGL_3D_CTX_SURF_FRONT_AMBIENT, Ambient,
		   XGL_3D_CTX_SURF_FRONT_DIFFUSE, Diffuse,
		   XGL_3D_CTX_SURF_FRONT_SPECULAR, Specular,
		   XGL_3D_CTX_SURF_FRONT_SPECULAR_POWER, 
		   prop->GetSpecularPower(),
		   XGL_3D_CTX_SURF_FRONT_SPECULAR_COLOR, &specularColor,
		   XGL_CTX_SURF_FRONT_COLOR, &diffuseColor,
		   XGL_CTX_LINE_COLOR, &DiffuseColor,
		   XGL_3D_CTX_SURF_BACK_AMBIENT, Ambient,
		   XGL_3D_CTX_SURF_BACK_DIFFUSE, Diffuse,
		   XGL_3D_CTX_SURF_BACK_SPECULAR, Specular,
		   XGL_3D_CTX_SURF_BACK_SPECULAR_POWER, 
		   prop->GetSpecularPower(),
		   XGL_3D_CTX_SURF_BACK_SPECULAR_COLOR, &specularColor,
		   XGL_3D_CTX_SURF_BACK_COLOR, &diffuseColor,
		   XGL_3D_CTX_SURF_FRONT_TRANSP, 1.0-prop->GetTransparency(),
		   XGL_3D_CTX_SURF_BACK_TRANSP, 1.0-prop->GetTransparency(),
		   NULL);
    }
  else 
    {
    xgl_object_set(*context,
		 XGL_3D_CTX_SURF_BACK_AMBIENT, Ambient,
		 XGL_3D_CTX_SURF_BACK_DIFFUSE, Diffuse,
		 XGL_3D_CTX_SURF_BACK_SPECULAR, Specular,
		 XGL_3D_CTX_SURF_BACK_SPECULAR_POWER, prop->GetSpecularPower(),
		 XGL_3D_CTX_SURF_BACK_SPECULAR_COLOR, &specularColor,
		 XGL_3D_CTX_SURF_BACK_COLOR, &diffuseColor,
		 XGL_3D_CTX_SURF_BACK_TRANSP, 1.0-prop->GetTransparency(),
		 NULL);
    return;
    }		 

  switch (prop->GetRepresentation()) 
    {
    case VL_POINTS:
      xgl_object_set(*context,
		     XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_HOLLOW,
		     XGL_3D_CTX_SURF_BACK_FILL_STYLE, XGL_SURF_FILL_HOLLOW,
		     NULL);
      break;
    case VL_WIREFRAME:
      xgl_object_set(*context,
		     XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_HOLLOW,
		     XGL_3D_CTX_SURF_BACK_FILL_STYLE, XGL_SURF_FILL_HOLLOW,
		     NULL);
      break;
    case VL_SURFACE:
      xgl_object_set(*context,
		     XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_SOLID,
		     XGL_3D_CTX_SURF_BACK_FILL_STYLE, XGL_SURF_FILL_SOLID,
		     NULL);
      break;
    default: 
      xgl_object_set(*context,
		     XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_SOLID,
		     XGL_3D_CTX_SURF_BACK_FILL_STYLE, XGL_SURF_FILL_SOLID,
		     NULL);
      break;
    }

  // set interpolation 
  switch (prop->GetInterpolation()) 
    {
    case VL_FLAT:
      method = XGL_ILLUM_PER_FACET;
      line_method = FALSE;
      break;
    case VL_GOURAUD:
    case VL_PHONG:
      method = XGL_ILLUM_PER_VERTEX;
      line_method = TRUE;
      break;
    default:
      method = XGL_ILLUM_PER_VERTEX;
      line_method = TRUE;
      break;
    }
  xgl_object_set(*context,
		 XGL_3D_CTX_SURF_FRONT_ILLUMINATION, method,
		 XGL_3D_CTX_SURF_BACK_ILLUMINATION, method,
		 XGL_3D_CTX_LINE_COLOR_INTERP, line_method,
		 NULL);
  
  // Tell the geometry primitives about the default properties 
  vlXglrPrimitive::SetProperty(prop);
}
