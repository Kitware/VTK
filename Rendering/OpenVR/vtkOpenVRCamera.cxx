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
  this->LeftEyeProjection = 0;
  this->RightEyeProjection = 0;

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
  if (this->LeftEyeProjection)
  {
    this->LeftEyeProjection->Delete();
    this->RightEyeProjection->Delete();
    this->LeftEyeProjection = 0;
    this->RightEyeProjection = 0;
  }

  this->LeftEyeTCDCMatrix->Delete();
  this->RightEyeTCDCMatrix->Delete();
}

void vtkOpenVRCamera::GetHMDEyePoses(vtkRenderer *ren)
{
  vtkOpenVRRenderWindow *win =
    vtkOpenVRRenderWindow::SafeDownCast(ren->GetRenderWindow());

  vr::IVRSystem *hMD = win->GetHMD();

  // left handed coordinate system so have to -1*Z
  vr::HmdMatrix34_t matEye = hMD->GetEyeToHeadTransform( vr::Eye_Left );
  this->LeftEyePose[0] = matEye.m[0][3];
  this->LeftEyePose[1] = matEye.m[1][3];
  this->LeftEyePose[2] = -matEye.m[2][3];

  matEye = hMD->GetEyeToHeadTransform( vr::Eye_Right );
  this->RightEyePose[0] = matEye.m[0][3];
  this->RightEyePose[1] = matEye.m[1][3];
  this->RightEyePose[2] = -matEye.m[2][3];
}

void vtkOpenVRCamera::GetHMDEyeProjections(vtkRenderer *ren)
{
  vtkOpenVRRenderWindow *win =
    vtkOpenVRRenderWindow::SafeDownCast(ren->GetRenderWindow());

  vr::IVRSystem *hMD = win->GetHMD();

  double znear = this->ClippingRange[0];
  double zfar = this->ClippingRange[1];

  float fxmin, fxmax, fymin, fymax;
  double xmin, xmax, ymin, ymax;

  // note docs are propbably wrong in OpenVR arg list for this func
  hMD->GetProjectionRaw( vr::Eye_Left, &fxmin, &fxmax, &fymin, &fymax);
  xmin = fxmin*znear;
  xmax = fxmax*znear;
  ymin = fymin*znear;
  ymax = fymax*znear;

  this->LeftEyeProjection->Zero();
  this->LeftEyeProjection->SetElement(0, 0, 2*znear/(xmax - xmin));
  this->LeftEyeProjection->SetElement(1, 1, 2*znear/(ymax - ymin));
  this->LeftEyeProjection->SetElement(2, 0, (xmin + xmax)/(xmax - xmin));
  this->LeftEyeProjection->SetElement(2, 1, (ymin + ymax)/(ymax - ymin));
  this->LeftEyeProjection->SetElement(2, 2, -(znear + zfar)/(zfar - znear));
  this->LeftEyeProjection->SetElement(2, 3, -1);
  this->LeftEyeProjection->SetElement(3, 2, -2*znear*zfar/(zfar - znear));

  hMD->GetProjectionRaw( vr::Eye_Right, &fxmin, &fxmax, &fymin, &fymax);
  xmin = fxmin*znear;
  xmax = fxmax*znear;
  ymin = fymin*znear;
  ymax = fymax*znear;

  this->RightEyeProjection->Zero();
  this->RightEyeProjection->SetElement(0, 0, 2*znear/(xmax - xmin));
  this->RightEyeProjection->SetElement(1, 1, 2*znear/(ymax - ymin));
  this->RightEyeProjection->SetElement(2, 0, (xmin + xmax)/(xmax - xmin));
  this->RightEyeProjection->SetElement(2, 1, (ymin + ymax)/(ymax - ymin));
  this->RightEyeProjection->SetElement(2, 2, -(znear + zfar)/(zfar - znear));
  this->RightEyeProjection->SetElement(2, 3, -1);
  this->RightEyeProjection->SetElement(3, 2, -2*znear*zfar/(zfar - znear));
}

void vtkOpenVRCamera::ApplyEyePose(bool left, double factor)
{
  double distance = this->GetDistance();

  double *dop = this->GetDirectionOfProjection();
  double *vup = this->GetViewUp();
  double vright[3];
  vtkMath::Cross(dop,vup,vright);

  double *offset = (left ? this->LeftEyePose : this->RightEyePose);
  double newOffset[3];
  newOffset[0] = factor*(offset[0]*vright[0] + offset[1]*vup[0] - offset[2]*dop[0])*distance;
  newOffset[1] = factor*(offset[0]*vright[1] + offset[1]*vup[1] - offset[2]*dop[1])*distance;
  newOffset[2] = factor*(offset[0]*vright[2] + offset[1]*vup[2] - offset[2]*dop[2])*distance;
  double *pos = this->GetPosition();
  this->SetPosition(pos[0]+newOffset[0], pos[1] + newOffset[1], pos[2] + newOffset[2]);
  double *fp = this->GetFocalPoint();
  this->SetFocalPoint(fp[0]+newOffset[0], fp[1] + newOffset[1], fp[2] + newOffset[2]);
}

