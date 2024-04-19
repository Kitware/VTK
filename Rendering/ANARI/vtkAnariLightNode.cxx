// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariLightNode.h"
#include "vtkAnariCameraNode.h"
#include "vtkAnariProfiling.h"
#include "vtkAnariRendererNode.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#include <anari/anari_cpp.hpp>
#include <anari/anari_cpp/ext/std.h>

#include <vector>

using vec3 = anari::std_types::vec3;

VTK_ABI_NAMESPACE_BEGIN

struct vtkAnariLightNodeInternals
{
  vtkAnariRendererNode* RendererNode{ nullptr };
  anari::Light AnariLight{ nullptr };
};

//============================================================================
vtkInformationKeyMacro(vtkAnariLightNode, IS_AMBIENT, Integer);
vtkInformationKeyMacro(vtkAnariLightNode, RADIUS, Double);
vtkInformationKeyMacro(vtkAnariLightNode, FALLOFF_ANGLE, Double);
vtkInformationKeyMacro(vtkAnariLightNode, LIGHT_SCALE, Double);

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariLightNode);

//----------------------------------------------------------------------------
vtkAnariLightNode::vtkAnariLightNode()
{
  this->Internals = new vtkAnariLightNodeInternals;
}

//----------------------------------------------------------------------------
vtkAnariLightNode::~vtkAnariLightNode()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkAnariLightNode::SetLightScale(double value, vtkLight* light)
{
  if (!light)
  {
    return;
  }

  vtkInformation* info = light->GetInformation();
  info->Set(vtkAnariLightNode::LIGHT_SCALE(), value);
}

//----------------------------------------------------------------------------
double vtkAnariLightNode::GetLightScale(vtkLight* light)
{
  if (!light)
  {
    return 1.0;
  }

  vtkInformation* info = light->GetInformation();

  if (info && info->Has(vtkAnariLightNode::LIGHT_SCALE()))
  {
    return (info->Get(vtkAnariLightNode::LIGHT_SCALE()));
  }

  return 1.0;
}

//----------------------------------------------------------------------------
void vtkAnariLightNode::SetIsAmbient(int value, vtkLight* light)
{
  if (!light)
  {
    return;
  }

  vtkInformation* info = light->GetInformation();
  info->Set(vtkAnariLightNode::IS_AMBIENT(), value);
}

