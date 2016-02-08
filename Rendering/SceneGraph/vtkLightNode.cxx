/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLightNode.h"
#include "vtkObjectFactory.h"

#include "vtkLight.h"

//============================================================================
vtkStandardNewMacro(vtkLightNode);

//----------------------------------------------------------------------------
vtkLightNode::vtkLightNode()
{
  this->AmbientColor[0] =
    this->AmbientColor[1] =
    this->AmbientColor[2] = 0.0;
  this->AttenuationValues[0] =
    this->AttenuationValues[1] =
    this->AttenuationValues[2] = 0.0;
  this->ConeAngle = 0.0;
  this->DiffuseColor[0] = this->DiffuseColor[1] = this->DiffuseColor[2] = 0.0;
  this->Exponent = 0.0;
  this->FocalPoint[0] = this->FocalPoint[1] = this->FocalPoint[2] = 0.0;
  this->Intensity = 0.0;
  this->LightType = 0;
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->Positional = false;
  this->SpecularColor[0] =
    this->SpecularColor[1] =
    this->SpecularColor[2] = 0.0;
  this->Switch = false;
}

//----------------------------------------------------------------------------
vtkLightNode::~vtkLightNode()
{
}

//----------------------------------------------------------------------------
void vtkLightNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkLightNode::SynchronizeSelf()
{
  vtkLight *mine = vtkLight::SafeDownCast
    (this->GetRenderable());
  if (!mine)
    {
    return;
    }

  //get state from our renderable
  mine->GetAmbientColor(this->AmbientColor);
  mine->GetAttenuationValues(this->AttenuationValues);
  this->ConeAngle = mine->GetConeAngle();
  mine->GetDiffuseColor(this->DiffuseColor);
  this->Exponent = mine->GetExponent();
  mine->GetFocalPoint(this->FocalPoint);
  this->Intensity = mine->GetIntensity();
  this->LightType = mine->GetLightType();
  mine->GetPosition(this->Position);
  this->Positional = mine->GetPositional();
  mine->GetSpecularColor(this->SpecularColor);
  this->Switch = mine->GetSwitch();
  /*
    GetTransformMatrix()
  */
}
