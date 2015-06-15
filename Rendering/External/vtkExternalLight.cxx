/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExternalLight.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExternalLight.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGL.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkExternalLight);

//-----------------------------------------------------------------------------
vtkExternalLight::vtkExternalLight()
{
  this->LightIndex = GL_LIGHT0;
  this->ReplaceMode = INDIVIDUAL_PARAMS;

  // By default, nothing is set by user
  this->PositionSet = false;
  this->FocalPointSet = false;
  this->AmbientColorSet = false;
  this->DiffuseColorSet = false;
  this->SpecularColorSet = false;
  this->IntensitySet = false;
  this->ConeAngleSet = false;
  this->AttenuationValuesSet = false;
  this->ExponentSet = false;
  this->PositionalSet = false;

  // Set the default light type to headlight
  this->LightType = VTK_LIGHT_TYPE_HEADLIGHT;
}

//-----------------------------------------------------------------------------
vtkExternalLight::~vtkExternalLight()
{
}

//-----------------------------------------------------------------------------
void vtkExternalLight::SetPosition(double position1,
                                   double position2,
                                   double position3)
{
  this->Superclass::SetPosition(position1, position2, position3);
  this->PositionSet = true;
}

//-----------------------------------------------------------------------------
void vtkExternalLight::SetFocalPoint(double focalpoint1,
                                     double focalpoint2,
                                     double focalpoint3)
{
  this->Superclass::SetFocalPoint(focalpoint1, focalpoint2, focalpoint3);
  this->FocalPointSet = true;
}

//-----------------------------------------------------------------------------
void vtkExternalLight::SetAmbientColor(double color1,
                                       double color2,
                                       double color3)
{
  this->Superclass::SetAmbientColor(color1, color2, color3);
  this->AmbientColorSet = true;
}

//-----------------------------------------------------------------------------
void vtkExternalLight::SetDiffuseColor(double color1,
                                       double color2,
                                       double color3)
{
  this->Superclass::SetDiffuseColor(color1, color2, color3);
  this->DiffuseColorSet = true;
}

//-----------------------------------------------------------------------------
void vtkExternalLight::SetSpecularColor(double color1,
                                        double color2,
                                        double color3)
{
  this->Superclass::SetSpecularColor(color1, color2, color3);
  this->SpecularColorSet = true;
}

//-----------------------------------------------------------------------------
void vtkExternalLight::SetIntensity(double intensity)
{
  this->Superclass::SetIntensity(intensity);
  this->IntensitySet = true;
}

//-----------------------------------------------------------------------------
void vtkExternalLight::SetConeAngle(double angle)
{
  this->Superclass::SetConeAngle(angle);
  this->ConeAngleSet = true;
}

//-----------------------------------------------------------------------------
void vtkExternalLight::SetAttenuationValues(double value1,
                                            double value2,
                                            double value3)
{
  this->Superclass::SetAttenuationValues(value1, value2, value3);
  this->AttenuationValuesSet = true;
}

//-----------------------------------------------------------------------------
void vtkExternalLight::SetExponent(double exp)
{
  this->Superclass::SetExponent(exp);
  this->ExponentSet = true;
}

//-----------------------------------------------------------------------------
void vtkExternalLight::SetPositional(int p)
{
  this->Superclass::SetPositional(p);
  this->PositionalSet = true;
}

//-----------------------------------------------------------------------------
void vtkExternalLight::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LightIndex: " << this->LightIndex << "\n";
  os << indent << "ReplaceMode: " << this->ReplaceMode << "\n";

  os << indent << "PositionSet: " << this->PositionSet << "\n";
  os << indent << "FocalPointSet: " << this->FocalPointSet << "\n";
  os << indent << "AmbientColorSet: " << this->AmbientColorSet << "\n";
  os << indent << "DiffuseColorSet: " << this->DiffuseColorSet << "\n";
  os << indent << "SpecularColorSet: " << this->SpecularColorSet << "\n";
  os << indent << "IntensitySet: " << this->IntensitySet << "\n";
  os << indent << "ConeAngleSet: " << this->ConeAngleSet << "\n";
  os << indent << "AttenuationValuesSet: " <<
    this->AttenuationValuesSet << "\n";
  os << indent << "ExponentSet: " << this->ExponentSet << "\n";
  os << indent << "PositionalSet: " << this->PositionalSet << "\n";
}
