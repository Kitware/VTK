// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAvatar.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkAvatar);

//------------------------------------------------------------------------------
void vtkAvatar::PrintSelf(std::ostream& os, vtkIndent indent)
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

  for (int i = 0; i < NUM_BODY; ++i)
  {
    this->BodyPosition[i][0] = 0.0;
    this->BodyPosition[i][1] = 0.0;
    this->BodyPosition[i][2] = 0.0;

    this->BodyOrientation[i][0] = 0.0;
    this->BodyOrientation[i][1] = 0.0;
    this->BodyOrientation[i][2] = 0.0;
  }
  this->UpVector[0] = 0.0;
  this->UpVector[1] = 1.0;
  this->UpVector[2] = 0.0;

  UseLeftHand = true;
  UseRightHand = true;
  ShowHandsOnly = false;
}

//------------------------------------------------------------------------------
vtkAvatar::~vtkAvatar() = default;
VTK_ABI_NAMESPACE_END
