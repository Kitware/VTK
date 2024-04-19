// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenVRCamera.h"

#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLState.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkPerspectiveTransform.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenVRCamera);

vtkOpenVRCamera::vtkOpenVRCamera() = default;
vtkOpenVRCamera::~vtkOpenVRCamera() = default;

// a reminder, with vtk order matrices multiplcation goes right to left
// e.g. vtkMatrix4x4::Multiply(BtoC, AtoB, AtoC);

namespace
{
void setMatrixFromOpenVRMatrix(vtkMatrix4x4* result, const vr::HmdMatrix34_t& vrMat)
{
  // because openvr is in left handed coords we
  // have to invert z, apply the transform, then invert z again
  for (vtkIdType i = 0; i < 3; ++i)
  {
    for (vtkIdType j = 0; j < 4; ++j)
    {
      result->SetElement(i, j, ((i == 2) != (j == 2) ? -1 : 1) * vrMat.m[i][j]);
    }
  }

  // Add last row
  result->SetElement(3, 0, 0.0);
  result->SetElement(3, 1, 0.0);
  result->SetElement(3, 2, 0.0);
  result->SetElement(3, 3, 1.0);
  result->Invert();
}
}

//------------------------------------------------------------------------------
// we could try to do some smart caching here where we only
// check the eyetohead transform when the ipd changes etc
void vtkOpenVRCamera::UpdateHMDToEyeMatrices(vtkRenderer* ren)
{
  vtkOpenVRRenderWindow* win = vtkOpenVRRenderWindow::SafeDownCast(ren->GetRenderWindow());

  vr::IVRSystem* hMD = win->GetHMD();

  vr::HmdMatrix34_t matEye = hMD->GetEyeToHeadTransform(vr::Eye_Left);
  setMatrixFromOpenVRMatrix(this->HMDToLeftEyeMatrix, matEye);

  matEye = hMD->GetEyeToHeadTransform(vr::Eye_Right);
  setMatrixFromOpenVRMatrix(this->HMDToRightEyeMatrix, matEye);
}

//------------------------------------------------------------------------------
void vtkOpenVRCamera::UpdateWorldToEyeMatrices(vtkRenderer* ren)
{
  // could do this next call only every now and then as these matrices
  // rarely change typically only when the user is changing the ipd
  this->UpdateHMDToEyeMatrices(ren);

  vtkOpenVRRenderWindow* win = vtkOpenVRRenderWindow::SafeDownCast(ren->GetRenderWindow());

  auto hmdHandle = win->GetDeviceHandleForOpenVRHandle(vr::k_unTrackedDeviceIndex_Hmd);

  // first we get the physicalToHMDMatrix (by inverting deviceToPhysical for the HMD)
  auto* deviceToPhysical = win->GetDeviceToPhysicalMatrixForDeviceHandle(hmdHandle);
  if (!deviceToPhysical)
  {
    return;
  }
  this->PhysicalToHMDMatrix->DeepCopy(deviceToPhysical);
  this->PhysicalToHMDMatrix->Invert();

  // compute the physicalToEye matrices
  vtkMatrix4x4::Multiply4x4(
    this->HMDToLeftEyeMatrix, this->PhysicalToHMDMatrix, this->PhysicalToLeftEyeMatrix);
  vtkMatrix4x4::Multiply4x4(
    this->HMDToRightEyeMatrix, this->PhysicalToHMDMatrix, this->PhysicalToRightEyeMatrix);

  // get the world to physical matrix by inverting phsycialToWorld
  win->GetPhysicalToWorldMatrix(this->WorldToPhysicalMatrix);
  this->WorldToPhysicalMatrix->Invert();

  // compute the world to eye matrices
  vtkMatrix4x4::Multiply4x4(
    this->PhysicalToLeftEyeMatrix, this->WorldToPhysicalMatrix, this->WorldToLeftEyeMatrix);
  vtkMatrix4x4::Multiply4x4(
    this->PhysicalToRightEyeMatrix, this->WorldToPhysicalMatrix, this->WorldToRightEyeMatrix);
}

