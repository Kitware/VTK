/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Light.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <stdlib.h>
#include <iostream.h>
#include "Light.hh"
#include "Renderer.hh"
#include "RenderW.hh"
#include "LgtDev.hh"

// Description:
// Create a light with focal point at origin and position=(0,0,1).
// Light color is white, intensity=1, and the light is turned on.
vtkLight::vtkLight()
{
  this->FocalPoint[0] = 0.0;
  this->FocalPoint[1] = 0.0;
  this->FocalPoint[2] = 0.0;

  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 1.0;

  this->Color[0] = 1.0;
  this->Color[1] = 1.0;
  this->Color[2] = 1.0;

  this->Switch = 1;

  this->Intensity = 1.0;
  this->Positional = 0;
  this->ConeAngle= 30;
  this->AttenuationValues[0] = 1;
  this->AttenuationValues[1] = 0;
  this->AttenuationValues[2] = 0;
  this->Exponent = 1;
  this->Device = NULL;
}

vtkLight::~vtkLight()
{
  if (this->Device)
    {
    delete this->Device;
    }
}

void vtkLight::Render(vtkRenderer *ren,int light_index)
{
  if (!this->Device)
    {
    this->Device = ren->GetRenderWindow()->MakeLight();
    }
  this->Device->Render(this,ren,light_index);
}

void vtkLight::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "AttenuationValues: (" << this->AttenuationValues[0] << ", " 
    << this->AttenuationValues[1] << ", " << this->AttenuationValues[2] << ")\n";
  os << indent << "Color: (" << this->Color[0] << ", " 
    << this->Color[1] << ", " << this->Color[2] << ")\n";
  os << indent << "Cone Angle: " << this->ConeAngle << "\n";
  if ( this->Device )
    {
    os << indent << "Device:\n";
    this->Device->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Device: (none)\n";
    }
  os << indent << "Exponent: " << this->Exponent << "\n";
  os << indent << "Focal Point: (" << this->FocalPoint[0] << ", " 
    << this->FocalPoint[1] << ", " << this->FocalPoint[2] << ")\n";
  os << indent << "Intensity: " << this->Intensity << "\n";
  os << indent << "Position: (" << this->Position[0] << ", " 
    << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Positional: " << (this->Positional ? "On\n" : "Off\n");
  os << indent << "Switch: " << (this->Switch ? "On\n" : "Off\n");
}




