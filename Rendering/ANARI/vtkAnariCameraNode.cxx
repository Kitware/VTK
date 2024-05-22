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

#include <anari/anari_cpp.hpp>
#include <anari/anari_cpp/ext/std.h>

using vec2 = anari::std_types::vec2;
using vec3 = anari::std_types::vec3;
using box2 = anari::std_types::box2;

VTK_ABI_NAMESPACE_BEGIN

struct vtkAnariCameraNodeInternals
{
  anari::Device AnariDevice{ nullptr };
  anari::Camera AnariCamera{ nullptr };
  bool IsParallelProjection{ false };
  vtkAnariRendererNode* RendererNode{ nullptr };
};

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
    this->Internals->RendererNode = nullptr;
  }
}

//----------------------------------------------------------------------------
vtkAnariCameraNode::vtkAnariCameraNode()
{
  this->Internals = new vtkAnariCameraNodeInternals;
}

//----------------------------------------------------------------------------
vtkAnariCameraNode::~vtkAnariCameraNode()
{
  if (this->Internals->AnariDevice)
  {
    anari::release(this->Internals->AnariDevice, this->Internals->AnariCamera);
    anari::release(this->Internals->AnariDevice, this->Internals->AnariDevice);
  }

  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkAnariCameraNode::Build(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariCameraNode::Build", vtkAnariProfiling::BROWN);
  if (!prepass || !CameraWasModified())
  {
    return;
  }

  if (this->Internals->RendererNode == nullptr)
  {
    this->Internals->RendererNode =
      static_cast<vtkAnariRendererNode*>(this->GetFirstAncestorOfType("vtkAnariRendererNode"));
  }

  this->UpdateAnariObjectHandles();
}

//----------------------------------------------------------------------------
void vtkAnariCameraNode::Synchronize(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariCameraNode::Synchronize", vtkAnariProfiling::BROWN);
  if (!prepass || !CameraWasModified())
  {
    return;
  }
  this->UpdateAnariCameraParameters();
  this->RenderTime = GetVtkCamera()->GetMTime();
}

void vtkAnariCameraNode::UpdateAnariObjectHandles()
{
  if (!this->Internals->AnariDevice)
  {
    this->Internals->AnariDevice = this->Internals->RendererNode->GetAnariDevice();
    anari::retain(this->Internals->AnariDevice, this->Internals->AnariDevice);
  }

  const bool parallel = this->GetVtkCamera()->GetParallelProjection();
  if (!this->Internals->AnariCamera || parallel != this->Internals->IsParallelProjection)
  {
    anari::release(this->Internals->AnariDevice, this->Internals->AnariCamera);
    this->Internals->AnariCamera = anari::newObject<anari::Camera>(
      this->Internals->AnariDevice, parallel ? "orthographic" : "perspective");
    this->Internals->IsParallelProjection = parallel;
    this->Internals->RendererNode->SetCamera(this->Internals->AnariCamera);
  }
}

void vtkAnariCameraNode::UpdateAnariCameraParameters()
{
  bool stereo = false;
  bool right = false;

  vtkRenderer* ren = vtkRenderer::SafeDownCast(this->Internals->RendererNode->GetRenderable());
  vtkRenderWindow* rwin = vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
  if (rwin)
  {
    auto anariExtensions = this->Internals->RendererNode->GetAnariDeviceExtensions();
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

  int* const ts = this->Internals->RendererNode->GetScale();

  if (this->Internals->IsParallelProjection)
  {
    // height of the image plane in world units
    double height = cam->GetParallelScale() * 2 * ts[0];
    anari::setParameter(this->Internals->AnariDevice, this->Internals->AnariCamera, "height",
      static_cast<float>(height));
  }
  else
  {
    // The field of view (angle in radians) of the frame's height
    float fovyDegrees = static_cast<float>(cam->GetViewAngle()) * static_cast<float>(ts[0]);
    float fovyRadians = vtkMath::RadiansFromDegrees(fovyDegrees);
    anari::setParameter(
      this->Internals->AnariDevice, this->Internals->AnariCamera, "fovy", fovyRadians);
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

    anari::setParameter(
      this->Internals->AnariDevice, this->Internals->AnariCamera, "stereoMode", stereoMode);

    float interPupDist = static_cast<float>(myEyeSeparation);
    anari::setParameter(this->Internals->AnariDevice, this->Internals->AnariCamera,
      "interpupillaryDistance", interPupDist);
  }
  else
  {
    shiftedCamPos[0] = static_cast<float>(pos[0]);
  }

  // Set position parameter
  anari::setParameter(
    this->Internals->AnariDevice, this->Internals->AnariCamera, "position", shiftedCamPos);

  // Set focusDistance parameter
  double myFocalDistance = cam->GetFocalDistance();
  float focusDistance = myFocalDistance > 0.0 ? static_cast<float>(myFocalDistance) : 1.0f;
  anari::setParameter(
    this->Internals->AnariDevice, this->Internals->AnariCamera, "focusDistance", focusDistance);

  // Set apertureRadius parameter
  double myFocalDisk = cam->GetFocalDisk();
  float apertureRadius = myFocalDistance > 0.0 ? static_cast<float>(0.5 * myFocalDisk) : 0.0f;
  anari::setParameter(
    this->Internals->AnariDevice, this->Internals->AnariCamera, "apertureRadius", apertureRadius);

  // Set aspect ratio parameter
  int tiledSize[2];
  this->Internals->RendererNode->GetSize(tiledSize);
  float aspect = static_cast<float>(tiledSize[0]) / static_cast<float>(tiledSize[1]);
  anari::setParameter(this->Internals->AnariDevice, this->Internals->AnariCamera, "aspect", aspect);

  // Set near and far clip plane distances
  double clippingRange[2];
  cam->GetClippingRange(clippingRange);
  anari::setParameter(this->Internals->AnariDevice, this->Internals->AnariCamera, "near",
    static_cast<float>(clippingRange[0]));
  anari::setParameter(this->Internals->AnariDevice, this->Internals->AnariCamera, "far",
    static_cast<float>(clippingRange[1]));

  // Set up parameter
  double* const up = cam->GetViewUp();
  vec3 cameraUp = { static_cast<float>(up[0]), static_cast<float>(up[1]),
    static_cast<float>(up[2]) };
  anari::setParameter(this->Internals->AnariDevice, this->Internals->AnariCamera, "up", cameraUp);

  // Set direction parameter
  vec3 cameraDirection = { static_cast<float>(myFocalPoint[0] - shiftedCamPos[0]),
    static_cast<float>(myFocalPoint[1] - shiftedCamPos[1]),
    static_cast<float>(myFocalPoint[2] - shiftedCamPos[2]) };
  anari::setParameter(
    this->Internals->AnariDevice, this->Internals->AnariCamera, "direction", cameraDirection);

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

    anari::setParameter(
      this->Internals->AnariDevice, this->Internals->AnariCamera, "transform", matrixF);
  }

  // Region of the sensor in normalized screen-space coordinates
  double viewPort[4] = { 0, 0, 1, 1 };
  this->Internals->RendererNode->GetViewport(viewPort);

  box2 imageRegion = { vec2{ static_cast<float>(viewPort[0]), static_cast<float>(viewPort[1]) },
    vec2{ static_cast<float>(viewPort[2]), static_cast<float>(viewPort[3]) } };
  anari::setParameter(
    this->Internals->AnariDevice, this->Internals->AnariCamera, "imageRegion", imageRegion);

  anari::commitParameters(this->Internals->AnariDevice, this->Internals->AnariCamera);
}

vtkCamera* vtkAnariCameraNode::GetVtkCamera() const
{
  return static_cast<vtkCamera*>(this->Renderable);
}

bool vtkAnariCameraNode::CameraWasModified() const
{
  return this->RenderTime < GetVtkCamera()->GetMTime();
}

VTK_ABI_NAMESPACE_END
