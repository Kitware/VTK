// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2015, Valve Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkZSpaceRenderer.h"

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkCullerCollection.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkZSpaceCamera.h"
#include "vtkZSpaceSDKManager.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkZSpaceRenderer);

//------------------------------------------------------------------------------
vtkZSpaceRenderer::vtkZSpaceRenderer()
{
  // Needed in VR-like environments to be able to see the actors
  this->GetCullers()->RemoveAllItems();
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderer::ResetCamera(const double bounds[6])
{
  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();

  // Get the viewer scale, camera position and camera view up from zSpace
  double position[3];
  double viewUp[3];
  sdkManager->CalculateFrustumFit(bounds, position, viewUp);

  // Set the position, view up and focal point
  double center[3];
  center[0] = (bounds[0] + bounds[1]) / 2.0;
  center[1] = (bounds[2] + bounds[3]) / 2.0;
  center[2] = (bounds[4] + bounds[5]) / 2.0;

  double vn[3];
  vtkCamera* camera = this->GetActiveCamera();
  camera->GetViewPlaneNormal(vn);
  camera->SetViewUp(viewUp[0], viewUp[1], viewUp[2]);
  camera->SetFocalPoint(center[0], center[1], center[2]);
  camera->SetPosition(center[0] + vn[0] * position[0], center[1] + vn[1] * position[1],
    center[2] + vn[2] * position[2]);

  // Set the near and far clip depending on the vtk clipping range and the viewer scale
  this->ResetCameraClippingRange(bounds);
  double clippingRange[2];
  camera->GetClippingRange(clippingRange);

  const float viewerScale = sdkManager->GetViewerScale();
  // A too little value for the nearPlane can result to visual artifacts in stereo
  // (only one eye receiving the image at a time).
  // Hence we set here an empirically defined minimal value.
  const float nearPlane = std::max(0.5 * clippingRange[0] / viewerScale, 0.01);
  const float farPlane = 5.0 * clippingRange[1] / viewerScale;

  // Give the near and far plane to zSpace SDK
  sdkManager->SetClippingRange(nearPlane, farPlane);

  double factor = 0.0;
  double units = 0.0;
  vtkMapper::GetResolveCoincidentTopologyLineOffsetParameters(factor, units);
  vtkMapper::SetResolveCoincidentTopologyLineOffsetParameters(
    viewerScale * factor, viewerScale * units);
}

//------------------------------------------------------------------------------
vtkCamera* vtkZSpaceRenderer::MakeCamera()
{
  vtkZSpaceCamera* cam = vtkZSpaceCamera::New();
  this->InvokeEvent(vtkCommand::CreateCameraEvent, cam);
  return cam;
}

VTK_ABI_NAMESPACE_END
