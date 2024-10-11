// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_4_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkVRRenderWindow.h"

#include "vtkOpenGLState.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkTransform.h"
#include "vtkVRCamera.h"
#include "vtkVRModel.h"
#include "vtkVRRenderer.h"

// include what we need for the helper window
#ifdef WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif
#ifdef VTK_USE_X
#include "vtkXOpenGLRenderWindow.h"
#endif
#ifdef VTK_USE_COCOA
#include "vtkCocoaRenderWindow.h"
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
#define stricmp strcasecmp
#endif

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkVRRenderWindow::vtkVRRenderWindow()
{
  this->StereoCapableWindow = 1;
  this->StereoRender = 1;
  this->UseOffScreenBuffers = true;
  this->Size[0] = 640;
  this->Size[1] = 720;
  this->Position[0] = 100;
  this->Position[1] = 100;
  this->HelperWindow = vtkOpenGLRenderWindow::SafeDownCast(vtkRenderWindow::New());

  if (!this->HelperWindow)
  {
    vtkErrorMacro(<< "Failed to create render window");
  }
}

//------------------------------------------------------------------------------
vtkVRRenderWindow::~vtkVRRenderWindow()
{
  vtkRenderer* ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ((ren = this->Renderers->GetNextRenderer(rit)))
  {
    ren->SetRenderWindow(nullptr);
  }

  if (this->HelperWindow)
  {
    this->HelperWindow->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ContextId: " << this->HelperWindow->GetGenericContext() << "\n";
  os << indent << "Window Id: " << this->HelperWindow->GetGenericWindowId() << "\n";
  os << indent << "VRInitialized: " << this->VRInitialized << "\n";
  os << indent << "PhysicalViewDirection: (" << this->PhysicalViewDirection[0] << ", "
     << this->PhysicalViewDirection[1] << ", " << this->PhysicalViewDirection[2] << ")\n";
  os << indent << "PhysicalViewUp: (" << this->PhysicalViewUp[0] << ", " << this->PhysicalViewUp[1]
     << ", " << this->PhysicalViewUp[2] << ")\n";
  os << indent << "PhysicalTranslation: (" << this->PhysicalTranslation[0] << ", "
     << this->PhysicalTranslation[1] << ", " << this->PhysicalTranslation[2] << ")\n";
  os << indent << "PhysicalScale: " << this->PhysicalScale << "\n";
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::ReleaseGraphicsResources(vtkWindow* renWin)
{
  this->Superclass::ReleaseGraphicsResources(renWin);

  for (FramebufferDesc& fbo : this->FramebufferDescs)
  {
    glDeleteFramebuffers(1, &fbo.ResolveFramebufferId);
  }
  for (auto& model : this->DeviceHandleToDeviceDataMap)
  {
    if (model.second.Model)
    {
      model.second.Model->ReleaseGraphicsResources(renWin);
    }
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetHelperWindow(vtkOpenGLRenderWindow* win)
{
  if (this->HelperWindow == win)
  {
    return;
  }

  if (this->HelperWindow)
  {
    this->ReleaseGraphicsResources(this);
    this->HelperWindow->Delete();
  }

  this->HelperWindow = win;
  if (win)
  {
    win->Register(this);
  }

  this->Modified();
}

void vtkVRRenderWindow::AddDeviceHandle(uint32_t handle)
{
  auto found = this->DeviceHandleToDeviceDataMap.find(handle);
  if (found == this->DeviceHandleToDeviceDataMap.end())
  {
    this->DeviceHandleToDeviceDataMap[handle] = {};
  }
}

void vtkVRRenderWindow::AddDeviceHandle(uint32_t handle, vtkEventDataDevice device)
{
  auto found = this->DeviceHandleToDeviceDataMap.find(handle);
  if (found == this->DeviceHandleToDeviceDataMap.end())
  {
    this->DeviceHandleToDeviceDataMap[handle] = {};
    found = this->DeviceHandleToDeviceDataMap.find(handle);
  }
  found->second.Device = device;
}

void vtkVRRenderWindow::SetModelForDeviceHandle(uint32_t handle, vtkVRModel* model)
{
  auto found = this->DeviceHandleToDeviceDataMap.find(handle);
  if (found == this->DeviceHandleToDeviceDataMap.end())
  {
    this->DeviceHandleToDeviceDataMap[handle] = {};
    found = this->DeviceHandleToDeviceDataMap.find(handle);
  }
  found->second.Model = model;
}

vtkVRModel* vtkVRRenderWindow::GetModelForDevice(vtkEventDataDevice idx)
{
  auto handle = this->GetDeviceHandleForDevice(idx);
  return this->GetModelForDeviceHandle(handle);
}

vtkVRModel* vtkVRRenderWindow::GetModelForDeviceHandle(uint32_t handle)
{
  auto found = this->DeviceHandleToDeviceDataMap.find(handle);
  if (found == this->DeviceHandleToDeviceDataMap.end())
  {
    return nullptr;
  }
  return found->second.Model;
}

vtkMatrix4x4* vtkVRRenderWindow::GetDeviceToPhysicalMatrixForDevice(vtkEventDataDevice idx)
{
  auto handle = this->GetDeviceHandleForDevice(idx);
  return this->GetDeviceToPhysicalMatrixForDeviceHandle(handle);
}

vtkMatrix4x4* vtkVRRenderWindow::GetDeviceToPhysicalMatrixForDeviceHandle(uint32_t handle)
{
  auto found = this->DeviceHandleToDeviceDataMap.find(handle);
  if (found == this->DeviceHandleToDeviceDataMap.end())
  {
    return nullptr;
  }
  return found->second.DeviceToPhysicalMatrix;
}

uint32_t vtkVRRenderWindow::GetDeviceHandleForDevice(vtkEventDataDevice idx, uint32_t index)
{
  for (auto& deviceData : this->DeviceHandleToDeviceDataMap)
  {
    if (deviceData.second.Device == idx && deviceData.second.Index == index)
    {
      return deviceData.first;
    }
  }
  return InvalidDeviceIndex;
}

uint32_t vtkVRRenderWindow::GetNumberOfDeviceHandlesForDevice(vtkEventDataDevice dev)
{
  uint32_t count = 0;
  for (auto& deviceData : this->DeviceHandleToDeviceDataMap)
  {
    if (deviceData.second.Device == dev)
    {
      count++;
    }
  }
  return count;
}

// default implementation just uses the vtkEventDataDevice
vtkEventDataDevice vtkVRRenderWindow::GetDeviceForDeviceHandle(uint32_t handle)
{
  auto found = this->DeviceHandleToDeviceDataMap.find(handle);
  if (found == this->DeviceHandleToDeviceDataMap.end())
  {
    return vtkEventDataDevice::Unknown;
  }
  return found->second.Device;
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::InitializeViewFromCamera(vtkCamera* srccam)
{
  vtkRenderer* ren = vtkRenderer::SafeDownCast(this->GetRenderers()->GetItemAsObject(0));
  if (!ren)
  {
    vtkErrorMacro("The renderer must be set prior to calling InitializeViewFromCamera");
    return;
  }

  vtkVRCamera* cam = vtkVRCamera::SafeDownCast(ren->GetActiveCamera());
  if (!cam)
  {
    vtkErrorMacro(
      "The renderer's active camera must be set prior to calling InitializeViewFromCamera");
    return;
  }

  // make sure the view up is reasonable based on the view up
  // that was set in PV
  double distance = sin(vtkMath::RadiansFromDegrees(srccam->GetViewAngle()) / 2.0) *
    srccam->GetDistance() / sin(vtkMath::RadiansFromDegrees(cam->GetViewAngle()) / 2.0);

  double* oldVup = srccam->GetViewUp();
  int maxIdx = fabs(oldVup[0]) > fabs(oldVup[1]) ? (fabs(oldVup[0]) > fabs(oldVup[2]) ? 0 : 2)
                                                 : (fabs(oldVup[1]) > fabs(oldVup[2]) ? 1 : 2);

  cam->SetViewUp((maxIdx == 0 ? (oldVup[0] > 0 ? 1 : -1) : 0.0),
    (maxIdx == 1 ? (oldVup[1] > 0 ? 1 : -1) : 0.0), (maxIdx == 2 ? (oldVup[2] > 0 ? 1 : -1) : 0.0));
  this->SetPhysicalViewUp((maxIdx == 0 ? (oldVup[0] > 0 ? 1 : -1) : 0.0),
    (maxIdx == 1 ? (oldVup[1] > 0 ? 1 : -1) : 0.0), (maxIdx == 2 ? (oldVup[2] > 0 ? 1 : -1) : 0.0));

  double* oldFP = srccam->GetFocalPoint();
  double* cvup = cam->GetViewUp();
  cam->SetFocalPoint(oldFP);
  this->SetPhysicalTranslation(
    cvup[0] * distance - oldFP[0], cvup[1] * distance - oldFP[1], cvup[2] * distance - oldFP[2]);
  this->SetPhysicalScale(distance);

  double* oldDOP = srccam->GetDirectionOfProjection();
  int dopMaxIdx = fabs(oldDOP[0]) > fabs(oldDOP[1]) ? (fabs(oldDOP[0]) > fabs(oldDOP[2]) ? 0 : 2)
                                                    : (fabs(oldDOP[1]) > fabs(oldDOP[2]) ? 1 : 2);
  this->SetPhysicalViewDirection((dopMaxIdx == 0 ? (oldDOP[0] > 0 ? 1 : -1) : 0.0),
    (dopMaxIdx == 1 ? (oldDOP[1] > 0 ? 1 : -1) : 0.0),
    (dopMaxIdx == 2 ? (oldDOP[2] > 0 ? 1 : -1) : 0.0));
  double* idop = this->GetPhysicalViewDirection();
  cam->SetPosition(
    -idop[0] * distance + oldFP[0], -idop[1] * distance + oldFP[1], -idop[2] * distance + oldFP[2]);

  ren->ResetCameraClippingRange();
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::MakeCurrent()
{
  if (this->HelperWindow)
  {
    this->HelperWindow->MakeCurrent();
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::ReleaseCurrent()
{
  if (this->HelperWindow)
  {
    this->HelperWindow->ReleaseCurrent();
  }
}

//------------------------------------------------------------------------------
vtkOpenGLState* vtkVRRenderWindow::GetState()
{
  if (this->HelperWindow)
  {
    return this->HelperWindow->GetState();
  }
  return this->Superclass::GetState();
}

//------------------------------------------------------------------------------
bool vtkVRRenderWindow::IsCurrent()
{
  return this->HelperWindow ? this->HelperWindow->IsCurrent() : false;
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::AddRenderer(vtkRenderer* ren)
{
  if (ren && !vtkVRRenderer::SafeDownCast(ren))
  {
    vtkErrorMacro("vtkVRRenderWindow::AddRenderer: Failed to add renderer of type "
      << ren->GetClassName() << ": A subclass of vtkVRRenderer is expected");
    return;
  }
  this->Superclass::AddRenderer(ren);
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::Render()
{
  this->MakeCurrent();
  this->GetState()->ResetGLViewportState();
  this->Superclass::Render();
}

//------------------------------------------------------------------------------
bool vtkVRRenderWindow::GetDeviceToWorldMatrixForDevice(
  vtkEventDataDevice device, vtkMatrix4x4* deviceToWorldMatrix)
{
  vtkMatrix4x4* deviceToPhysicalMatrix = this->GetDeviceToPhysicalMatrixForDevice(device);

  if (deviceToPhysicalMatrix)
  {
    // we use deviceToWorldMatrix here to temporarily store physicalToWorld
    // to avoid having to use a temp matrix. We use a new pointer just to
    // keep the code easier to read.
    vtkMatrix4x4* physicalToWorldMatrix = deviceToWorldMatrix;
    this->GetPhysicalToWorldMatrix(physicalToWorldMatrix);
    vtkMatrix4x4::Multiply4x4(physicalToWorldMatrix, deviceToPhysicalMatrix, deviceToWorldMatrix);
    return true;
  }
  return false;
}

bool vtkVRRenderWindow::GetDeviceToWorldMatrixForDeviceHandle(
  uint32_t handle, vtkMatrix4x4* deviceToWorldMatrix)
{
  vtkMatrix4x4* deviceToPhysicalMatrix = this->GetDeviceToPhysicalMatrixForDeviceHandle(handle);

  if (deviceToPhysicalMatrix)
  {
    // we use deviceToWorldMatrix here to temporarily store physicalToWorld
    // to avoid having to use a temp matrix. We use a new pointer just to
    // keep the code easier to read.
    vtkMatrix4x4* physicalToWorldMatrix = deviceToWorldMatrix;
    this->GetPhysicalToWorldMatrix(physicalToWorldMatrix);
    vtkMatrix4x4::Multiply4x4(physicalToWorldMatrix, deviceToPhysicalMatrix, deviceToWorldMatrix);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
int* vtkVRRenderWindow::GetScreenSize()
{
  if (this->GetSizeFromAPI())
  {
    this->ScreenSize[0] = this->Size[0];
    this->ScreenSize[1] = this->Size[1];
  }
  return this->ScreenSize;
}

//------------------------------------------------------------------------------
void vtkVRRenderWindow::SetSize(int width, int height)
{
  if ((this->Size[0] != width) || (this->Size[1] != height))
  {
    this->Superclass::SetSize(width, height);

    if (this->Interactor)
    {
      this->Interactor->SetSize(width, height);
    }
  }
}

void vtkVRRenderWindow::SetTrackHMD(bool trackHMD)
{
  vtkDebugMacro(<< " setting TrackHMD to " << trackHMD);
  if (this->TrackHMD != trackHMD)
  {
    this->TrackHMD = trackHMD;
    this->Modified();
  }
}

bool vtkVRRenderWindow::GetTrackHMD()
{
  vtkDebugMacro(<< " returning TrackHMD of " << this->TrackHMD);
  return this->TrackHMD;
}

VTK_ABI_NAMESPACE_END
