// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUActor.h"

#include "Private/vtkWebGPUActorInternals.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"

#include "vtkImageData.h"
#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkWebGPUComputePointCloudMapper.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"
#include "vtkWebGPUTexture.h"
#include "vtkWindow.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUActor);

//------------------------------------------------------------------------------
vtkWebGPUActor::vtkWebGPUActor()
  : Internals(new vtkWebGPUActorInternals())
{
}

//------------------------------------------------------------------------------
vtkWebGPUActor::~vtkWebGPUActor() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPUActor::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::PrintSelf(ostream& os, vtkIndent indent)
{
  const auto& internals = (*this->Internals);
  os << indent << "ModelTransformsLastUpdated: " << internals.ModelTransformsLastUpdated << '\n';
  os << indent << "ShadingOptionsLastUpdated: " << internals.ShadingOptionsLastUpdated << '\n';
  os << indent << "RenderOptionsLastUpdated: " << internals.RenderOptionsLastUpdated << '\n';
  os << indent << "DeviceResourcesBuildTimestamp: " << internals.DeviceResourcesBuildTimestamp
     << '\n';
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::ReleaseGraphicsResources(vtkWindow* window)
{
  auto& internals = (*this->Internals);
  internals.ActorBindGroupLayout = nullptr;
  internals.ActorBindGroup = nullptr;
  internals.ActorBuffer = nullptr;
  internals.MapperHasOpaqueGeometry = {};
  internals.MapperHasTranslucentPolygonalGeometry = {};
  this->Superclass::ReleaseGraphicsResources(window);
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::ShallowCopy(vtkProp* other)
{
  auto& internals = (*this->Internals);
  if (auto* wgpuActor = vtkWebGPUActor::SafeDownCast(other))
  {
    const auto& otherInternals = (*wgpuActor->Internals);

    internals.CachedActorInfo = otherInternals.CachedActorInfo;

    internals.MapperHasOpaqueGeometry = otherInternals.MapperHasOpaqueGeometry;
    internals.MapperHasTranslucentPolygonalGeometry =
      otherInternals.MapperHasTranslucentPolygonalGeometry;

    internals.ActorBindGroupLayout = otherInternals.ActorBindGroupLayout;
    internals.ActorBindGroup = otherInternals.ActorBindGroup;
    internals.ActorBuffer = otherInternals.ActorBuffer;

    internals.MCWCMatrix->DeepCopy(otherInternals.MCWCMatrix);
    internals.NormalMatrix->DeepCopy(otherInternals.NormalMatrix);
    internals.NormalTransform->DeepCopy(otherInternals.NormalTransform);
  }
  this->Superclass::ShallowCopy(other);
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::Render(vtkRenderer* renderer, vtkMapper* mapper)
{
  auto& internals = (*this->Internals);
  if (auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer))
  {
    auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(wgpuRenderer->GetRenderWindow());
    auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
    switch (wgpuRenderer->GetRenderStage())
    {
      case vtkWebGPURenderer::AwaitingPreparation:
        break;
      case vtkWebGPURenderer::SyncDeviceResources:
      {
        if (auto* wgpuTexture = vtkWebGPUTexture::SafeDownCast(this->GetTexture()))
        {
          wgpuTexture->Render(renderer);
        }
        if (this->NeedToRecreateBindGroups())
        {
          this->CreateBindGroups(wgpuConfiguration);
        }
        // reset this flag because the `mapper->Render()` call shall invalidate the bundle if it
        // determines that the render bundle needs to be recorded once again.
        mapper->Render(renderer, this);
        bool updateBuffers = this->CacheActorRenderOptions();
        updateBuffers |= this->CacheActorShadeOptions();
        updateBuffers |= this->CacheActorTransforms();
        updateBuffers |= this->CacheActorId();
        if (updateBuffers)
        {
          wgpuConfiguration->WriteBuffer(internals.ActorBuffer, 0,
            this->GetCachedActorInformation(), this->GetCacheSizeBytes(), "ActorBufferUpdate");
        }
        break;
      }
      case vtkWebGPURenderer::RecordingCommands:
        if (wgpuRenderer->GetUseRenderBundles() && this->SupportRenderBundles())
        {
          if (wgpuRenderer->GetRebuildRenderBundle())
          {
            wgpuRenderer->GetRenderBundleEncoder().SetBindGroup(1, internals.ActorBindGroup);
            mapper->Render(renderer, this);
          }
          // else, no need to record draw commands.
        }
        else
        {
          wgpuRenderer->GetRenderPassEncoder().SetBindGroup(1, internals.ActorBindGroup);
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
void vtkWebGPUActor::SetId(vtkTypeUInt32 id)
{
  this->Internals->Id = id;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkWebGPUActor::HasOpaqueGeometry()
{
  auto& internals = (*this->Internals);
  bool isOpaque = false;
  if (this->Mapper)
  {
    if (internals.MapperHasOpaqueGeometry.IsOutdated(this->Mapper))
    {
      isOpaque = this->Superclass::HasOpaqueGeometry();
      internals.MapperHasOpaqueGeometry.SetValue(isOpaque);
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
      isOpaque &= internals.MapperHasOpaqueGeometry.GetValue();
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
  auto& internals = (*this->Internals);
  bool isTranslucent = false;
  if (this->Mapper)
  {
    if (internals.MapperHasTranslucentPolygonalGeometry.IsOutdated(this->Mapper))
    {
      isTranslucent = this->Superclass::HasTranslucentPolygonalGeometry();
      internals.MapperHasTranslucentPolygonalGeometry.SetValue(isTranslucent);
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
      isTranslucent &= internals.MapperHasTranslucentPolygonalGeometry.GetValue();
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
  auto& internals = (*this->Internals);
  vtkMTimeType rwTime = 0;
  if (this->CoordinateSystem != WORLD && this->CoordinateSystemRenderer)
  {
    rwTime = this->CoordinateSystemRenderer->GetVTKWindow()->GetMTime();
  }

  // has the actor changed or is in device coords?
  if (this->GetMTime() > internals.ModelTransformsLastUpdated ||
    rwTime > internals.ModelTransformsLastUpdated || this->CoordinateSystem == DEVICE)
  {
    this->GetModelToWorldMatrix(internals.MCWCMatrix);

    internals.MCWCMatrix->Transpose();

    if (this->GetIsIdentity())
    {
      internals.NormalMatrix->Identity();
    }
    else
    {
      internals.NormalTransform->SetMatrix(this->Matrix);
      vtkMatrix4x4* mat4 = internals.NormalTransform->GetMatrix();
      for (int i = 0; i < 3; ++i)
      {
        for (int j = 0; j < 3; ++j)
        {
          internals.NormalMatrix->SetElement(i, j, mat4->GetElement(i, j));
        }
      }
    }
    internals.NormalMatrix->Invert();
    internals.ModelTransformsLastUpdated.Modified();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
const void* vtkWebGPUActor::GetCachedActorInformation()
{
  return &(this->Internals->CachedActorInfo);
}

//------------------------------------------------------------------------------
std::size_t vtkWebGPUActor::GetCacheSizeBytes()
{
  return sizeof(vtkWebGPUActorInternals::ActorBlock);
}

//------------------------------------------------------------------------------
bool vtkWebGPUActor::CacheActorTransforms()
{
  auto& internals = (*this->Internals);
  if (this->UpdateKeyMatrices())
  {
    auto& transform = internals.CachedActorInfo.Transform;
    // stage world
    std::transform(internals.MCWCMatrix->GetData(), internals.MCWCMatrix->GetData() + 16,
      &(transform.World[0][0]), [](double& v) -> float { return static_cast<float>(v); });
    // stage normal
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        transform.Normal[i][j] = internals.NormalMatrix->GetElement(i, j);
      }
    }
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkWebGPUActor::CacheActorRenderOptions()
{
  auto& internals = (*this->Internals);
  auto* displayProperty = this->GetProperty();
  if (displayProperty->GetMTime() > internals.RenderOptionsLastUpdated ||
    this->GetMTime() > internals.RenderOptionsLastUpdated)
  {
    auto& ro = internals.CachedActorInfo.RenderOpts;
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
    ro.Flags2 = displayProperty->GetLighting();
    internals.RenderOptionsLastUpdated.Modified();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkWebGPUActor::CacheActorShadeOptions()
{
  auto& internals = (*this->Internals);
  auto* displayProperty = this->GetProperty();
  if (displayProperty->GetMTime() > internals.ShadingOptionsLastUpdated ||
    this->GetMTime() > internals.ShadingOptionsLastUpdated)
  {
    auto& so = internals.CachedActorInfo.ColorOpts;
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
    internals.ShadingOptionsLastUpdated.Modified();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkWebGPUActor::CacheActorId()
{
  auto& internals = (*this->Internals);
  if (internals.CachedActorInfo.ColorOpts.Id != internals.Id)
  {
    internals.CachedActorInfo.ColorOpts.Id = internals.Id;
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkWebGPUActor::NeedToRecreateBindGroups()
{
  auto& internals = (*this->Internals);
  if (!(internals.ActorBindGroup && internals.ActorBindGroupLayout && internals.ActorBuffer))
  {
    return true;
  }
  if (auto* texture = this->GetTexture())
  {
    if (texture->GetMTime() > internals.DeviceResourcesBuildTimestamp)
    {
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::CreateBindGroups(vtkWebGPUConfiguration* wgpuConfiguration)
{
  auto& internals = (*this->Internals);
  const auto& device = wgpuConfiguration->GetDevice();

  const auto actorDescription = this->GetObjectDescription();
  const auto bufferLabel = "ActorBlock-" + actorDescription;
  const auto bufferSize = vtkWebGPUConfiguration::Align(vtkWebGPUActor::GetCacheSizeBytes(), 32);
  internals.ActorBuffer = wgpuConfiguration->CreateBuffer(bufferSize,
    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, false, bufferLabel.c_str());

  std::uint32_t bindingIdBGL = 0;
  std::vector<wgpu::BindGroupLayoutEntry> bglEntries;
  // ActorBlock
  bglEntries.emplace_back(vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
    bindingIdBGL++, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
    wgpu::BufferBindingType::ReadOnlyStorage });
  // Actor texture
  if (auto* wgpuTexture = vtkWebGPUTexture::SafeDownCast(this->GetTexture()))
  {
    if (auto devRc = wgpuTexture->GetDeviceResource())
    {
      bglEntries.emplace_back(
        devRc->MakeSamplerBindGroupLayoutEntry(bindingIdBGL++, wgpu::ShaderStage::Fragment));
      bglEntries.emplace_back(
        devRc->MakeTextureViewBindGroupLayoutEntry(bindingIdBGL++, wgpu::ShaderStage::Fragment));
    }
  }

  internals.ActorBindGroupLayout =
    vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device, bglEntries, actorDescription);
  std::uint32_t bindingIdBG = 0;
  std::vector<wgpu::BindGroupEntry> bgEntries;
  // ActorBlock
  auto actorBindingInit = vtkWebGPUBindGroupInternals::BindingInitializationHelper{ bindingIdBG++,
    internals.ActorBuffer, 0, bufferSize };
  bgEntries.emplace_back(actorBindingInit.GetAsBinding());
  // Actor texture
  if (auto* wgpuTexture = vtkWebGPUTexture::SafeDownCast(this->GetTexture()))
  {
    if (auto devRc = wgpuTexture->GetDeviceResource())
    {
      bgEntries.emplace_back(devRc->MakeSamplerBindGroupEntry(bindingIdBG++));
      bgEntries.emplace_back(devRc->MakeTextureViewBindGroupEntry(bindingIdBG++));
    }
  }

  internals.ActorBindGroup = vtkWebGPUBindGroupInternals::MakeBindGroup(
    device, internals.ActorBindGroupLayout, bgEntries, actorDescription);
  internals.DeviceResourcesBuildTimestamp.Modified();
  // Reset timestamps because the previous buffer is now gone and contents of the buffer will need
  // to be re-uploaded.
  internals.ModelTransformsLastUpdated = vtkTimeStamp();
  internals.ShadingOptionsLastUpdated = vtkTimeStamp();
  internals.RenderOptionsLastUpdated = vtkTimeStamp();
}
VTK_ABI_NAMESPACE_END
