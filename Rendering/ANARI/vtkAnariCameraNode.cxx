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
  }
}

//----------------------------------------------------------------------------
void vtkAnariCameraNode::Render(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariCameraNode::Render", vtkAnariProfiling::BROWN);

  if (prepass)
  {
    vtkAnariRendererNode* anariRendererNode =
      static_cast<vtkAnariRendererNode*>(this->GetFirstAncestorOfType("vtkAnariRendererNode"));
    vtkRenderer* ren = vtkRenderer::SafeDownCast(anariRendererNode->GetRenderable());
    vtkRenderWindow* rwin = vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());

    anari::Device anariDevice = anariRendererNode->GetAnariDevice();
    vtkCamera* cam = static_cast<vtkCamera*>(this->Renderable);
    auto anariExtensions = anariRendererNode->GetAnariDeviceExtensions();

    vtkMTimeType modifiedTime = cam->GetMTime();

    if (this->RenderTime >= modifiedTime)
    {
      return;
    }

    this->RenderTime = modifiedTime;

    bool stereo = false;
    bool right = false;

    if (rwin)
    {
      if (rwin->GetStereoRender() == 1 && anariExtensions.ANARI_KHR_CAMERA_STEREO)
      {
        stereo = true;
      }
    }

    double myDistance = cam->GetDistance();
    double myEyeSeparation = cam->GetEyeSeparation();
    double myScaledEyeSeparation = myEyeSeparation * myDistance;
    double shiftDistance = (myScaledEyeSeparation / 2);
    double* myFocalPoint = cam->GetFocalPoint();

    if (!cam->GetLeftEye())
    {
      right = true;
    }

    // Create ANARI Camera
    anari::Camera anariCamera = nullptr;
    int* const ts = anariRendererNode->GetScale();

    if (cam->GetParallelProjection())
    {
      anariCamera = anari::newObject<anari::Camera>(anariDevice, "orthographic");

      // height of the image plane in world units
      double height = cam->GetParallelScale() * 2 * ts[0];
      anari::setParameter(anariDevice, anariCamera, "height", static_cast<float>(height));
    }
    else
    {
      anariCamera = anari::newObject<anari::Camera>(anariDevice, "perspective");

      // The field of view (angle in radians) of the frame's height
      float fovyDegrees = static_cast<float>(cam->GetViewAngle()) * static_cast<float>(ts[0]);
      float fovyRadians = vtkMath::RadiansFromDegrees(fovyDegrees);
      anari::setParameter(anariDevice, anariCamera, "fovy", fovyRadians);
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

      anari::setParameter(anariDevice, anariCamera, "stereoMode", stereoMode);

      float interPupDist = static_cast<float>(myEyeSeparation);
      anari::setParameter(anariDevice, anariCamera, "interpupillaryDistance", interPupDist);
    }
    else
    {
      shiftedCamPos[0] = static_cast<float>(pos[0]);
    }

    // Set position parameter
    anari::setParameter(anariDevice, anariCamera, "position", shiftedCamPos);

    // Set focusDistance parameter
    double myFocalDistance = cam->GetFocalDistance();
    float focusDistance = myFocalDistance > 0.0 ? static_cast<float>(myFocalDistance) : 1.0f;
    anari::setParameter(anariDevice, anariCamera, "focusDistance", focusDistance);

    // Set apertureRadius parameter
    double myFocalDisk = cam->GetFocalDisk();
    float apertureRadius = myFocalDistance > 0.0 ? static_cast<float>(0.5 * myFocalDisk) : 0.0f;
    anari::setParameter(anariDevice, anariCamera, "apertureRadius", apertureRadius);

    // Set aspect ratio parameter
    int tiledSize[2];
    anariRendererNode->GetSize(tiledSize);
    float aspect = static_cast<float>(tiledSize[0]) / static_cast<float>(tiledSize[1]);
    anari::setParameter(anariDevice, anariCamera, "aspect", aspect);

    // Set near and far clip plane distances
    double clippingRange[2];
    cam->GetClippingRange(clippingRange);
    anari::setParameter(anariDevice, anariCamera, "near", static_cast<float>(clippingRange[0]));
    anari::setParameter(anariDevice, anariCamera, "far", static_cast<float>(clippingRange[1]));

    // Set up parameter
    double* const up = cam->GetViewUp();
    vec3 cameraUp = { static_cast<float>(up[0]), static_cast<float>(up[1]),
      static_cast<float>(up[2]) };
    anari::setParameter(anariDevice, anariCamera, "up", cameraUp);

    // Set direction parameter
    vec3 cameraDirection = { static_cast<float>(myFocalPoint[0] - shiftedCamPos[0]),
      static_cast<float>(myFocalPoint[1] - shiftedCamPos[1]),
      static_cast<float>(myFocalPoint[2] - shiftedCamPos[2]) };
    anari::setParameter(anariDevice, anariCamera, "direction", cameraDirection);

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

      anari::setParameter(anariDevice, anariCamera, "transform", matrixF);
    }

    // Region of the sensor in normalized screen-space coordinates
    double viewPort[4] = { 0, 0, 1, 1 };
    anariRendererNode->GetViewport(viewPort);

    box2 imageRegion = { vec2{ static_cast<float>(viewPort[0]), static_cast<float>(viewPort[1]) },
      vec2{ static_cast<float>(viewPort[2]), static_cast<float>(viewPort[3]) } };
    anari::setParameter(anariDevice, anariCamera, "imageRegion", imageRegion);

    anari::commitParameters(anariDevice, anariCamera);
    anariRendererNode->AddCamera(anariCamera, true);
  }
}

VTK_ABI_NAMESPACE_END
