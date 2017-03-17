/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRCamera.h"

#include "vtkMatrix4x4.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenGLError.h"
// #include "vtkTransform.h"

#include <cmath>

vtkStandardNewMacro(vtkOpenVRCamera);


vtkOpenVRCamera::vtkOpenVRCamera()
{
  this->LeftEyePose = 0;
  this->RightEyePose = 0;
  this->LeftEyeProjection = 0;
  this->RightEyeProjection = 0;

  this->RightWCDCMatrix = vtkMatrix4x4::New();
  this->RightWCVCMatrix = vtkMatrix4x4::New();
  this->RightVCDCMatrix = vtkMatrix4x4::New();

  this->LeftEyeTCDCMatrix = vtkMatrix4x4::New();
  this->RightEyeTCDCMatrix = vtkMatrix4x4::New();

  this->Translation[0] = 0.0;
  this->Translation[1] = 0.0;
  this->Translation[2] = 0.0;

  // approximate for Vive
  // we use the projection matrix directly from the vive
  // so this is just to help make view <--> display
  // adjustments reasonable, not correct, just reasonable
  this->SetViewAngle(110.0);
}

vtkOpenVRCamera::~vtkOpenVRCamera()
{
  if (this->LeftEyePose)
  {
    this->LeftEyePose->Delete();
    this->RightEyePose->Delete();
    this->LeftEyeProjection->Delete();
    this->RightEyeProjection->Delete();
    this->LeftEyePose = 0;
    this->RightEyePose = 0;
    this->LeftEyeProjection = 0;
    this->RightEyeProjection = 0;
  }

  this->RightWCDCMatrix->Delete();
  this->RightWCVCMatrix->Delete();
  this->RightVCDCMatrix->Delete();

  this->LeftEyeTCDCMatrix->Delete();
  this->RightEyeTCDCMatrix->Delete();
}

void vtkOpenVRCamera::GetHMDEyePoses(vtkRenderer *ren)
{
  vtkOpenVRRenderWindow *win =
    vtkOpenVRRenderWindow::SafeDownCast(ren->GetRenderWindow());

  vr::IVRSystem *hMD = win->GetHMD();

  double distance = this->GetDistance();
  vr::HmdMatrix34_t matEye = hMD->GetEyeToHeadTransform( vr::Eye_Left );
  this->LeftEyePose->Identity();
  for(int i = 0; i < 3; ++i)
  {
    this->LeftEyePose->SetElement(i, 3, -matEye.m[i][3]*distance);
  }
  matEye = hMD->GetEyeToHeadTransform( vr::Eye_Right );
  this->RightEyePose->Identity();
  for(int i = 0; i < 3; ++i)
  {
    this->RightEyePose->SetElement(i, 3, -matEye.m[i][3]*distance);
  }
}

void vtkOpenVRCamera::GetHMDEyeProjections(vtkRenderer *ren)
{
  vtkOpenVRRenderWindow *win =
    vtkOpenVRRenderWindow::SafeDownCast(ren->GetRenderWindow());

  vr::IVRSystem *hMD = win->GetHMD();

  vr::HmdMatrix44_t mat =
    hMD->GetProjectionMatrix( vr::Eye_Left,
      this->ClippingRange[0],
      this->ClippingRange[1], vr::API_OpenGL);
  for(int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      this->LeftEyeProjection->SetElement(i, j, mat.m[i][j]);
    }
  }

  mat =
    hMD->GetProjectionMatrix( vr::Eye_Right,
      this->ClippingRange[0],
      this->ClippingRange[1], vr::API_OpenGL);
  for(int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      this->RightEyeProjection->SetElement(i, j, mat.m[i][j]);
    }
  }

  this->RightEyeProjection->Transpose();
  this->LeftEyeProjection->Transpose();
}

