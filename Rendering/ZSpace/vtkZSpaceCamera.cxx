// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkZSpaceCamera.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPerspectiveTransform.h"
#include "vtkTransform.h"
#include "vtkZSpaceSDKManager.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkZSpaceCamera);

//----------------------------------------------------------------------------
void vtkZSpaceCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkMatrix4x4* vtkZSpaceCamera::GetModelViewTransformMatrix()
{
  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();

  vtkMatrix4x4* zSpaceViewMatrix = this->GetStereo()
    ? sdkManager->GetStereoViewMatrix(this->GetLeftEye())
    : sdkManager->GetCenterEyeViewMatrix();

  this->ViewTransform->SetMatrix(zSpaceViewMatrix);

  this->Transform->Identity();
  this->Transform->SetupCamera(this->Position, this->FocalPoint, this->ViewUp);

  this->ViewTransform->Concatenate(this->Transform->GetMatrix());

  return this->ViewTransform->GetMatrix();
}

//------------------------------------------------------------------------------
vtkMatrix4x4* vtkZSpaceCamera::GetProjectionTransformMatrix(
  double vtkNotUsed(aspect), double vtkNotUsed(nearz), double vtkNotUsed(farz))
{
  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();

  vtkMatrix4x4* zSpaceProjectionMatrix = this->GetStereo()
    ? sdkManager->GetStereoProjectionMatrix(this->GetLeftEye())
    : sdkManager->GetCenterEyeProjectionMatrix();
  return zSpaceProjectionMatrix;
}

VTK_ABI_NAMESPACE_END
