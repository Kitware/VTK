// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebXRCamera.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLState.h"
#include "vtkRenderer.h"
#include "vtkWebXR.h"
#include "vtkWebXRRenderWindow.h"
#include "vtkWebXRUtilities.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebXRCamera);

//------------------------------------------------------------------------------
void vtkWebXRCamera::Render(vtkRenderer* ren)
{
  vtkOpenGLClearErrorMacro();

  vtkWebXRRenderWindow* win = vtkWebXRRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = win->GetState();
  this->UpdateStereo(ren);
  int renSize[2];
  win->GetRenderBufferSize(renSize[0], renSize[1]);

  if (!ren->GetSelector())
  {
    if (this->GetLeftEye())
    {
      ostate->vtkglViewport(0, 0, renSize[0] / 2, renSize[1]);
      ostate->vtkglScissor(0, 0, renSize[0] / 2, renSize[1]);
    }
    else
    {
      ostate->vtkglViewport(renSize[0] / 2, 0, renSize[0] / 2, renSize[1]);
      ostate->vtkglScissor(renSize[0] / 2, 0, renSize[0] / 2, renSize[1]);
    }
  }

  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
void vtkWebXRCamera::UpdateWorldToEyeMatrices([[maybe_unused]] vtkRenderer* ren)
{
#ifdef __EMSCRIPTEN__
  vtkWebXRRenderWindow* win = vtkWebXRRenderWindow::SafeDownCast(ren->GetRenderWindow());

  // Get physical to world matrix, which we then invert as we are trying to
  // compute the world to view matrix which we do as
  // viewCoord = physicalToEye * worldToPhysical * worldCoord
  win->GetPhysicalToWorldMatrix(this->WorldToPhysicalMatrix);
  this->WorldToPhysicalMatrix->Invert();
  // at this point it is now correctly worldToPhysical

  WebXRView views[2]; // support only 2 views
  int nbViews;
  webxr_get_views(views, &nbViews, 2);

  // Left eye
  vtkWebXRUtilities::SetMatrixFromWebXRMatrix(
    this->PhysicalToLeftEyeMatrix, views[0].viewPose.matrix);
  this->PhysicalToLeftEyeMatrix->Invert();
  vtkMatrix4x4::Multiply4x4(
    this->PhysicalToLeftEyeMatrix, this->WorldToPhysicalMatrix, this->WorldToLeftEyeMatrix);

  // Right eye
  vtkWebXRUtilities::SetMatrixFromWebXRMatrix(
    this->PhysicalToRightEyeMatrix, views[1].viewPose.matrix);
  this->PhysicalToRightEyeMatrix->Invert();
  vtkMatrix4x4::Multiply4x4(
    this->PhysicalToRightEyeMatrix, this->WorldToPhysicalMatrix, this->WorldToRightEyeMatrix);
#endif
}

//------------------------------------------------------------------------------
void vtkWebXRCamera::UpdateEyeToProjectionMatrices([[maybe_unused]] vtkRenderer* ren)
{
#ifdef __EMSCRIPTEN__
  WebXRView views[2]; // support only 2 views
  int nbViews;
  webxr_get_views(views, &nbViews, 2);

  // Left eye
  vtkWebXRUtilities::SetMatrixFromWebXRMatrix(
    this->LeftEyeToProjectionMatrix, views[0].projectionMatrix);

  // Right eye
  vtkWebXRUtilities::SetMatrixFromWebXRMatrix(
    this->RightEyeToProjectionMatrix, views[1].projectionMatrix);
#endif
}

VTK_ABI_NAMESPACE_END
