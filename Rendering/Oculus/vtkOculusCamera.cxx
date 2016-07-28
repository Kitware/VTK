/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOculusCamera.h"

#include "vtkMatrix4x4.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkOculusRenderWindow.h"
#include "vtkOpenGLError.h"

#include <cmath>

vtkStandardNewMacro(vtkOculusCamera);


vtkOculusCamera::vtkOculusCamera()
{
  this->LeftEyePose = 0;
  this->RightEyePose = 0;
  this->LeftEyeProjection = 0;
  this->RightEyeProjection = 0;

  this->RightWCDCMatrix = vtkMatrix4x4::New();
  this->RightWCVCMatrix = vtkMatrix4x4::New();
  this->RightNormalMatrix = vtkMatrix3x3::New();
  this->RightVCDCMatrix = vtkMatrix4x4::New();

  this->LeftEyeTCDCMatrix = vtkMatrix4x4::New();
  this->RightEyeTCDCMatrix = vtkMatrix4x4::New();

  this->Scale = 1.0;
  this->Translation[0] = 0.0;
  this->Translation[1] = 0.0;
  this->Translation[2] = 0.0;

  // approximate for Oculus
  this->SetViewAngle(100.0);
}

vtkOculusCamera::~vtkOculusCamera()
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
  this->RightNormalMatrix->Delete();
  this->RightVCDCMatrix->Delete();

  this->LeftEyeTCDCMatrix->Delete();
  this->RightEyeTCDCMatrix->Delete();
}

void vtkOculusCamera::GetHMDEyePoses(vtkRenderer *ren)
{
  vtkOculusRenderWindow *win =
    vtkOculusRenderWindow::SafeDownCast(ren->GetRenderWindow());

  ovrVector3f *eyeOffsets = win->GetHMDToEyeViewOffsets();

  this->LeftEyePose->Identity();
  this->LeftEyePose->SetElement(0, 3, -eyeOffsets[0].x);
  this->LeftEyePose->SetElement(1, 3, -eyeOffsets[0].y);
  this->LeftEyePose->SetElement(2, 3, -eyeOffsets[0].z);

  this->RightEyePose->Identity();
  this->RightEyePose->SetElement(0, 3, -eyeOffsets[1].x);
  this->RightEyePose->SetElement(1, 3, -eyeOffsets[1].y);
  this->RightEyePose->SetElement(2, 3, -eyeOffsets[1].z);
}

void vtkOculusCamera::GetHMDEyeProjections(vtkRenderer *ren)
{
  vtkOculusRenderWindow *win =
    vtkOculusRenderWindow::SafeDownCast(ren->GetRenderWindow());

  ovrMatrix4f proj = ovrMatrix4f_Projection(win->GetOVRLayer().Fov[0],
    this->ClippingRange[0]*this->Scale,
    this->ClippingRange[1]*this->Scale,
    0);

  for(int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      this->LeftEyeProjection->SetElement(j, i, proj.M[i][j]);
      }
    }

  proj = ovrMatrix4f_Projection(win->GetOVRLayer().Fov[1],
    this->ClippingRange[0]*this->Scale,
    this->ClippingRange[1]*this->Scale,
    0);

  for(int i = 0; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      this->RightEyeProjection->SetElement(j, i, proj.M[i][j]);
      }
    }
}

// Implement base class method.
void vtkOculusCamera::Render(vtkRenderer *ren)
{
  vtkOpenGLClearErrorMacro();

  vtkOculusRenderWindow *win =
    vtkOculusRenderWindow::SafeDownCast(ren->GetRenderWindow());

  // make sure clipping range is not crazy
  // with Oculus we know we are in meters so
  // we can use that knowledge to make the
  // clipping range reasonable
  double nRange[2];
  this->GetClippingRange(nRange);
  nRange[0] = nRange[0]*this->Scale;
  nRange[1] = nRange[1]*this->Scale;
  // to see controllers etc make sure near is around 20 cm
  // people tend to go cross eyed at 20 cm so we use that
  // as a limit
  nRange[0] = 0.10;
  // to see transmitters make sure far is at least 6 meters
  if (nRange[1] < 6.0)
    {
    nRange[1] = 6.0;
    }
  // make sure far is not crazy far > 500m
  if (nRange[1] > 500.0)
    {
    nRange[1] = 500.0;
    }
  this->SetClippingRange(nRange[0]/this->Scale, nRange[1]/this->Scale);

  // if were on a stereo renderer draw to special parts of screen
  if (this->LeftEye)
    {
    // Left Eye
    ovrRecti vp = win->GetOVRLayer().Viewport[0];
    glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
    }
  else
    {
    ovrRecti vp = win->GetOVRLayer().Viewport[1];
    glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
    }

  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase()
      && !ren->GetIsPicking())
    {
    ren->Clear();
    }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

void vtkOculusCamera::GetKeyMatrices(vtkRenderer *ren, vtkMatrix4x4 *&wcvc,
        vtkMatrix3x3 *&normMat, vtkMatrix4x4 *&vcdc, vtkMatrix4x4 *&wcdc)
{
  // get the eye pose and projection matricies once
  if (!this->LeftEyePose)
    {
    this->LeftEyePose = vtkMatrix4x4::New();
    this->RightEyePose = vtkMatrix4x4::New();
    this->LeftEyeProjection = vtkMatrix4x4::New();
    this->RightEyeProjection = vtkMatrix4x4::New();

    this->GetHMDEyePoses(ren);
    }

  // has the camera changed?
  if (ren != this->LastRenderer ||
      this->MTime > this->KeyMatrixTime ||
      ren->GetMTime() > this->KeyMatrixTime)
    {
    this->GetHMDEyeProjections(ren);
    // build both eye views, faster to do it all at once as
    // some calculations are shared

    // first apply the scale
    this->WCVCMatrix->Zero();
    this->WCVCMatrix->Element[0][0] = this->Scale;
    this->WCVCMatrix->Element[1][1] = this->Scale;
    this->WCVCMatrix->Element[2][2] = this->Scale;
    this->WCVCMatrix->Element[3][3] = 1.0;

    vtkMatrix4x4::Multiply4x4(this->WCVCMatrix,
      this->GetModelViewTransformMatrix(),this->WCVCMatrix);
    vtkMatrix4x4::Multiply4x4(this->RightEyePose, this->WCVCMatrix, this->RightWCVCMatrix);
    vtkMatrix4x4::Multiply4x4(this->LeftEyePose, this->WCVCMatrix, this->WCVCMatrix);

    for(int i = 0; i < 3; ++i)
      {
      for (int j = 0; j < 3; ++j)
        {
        this->NormalMatrix->SetElement(i, j, this->WCVCMatrix->GetElement(i, j));
        this->RightNormalMatrix->SetElement(i, j, this->RightWCVCMatrix->GetElement(i, j));
        }
      }
    this->NormalMatrix->Invert();
    this->RightNormalMatrix->Invert();

    this->WCVCMatrix->Transpose();
    this->RightWCVCMatrix->Transpose();

    vtkMatrix4x4::Multiply4x4(this->WCVCMatrix, this->LeftEyeProjection, this->WCDCMatrix);
    vtkMatrix4x4::Multiply4x4(this->RightWCVCMatrix, this->RightEyeProjection, this->RightWCDCMatrix);

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
    normMat = this->RightNormalMatrix;
    vcdc = this->RightEyeProjection;
    wcdc = this->RightWCDCMatrix;
    }
}

void vtkOculusCamera::GetTrackingToDCMatrix(vtkMatrix4x4 *&tcdc)
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
