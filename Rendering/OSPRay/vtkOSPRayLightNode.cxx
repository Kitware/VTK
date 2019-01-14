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

#include "vtkCollectionIterator.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOSPRayRendererNode.h"

#include "ospray/version.h"
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
  ospRelease((OSPLight)this->OLight);
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
void vtkOSPRayLightNode::SetIsAmbient(int value, vtkLight *light)
{
  if (!light)
  {
    return;
  }
  vtkInformation *info = light->GetInformation();
  info->Set(vtkOSPRayLightNode::IS_AMBIENT(), value);
}

//----------------------------------------------------------------------------
int vtkOSPRayLightNode::GetIsAmbient(vtkLight *light)
{
  if (!light)
  {
    return 0;
  }
  vtkInformation *info = light->GetInformation();
  if (info && info->Has(vtkOSPRayLightNode::IS_AMBIENT()))
  {
    return (info->Get(vtkOSPRayLightNode::IS_AMBIENT()));
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkOSPRayLightNode::SetRadius(double value, vtkLight *light)
{
  if (!light)
  {
    return;
  }
  vtkInformation *info = light->GetInformation();
  info->Set(vtkOSPRayLightNode::RADIUS(), value);
}

//----------------------------------------------------------------------------
double vtkOSPRayLightNode::GetRadius(vtkLight *light)
{
  if (!light)
  {
    return 0.0;
  }
  vtkInformation *info = light->GetInformation();
  if (info && info->Has(vtkOSPRayLightNode::RADIUS()))
  {
    return (info->Get(vtkOSPRayLightNode::RADIUS()));
  }
  return 0.0;
}

//----------------------------------------------------------------------------
OSPLight vtkOSPRayLightNode::NewLight(vtkOSPRayRendererNode *orn,
                                      OSPRenderer oRenderer,
                                      const std::string& lightType)
{
  OSPLight result;
#if OSPRAY_VERSION_MAJOR == 1 && OSPRAY_VERSION_MINOR >= 5
  (void)oRenderer;
  const std::string rendererType = vtkOSPRayRendererNode::GetRendererType(orn->GetRenderer());
  result = ospNewLight2(rendererType.c_str(), lightType.c_str());
#else
  (void)orn;
  result = ospNewLight(oRenderer, lightType.c_str());
#endif

  if (!result)
  {
    vtkGenericWarningMacro("Failed to create OSPRay light: " << lightType);
  }

  return result;
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
    ospRelease((OSPLight)this->OLight);
    OSPLight ospLight;

    vtkOSPRayRendererNode *orn =
      static_cast<vtkOSPRayRendererNode *>(
        this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
    OSPRenderer oRenderer = orn->GetORenderer();

    vtkLight *light = vtkLight::SafeDownCast(this->GetRenderable());
    int lt = light->GetLightType();
    float color[3] = {0.0,0.0,0.0};
    if (light->GetSwitch())
    {
      color[0] = static_cast<float>(light->GetDiffuseColor()[0]);
      color[1] = static_cast<float>(light->GetDiffuseColor()[1]);
      color[2] = static_cast<float>(light->GetDiffuseColor()[2]);
    }
    if (vtkOSPRayLightNode::GetIsAmbient(light))
    {
      ospLight = NewLight(orn, oRenderer, "ambient");
      color[0] = static_cast<float>(light->GetDiffuseColor()[0]);
      color[1] = static_cast<float>(light->GetDiffuseColor()[1]);
      color[2] = static_cast<float>(light->GetDiffuseColor()[2]);
      ospSet3f(ospLight, "color", color[0], color[1], color[2]);
      float fI = static_cast<float>
        (0.13f*
         vtkOSPRayLightNode::LightScale*
         light->GetIntensity()*
         vtkMath::Pi()
         );
      ospSet1f(ospLight, "intensity", fI);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
    }
    else if (light->GetPositional())
    {
      double px, py, pz;
      light->GetTransformedPosition(px, py, pz);
      if (lt == VTK_LIGHT_TYPE_SCENE_LIGHT) //todo: hacky and doesn't fix GL
        {
        double *p = light->GetPosition();
        px = p[0];
        py = p[1];
        pz = p[2];
        }
      float coneAngle = static_cast<float>(light->GetConeAngle());
      if (coneAngle <= 0.0)
      {
        ospLight = NewLight(orn, oRenderer, "PointLight");
      }
      else
      {
        ospLight = NewLight(orn, oRenderer, "SpotLight");
        double fx, fy, fz;
        light->GetTransformedFocalPoint(fx, fy, fz);
        if (lt == VTK_LIGHT_TYPE_SCENE_LIGHT)
        {
          double *p = light->GetFocalPoint();
          fx = p[0];
          fy = p[1];
          fz = p[2];
        }
        double direction[3];
        direction[0] = fx - px;
        direction[1] = fy - py;
        direction[2] = fz - pz;
        ospSet3f(ospLight, "direction",
                 direction[0], direction[1], direction[2]);
        // OpenGL interprets this as a half-angle. Mult by 2 for consistency.
        ospSet1f(ospLight, "openingAngle", 2 * coneAngle);
        //TODO: penumbraAngle
      }
      ospSet3f(ospLight, "color", color[0], color[1], color[2]);
      float fI = static_cast<float>
        (vtkOSPRayLightNode::LightScale*
         light->GetIntensity()*
         vtkMath::Pi()
         );
      ospSet1i(ospLight, "isVisible", 0);
      ospSet1f(ospLight, "intensity", fI);
      ospSet3f(ospLight, "position", px, py, pz);
      float r = static_cast<float>(vtkOSPRayLightNode::GetRadius(light));
      ospSet1f(ospLight, "radius", r);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
    }
    else
    {
      double px, py, pz;
      double fx, fy, fz;
      light->GetTransformedPosition(px, py, pz);
      light->GetTransformedFocalPoint(fx, fy, fz);
      if (lt == VTK_LIGHT_TYPE_SCENE_LIGHT)
      {
        double *p = light->GetPosition();
        px = p[0];
        py = p[1];
        pz = p[2];
        p = light->GetFocalPoint();
        fx = p[0];
        fy = p[1];
        fz = p[2];
      }
      double direction[3];
      direction[0] = fx - px;
      direction[1] = fy - py;
      direction[2] = fz - pz;
      ospLight = NewLight(orn, oRenderer, "DirectionalLight");
      ospSet3f(ospLight, "color", color[0], color[1], color[2]);
      float fI = static_cast<float>
        (vtkOSPRayLightNode::LightScale*
         light->GetIntensity()*
         vtkMath::Pi());
      ospSet1f(ospLight, "intensity", fI);
      vtkMath::Normalize(direction);
      ospSet3f(ospLight, "direction",
               direction[0], direction[1], direction[2]);
      float r = static_cast<float>(vtkOSPRayLightNode::GetRadius(light));
      ospSet1f(ospLight, "angularDiameter", r);
      ospCommit(ospLight);
      orn->AddLight(ospLight);
    }
    this->OLight = ospLight;
  }
}