// Implement base class method.
void vtkOpenVRCamera::Render(vtkRenderer *ren)
{
  vtkOpenGLClearErrorMacro();

  vtkOpenVRRenderWindow *win =
    vtkOpenVRRenderWindow::SafeDownCast(ren->GetRenderWindow());
  int renSize[2];
  win->GetRenderBufferSize(renSize[0],renSize[1]);

  // get the eye pose and projection matricies once
  if (!this->LeftEyeProjection)
  {
    this->LeftEyeProjection = vtkMatrix4x4::New();
    this->RightEyeProjection = vtkMatrix4x4::New();
    this->GetHMDEyePoses(ren);
  }

  // if were on a stereo renderer draw to special parts of screen
  if (this->LeftEye)
  {
    // Left Eye
    if (win->GetMultiSamples())
    {
      glEnable( GL_MULTISAMPLE );
    }
    glBindFramebuffer( GL_FRAMEBUFFER, win->GetLeftRenderBufferId());

    // adjust for left eye position
    this->ApplyEyePose(true, 1.0);
  }
  else
  {
    // right eye
    if (win->GetMultiSamples())
    {
      glEnable( GL_MULTISAMPLE );
    }
    glBindFramebuffer( GL_FRAMEBUFFER, win->GetRightRenderBufferId());

    // adjust for left eye position
    this->ApplyEyePose(true, -1.0);
    // adjust for right eye position
    this->ApplyEyePose(false, 1.0);
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
  // has the camera changed?
  if (ren != this->LastRenderer ||
      this->MTime > this->KeyMatrixTime ||
      ren->GetMTime() > this->KeyMatrixTime)
  {
    vtkMatrix4x4 *w2v = this->GetModelViewTransformMatrix();
    this->WCVCMatrix->DeepCopy(w2v);

    if (this->LeftEye)
    {
      this->GetHMDEyeProjections(ren);

      // only compute normal matrix once
      for(int i = 0; i < 3; ++i)
      {
        for (int j = 0; j < 3; ++j)
        {
          this->NormalMatrix->SetElement(i, j, w2v->GetElement(i, j));
        }
      }
      this->NormalMatrix->Invert();
    }

    this->WCVCMatrix->Transpose();

    if (this->LeftEye)
    {
      vtkOpenVRRenderWindow *win =
        vtkOpenVRRenderWindow::SafeDownCast(ren->GetRenderWindow());

      vtkMatrix4x4::Multiply4x4(this->WCVCMatrix, this->LeftEyeProjection, this->WCDCMatrix);

      // build the tracking to device coordinate matrix
      this->PoseTransform->Identity();
      double *trans = this->Translation;
      this->PoseTransform->Translate(-trans[0],-trans[1],-trans[2]);
      double scale = this->GetDistance();
      this->PoseTransform->Scale(scale,scale,scale);

      // deal with Vive to World rotations
      double *vup = win->GetInitialViewUp();
      double *dop = win->GetInitialViewDirection();
      double vr[3];
      vtkMath::Cross(dop,vup,vr);
      double rot[16] = {
        vr[0], vup[0], -dop[0], 0.0,
        vr[1], vup[1], -dop[1], 0.0,
        vr[2], vup[2], -dop[2], 0.0,
        0.0, 0.0, 0.0, 1.0};

      this->PoseTransform->Concatenate(rot);

      this->LeftEyeTCDCMatrix->DeepCopy(this->PoseTransform->GetMatrix());
      this->LeftEyeTCDCMatrix->Transpose();

      vtkMatrix4x4::Multiply4x4(this->LeftEyeTCDCMatrix,
        this->WCDCMatrix,
        this->LeftEyeTCDCMatrix);
    }
    else
    {
      vtkMatrix4x4::Multiply4x4(this->WCVCMatrix, this->RightEyeProjection, this->WCDCMatrix);

      this->RightEyeTCDCMatrix->DeepCopy(this->PoseTransform->GetMatrix());
      this->RightEyeTCDCMatrix->Transpose();

      vtkMatrix4x4::Multiply4x4(this->RightEyeTCDCMatrix,
        this->WCDCMatrix,
        this->RightEyeTCDCMatrix);
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

void vtkOpenVRCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
