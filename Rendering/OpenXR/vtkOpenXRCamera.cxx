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
  this->ClippingRange[0] = 0.1;
  this->ClippingRange[1] = 100.0;
}

//------------------------------------------------------------------------------
void vtkOpenXRCamera::UpdateViewTransform(vtkOpenXRRenderWindow* win)
{
  const uint32_t eye = this->LeftEye ? LEFT_EYE : RIGHT_EYE;
  const XrPosef& xrPose = vtkOpenXRManager::GetInstance()->GetViewPose(eye);

  // Convert a XrPosef to a vtk view matrix
  vtkNew<vtkMatrix4x4> viewMatrix;
  vtkOpenXRUtilities::CreateViewMatrix(viewMatrix, xrPose);

  // Transform from physical to world space
  vtkNew<vtkMatrix4x4> physicalToWorldMatrix;
  win->GetPhysicalToWorldMatrix(physicalToWorldMatrix);

  // Remove scale
  /*physicalToWorldMatrix->SetElement(0,0,1.0);
  physicalToWorldMatrix->SetElement(1,1,1.0);
  physicalToWorldMatrix->SetElement(2,2,1.0);*/

  vtkMatrix4x4::Multiply4x4(viewMatrix, physicalToWorldMatrix, viewMatrix);

  this->ModelViewTransform->Identity();
  this->ModelViewTransform->SetMatrix(viewMatrix);
}

//------------------------------------------------------------------------------
void vtkOpenXRCamera::UpdateProjectionMatrix()
{
  const uint32_t eye = this->LeftEye ? LEFT_EYE : RIGHT_EYE;
  const XrFovf& xrFov = vtkOpenXRManager::GetInstance()->GetProjectionFov(eye);

  double znear = this->ClippingRange[0];
  double zfar = this->ClippingRange[1];

  vtkNew<vtkMatrix4x4> projMatrix;
  vtkOpenXRUtilities::CreateProjectionFov(projMatrix, xrFov, znear, zfar);

  this->SetUseExplicitProjectionTransformMatrix(true);
  this->SetExplicitProjectionTransformMatrix(projMatrix);
}

//------------------------------------------------------------------------------
void vtkOpenXRCamera::Render(vtkRenderer* ren)
{
  vtkOpenGLClearErrorMacro();

  vtkOpenXRRenderWindow* win = vtkOpenXRRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = win->GetState();

  // Update the projection and view transform depending on LeftEye value
  this->UpdateViewTransform(win);
  this->UpdateProjectionMatrix();

  int renSize[2];
  win->GetRenderBufferSize(renSize[0], renSize[1]);

  ostate->vtkglViewport(0, 0, renSize[0], renSize[1]);
  ostate->vtkglScissor(0, 0, renSize[0], renSize[1]);
  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}
