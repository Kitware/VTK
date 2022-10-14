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

#include <cassert>
#include <cmath>

#include "vtkOpenXRUtilities.h"

vtkStandardNewMacro(vtkOpenXRCamera);

vtkOpenXRCamera::vtkOpenXRCamera() = default;
vtkOpenXRCamera::~vtkOpenXRCamera() = default;

//------------------------------------------------------------------------------
void vtkOpenXRCamera::UpdateWorldToEyeMatrices(vtkRenderer* ren)
{
  vtkOpenXRRenderWindow* win = vtkOpenXRRenderWindow::SafeDownCast(ren->GetRenderWindow());

  // Get physical to world matrix, which we then invert as we are trying to
  // compute the world to view matrix which we do as
  // viewCoord = physicalToEye * worldToPhysical * worldCoord
  win->GetPhysicalToWorldMatrix(this->WorldToPhysicalMatrix);
  this->WorldToPhysicalMatrix->Invert();
  // at this point it is now correctly worldToPhysical

  const XrPosef* xrPose = vtkOpenXRManager::GetInstance().GetViewPose(LEFT_EYE);
  if (xrPose == nullptr)
  {
    vtkErrorMacro(<< "No pose for left eye, cannot update view transform");
    return;
  }
  // Convert a XrPosef to a vtk view matrix
  vtkOpenXRUtilities::SetMatrixFromXrPose(this->PhysicalToLeftEyeMatrix, *xrPose);
  this->PhysicalToLeftEyeMatrix->Invert();
  vtkMatrix4x4::Multiply4x4(
    this->PhysicalToLeftEyeMatrix, this->WorldToPhysicalMatrix, this->WorldToLeftEyeMatrix);

  xrPose = vtkOpenXRManager::GetInstance().GetViewPose(RIGHT_EYE);
  if (xrPose == nullptr)
  {
    vtkErrorMacro(<< "No pose for right eye, cannot update view transform");
    return;
  }
  // Convert a XrPosef to a vtk view matrix
  vtkOpenXRUtilities::SetMatrixFromXrPose(this->PhysicalToRightEyeMatrix, *xrPose);
  this->PhysicalToRightEyeMatrix->Invert();
  vtkMatrix4x4::Multiply4x4(
    this->PhysicalToRightEyeMatrix, this->WorldToPhysicalMatrix, this->WorldToRightEyeMatrix);
}

//------------------------------------------------------------------------------
void vtkOpenXRCamera::UpdateEyeToProjectionMatrices(vtkRenderer* ren)
{
  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(ren->GetRenderWindow());

  double scale = win->GetPhysicalScale();
  double znear = this->ClippingRange[0] / scale;
  double zfar = this->ClippingRange[1] / scale;

  XrFovf const* xrFov = vtkOpenXRManager::GetInstance().GetProjectionFov(LEFT_EYE);
  if (xrFov == nullptr)
  {
    vtkErrorMacro(<< "No fov for left eye, cannot update projection matrix");
    return;
  }
  vtkOpenXRUtilities::CreateProjectionFov(this->LeftEyeToProjectionMatrix, *xrFov, znear, zfar);

  xrFov = vtkOpenXRManager::GetInstance().GetProjectionFov(RIGHT_EYE);
  if (xrFov == nullptr)
  {
    vtkErrorMacro(<< "No fov for right eye, cannot update projection matrix");
    return;
  }
  vtkOpenXRUtilities::CreateProjectionFov(this->RightEyeToProjectionMatrix, *xrFov, znear, zfar);
}

//------------------------------------------------------------------------------
void vtkOpenXRCamera::Render(vtkRenderer* ren)
{
  vtkOpenGLClearErrorMacro();

  vtkOpenXRRenderWindow* win = vtkOpenXRRenderWindow::SafeDownCast(ren->GetRenderWindow());
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
  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}
