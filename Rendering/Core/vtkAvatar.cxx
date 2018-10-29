/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAvatar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAvatar.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkAvatar)

//------------------------------------------------------------------------------
void vtkAvatar::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkAvatar::vtkAvatar()
{
  this->HeadPosition[0] = 0.0;
  this->HeadPosition[1] = 0.0;
  this->HeadPosition[2] = 0.0;

  this->HeadOrientation[0] = 0.0;
  this->HeadOrientation[1] = 0.0;
  this->HeadOrientation[2] = 0.0;

  this->RightHandPosition[0] = 0.0;
  this->RightHandPosition[1] = 0.0;
  this->RightHandPosition[2] = 0.0;

  this->RightHandOrientation[0] = 0.0;
  this->RightHandOrientation[1] = 0.0;
  this->RightHandOrientation[2] = 0.0;

  this->LeftHandPosition[0] = 0.0;
  this->LeftHandPosition[1] = 0.0;
  this->LeftHandPosition[2] = 0.0;

  this->LeftHandOrientation[0] = 0.0;
  this->LeftHandOrientation[1] = 0.0;
  this->LeftHandOrientation[2] = 0.0;
}

//------------------------------------------------------------------------------
vtkAvatar::~vtkAvatar() = default;
