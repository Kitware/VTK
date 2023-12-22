// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVRHMDCamera.h"

#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLState.h"
#include "vtkPerspectiveTransform.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkVRRenderWindow.h"

#include <cmath>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkVRHMDCamera::vtkVRHMDCamera()
{
  // approximate for Vive
  // we use the projection matrix directly from the vive
  // so this is just to help make view <--> display
  // adjustments reasonable, not correct, just reasonable
  this->SetViewAngle(110.0);
}

vtkVRHMDCamera::~vtkVRHMDCamera() = default;

// a reminder, with vtk order matrices multiplcation goes right to left
// e.g. vtkMatrix4x4::Multiply(BtoC, AtoB, AtoC);

//------------------------------------------------------------------------------
void vtkVRHMDCamera::Render(vtkRenderer* ren)
{
  vtkOpenGLClearErrorMacro();

  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = win->GetState();

  int renSize[2];
  win->GetRenderBufferSize(renSize[0], renSize[1]);

  // if were on a stereo renderer draw to special parts of screen
  if (win->GetMultiSamples() && !ren->GetSelector())
  {
    ostate->vtkglEnable(GL_MULTISAMPLE);
  }

  ostate->vtkglViewport(0, 0, renSize[0], renSize[1]);
  ostate->vtkglScissor(0, 0, renSize[0], renSize[1]);
  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
void vtkVRHMDCamera::GetKeyMatrices(vtkRenderer* ren, vtkMatrix4x4*& wcvc, vtkMatrix3x3*& normMat,
  vtkMatrix4x4*& vcdc, vtkMatrix4x4*& wcdc)
{
  if (ren->GetSelector())
  {
    return this->Superclass::GetKeyMatrices(ren, wcvc, normMat, vcdc, wcdc);
  }

  // has the camera changed?
  if (ren != this->LastRenderer || this->MTime > this->KeyMatrixTime ||
    ren->GetMTime() > this->KeyMatrixTime)
  {
    vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(ren->GetRenderWindow());

    // on left eye render we compute the NormalMatrix, this should be fairly
    // accurate for both eyes.
    if (this->LeftEye)
    {
      this->UpdateWorldToEyeMatrices(ren);
      this->UpdateEyeToProjectionMatrices(ren);

      vtkMatrix4x4* w2v = this->GetModelViewTransformMatrix();
      this->WCVCMatrix->DeepCopy(w2v);

      // only compute normal matrix once
      for (int i = 0; i < 3; ++i)
      {
        for (int j = 0; j < 3; ++j)
        {
          this->NormalMatrix->SetElement(i, j, w2v->GetElement(i, j));
        }
      }
      this->NormalMatrix->Invert();
    }

    if (this->LeftEye)
    {
      this->SetCameraFromWorldToDeviceMatrix(this->WorldToLeftEyeMatrix, win->GetPhysicalScale());
      this->ModelViewTransform->SetMatrix(this->WorldToLeftEyeMatrix);
      this->WCVCMatrix->DeepCopy(this->WorldToLeftEyeMatrix);
      this->WCVCMatrix->Transpose();

      vtkMatrix4x4::Multiply4x4(
        this->LeftEyeToProjectionMatrix, this->WorldToLeftEyeMatrix, this->WCDCMatrix);
      this->WCDCMatrix->Transpose();

      vtkMatrix4x4::Multiply4x4(this->LeftEyeToProjectionMatrix, this->PhysicalToLeftEyeMatrix,
        this->PhysicalToProjectionMatrixForLeftEye);

      this->VCDCMatrix->DeepCopy(this->LeftEyeToProjectionMatrix);
      this->VCDCMatrix->Transpose();
    }
    else
    {
      this->SetCameraFromWorldToDeviceMatrix(this->WorldToRightEyeMatrix, win->GetPhysicalScale());
      this->ModelViewTransform->SetMatrix(this->WorldToRightEyeMatrix);
      this->WCVCMatrix->DeepCopy(this->WorldToRightEyeMatrix);
      this->WCVCMatrix->Transpose();

      vtkMatrix4x4::Multiply4x4(
        this->RightEyeToProjectionMatrix, this->WorldToRightEyeMatrix, this->WCDCMatrix);
      this->WCDCMatrix->Transpose();

      vtkMatrix4x4::Multiply4x4(this->RightEyeToProjectionMatrix, this->PhysicalToRightEyeMatrix,
        this->PhysicalToProjectionMatrixForRightEye);

      this->VCDCMatrix->DeepCopy(this->RightEyeToProjectionMatrix);
      this->VCDCMatrix->Transpose();
    }

    this->KeyMatrixTime.Modified();
    this->LastRenderer = ren;
  }

  wcdc = this->WCDCMatrix;
  wcvc = this->WCVCMatrix;
  normMat = this->NormalMatrix;
  vcdc = this->VCDCMatrix;
}

//------------------------------------------------------------------------------
void vtkVRHMDCamera::GetPhysicalToProjectionMatrix(vtkMatrix4x4*& physToProjectionMat)
{
  if (this->LeftEye)
  {
    physToProjectionMat = this->PhysicalToProjectionMatrixForLeftEye;
  }
  else
  {
    physToProjectionMat = this->PhysicalToProjectionMatrixForRightEye;
  }
}

//------------------------------------------------------------------------------
void vtkVRHMDCamera::ComputeProjectionTransform(double aspect, double nearz, double farz)
{
  if (this->GetTrackHMD())
  {
    // Use the left and right matrices explicitly created
    this->ProjectionTransform->Identity();
    if (this->LeftEye)
    {
      this->ProjectionTransform->Concatenate(this->LeftEyeToProjectionMatrix);
    }
    else
    {
      this->ProjectionTransform->Concatenate(this->RightEyeToProjectionMatrix);
    }
  }
  else
  {
    // TrackHMD is disabled for picking (see vtkVRHardwarePicker::PickProp). In this case, we can
    // use the default projection transform computation done by vtkCamera.
    this->Superclass::ComputeProjectionTransform(aspect, nearz, farz);
  }
}

//------------------------------------------------------------------------------
void vtkVRHMDCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PhysicalToLeftEyeMatrix: ";
  this->PhysicalToLeftEyeMatrix->PrintSelf(os, indent);
  os << indent << "PhysicalToRightEyeMatrix: ";
  this->PhysicalToRightEyeMatrix->PrintSelf(os, indent);

  os << indent << "WorldToLeftEyeMatrix: ";
  this->WorldToLeftEyeMatrix->PrintSelf(os, indent);
  os << indent << "WorldToRightEyeMatrix: ";
  this->WorldToRightEyeMatrix->PrintSelf(os, indent);

  os << indent << "LeftEyeToProjectionMatrix: ";
  this->LeftEyeToProjectionMatrix->PrintSelf(os, indent);
  os << indent << "RightEyeToProjectionMatrix: ";
  this->RightEyeToProjectionMatrix->PrintSelf(os, indent);

  os << indent << "PhysicalToProjectionMatrixForLeftEye: ";
  this->PhysicalToProjectionMatrixForLeftEye->PrintSelf(os, indent);
  os << indent << "PhysicalToProjectionMatrixForRightEye: ";
  this->PhysicalToProjectionMatrixForRightEye->PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
