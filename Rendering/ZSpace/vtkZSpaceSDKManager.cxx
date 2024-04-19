// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkZSpaceSDKManager.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"

#if VTK_ZSPACE_USE_COMPAT_SDK
#include "vtkZSpaceCoreCompatibilitySDKManager.h"
#else
#include "vtkZSpaceCoreSDKManager.h"
#endif

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkZSpaceSDKManager::vtkZSpaceSDKManager() = default;

//----------------------------------------------------------------------------
vtkZSpaceSDKManager::~vtkZSpaceSDKManager() = default;

//----------------------------------------------------------------------------
vtkZSpaceSDKManager* vtkZSpaceSDKManager::GetInstance()
{
  static vtkSmartPointer<vtkZSpaceSDKManager> instance = nullptr;
  if (instance.GetPointer() == nullptr)
  {
#if VTK_ZSPACE_USE_COMPAT_SDK
    instance = vtkSmartPointer<vtkZSpaceCoreCompatibilitySDKManager>::New();
    vtkDebugWithObjectMacro(instance, "USING CORE COMPATIBILITY ZSPACE SDK");
#else
    instance = vtkSmartPointer<vtkZSpaceCoreSDKManager>::New();
    vtkDebugWithObjectMacro(instance, "USING LEGACY ZSPACE SDK");
#endif
  }

  return instance;
}

//----------------------------------------------------------------------------
void vtkZSpaceSDKManager::SetRenderWindow(vtkRenderWindow* renderWindow)
{
  this->RenderWindow = renderWindow;
};

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::Update()
{
  this->UpdateViewport();
  this->UpdateViewAndProjectionMatrix();
  this->UpdateTrackers();
  this->UpdateButtonState();
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::SetClippingRange(const float nearPlane, const float farPlane)
{
  this->NearPlane = nearPlane;
  this->FarPlane = farPlane;
}

//------------------------------------------------------------------------------
vtkMatrix4x4* vtkZSpaceSDKManager::GetStereoViewMatrix(bool leftEye)
{
  return leftEye ? this->LeftEyeViewMatrix : this->RightEyeViewMatrix;
}

//------------------------------------------------------------------------------
vtkMatrix4x4* vtkZSpaceSDKManager::GetStereoProjectionMatrix(bool leftEye)
{
  return leftEye ? this->LeftEyeProjectionMatrix : this->RightEyeProjectionMatrix;
}

//------------------------------------------------------------------------------
void vtkZSpaceSDKManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "WindowX: " << this->WindowX << "\n";
  os << indent << "WindowY: " << this->WindowY << "\n";
  os << indent << "WindowWidth: " << this->WindowWidth << "\n";
  os << indent << "WindowHeight: " << this->WindowHeight << "\n";
  os << indent << "NbDisplays: " << this->Displays.size() << "\n";
  for (auto const& display : this->Displays)
  {
    os << indent << "\t" << display << "\n";
  }
  os << indent << "StylusTargets: " << this->StylusTargets << "\n";
  os << indent << "HeadTargets: " << this->HeadTargets << "\n";
  os << indent << "SecondaryTargets: " << this->SecondaryTargets << "\n";
  os << indent << "InterPupillaryDistance: " << this->InterPupillaryDistance << "\n";
  os << indent << "ViewerScale: " << this->ViewerScale << "\n";
  os << indent << "NearPlane: " << this->NearPlane << "\n";
  os << indent << "FarPlane: " << this->FarPlane << "\n";
  os << indent << "LeftButtonState: " << this->LeftButtonState << "\n";
  os << indent << "MiddleButtonState: " << this->MiddleButtonState << "\n";
  os << indent << "RightButtonState: " << this->RightButtonState << "\n";
}

VTK_ABI_NAMESPACE_END