//------------------------------------------------------------------------------
void vtkOpenVRCamera::UpdateEyeToProjectionMatrices(vtkRenderer* ren)
{
  vtkOpenVRRenderWindow* win = vtkOpenVRRenderWindow::SafeDownCast(ren->GetRenderWindow());

  vr::IVRSystem* hMD = win->GetHMD();

  double scale = win->GetPhysicalScale();
  double znear = this->ClippingRange[0] / scale;
  double zfar = this->ClippingRange[1] / scale;

  float fxmin, fxmax, fymin, fymax;
  double xmin, xmax, ymin, ymax;

  // note docs are probably wrong in OpenVR arg list for this func
  hMD->GetProjectionRaw(vr::Eye_Left, &fxmin, &fxmax, &fymin, &fymax);
  xmin = fxmin * znear;
  xmax = fxmax * znear;
  ymin = fymin * znear;
  ymax = fymax * znear;

  this->LeftEyeToProjectionMatrix->Zero();
  this->LeftEyeToProjectionMatrix->SetElement(0, 0, 2 * znear / (xmax - xmin));
  this->LeftEyeToProjectionMatrix->SetElement(1, 1, 2 * znear / (ymax - ymin));
  this->LeftEyeToProjectionMatrix->SetElement(0, 2, (xmin + xmax) / (xmax - xmin));
  this->LeftEyeToProjectionMatrix->SetElement(1, 2, (ymin + ymax) / (ymax - ymin));
  this->LeftEyeToProjectionMatrix->SetElement(2, 2, -(znear + zfar) / (zfar - znear));
  this->LeftEyeToProjectionMatrix->SetElement(3, 2, -1);
  this->LeftEyeToProjectionMatrix->SetElement(2, 3, -2 * znear * zfar / (zfar - znear));

  hMD->GetProjectionRaw(vr::Eye_Right, &fxmin, &fxmax, &fymin, &fymax);
  xmin = fxmin * znear;
  xmax = fxmax * znear;
  ymin = fymin * znear;
  ymax = fymax * znear;

  this->RightEyeToProjectionMatrix->Zero();
  this->RightEyeToProjectionMatrix->SetElement(0, 0, 2 * znear / (xmax - xmin));
  this->RightEyeToProjectionMatrix->SetElement(1, 1, 2 * znear / (ymax - ymin));
  this->RightEyeToProjectionMatrix->SetElement(0, 2, (xmin + xmax) / (xmax - xmin));
  this->RightEyeToProjectionMatrix->SetElement(1, 2, (ymin + ymax) / (ymax - ymin));
  this->RightEyeToProjectionMatrix->SetElement(2, 2, -(znear + zfar) / (zfar - znear));
  this->RightEyeToProjectionMatrix->SetElement(3, 2, -1);
  this->RightEyeToProjectionMatrix->SetElement(2, 3, -2 * znear * zfar / (zfar - znear));
}

//------------------------------------------------------------------------------
void vtkOpenVRCamera::Render(vtkRenderer* ren)
{
  vtkOpenGLClearErrorMacro();

  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = win->GetState();

  int renSize[2];
  win->GetRenderBufferSize(renSize[0], renSize[1]);

  // if were on a stereo renderer draw to special parts of screen
  if (this->LeftEye)
  {
    // Left Eye
    if (win->GetMultiSamples() && !ren->GetSelector())
    {
      ostate->vtkglEnable(GL_MULTISAMPLE);
    }
  }
  else
  {
    // right eye
    if (win->GetMultiSamples() && !ren->GetSelector())
    {
      ostate->vtkglEnable(GL_MULTISAMPLE);
    }
  }

  ostate->vtkglViewport(0, 0, renSize[0], renSize[1]);
  ostate->vtkglScissor(0, 0, renSize[0], renSize[1]);
  ren->Clear();
  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}
VTK_ABI_NAMESPACE_END