// Implement base class method.
void vtkOpenVRCamera::Render(vtkRenderer *ren)
{
  vtkOpenGLClearErrorMacro();

  vtkOpenVRRenderWindow *win =
    vtkOpenVRRenderWindow::SafeDownCast(ren->GetRenderWindow());
  int renSize[2];
  win->GetRenderBufferSize(renSize[0],renSize[1]);

  // if were on a stereo renderer draw to special parts of screen
  if (this->LeftEye)
  {
    // make sure clipping range is not crazy
    // with OpenVR we know we are in meters so
    // we can use that knowledge to make the
    // clipping range reasonable
    double nRange[2];
    this->GetClippingRange(nRange);
    double distance = this->GetDistance();
    nRange[0] = nRange[0]/distance;
    nRange[1] = nRange[1]/distance;
    // to see controllers etc make sure near is around 10 cm
    // people tend to go cross eyed at +20 cm from HMD so we use that
    // as a limit
    nRange[0] = 0.2;
    // to see transmitters make sure far is at least 6 meters
    if (nRange[1] < 6.0)
    {
      nRange[1] = 6.0;
    }
    // make sure far is not crazy far > 100m
    if (nRange[1] > 100.0)
    {
      nRange[1] = 100.0;
    }
    this->SetClippingRange(nRange[0]*distance, nRange[1]*distance);
    // Left Eye
    if (win->GetMultiSamples())
    {
      glEnable( GL_MULTISAMPLE );
    }
    glBindFramebuffer( GL_FRAMEBUFFER, win->GetLeftRenderBufferId());
  }
  else
  {
    // right eye
    if (win->GetMultiSamples())
    {
      glEnable( GL_MULTISAMPLE );
    }
    glBindFramebuffer( GL_FRAMEBUFFER, win->GetRightRenderBufferId());
  }

  glViewport(0, 0, renSize[0], renSize[1] );
  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase()
      && !ren->GetIsPicking())
  {
    ren->Clear();
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

void vtkOpenVRCamera::GetKeyMatrices(vtkRenderer *ren, vtkMatrix4x4 *&wcvc,
        vtkMatrix3x3 *&normMat, vtkMatrix4x4 *&vcdc, vtkMatrix4x4 *&wcdc)
{
  static double lastDistance = 0.0;

  // get the eye pose and projection matricies once
  if (!this->LeftEyePose)
  {
    this->LeftEyePose = vtkMatrix4x4::New();
    this->RightEyePose = vtkMatrix4x4::New();
    this->LeftEyeProjection = vtkMatrix4x4::New();
    this->RightEyeProjection = vtkMatrix4x4::New();
    this->GetHMDEyePoses(ren);
    lastDistance = this->GetDistance();
  }

  // has the camera changed?
  if (ren != this->LastRenderer ||
      this->MTime > this->KeyMatrixTime ||
      ren->GetMTime() > this->KeyMatrixTime)
  {
    for(int i = 0; i < 3; ++i)
    {
      double distance = this->GetDistance();
      if (distance != lastDistance)
      {
        this->LeftEyePose->SetElement(
          i, 3, this->LeftEyePose->GetElement(i, 3)*distance/lastDistance);
        this->RightEyePose->SetElement(
          i, 3, this->RightEyePose->GetElement(i, 3)*distance/lastDistance);
        lastDistance = distance;
      }
    }
    this->GetHMDEyeProjections(ren);
    // build both eye views, faster to do it all at once as
    // some calculations are shared

    vtkMatrix4x4 *w2v = this->GetModelViewTransformMatrix();
    vtkMatrix4x4::Multiply4x4(this->RightEyePose, w2v, this->RightWCVCMatrix);
    vtkMatrix4x4::Multiply4x4(this->LeftEyePose, w2v, this->WCVCMatrix);

    for(int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        this->NormalMatrix->SetElement(i, j, w2v->GetElement(i, j));
      }
    }
    this->NormalMatrix->Invert();

    this->WCVCMatrix->Transpose();
    this->RightWCVCMatrix->Transpose();

    vtkMatrix4x4::Multiply4x4(this->WCVCMatrix, this->LeftEyeProjection, this->WCDCMatrix);
    vtkMatrix4x4::Multiply4x4(this->RightWCVCMatrix, this->RightEyeProjection, this->RightWCDCMatrix);

    // build the tracking to device coordinate matrix
    this->PoseTransform->Identity();
    double *trans = this->Translation;
    this->PoseTransform->Translate(-trans[0],-trans[1],-trans[2]);
    double scale = this->GetDistance();
    this->PoseTransform->Scale(scale,scale,scale);
    this->LeftEyeTCDCMatrix->DeepCopy(this->PoseTransform->GetMatrix());
    this->LeftEyeTCDCMatrix->Transpose();

    vtkMatrix4x4::Multiply4x4(this->LeftEyeTCDCMatrix,
      this->RightWCDCMatrix,
      this->RightEyeTCDCMatrix);
    vtkMatrix4x4::Multiply4x4(this->LeftEyeTCDCMatrix,
      this->WCDCMatrix,
      this->LeftEyeTCDCMatrix);

    this->KeyMatrixTime.Modified();
    this->LastRenderer = ren;
  }

  if (this->LeftEye)
  {
    wcvc = this->WCVCMatrix;
    normMat = this->NormalMatrix;
    vcdc = this->LeftEyeProjection;
    wcdc = this->WCDCMatrix;
  }
  else
  {
    wcvc = this->RightWCVCMatrix;
    normMat = this->NormalMatrix;
    vcdc = this->RightEyeProjection;
    wcdc = this->RightWCDCMatrix;
  }
}

void vtkOpenVRCamera::GetTrackingToDCMatrix(vtkMatrix4x4 *&tcdc)
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
