// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariCameraNode.h"
#include "vtkAnariProfiling.h"
#include "vtkAnariRendererNode.h"

#include "vtkCamera.h"
#include "vtkHomogeneousTransform.h"
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"

#include <anari/anari_cpp/ext/std.h>

using vec2 = anari::std_types::vec2;
using vec3 = anari::std_types::vec3;
using box2 = anari::std_types::box2;

VTK_ABI_NAMESPACE_BEGIN

//============================================================================
vtkStandardNewMacro(vtkAnariCameraNode);

//----------------------------------------------------------------------------
void vtkAnariCameraNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkAnariCameraNode::Invalidate(bool prepass)
{
  if (prepass)
  {
    this->RenderTime = 0;
    this->RendererNode = nullptr;
  }
}

//----------------------------------------------------------------------------
vtkAnariCameraNode::~vtkAnariCameraNode()
{
  if (this->AnariDevice)
  {
    anari::release(this->AnariDevice, this->AnariCamera);
    anari::release(this->AnariDevice, this->AnariDevice);
  }
}

//----------------------------------------------------------------------------
void vtkAnariCameraNode::Build(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariCameraNode::Build", vtkAnariProfiling::BROWN);
  if (!prepass || !NodeWasModified())
    return;

  if (this->RendererNode == nullptr)
  {
    this->RendererNode =
      static_cast<vtkAnariRendererNode*>(this->GetFirstAncestorOfType("vtkAnariRendererNode"));
  }

  this->UpdateAnariObjectHandles();
}

//----------------------------------------------------------------------------
void vtkAnariCameraNode::Synchronize(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariCameraNode::Synchronize", vtkAnariProfiling::BROWN);
  if (!prepass || !NodeWasModified())
    return;
  this->UpdateAnariCameraParameters();
}

//----------------------------------------------------------------------------
void vtkAnariCameraNode::Render(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariCameraNode::Render", vtkAnariProfiling::BROWN);
  if (!prepass || !NodeWasModified())
    return;
  this->RenderTime = GetVtkCamera()->GetMTime();
  this->RendererNode->SetCamera(this->AnariCamera);
}

void vtkAnariCameraNode::UpdateAnariObjectHandles()
{
  if (!this->AnariDevice)
  {
    this->AnariDevice = this->RendererNode->GetAnariDevice();
    anari::retain(this->AnariDevice, this->AnariDevice);
  }

  const bool parallel = this->GetVtkCamera()->GetParallelProjection();
  if (!this->AnariCamera || parallel != this->IsParallelProjection)
  {
    anari::release(this->AnariDevice, this->AnariCamera);
    this->AnariCamera =
      anari::newObject<anari::Camera>(this->AnariDevice, parallel ? "orthographic" : "perspective");
    this->IsParallelProjection = parallel;
  }
}

