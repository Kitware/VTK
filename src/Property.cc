/*=========================================================================

  Program:   OSCAR 
  Module:    Property.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#include <stdlib.h>
#include "Property.hh"

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
  this->Interpolation = 1;   /* gouraud */
  this->Representation = 2;  /* solid */
  this->EdgeVisibility = 0;
  this->Backface = 0;
  this->Subdivide = 0;
}


void vlProperty::SetFlat (void)
{
  this->Interpolation= FLAT;
}

void vlProperty::SetGouraud (void)
{
  this->Interpolation = GOURAUD;
}

void vlProperty::SetPhong (void)
{
  this->Interpolation = PHONG;
}

void vlProperty::SetPoints (void)
{
  this->Interpolation = POINTS;
}

void vlProperty::SetWireframe (void)
{
  this->Representation = WIREFRAME;
}

void vlProperty::SetSurface (void)
{
  this->Representation = SURFACE;
}

/* this is a standard vector set method except that it sets the */
/* ambient diffuse and specular colors as well                  */
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



float vlProperty::GetTransparency (void)
{
  return this->Transparency;
}

int vlProperty::GetRepresentation (void)
{
  return this->Representation;
}

int vlProperty::GetInterpolation (void)
{
  return this->Interpolation;
}
