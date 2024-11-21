// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUActor.h"

#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkWebGPUComputePointCloudMapper.h"
#include "vtkWebGPURenderer.h"
#include "vtkWindow.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUActor);

//------------------------------------------------------------------------------
vtkWebGPUActor::vtkWebGPUActor() = default;

//------------------------------------------------------------------------------
vtkWebGPUActor::~vtkWebGPUActor() = default;

//------------------------------------------------------------------------------
void vtkWebGPUActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ModelTransformsBuildTimestamp: " << this->ModelTransformsBuildTimestamp << '\n';
  os << indent << "ShadingOptionsBuildTimestamp: " << this->ShadingOptionsBuildTimestamp << '\n';
  os << indent << "RenderOptionsBuildTimestamp: " << this->RenderOptionsBuildTimestamp << '\n';
  os << indent << "BundleInvalidated: " << this->BundleInvalidated << '\n';
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::Render(vtkRenderer* renderer, vtkMapper* mapper)
{
  if (auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer))
  {
    switch (wgpuRenderer->GetRenderStage())
    {
      case vtkWebGPURenderer::AwaitingPreparation:
        break;
      case vtkWebGPURenderer::UpdatingBuffers:
        // reset this flag because the `mapper->Render()` call shall invalidate the bundle if it
        // determines that the render bundle needs to be recorded once again.
        this->BundleInvalidated = false;
        mapper->Render(renderer, this);
        this->CacheActorRenderOptions();
        this->CacheActorShadeOptions();
        this->CacheActorTransforms();
        break;
      case vtkWebGPURenderer::RecordingCommands:
        mapper->Render(renderer, this);
        break;
      case vtkWebGPURenderer::Finished:
      default:
        break;
    }
  }
  else
  {
    vtkErrorMacro("The renderer passed in vtkWebGPUActor::Render is not a WebGPU renderer.");
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUActor::SupportRenderBundles()
{
  vtkWebGPUComputePointCloudMapper* pointCloudMapper =
    vtkWebGPUComputePointCloudMapper::SafeDownCast(this->GetMapper());
  if (pointCloudMapper != nullptr)
  {
    // This actor is using the point cloud mapper which doesn't support render bundles.
    return false;
  }

  // Assuming that any other mapper supports render bundles
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUActor::UpdateKeyMatrices()
{
  vtkMTimeType rwTime = 0;
  if (this->CoordinateSystem != WORLD && this->CoordinateSystemRenderer)
  {
    rwTime = this->CoordinateSystemRenderer->GetVTKWindow()->GetMTime();
  }

  // has the actor changed or is in device coords?
  if (this->GetMTime() > this->ModelTransformsBuildTimestamp ||
    rwTime > this->ModelTransformsBuildTimestamp || this->CoordinateSystem == DEVICE)
  {
    this->GetModelToWorldMatrix(this->MCWCMatrix);

    this->MCWCMatrix->Transpose();

    if (this->GetIsIdentity())
    {
      this->NormalMatrix->Identity();
    }
    else
    {
      this->NormalTransform->SetMatrix(this->Matrix);
      vtkMatrix4x4* mat4 = this->NormalTransform->GetMatrix();
      for (int i = 0; i < 3; ++i)
      {
        for (int j = 0; j < 3; ++j)
        {
          this->NormalMatrix->SetElement(i, j, mat4->GetElement(i, j));
        }
      }
    }
    this->NormalMatrix->Invert();
    this->ModelTransformsBuildTimestamp.Modified();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::CacheActorTransforms()
{
  if (this->UpdateKeyMatrices())
  {
    auto& transform = this->CachedActorInfo.Transform;
    // stage world
    std::transform(this->MCWCMatrix->GetData(), this->MCWCMatrix->GetData() + 16,
      &(transform.World[0][0]), [](double& v) -> float { return static_cast<float>(v); });
    // stage normal
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        transform.Normal[i][j] = this->NormalMatrix->GetElement(i, j);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::CacheActorRenderOptions()
{
  auto* displayProperty = this->GetProperty();
  if (displayProperty->GetMTime() > this->RenderOptionsBuildTimestamp ||
    this->GetMTime() > this->RenderOptionsBuildTimestamp)
  {
    auto& ro = this->CachedActorInfo.RenderOpts;
    ro.PointSize = displayProperty->GetPointSize();
    ro.LineWidth = displayProperty->GetLineWidth();
    ro.EdgeWidth = displayProperty->GetEdgeWidth();
    // input property is shifted by `k` positions and then the shifted value is added to flags
    // to update the bit at `k` position.
    ro.Flags = displayProperty->GetRepresentation() | (displayProperty->GetEdgeVisibility() << 2) |
      (displayProperty->GetVertexVisibility() << 3) |
      (displayProperty->GetUseLineWidthForEdgeThickness() << 4) |
      (displayProperty->GetRenderPointsAsSpheres() << 5) |
      (displayProperty->GetRenderLinesAsTubes() << 6) |
      (static_cast<int>(displayProperty->GetPoint2DShape()) << 7);
    this->RenderOptionsBuildTimestamp.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::CacheActorShadeOptions()
{
  auto* displayProperty = this->GetProperty();
  if (displayProperty->GetMTime() > this->ShadingOptionsBuildTimestamp ||
    this->GetMTime() > this->ShadingOptionsBuildTimestamp)
  {
    auto& so = this->CachedActorInfo.ShadeOpts;
    so.AmbientIntensity = displayProperty->GetAmbient();
    so.DiffuseIntensity = displayProperty->GetDiffuse();
    so.SpecularIntensity = displayProperty->GetSpecular();
    so.SpecularPower = displayProperty->GetSpecularPower();
    so.Opacity = displayProperty->GetOpacity();
    for (int i = 0; i < 3; ++i)
    {
      so.AmbientColor[i] = displayProperty->GetAmbientColor()[i];
      so.DiffuseColor[i] = displayProperty->GetDiffuseColor()[i];
      so.SpecularColor[i] = displayProperty->GetSpecularColor()[i];
      so.EdgeColor[i] = displayProperty->GetEdgeColor()[i];
      so.VertexColor[i] = displayProperty->GetVertexColor()[i];
    }
    this->ShadingOptionsBuildTimestamp.Modified();
  }
}
VTK_ABI_NAMESPACE_END