void vtkAnariCameraNode::UpdateAnariCameraParameters()
{
  bool stereo = false;
  bool right = false;

  vtkRenderer* ren = vtkRenderer::SafeDownCast(this->RendererNode->GetRenderable());
  vtkRenderWindow* rwin = vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
  if (rwin)
  {
    auto anariExtensions = this->RendererNode->GetAnariDeviceExtensions();
    if (rwin->GetStereoRender() == 1 && anariExtensions.ANARI_KHR_CAMERA_STEREO)
    {
      stereo = true;
    }
  }

  auto* cam = GetVtkCamera();
  double myDistance = cam->GetDistance();
  double myEyeSeparation = cam->GetEyeSeparation();
  double myScaledEyeSeparation = myEyeSeparation * myDistance;
  double shiftDistance = (myScaledEyeSeparation / 2);
  double* myFocalPoint = cam->GetFocalPoint();

  if (!cam->GetLeftEye())
  {
    right = true;
  }

  int* const ts = this->RendererNode->GetScale();

  if (this->IsParallelProjection)
  {
    // height of the image plane in world units
    double height = cam->GetParallelScale() * 2 * ts[0];
    anari::setParameter(this->AnariDevice, this->AnariCamera, "height", static_cast<float>(height));
  }
  else
  {
    // The field of view (angle in radians) of the frame's height
    float fovyDegrees = static_cast<float>(cam->GetViewAngle()) * static_cast<float>(ts[0]);
    float fovyRadians = vtkMath::RadiansFromDegrees(fovyDegrees);
    anari::setParameter(this->AnariDevice, this->AnariCamera, "fovy", fovyRadians);
  }

  // Set stereoMode and interpupillaryDistance parameters
  double* const pos = cam->GetPosition();
  vec3 shiftedCamPos = { 0.0f, static_cast<float>(pos[1]), static_cast<float>(pos[2]) };

  if (stereo)
  {
    std::string stereoMode = "right";

    if (!right)
    {
      stereoMode = "left";
      shiftedCamPos[0] = static_cast<float>(pos[0] - shiftDistance);
    }
    else
    {
      shiftedCamPos[0] = static_cast<float>(pos[0] + shiftDistance);
    }

    anari::setParameter(this->AnariDevice, this->AnariCamera, "stereoMode", stereoMode);

    float interPupDist = static_cast<float>(myEyeSeparation);
    anari::setParameter(
      this->AnariDevice, this->AnariCamera, "interpupillaryDistance", interPupDist);
  }
  else
  {
    shiftedCamPos[0] = static_cast<float>(pos[0]);
  }

  // Set position parameter
  anari::setParameter(this->AnariDevice, this->AnariCamera, "position", shiftedCamPos);

  // Set focusDistance parameter
  double myFocalDistance = cam->GetFocalDistance();
  float focusDistance = myFocalDistance > 0.0 ? static_cast<float>(myFocalDistance) : 1.0f;
  anari::setParameter(this->AnariDevice, this->AnariCamera, "focusDistance", focusDistance);

  // Set apertureRadius parameter
  double myFocalDisk = cam->GetFocalDisk();
  float apertureRadius = myFocalDistance > 0.0 ? static_cast<float>(0.5 * myFocalDisk) : 0.0f;
  anari::setParameter(this->AnariDevice, this->AnariCamera, "apertureRadius", apertureRadius);

  // Set aspect ratio parameter
  int tiledSize[2];
  this->RendererNode->GetSize(tiledSize);
  float aspect = static_cast<float>(tiledSize[0]) / static_cast<float>(tiledSize[1]);
  anari::setParameter(this->AnariDevice, this->AnariCamera, "aspect", aspect);

  // Set near and far clip plane distances
  double clippingRange[2];
  cam->GetClippingRange(clippingRange);
  anari::setParameter(
    this->AnariDevice, this->AnariCamera, "near", static_cast<float>(clippingRange[0]));
  anari::setParameter(
    this->AnariDevice, this->AnariCamera, "far", static_cast<float>(clippingRange[1]));

  // Set up parameter
  double* const up = cam->GetViewUp();
  vec3 cameraUp = { static_cast<float>(up[0]), static_cast<float>(up[1]),
    static_cast<float>(up[2]) };
  anari::setParameter(this->AnariDevice, this->AnariCamera, "up", cameraUp);

  // Set direction parameter
  vec3 cameraDirection = { static_cast<float>(myFocalPoint[0] - shiftedCamPos[0]),
    static_cast<float>(myFocalPoint[1] - shiftedCamPos[1]),
    static_cast<float>(myFocalPoint[2] - shiftedCamPos[2]) };
  anari::setParameter(this->AnariDevice, this->AnariCamera, "direction", cameraDirection);

  // Additional world-space transformation matrix
  vtkHomogeneousTransform* transform = cam->GetUserTransform();

  if (transform != nullptr)
  {
    double* matrix = transform->GetMatrix()->GetData();
    float matrixF[16];

    for (int i = 0; i < 16; i++)
    {
      matrixF[i] = static_cast<float>(matrix[i]);
    }

    anari::setParameter(this->AnariDevice, this->AnariCamera, "transform", matrixF);
  }

  // Region of the sensor in normalized screen-space coordinates
  double viewPort[4] = { 0, 0, 1, 1 };
  this->RendererNode->GetViewport(viewPort);

  box2 imageRegion = { vec2{ static_cast<float>(viewPort[0]), static_cast<float>(viewPort[1]) },
    vec2{ static_cast<float>(viewPort[2]), static_cast<float>(viewPort[3]) } };
  anari::setParameter(this->AnariDevice, this->AnariCamera, "imageRegion", imageRegion);

  anari::commitParameters(this->AnariDevice, this->AnariCamera);
}

vtkCamera* vtkAnariCameraNode::GetVtkCamera() const
{
  return static_cast<vtkCamera*>(this->Renderable);
}

bool vtkAnariCameraNode::NodeWasModified() const
{
  return this->RenderTime < GetVtkCamera()->GetMTime();
}

VTK_ABI_NAMESPACE_END
