/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayLightNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayLightNode.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkOSPRayCameraNode.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkTransform.h"

#include <vector>

vtkInformationKeyMacro(vtkOSPRayLightNode, IS_AMBIENT, Integer);
vtkInformationKeyMacro(vtkOSPRayLightNode, RADIUS, Double);

//============================================================================
double vtkOSPRayLightNode::LightScale = 1.0;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayLightNode);

//----------------------------------------------------------------------------
vtkOSPRayLightNode::vtkOSPRayLightNode()
{
  this->OLight = nullptr;
}

//----------------------------------------------------------------------------
vtkOSPRayLightNode::~vtkOSPRayLightNode()
{
  vtkOSPRayRendererNode* orn = vtkOSPRayRendererNode::GetRendererNode(this);
  if (orn)
  {
    RTW::Backend* backend = orn->GetBackend();
    if (backend != nullptr)
      ospRelease((OSPLight)this->OLight);
  }
}

//----------------------------------------------------------------------------
void vtkOSPRayLightNode::SetLightScale(double s)
{
  vtkOSPRayLightNode::LightScale = s;
}

//----------------------------------------------------------------------------
double vtkOSPRayLightNode::GetLightScale()
{
  return vtkOSPRayLightNode::LightScale;
}

//----------------------------------------------------------------------------
void vtkOSPRayLightNode::SetIsAmbient(int value, vtkLight* light)
{
  if (!light)
  {
    return;
  }
  vtkInformation* info = light->GetInformation();
  info->Set(vtkOSPRayLightNode::IS_AMBIENT(), value);
}