//----------------------------------------------------------------------------
int vtkAnariLightNode::GetIsAmbient(vtkLight* light)
{
  if (!light)
  {
    return 0;
  }

  vtkInformation* info = light->GetInformation();

  if (info && info->Has(vtkAnariLightNode::IS_AMBIENT()))
  {
    return (info->Get(vtkAnariLightNode::IS_AMBIENT()));
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkAnariLightNode::SetRadius(double value, vtkLight* light)
{
  if (!light)
  {
    return;
  }

  vtkInformation* info = light->GetInformation();
  info->Set(vtkAnariLightNode::RADIUS(), value);
}

//----------------------------------------------------------------------------
double vtkAnariLightNode::GetRadius(vtkLight* light)
{
  if (!light)
  {
    return 0.0;
  }

  vtkInformation* info = light->GetInformation();

  if (info && info->Has(vtkAnariLightNode::RADIUS()))
  {
    return (info->Get(vtkAnariLightNode::RADIUS()));
  }

  return 0.0;
}

//----------------------------------------------------------------------------
void vtkAnariLightNode::SetFalloffAngle(double value, vtkLight* light)
{
  if (!light)
  {
    return;
  }

  vtkInformation* info = light->GetInformation();
  info->Set(vtkAnariLightNode::FALLOFF_ANGLE(), value);
}

//----------------------------------------------------------------------------
double vtkAnariLightNode::GetFalloffAngle(vtkLight* light)
{
  if (!light)
  {
    return 0.1;
  }

  vtkInformation* info = light->GetInformation();

  if (info && info->Has(vtkAnariLightNode::FALLOFF_ANGLE()))
  {
    return (info->Get(vtkAnariLightNode::FALLOFF_ANGLE()));
  }

  return 0.1;
}

//----------------------------------------------------------------------------
void vtkAnariLightNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkAnariLightNode::Invalidate(bool prepass)
{
  if (prepass)
  {
    this->RenderTime = 0;
  }
}

//----------------------------------------------------------------------------
void vtkAnariLightNode::Build(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariLightNode::Build", vtkAnariProfiling::BROWN);
  if (!prepass || !LightWasModified())
  {
    return;
  }

  if (this->Internals->RendererNode == nullptr)
  {
    this->Internals->RendererNode =
      static_cast<vtkAnariRendererNode*>(this->GetFirstAncestorOfType("vtkAnariRendererNode"));
  }
}

//----------------------------------------------------------------------------
void vtkAnariLightNode::Synchronize(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariLightNode::Synchronize", vtkAnariProfiling::BROWN);
  if (!prepass || !LightWasModified())
  {
    return;
  }

  vtkLight* light = this->GetVtkLight();
  this->RenderTime = light->GetMTime();

  auto anariDevice = this->Internals->RendererNode->GetAnariDevice();
  auto vtkRenderer = this->Internals->RendererNode->GetRenderer();
  auto anariExtensions = this->Internals->RendererNode->GetAnariDeviceExtensions();

  this->ClearLight();

  const char* device = vtkAnariRendererNode::GetLibraryName(vtkRenderer);
  const char* deviceSubtype = vtkAnariRendererNode::GetDeviceSubtype(vtkRenderer);

  vtkOpenGLRenderer* openGLRenderer =
    vtkOpenGLRenderer::SafeDownCast(this->Internals->RendererNode->GetRenderable());
  vtkTransform* userLightTransform = openGLRenderer->GetUserLightTransform();

  vtkNew<vtkMatrix4x4> cameraTransform;
  vtkNew<vtkMatrix4x4> invCameraTransform;

  if (userLightTransform)
  {
    vtkAnariCameraNode* anariCameraNode = static_cast<vtkAnariCameraNode*>(
      this->Internals->RendererNode->GetFirstChildOfType("vtkAnariCameraNode"));
    vtkCamera* vtkCamera = vtkCamera::SafeDownCast(anariCameraNode->GetRenderable());
    vtkCamera->GetModelViewTransformObject()->GetMatrix(cameraTransform);
    vtkMatrix4x4::Invert(cameraTransform, invCameraTransform);
  }

  // Light Position
  double position[3];
  light->GetPosition(position);

  // Light Direction
  double focalPoint[3];
  light->GetFocalPoint(focalPoint);

  if (light->LightTypeIsCameraLight())
  {
    light->GetTransformedPosition(position);
    light->GetTransformedFocalPoint(focalPoint);
  }

  if (!light->LightTypeIsSceneLight() && userLightTransform)
  {
    cameraTransform->MultiplyPoint(position, position);
    userLightTransform->TransformPoint(position, position);
    invCameraTransform->MultiplyPoint(position, position);

    cameraTransform->MultiplyPoint(focalPoint, focalPoint);
    userLightTransform->TransformPoint(focalPoint, focalPoint);
    invCameraTransform->MultiplyPoint(focalPoint, focalPoint);
  }

  vec3 lightPosition = { static_cast<float>(position[0]), static_cast<float>(position[1]),
    static_cast<float>(position[2]) };

  double direction[3];
  vtkMath::Subtract(focalPoint, position, direction);
  vtkMath::Normalize(direction);

  vec3 lightDirection = { static_cast<float>(direction[0]), static_cast<float>(direction[1]),
    static_cast<float>(direction[2]) };

  // Light color
  vec3 lightColor = { 0.0f, 0.0f, 0.0f };

  if (light->GetSwitch())
  {
    lightColor[0] = static_cast<float>(light->GetDiffuseColor()[0]);
    lightColor[1] = static_cast<float>(light->GetDiffuseColor()[1]);
    lightColor[2] = static_cast<float>(light->GetDiffuseColor()[2]);
  }

  // Light intensity
  float lightIntensity = static_cast<float>(
    vtkAnariLightNode::GetLightScale(light) * light->GetIntensity() * vtkMath::Pi());

  anari::Light anariLight = nullptr;
  vtkTexture* envTexture = vtkRenderer->GetEnvironmentTexture();
  bool useHDRI = vtkRenderer->GetUseImageBasedLighting() && envTexture;
  int isAmbient = vtkAnariLightNode::GetIsAmbient(light);

  if (isAmbient)
  {
    vtkDebugMacro(<< "Ambient Light");
    double ambientColor[3] = { light->GetAmbientColor()[0], light->GetAmbientColor()[1],
      light->GetAmbientColor()[2] };
    vtkAnariRendererNode::SetAmbientColor(ambientColor, vtkRenderer);
  }
  else if (useHDRI)
  {
    if (anariExtensions.ANARI_KHR_LIGHT_HDRI)
    {
      anariLight = anari::newObject<anari::Light>(anariDevice, "hdri");

      // Direction to which the center of the texture will be mapped to
      anari::setParameter(anariDevice, anariLight, "direction", lightDirection);

      // Environment map
      vtkImageData* imageData = envTexture->GetInput();

      int comps = imageData->GetNumberOfScalarComponents();
      comps = comps > 3 ? 3 : comps;
      std::vector<vec3> floatData;

      // Get the needed image data attributes
      int extent[6];
      imageData->GetExtent(extent);
      int xsize = (extent[1] - extent[0]) + 1;
      int ysize = (extent[3] - extent[2]) + 1;

      for (int i = 0; i < ysize; i++)
      {
        for (int j = 0; j < xsize; j++)
        {
          vec3 value;

          for (int k = 0; k < comps; k++)
          {
            value[k] = imageData->GetScalarComponentAsFloat(j, i, 0, k);
          }

          for (int k = comps; k < 3; k++)
          {
            value[k] = 0.0f;
          }

          floatData.emplace_back(value);
        }
      }

      auto array2D = anariNewArray2D(
        anariDevice, floatData.data(), nullptr, nullptr, ANARI_FLOAT32_VEC3, xsize, ysize);
      anari::setAndReleaseParameter(anariDevice, anariLight, "radiance", array2D);
    }
    else
    {
      vtkWarningMacro(<< "ANARI back-end " << device << ":" << deviceSubtype
                      << " doesn't support image based lighting (KHR_LIGHT_HDRI).");
    }
  }
  else if (light->GetPositional())
  {
    // VTK defines cone angle as a half-angle, ANARI uses full opening angle.
    const float coneAngle = static_cast<float>(light->GetConeAngle()) * 2.0f;

    if (coneAngle <= 0.f || coneAngle >= 180.f)
    {
      if (anariExtensions.ANARI_KHR_LIGHT_POINT)
      {
        anariLight = anari::newObject<anari::Light>(anariDevice, "point");
        vtkDebugMacro(<< "Point Light");

        // The position of the point light
        anari::setParameter(anariDevice, anariLight, "position", lightPosition);
        // The overall amount of light emitted by the light in a direction in W/sr
        anari::setParameter(anariDevice, anariLight, "intensity", lightIntensity);

        // The size of the point light
        if (anariExtensions.ANARI_KHR_AREA_LIGHTS)
        {
          float radius = static_cast<float>(vtkAnariLightNode::GetRadius(light));
          anari::setParameter(anariDevice, anariLight, "radius", radius);
        }
        else
        {
          vtkWarningMacro(<< "ANARI back-end " << device << ":" << deviceSubtype
                          << " doesn't support KHR_AREA_LIGHTS::radius");
        }
      }
      else
      {
        vtkWarningMacro(<< "ANARI back-end " << device << ":" << deviceSubtype
                        << " doesn't support point lights (KHR_LIGHT_POINT).");
      }
    }
    else
    {
      if (anariExtensions.ANARI_KHR_LIGHT_SPOT)
      {
        anariLight = anari::newObject<anari::Light>(anariDevice, "spot");
        vtkDebugMacro(<< "Spot Light");

        // The overall amount of light emitted by the light in a direction in W/sr
        anari::setParameter(anariDevice, anariLight, "intensity", lightIntensity);
        // The position of the point light
        anari::setParameter(anariDevice, anariLight, "position", lightPosition);
        // main emission direction, the axis of the spot
        anari::setParameter(anariDevice, anariLight, "direction", lightDirection);
        // full opening angle (in radians) of the spot; outside of this cone is no illumination
        anari::setParameter(
          anariDevice, anariLight, "openingAngle", vtkMath::RadiansFromDegrees(coneAngle));
        // size (angle in radians) of the region between the rim (of the illumination cone) and
        // full intensity of the spot; should be smaller than half of `openingAngle`
        anari::setParameter(anariDevice, anariLight, "falloffAngle",
          static_cast<float>(vtkAnariLightNode::GetFalloffAngle(light)));
      }
      else
      {
        vtkWarningMacro(<< "ANARI back-end " << device << ":" << deviceSubtype
                        << " doesn't support spotlights (KHR_LIGHT_SPOT).");
      }
    }
  }
  else
  {
    if (anariExtensions.ANARI_KHR_LIGHT_DIRECTIONAL)
    {
      anariLight = anari::newObject<anari::Light>(anariDevice, "directional");
      vtkDebugMacro(<< "Directional Light");

      // main emission direction of the directional light
      anari::setParameter(anariDevice, anariLight, "direction", lightDirection);
      // the amount of light arriving at a surface point, assuming the light is
      // oriented towards to the surface, in W/m^2^
      float irradiance =
        static_cast<float>((vtkAnariLightNode::GetLightScale(light) * light->GetIntensity()) /
          vtkMath::Distance2BetweenPoints(position, focalPoint));
      anari::setParameter(anariDevice, anariLight, "irradiance", irradiance);

      if (anariExtensions.ANARI_KHR_AREA_LIGHTS)
      {
        float radius = static_cast<float>(vtkAnariLightNode::GetRadius(light));
        // apparent size (angle in radians) of the light
        anari::setParameter(anariDevice, anariLight, "angularDiameter", radius);
      }
      else
      {
        vtkWarningMacro(<< "ANARI back-end " << device << ":" << deviceSubtype
                        << " doesn't support KHR_AREA_LIGHTS::angularDiameter");
      }
    }
    else
    {
      vtkWarningMacro(<< "ANARI back-end " << device << ":" << deviceSubtype
                      << " doesn't support directional lights (KHR_LIGHT_DIRECTIONAL).");
    }
  }

  if (anariLight != nullptr)
  {
    // All light sources accept the following parameters
    anari::setParameter(anariDevice, anariLight, "color", lightColor);

    if (anariExtensions.ANARI_KHR_AREA_LIGHTS)
    {
      bool isVisible = light->GetSwitch() ? true : false;
      anari::setParameter(anariDevice, anariLight, "visible", isVisible);
    }
    else
    {
      vtkWarningMacro(<< "ANARI back-end " << device << ":" << deviceSubtype
                      << " doesn't support KHR_AREA_LIGHTS::visible");
    }

    anari::commitParameters(anariDevice, anariLight);
  }

  this->Internals->AnariLight = anariLight;
}

//----------------------------------------------------------------------------
void vtkAnariLightNode::Render(bool prepass)
{
  vtkAnariProfiling startProfiling("vtkAnariLightNode::Render", vtkAnariProfiling::BROWN);

  if (!prepass)
  {
    return;
  }

  if (this->Internals->RendererNode != nullptr)
  {
    this->Internals->RendererNode->AddLight(this->Internals->AnariLight);
  }
}

void vtkAnariLightNode::ClearLight()
{
  if (this->Internals->RendererNode != nullptr)
  {
    anari::Device anariDevice = this->Internals->RendererNode->GetAnariDevice();

    if (anariDevice)
    {
      anari::release(anariDevice, this->Internals->AnariLight);
      this->Internals->AnariLight = nullptr;
    }
  }
}

vtkLight* vtkAnariLightNode::GetVtkLight() const
{
  return static_cast<vtkLight*>(this->Renderable);
}

bool vtkAnariLightNode::LightWasModified() const
{
  return this->RenderTime < GetVtkLight()->GetMTime();
}

VTK_ABI_NAMESPACE_END
