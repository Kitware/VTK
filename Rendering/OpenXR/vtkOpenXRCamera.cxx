/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenXRCamera.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLState.h"
#include "vtkOpenXR.h"
#include "vtkOpenXRRenderWindow.h"
#include "vtkPerspectiveTransform.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"

#include "vtkMatrix3x3.h"
#include "vtkQuaternion.h"

#include "vtkMath.h"

#include "vtkOpenXRManager.h"

#include <cmath>

#include "vtkOpenXRUtilities.h"

vtkStandardNewMacro(vtkOpenXRCamera);

//------------------------------------------------------------------------------
vtkOpenXRCamera::vtkOpenXRCamera()
{
  // approximate for Vive
  // we use the projection matrix directly from the vive
  // so this is just to help make view <--> display
  // adjustments reasonable, not correct, just reasonable
  this->SetViewAngle(110.0);
}

//------------------------------------------------------------------------------
void vtkOpenXRCamera::GetTrackingToDCMatrix(vtkMatrix4x4*& tcdc)
{
  if (this->LeftEye)
  {
    tcdc = this->LeftEyeTCDCMatrix;
  }
  else
  {
    tcdc = this->RightEyeTCDCMatrix;
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRCamera::GetHMDEyePoses(vtkRenderer* ren)
{
  vtkOpenXRRenderWindow* win = vtkOpenXRRenderWindow::SafeDownCast(ren->GetRenderWindow());

  // Get physical to world matrix, which we then invert as we are trying to
  // compute the world to view matrix which we do as
  // viewCoord = physicalToEye * worldToPhysical * worldCoord
  vtkNew<vtkMatrix4x4> worldToPhysicalMatrix;
  win->GetPhysicalToWorldMatrix(worldToPhysicalMatrix);
  worldToPhysicalMatrix->Invert();
  // at this point it is now correctly worldToPhysical

  const XrPosef* xrPose = vtkOpenXRManager::GetInstance()->GetViewPose(LEFT_EYE);
  if (xrPose == nullptr)
  {
    vtkErrorMacro(<< "No pose for left eye, cannot update view transform");
    return;
  }
  // Convert a XrPosef to a vtk view matrix
  vtkOpenXRUtilities::CreateViewMatrix(this->LeftEyeView, *xrPose);
  vtkMatrix4x4::Multiply4x4(this->LeftEyeView, worldToPhysicalMatrix, this->LeftEyeView);

  xrPose = vtkOpenXRManager::GetInstance()->GetViewPose(RIGHT_EYE);
  if (xrPose == nullptr)
  {
    vtkErrorMacro(<< "No pose for right eye, cannot update view transform");
    return;
  }
  // Convert a XrPosef to a vtk view matrix
  vtkOpenXRUtilities::CreateViewMatrix(this->RightEyeView, *xrPose);
  vtkMatrix4x4::Multiply4x4(this->RightEyeView, worldToPhysicalMatrix, this->RightEyeView);
}

//------------------------------------------------------------------------------
void vtkOpenXRCamera::GetHMDEyeProjections(vtkRenderer*)
{
  double znear = this->ClippingRange[0];
  double zfar = this->ClippingRange[1];

  XrFovf const* xrFov = vtkOpenXRManager::GetInstance()->GetProjectionFov(LEFT_EYE);
  if (xrFov == nullptr)
  {
    vtkErrorMacro(<< "No fov for left eye, cannot update projection matrix");
    return;
  }
  vtkOpenXRUtilities::CreateProjectionFov(this->LeftEyeProjection, *xrFov, znear, zfar);
  this->LeftEyeProjection->Transpose();

  xrFov = vtkOpenXRManager::GetInstance()->GetProjectionFov(RIGHT_EYE);
  if (xrFov == nullptr)
  {
    vtkErrorMacro(<< "No fov for right eye, cannot update projection matrix");
    return;
  }
  vtkOpenXRUtilities::CreateProjectionFov(this->RightEyeProjection, *xrFov, znear, zfar);
  this->RightEyeProjection->Transpose();
}

//------------------------------------------------------------------------------
void vtkOpenXRCamera::GetKeyMatrices(vtkRenderer* ren, vtkMatrix4x4*& wcvc, vtkMatrix3x3*& normMat,
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
    if (this->LeftEye)
    {
      // update projections and poses
      this->GetHMDEyeProjections(ren);
      this->GetHMDEyePoses(ren);

      this->ModelViewTransform->Identity();
      this->ModelViewTransform->SetMatrix(this->LeftEyeView);

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

      vtkOpenXRRenderWindow* win = vtkOpenXRRenderWindow::SafeDownCast(ren->GetRenderWindow());

      this->WCVCMatrix->Transpose();

      vtkMatrix4x4::Multiply4x4(this->WCVCMatrix, this->LeftEyeProjection, this->WCDCMatrix);

      // build the tracking to device coordinate matrix
      this->PoseTransform->Identity();
      double trans[3];
      win->GetPhysicalTranslation(trans);
      this->PoseTransform->Translate(-trans[0], -trans[1], -trans[2]);
      double scale = win->GetPhysicalScale();
      this->PoseTransform->Scale(scale, scale, scale);

      // deal with Vive to World rotations
      double* vup = win->GetPhysicalViewUp();
      double* dop = win->GetPhysicalViewDirection();
      double vr[3];
      vtkMath::Cross(dop, vup, vr);
      double rot[16] = { vr[0], vup[0], -dop[0], 0.0, vr[1], vup[1], -dop[1], 0.0, vr[2], vup[2],
        -dop[2], 0.0, 0.0, 0.0, 0.0, 1.0 };

      this->PoseTransform->Concatenate(rot);

      this->LeftEyeTCDCMatrix->DeepCopy(this->PoseTransform->GetMatrix());
      this->LeftEyeTCDCMatrix->Transpose();

      vtkMatrix4x4::Multiply4x4(this->LeftEyeTCDCMatrix, this->WCDCMatrix, this->LeftEyeTCDCMatrix);
    }
    else
    {
      this->ModelViewTransform->Identity();
      this->ModelViewTransform->SetMatrix(this->RightEyeView);
      vtkMatrix4x4* w2v = this->GetModelViewTransformMatrix();
      this->WCVCMatrix->DeepCopy(w2v);
      this->WCVCMatrix->Transpose();

      vtkMatrix4x4::Multiply4x4(this->WCVCMatrix, this->RightEyeProjection, this->WCDCMatrix);

      this->RightEyeTCDCMatrix->DeepCopy(this->PoseTransform->GetMatrix());
      this->RightEyeTCDCMatrix->Transpose();

      vtkMatrix4x4::Multiply4x4(
        this->RightEyeTCDCMatrix, this->WCDCMatrix, this->RightEyeTCDCMatrix);
    }

    this->KeyMatrixTime.Modified();
    this->LastRenderer = ren;
  }

  wcdc = this->WCDCMatrix;
  wcvc = this->WCVCMatrix;
  normMat = this->NormalMatrix;

  if (this->LeftEye)
  {
    vcdc = this->LeftEyeProjection;
  }
  else
  {
    vcdc = this->RightEyeProjection;
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRCamera::Render(vtkRenderer* ren)
{
  vtkOpenGLClearErrorMacro();

  vtkOpenXRRenderWindow* win = vtkOpenXRRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = win->GetState();

  int renSize[2];
  win->GetRenderBufferSize(renSize[0], renSize[1]);

  // update mats
  vtkMatrix4x4* wcvc;
  vtkMatrix3x3* normMat;
  vtkMatrix4x4* vcdc;
  vtkMatrix4x4* wcdc;
  this->GetKeyMatrices(ren, wcvc, normMat, vcdc, wcdc);

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
  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
void vtkOpenXRCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
