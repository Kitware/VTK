// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPULight.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"
#include "vtkWebGPURenderer.h"

#include <cstring>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebGPULight);

//------------------------------------------------------------------------------
void vtkWebGPULight::Render(vtkRenderer* renderer, int)
{
  this->CacheLightInformation(renderer, renderer->GetActiveCamera());
}

//------------------------------------------------------------------------------
void vtkWebGPULight::CacheLightInformation(vtkRenderer* renderer, vtkCamera* camera)
{
  LightInfo& li = this->CachedLightInfo;
  std::memset(&li, 0, sizeof(li));

  li.Type = this->LightType;
  li.Color[0] = { static_cast<vtkTypeFloat32>(this->DiffuseColor[0] * this->Intensity) };
  li.Color[1] = { static_cast<vtkTypeFloat32>(this->DiffuseColor[1] * this->Intensity) };
  li.Color[2] = { static_cast<vtkTypeFloat32>(this->DiffuseColor[2] * this->Intensity) };

  auto wgpuRenderer = reinterpret_cast<vtkWebGPURenderer*>(renderer);
  if (wgpuRenderer->GetLightingComplexity() >=
    vtkWebGPURenderer::LightingComplexityEnum::Directional)
  {
    // for lightkit case there are some parameters to set
    vtkTransform* viewTF = camera->GetModelViewTransformObject();
    // get required info from light
    double* lfp = this->GetTransformedFocalPoint();
    double* lp = this->GetTransformedPosition();
    double lightDir[3];
    vtkMath::Subtract(lfp, lp, lightDir);
    vtkMath::Normalize(lightDir);
    double tDirView[3];
    viewTF->TransformNormal(lightDir, tDirView);

    if (!this->LightTypeIsSceneLight() && wgpuRenderer->GetUserLightTransform() != nullptr)
    {
      double* tDir = wgpuRenderer->GetUserLightTransform()->TransformNormal(tDirView);
      li.DirectionVC[0] = tDir[0];
      li.DirectionVC[1] = tDir[1];
      li.DirectionVC[2] = tDir[2];
    }
    else
    {
      // for lightkit case there are some parameters to set
      li.DirectionVC[0] = tDirView[0];
      li.DirectionVC[1] = tDirView[1];
      li.DirectionVC[2] = tDirView[2];
    }

    // we are done unless we have positional lights
    if (wgpuRenderer->GetLightingComplexity() >=
      vtkWebGPURenderer::LightingComplexityEnum::Positional)
    {
      // if positional lights pass down more parameters
      double* attn = this->AttenuationValues;
      li.Attenuation[0] = attn[0];
      li.Attenuation[1] = attn[1];
      li.Attenuation[2] = attn[2];
      double tlpView[3];
      viewTF->TransformPoint(lp, tlpView);
      if (!this->LightTypeIsSceneLight() && wgpuRenderer->GetUserLightTransform() != nullptr)
      {
        double* tlp = wgpuRenderer->GetUserLightTransform()->TransformPoint(tlpView);
        li.PositionVC[0] = tlp[0];
        li.PositionVC[1] = tlp[1];
        li.PositionVC[2] = tlp[2];
      }
      else
      {
        li.PositionVC[0] = tlpView[0];
        li.PositionVC[1] = tlpView[1];
        li.PositionVC[2] = tlpView[2];
      }
      li.ConeAngle = this->ConeAngle;
      li.Exponent = this->Exponent;
      li.Positional = this->Positional;
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPULight::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
