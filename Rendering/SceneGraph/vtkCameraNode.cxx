/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCameraNode.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"

//============================================================================
vtkStandardNewMacro(vtkCameraNode);

//----------------------------------------------------------------------------
vtkCameraNode::vtkCameraNode()
{
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->FocalPoint[0] = this->FocalPoint[1] = this->FocalPoint[2] = 0.0;
  this->ViewUp[0] = this->ViewUp[1] = this->ViewUp[2] = 0.0;
  this->ViewAngle = 0.0;
}

//----------------------------------------------------------------------------
vtkCameraNode::~vtkCameraNode()
{
}

//----------------------------------------------------------------------------
void vtkCameraNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkCameraNode::SynchronizeSelf()
{
  vtkCamera *mine = vtkCamera::SafeDownCast
    (this->GetRenderable());
  if (!mine)
    {
    return;
    }

  this->FocalPoint[0] = mine->GetFocalPoint()[0];
  this->FocalPoint[1] = mine->GetFocalPoint()[1];
  this->FocalPoint[2] = mine->GetFocalPoint()[2];
  this->Position[0] = mine->GetPosition()[0];
  this->Position[1] = mine->GetPosition()[1];
  this->Position[2] = mine->GetPosition()[2];
  this->ViewAngle = mine->GetViewAngle();
  this->ViewUp[0] = mine->GetViewUp()[0];
  this->ViewUp[1] = mine->GetViewUp()[1];
  this->ViewUp[2] = mine->GetViewUp()[2];
}