//----------------------------------------------------------------------------
int vtkOSPRayLightNode::GetIsAmbient(vtkLight* light)
{
  if (!light)
  {
    return 0;
  }
  vtkInformation* info = light->GetInformation();
  if (info && info->Has(vtkOSPRayLightNode::IS_AMBIENT()))
  {
    return (info->Get(vtkOSPRayLightNode::IS_AMBIENT()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkOSPRayLightNode::SetRadius(double value, vtkLight* light)
{
  if (!light)
  {
    return;
  }
  vtkInformation* info = light->GetInformation();
  info->Set(vtkOSPRayLightNode::RADIUS(), value);
}

//----------------------------------------------------------------------------
double vtkOSPRayLightNode::GetRadius(vtkLight* light)
{
  if (!light)
  {
    return 0.0;
  }
  vtkInformation* info = light->GetInformation();
  if (info && info->Has(vtkOSPRayLightNode::RADIUS()))
  {
    return (info->Get(vtkOSPRayLightNode::RADIUS()));
  }
  return 0.0;
}

//----------------------------------------------------------------------------
void vtkOSPRayLightNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayLightNode::Render(bool prepass)
{
  if (prepass)
  {
    vtkOSPRayRendererNode* orn =
      static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));

    vtkOpenGLRenderer* ren = vtkOpenGLRenderer::SafeDownCast(orn->GetRenderable());
    vtkTransform* userLightTransfo = ren->GetUserLightTransform();

    vtkNew<vtkMatrix4x4> camTransfo;
    vtkNew<vtkMatrix4x4> invCamTransfo;
    if (userLightTransfo)
    {
      vtkOSPRayCameraNode* ocam =
        static_cast<vtkOSPRayCameraNode*>(orn->GetFirstChildOfType("vtkOSPRayCameraNode"));

      vtkCamera* cam = vtkCamera::SafeDownCast(ocam->GetRenderable());

      cam->GetModelViewTransformObject()->GetMatrix(camTransfo);
      vtkMatrix4x4::Invert(camTransfo, invCamTransfo);
    }

    RTW::Backend* backend = orn->GetBackend();
    if (backend == nullptr)
      return;
    ospRelease((OSPLight)this->OLight);
    OSPLight ospLight;

    vtkLight* light = vtkLight::SafeDownCast(this->GetRenderable());

    float color[3] = { 0.0, 0.0, 0.0 };
    if (light->GetSwitch())
    {
      color[0] = static_cast<float>(light->GetDiffuseColor()[0]);
      color[1] = static_cast<float>(light->GetDiffuseColor()[1]);
      color[2] = static_cast<float>(light->GetDiffuseColor()[2]);
    }
    if (vtkOSPRayLightNode::GetIsAmbient(light))
    {
      ospLight = ospNewLight3("ambient");
      color[0] = static_cast<float>(light->GetDiffuseColor()[0]);
      color[1] = static_cast<float>(light->GetDiffuseColor()[1]);
      color[2] = static_cast<float>(light->GetDiffuseColor()[2]);
      ospSet3f(ospLight, "color", color[0], color[1], color[2]);
      float fI = static_cast<float>(
        0.13f * vtkOSPRayLightNode::LightScale * light->GetIntensity() * vtkMath::Pi());
      ospSet1f(ospLight, "intensity", fI);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
    }
    else if (light->GetPositional())
    {
      double position[4];
      light->GetPosition(position);
      position[3] = 1.0;

      if (light->LightTypeIsCameraLight())
      {
        light->TransformPoint(position, position);
      }

      if (!light->LightTypeIsSceneLight() && userLightTransfo)
      {
        camTransfo->MultiplyPoint(position, position);
        userLightTransfo->TransformPoint(position, position);
        invCamTransfo->MultiplyPoint(position, position);
      }

      float coneAngle = static_cast<float>(light->GetConeAngle());
      if (coneAngle <= 0.0 || coneAngle >= 90.0)
      {
        ospLight = ospNewLight3("PointLight");
      }
      else
      {
        ospLight = ospNewLight3("SpotLight");
        double focalPoint[4];
        light->GetFocalPoint(focalPoint);
        focalPoint[3] = 1.0;

        if (light->LightTypeIsCameraLight())
        {
          light->TransformPoint(focalPoint, focalPoint);
        }

        if (!light->LightTypeIsSceneLight() && userLightTransfo)
        {
          camTransfo->MultiplyPoint(focalPoint, focalPoint);
          userLightTransfo->TransformPoint(focalPoint, focalPoint);
          invCamTransfo->MultiplyPoint(focalPoint, focalPoint);
        }

        double direction[3];
        vtkMath::Subtract(focalPoint, position, direction);
        vtkMath::Normalize(direction);

        ospSet3f(ospLight, "direction", direction[0], direction[1], direction[2]);
        // OpenGL interprets this as a half-angle. Mult by 2 for consistency.
        ospSet1f(ospLight, "openingAngle", 2 * coneAngle);
        // TODO: penumbraAngle
      }
      ospSet3f(ospLight, "color", color[0], color[1], color[2]);
      float fI =
        static_cast<float>(vtkOSPRayLightNode::LightScale * light->GetIntensity() * vtkMath::Pi());
      ospSet1i(ospLight, "isVisible", 0);
      ospSet1f(ospLight, "intensity", fI);

      ospSet3f(ospLight, "position", position[0], position[1], position[2]);
      float r = static_cast<float>(vtkOSPRayLightNode::GetRadius(light));
      ospSet1f(ospLight, "radius", r);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
    }
    else
    {
      double position[3];
      double focalPoint[3];
      light->GetPosition(position);
      light->GetFocalPoint(focalPoint);

      double direction[4];
      vtkMath::Subtract(focalPoint, position, direction);
      vtkMath::Normalize(direction);
      direction[3] = 0.0;

      if (light->LightTypeIsCameraLight())
      {
        light->TransformVector(direction, direction);
      }

      if (!light->LightTypeIsSceneLight() && userLightTransfo)
      {
        camTransfo->MultiplyPoint(direction, direction);
        userLightTransfo->TransformNormal(direction, direction);
        invCamTransfo->MultiplyPoint(direction, direction);
      }

      ospLight = ospNewLight3("DirectionalLight");
      ospSet3f(ospLight, "color", color[0], color[1], color[2]);
      float fI =
        static_cast<float>(vtkOSPRayLightNode::LightScale * light->GetIntensity() * vtkMath::Pi());
      ospSet1f(ospLight, "intensity", fI);
      ospSet3f(ospLight, "direction", direction[0], direction[1], direction[2]);
      float r = static_cast<float>(vtkOSPRayLightNode::GetRadius(light));
      ospSet1f(ospLight, "angularDiameter", r);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
    }
    this->OLight = ospLight;
  }
}
