/*=========================================================================

  Program:   Visualization Library
  Module:    Property.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <stdlib.h>
#include "Property.hh"

// Description:
// Construct object with object color, ambient color, diffuse color,
// specular color, and edge color white; ambient coefficient=0; diffuse 
// coefficient=0; specular coefficient=0; specular power=1; Gouraud shading;
// and surface representation.
vlProperty::vlProperty()
{
  this->Color[0] = 1;
  this->Color[1] = 1;
  this->Color[2] = 1;

  this->AmbientColor[0] = 1;
  this->AmbientColor[1] = 1;
  this->AmbientColor[2] = 1;

  this->DiffuseColor[0] = 1;
  this->DiffuseColor[1] = 1;
  this->DiffuseColor[2] = 1;

  this->SpecularColor[0] = 1;
  this->SpecularColor[1] = 1;
  this->SpecularColor[2] = 1;

  this->EdgeColor[0] = 1;
  this->EdgeColor[1] = 1;
  this->EdgeColor[2] = 1;

  this->Ambient = 0.0;
  this->Diffuse = 1.0;
  this->Specular = 0.0;
  this->SpecularPower = 1.0;
  this->Transparency = 1.0;
  this->Interpolation = VL_GOURAUD;
  this->Representation = VL_SURFACE;
  this->EdgeVisibility = 0;
  this->Backface = 0;
  this->Subdivide = 0;
}


// Description:
// Set shading method to flat.
void vlProperty::SetFlat (void)
{
  this->Interpolation= VL_FLAT;
}

// Description:
// Set shading method to Gouraud.
void vlProperty::SetGouraud (void)
{
  this->Interpolation = VL_GOURAUD;
}

// Description:
// Set shading method to Phong.
void vlProperty::SetPhong (void)
{
  this->Interpolation = VL_PHONG;
}

// Description:
// Represent geometry with points.
void vlProperty::SetPoints (void)
{
  this->Interpolation = VL_POINTS;
}

// Description:
// Represent geometry as wireframe.
void vlProperty::SetWireframe (void)
{
  this->Representation = VL_WIREFRAME;
}

// Description:
// Represent geometry as surface.
void vlProperty::SetSurface (void)
{
  this->Representation = VL_SURFACE;
}

// Description:
// Set the color of the object. Has side effects in that it sets the
// ambient diffuse and specular colors as well.
void vlProperty::SetColor(float R,float G,float B)
{
  /* store the coordinates */
  this->Color[0] = R;
  this->AmbientColor[0] = R;
  this->DiffuseColor[0] = R;
  this->SpecularColor[0] = R;

  this->Color[1] = G;
  this->AmbientColor[1] = G;
  this->DiffuseColor[1] = G;
  this->SpecularColor[1] = G;

  this->Color[2] = B;
  this->AmbientColor[2] = B;
  this->DiffuseColor[2] = B;
  this->SpecularColor[2] = B;
}

 
void vlProperty::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlProperty::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);
    
    os << indent << "Ambient: " << this->Ambient << "\n";
    os << indent << "Ambient Color: (" << this->AmbientColor[0] << ", " 
      << this->AmbientColor[1] << ", " << this->AmbientColor[2] << ")\n";
    os << indent << "Backface: " << (this->Backface ? "On\n" : "Off\n");
    os << indent << "Color: (" << this->Color[0] << ", " 
      << this->Color[1] << ", " << this->Color[2] << ")\n";
    os << indent << "Diffuse: " << this->Diffuse << "\n";
    os << indent << "Diffuse Color: (" << this->DiffuseColor[0] << ", " 
      << this->DiffuseColor[1] << ", " << this->DiffuseColor[2] << ")\n";
    os << indent << "Edge Color: (" << this->EdgeColor[0] << ", " 
      << this->EdgeColor[1] << ", " << this->EdgeColor[2] << ")\n";
    os << indent << "Edge Visibility: " 
      << (this->EdgeVisibility ? "On\n" : "Off\n");
    os << indent << "Interpolation: ";
    switch (this->Interpolation) 
      {
    case 0: os << "VL_FLAT\n"; break;
    case 1: os << "VL_GOURAUD\n"; break;
    case 2: os << "VL_PHONG\n"; break;
    default: os << "unknown\n";
      }
    os << indent << "Representation: ";
    switch (this->Representation) 
      {
    case 0: os << "VL_POINTS\n"; break;
    case 1: os << "VL_WIREFRAME\n"; break;
    case 2: os << "VL_SURFACE\n"; break;
    default: os << "unknown\n";
      }
    os << indent << "Specular: " << this->Specular << "\n";
    os << indent << "Specular Color: (" << this->SpecularColor[0] << ", " 
      << this->SpecularColor[1] << ", " << this->SpecularColor[2] << ")\n";
    os << indent << "Specular Power: " << this->SpecularPower << "\n";
    os << indent << "Subdivide: " << (this->Subdivide ? "On\n" : "Off\n");
    os << indent << "Transparency: " << this->Transparency << "\n";
    }
}


