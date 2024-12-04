// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUActor.h"

#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"

#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkWebGPUComputePointCloudMapper.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"
#include "vtkWindow.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
void vtkWebGPUActor::MapperBooleanCache::SetValue(bool newValue)
{
  this->Value = newValue;
  this->TimeStamp.Modified();
}

//------------------------------------------------------------------------------
bool vtkWebGPUActor::MapperBooleanCache::IsOutdated(vtkMapper* mapper)
{
  return mapper->GetMTime() > this->TimeStamp;
}

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
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::ReleaseGraphicsResources(vtkWindow* window)
{
  this->ActorBindGroupLayout = nullptr;
  this->ActorBindGroup = nullptr;
  this->ActorBuffer = nullptr;
  this->MapperHasOpaqueGeometry = {};
  this->MapperHasTranslucentPolygonalGeometry = {};
  this->Superclass::ReleaseGraphicsResources(window);
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::Render(vtkRenderer* renderer, vtkMapper* mapper)
{
  if (auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer))
  {
    auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(wgpuRenderer->GetRenderWindow());
    auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
    switch (wgpuRenderer->GetRenderStage())
    {
      case vtkWebGPURenderer::AwaitingPreparation:
        break;
      case vtkWebGPURenderer::UpdatingBuffers:
      {
        if (!(this->ActorBindGroup && this->ActorBindGroupLayout && this->ActorBuffer))
        {
          this->AllocateResources(wgpuConfiguration);
        }
        // reset this flag because the `mapper->Render()` call shall invalidate the bundle if it
        // determines that the render bundle needs to be recorded once again.
        mapper->Render(renderer, this);
        bool updateBuffers = this->CacheActorRenderOptions();
        updateBuffers |= this->CacheActorShadeOptions();
        updateBuffers |= this->CacheActorTransforms();
        if (updateBuffers)
        {
          wgpuConfiguration->WriteBuffer(this->ActorBuffer, 0, this->GetCachedActorInformation(),
            this->GetCacheSizeBytes(), "ActorBufferUpdate");
        }
        break;
      }
      case vtkWebGPURenderer::RecordingCommands:
        if (wgpuRenderer->GetUseRenderBundles() && this->SupportRenderBundles())
        {
          if (wgpuRenderer->GetRebuildRenderBundle())
          {
            wgpuRenderer->GetRenderBundleEncoder().SetBindGroup(1, this->ActorBindGroup);
            mapper->Render(renderer, this);
          }
          // else, no need to record draw commands.
        }
        else
        {
          wgpuRenderer->GetRenderPassEncoder().SetBindGroup(1, this->ActorBindGroup);
          mapper->Render(renderer, this);
        }
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
vtkTypeBool vtkWebGPUActor::HasOpaqueGeometry()
{
  bool isOpaque = false;
  if (this->Mapper)
  {
    if (this->MapperHasOpaqueGeometry.IsOutdated(this->Mapper))
    {
      isOpaque = this->Superclass::HasOpaqueGeometry();
      this->MapperHasOpaqueGeometry.SetValue(isOpaque);
    }
    else
    {
      // nullify mapper so that superclass doesn't run the expensive
      // code path in vtkMapper::HasOpaqueGeometry.
      vtkMapper* tmpMapper = this->Mapper;
      this->Mapper = nullptr;
      isOpaque = this->Superclass::HasOpaqueGeometry();
      // restore
      this->Mapper = tmpMapper;
      isOpaque &= this->MapperHasOpaqueGeometry.GetValue();
    }
  }
  else
  {
    isOpaque = this->Superclass::HasOpaqueGeometry();
  }
  return isOpaque;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkWebGPUActor::HasTranslucentPolygonalGeometry()
{
  bool isTranslucent = false;
  if (this->Mapper)
  {
    if (this->MapperHasTranslucentPolygonalGeometry.IsOutdated(this->Mapper))
    {
      isTranslucent = this->Superclass::HasTranslucentPolygonalGeometry();
      this->MapperHasTranslucentPolygonalGeometry.SetValue(isTranslucent);
    }
    else
    {
      // nullify mapper so that superclass doesn't run the expensive
      // code path in vtkMapper::HasTranslucentPolygonalGeometry.
      vtkMapper* tmpMapper = this->Mapper;
      this->Mapper = nullptr;
      isTranslucent = this->Superclass::HasTranslucentPolygonalGeometry();
      // restore
      this->Mapper = tmpMapper;
      isTranslucent &= this->MapperHasTranslucentPolygonalGeometry.GetValue();
    }
  }
  else
  {
    isTranslucent = this->Superclass::HasTranslucentPolygonalGeometry();
  }
  return isTranslucent;
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
bool vtkWebGPUActor::CacheActorTransforms()
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
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkWebGPUActor::CacheActorRenderOptions()
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
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkWebGPUActor::CacheActorShadeOptions()
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
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::AllocateResources(vtkWebGPUConfiguration* wgpuConfiguration)
{
  const auto& device = wgpuConfiguration->GetDevice();

  const auto actorDescription = this->GetObjectDescription();
  const auto bufferLabel = "buffer@" + actorDescription;
  const auto bufferSize = vtkWebGPUConfiguration::Align(vtkWebGPUActor::GetCacheSizeBytes(), 32);
  this->ActorBuffer = wgpuConfiguration->CreateBuffer(bufferSize,
    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, false, bufferLabel.c_str());

  this->ActorBindGroupLayout = vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
    {
      // clang-format off
      // ActorBlocks
      { 0, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage },
      // clang-format on
    },
    "bgl@" + actorDescription);
  this->ActorBindGroup =
    vtkWebGPUBindGroupInternals::MakeBindGroup(device, this->ActorBindGroupLayout,
      {
        // clang-format off
        { 0, this->ActorBuffer, 0, bufferSize },
        // clang-format on
      },
      "bg@" + actorDescription);
  // Reset timestamps because the previous buffer is now gone and contents of the buffer will need
  // to be re-uploaded.
  this->ModelTransformsBuildTimestamp = vtkTimeStamp();
  this->ShadingOptionsBuildTimestamp = vtkTimeStamp();
  this->RenderOptionsBuildTimestamp = vtkTimeStamp();
}
VTK_ABI_NAMESPACE_END
