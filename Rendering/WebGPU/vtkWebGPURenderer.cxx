/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPURenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWebGPURenderer.h"
#include "vtkAbstractMapper.h"
#include "vtkHardwareSelector.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkType.h"
#include "vtkTypeUInt32Array.h"
#include "vtkWGPUContext.h"
#include "vtkWebGPUActor.h"
#include "vtkWebGPUCamera.h"
#include "vtkWebGPUClearPass.h"
#include "vtkWebGPUInternalsBindGroup.h"
#include "vtkWebGPUInternalsBindGroupLayout.h"
#include "vtkWebGPUInternalsBuffer.h"
#include "vtkWebGPULight.h"
#include "vtkWebGPURenderWindow.h"

#include <cstring>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPURenderer);

//------------------------------------------------------------------------------
vtkWebGPURenderer::vtkWebGPURenderer() = default;

//------------------------------------------------------------------------------
vtkWebGPURenderer::~vtkWebGPURenderer() = default;

//------------------------------------------------------------------------------
void vtkWebGPURenderer::PrintSelf(ostream& os, vtkIndent indent) {}

//------------------------------------------------------------------------------
std::size_t vtkWebGPURenderer::WriteSceneTransformsBuffer(std::size_t offset /*=0*/)
{
  std::size_t wroteBytes = 0;
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  const wgpu::Device& device = wgpuRenWin->GetDevice();
  const wgpu::Queue& queue = device.GetQueue();
  const auto size = vtkWebGPUCamera::GetCacheSizeBytes();
  const auto data =
    reinterpret_cast<vtkWebGPUCamera*>(this->ActiveCamera)->GetCachedSceneTransforms();
  queue.WriteBuffer(this->SceneTransformBuffer, offset, data, size);
  wroteBytes += size;
  return wroteBytes;
}

//------------------------------------------------------------------------------
std::size_t vtkWebGPURenderer::WriteLightsBuffer(std::size_t offset /*=0*/)
{
  std::size_t wroteBytes = 0;
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  const wgpu::Device& device = wgpuRenWin->GetDevice();
  const wgpu::Queue& queue = device.GetQueue();

  const vtkTypeUInt32 count = this->LightIDs.size();
  const auto size = vtkWebGPULight::GetCacheSizeBytes();
  std::vector<uint8_t> stage;
  stage.resize(sizeof(vtkTypeUInt32) + count * size);

  // number of lights.
  const uint8_t* countu8 = reinterpret_cast<const uint8_t*>(&count);
  std::memcpy(&stage[wroteBytes], countu8, sizeof(vtkTypeUInt32));
  wroteBytes += sizeof(vtkTypeUInt32);

  // the lights themselves.
  for (const auto& lightID : this->LightIDs)
  {
    vtkWebGPULight* wgpuLight =
      reinterpret_cast<vtkWebGPULight*>(this->Lights->GetItemAsObject(lightID));
    assert(wgpuLight != nullptr);

    const auto data = wgpuLight->GetCachedLightInformation();
    std::memcpy(&stage[wroteBytes], data, size);
    wroteBytes += size;
  }
  queue.WriteBuffer(this->SceneLightsBuffer, offset, stage.data(), wroteBytes);
  return wroteBytes;
}

//------------------------------------------------------------------------------
std::size_t vtkWebGPURenderer::WriteActorBlocksBuffer(std::size_t offset /*=0*/)
{
  std::size_t wroteBytes = 0;
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  const wgpu::Device& device = wgpuRenWin->GetDevice();
  const wgpu::Queue& queue = device.GetQueue();

  const auto size = vtkWGPUContext::Align(vtkWebGPUActor::GetCacheSizeBytes(), 256);
  std::vector<uint8_t> stage;
  stage.resize(this->PropArrayCount * size);
  for (int i = 0; i < this->PropArrayCount; ++i)
  {
    vtkWebGPUActor* wgpuActor = reinterpret_cast<vtkWebGPUActor*>(this->PropArray[i]);
    assert(wgpuActor != nullptr);

    const auto data = wgpuActor->GetCachedActorInformation();
    std::memcpy(&stage[wroteBytes], data, size);
    wroteBytes += size;
  }
  queue.WriteBuffer(this->ActorBlocksBuffer, offset, stage.data(), wroteBytes);
  return wroteBytes;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::CreateBuffers()
{
  const auto transformSize = vtkWebGPUCamera::GetCacheSizeBytes();
  const auto transformSizePadded = vtkWGPUContext::Align(transformSize, 32);

  const auto lightSize = sizeof(vtkTypeUInt32) // light count
    + this->LightIDs.size() * vtkWebGPULight::GetCacheSizeBytes();
  const auto lightSizePadded = vtkWGPUContext::Align(lightSize, 32);

  // use padded for actor because dynamic offsets are used.
  const auto actorBlkSize =
    this->PropArrayCount * vtkWGPUContext::Align(vtkWebGPUActor::GetCacheSizeBytes(), 256);

  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  const wgpu::Device& device = wgpuRenWin->GetDevice();
  bool createSceneBindGroup = false;
  bool createActorBindGroup = false;

  if (this->SceneTransformBuffer.Get() == nullptr ||
    (this->SceneTransformBuffer.GetSize() != transformSizePadded))
  {
    if (this->SceneTransformBuffer.Get() != nullptr)
    {
      this->SceneTransformBuffer.Destroy();
    }
    this->SceneTransformBuffer = vtkWebGPUInternalsBuffer::CreateABuffer(device,
      transformSizePadded, wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, false,
      "Transform uniform buffer for vtkRenderer");
    createSceneBindGroup = true;
  }

  if (this->SceneLightsBuffer.Get() == nullptr ||
    (this->SceneLightsBuffer.GetSize() != lightSizePadded))
  {
    if (this->SceneLightsBuffer.Get() != nullptr)
    {
      this->SceneLightsBuffer.Destroy();
    }
    this->SceneLightsBuffer = vtkWebGPUInternalsBuffer::CreateABuffer(device, lightSizePadded,
      wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, false,
      "Lights uniform buffer for vtkRenderer");
    createSceneBindGroup = true;
  }

  if (this->ActorBlocksBuffer.Get() == nullptr ||
    (this->ActorBlocksBuffer.GetSize() != actorBlkSize))
  {
    if (this->ActorBlocksBuffer.Get() != nullptr)
    {
      this->ActorBlocksBuffer.Destroy();
    }
    this->ActorBlocksBuffer = vtkWebGPUInternalsBuffer::CreateABuffer(device, actorBlkSize,
      wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, false,
      "Uniform buffer for all vtkActors in vtkRenderer");
    createActorBindGroup = true;
  }

  if (createSceneBindGroup)
  {
    this->SetupSceneBindGroup();
  }

  if (createActorBindGroup)
  {
    this->SetupActorBindGroup();
  }
}

//------------------------------------------------------------------------------
std::size_t vtkWebGPURenderer::UpdateBufferData()
{
  std::size_t wroteBytes = this->WriteActorBlocksBuffer();
  wroteBytes = this->WriteLightsBuffer();
  wroteBytes = this->WriteSceneTransformsBuffer();
  return wroteBytes;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::DeviceRender()
{
  vtkDebugMacro(<< __func__);
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());

  this->SetupBindGroupLayouts();
  this->UpdateCamera(); // brings the camera's transform matrices up-to-date.
  this->UpdateLightGeometry();
  this->UpdateLights();
  this->UpdateGeometry(); // mappers prepare geometry SSBO and pipeline layout.

  this->CreateBuffers();
  this->UpdateBufferData();

  this->BeginEncoding(); // all pipelines execute in single render pass, for now.
  this->ActiveCamera->UpdateViewport(this);

  ///@{ FIXME: Leaks memory when we recreate them.
  if (this->PropArrayCount != this->Bundles.size())
  {
    // FIXME: Re-use bundles that don't need to be re-built.
    this->Bundles.clear();
    // All props must re-bundle their commands.
    std::fill(this->ReBundleProps.begin(), this->ReBundleProps.end(), 1);
  }
  ///@}

  if (!this->UseRenderBundles)
  {
    this->WGPURenderEncoder.SetBindGroup(0, this->SceneBindGroup);
    this->RenderGeometry();
  }
  else
  {
    this->BundleCacheStats.TotalRequests = 0;
    this->BundleCacheStats.Hits = 0;
    this->BundleCacheStats.Misses = 0;
    if (this->Bundles.empty())
    {
      this->Bundles.resize(this->ReBundleProps.size());
      this->RenderGeometry();
    }
    else
    {
      this->RenderGeometry();
    }
    vtkDebugMacro(<< "Bundle cache summary:\n"
                  << "Total requests: " << this->BundleCacheStats.TotalRequests << "\n"
                  << "Hit ratio: "
                  << (this->BundleCacheStats.Hits / this->BundleCacheStats.TotalRequests) * 100
                  << "%\n"
                  << "Miss ratio: "
                  << (this->BundleCacheStats.Misses / this->BundleCacheStats.TotalRequests) * 100
                  << "%\n"
                  << "Hit: " << this->BundleCacheStats.Hits << "\n"
                  << "Miss: " << this->BundleCacheStats.Misses << "\n");
    this->WGPURenderEncoder.ExecuteBundles(this->Bundles.size(), this->Bundles.data());
  }
  this->EndEncoding();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::Clear()
{
  vtkDebugMacro(<< __func__);
  vtkNew<vtkWebGPUClearPass> clearPass;
  vtkRenderState state(this);
  clearPass->Render(&state);
}

//------------------------------------------------------------------------------
int vtkWebGPURenderer::RenderGeometry()
{
  this->NumberOfPropsRendered = 0;
  if (this->PropArrayCount == 0)
  {
    return 0;
  }
  this->DeviceRenderOpaqueGeometry(nullptr);
  return this->NumberOfPropsRendered;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderer::UpdateGeometry(vtkFrameBufferObjectBase* fbo /*=nullptr*/)
{
  this->NumberOfPropsUpdated = 0;
  if (this->PropArrayCount == 0)
  {
    return 0;
  }
  this->ReBundleProps.resize(this->PropArrayCount, 0);
  return this->UpdateOpaquePolygonalGeometry();
}

//------------------------------------------------------------------------------
int vtkWebGPURenderer::UpdateOpaquePolygonalGeometry()
{
  int result = 0;
  for (int i = 0; i < this->PropArrayCount; i++)
  {
    auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(this->PropArray[i]);
    wgpuActor->CacheActorRenderOptions();
    wgpuActor->CacheActorShadeOptions();
    wgpuActor->CacheActorTransforms();
    this->ReBundleProps[i] = wgpuActor->Update(this, wgpuActor->GetMapper());
  }
  this->NumberOfPropsUpdated += result;
  return result;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::DeviceRenderOpaqueGeometry(vtkFrameBufferObjectBase* vtkNotUsed(fbo))
{
  int result = 0;
  vtkNew<vtkTypeUInt32Array> offsets;
  // currently supports only bind group with dynamic offsets. (ActorBindGroup)
  offsets->SetNumberOfValues(1);

  std::size_t uboOffset = vtkWGPUContext::Align(vtkWebGPUActor::GetCacheSizeBytes(), 256);

  for (int i = 0; i < this->PropArrayCount; i++)
  {
    if (this->UseRenderBundles)
    {
      this->BundleCacheStats.TotalRequests++;
      if (this->ReBundleProps[i])
      {
        auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(this->PropArray[i]);
        offsets->SetValue(0, i * uboOffset);
        wgpuActor->SetDynamicOffsets(offsets);
        this->Bundles[i] = wgpuActor->RenderToBundle(this, wgpuActor->GetMapper());
        this->BundleCacheStats.Misses++;
      }
      else
      {
        // do nothing. reuse bundle for this prop.
        this->BundleCacheStats.Hits++;
      }
    }
    else
    {
      auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(this->PropArray[i]);
      offsets->SetValue(0, i * uboOffset);
      wgpuActor->SetDynamicOffsets(offsets);
      wgpuActor->Render(this, wgpuActor->GetMapper());
    }
    result += 1;
  }
  this->NumberOfPropsRendered += result;
}

///@{ TODO: Figure out translucent polygonal geometry. Better to do in a separate render pass?
//------------------------------------------------------------------------------
int vtkWebGPURenderer::UpdateTranslucentPolygonalGeometry()
{
  return 0;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::DeviceRenderTranslucentPolygonalGeometry(
  vtkFrameBufferObjectBase* vtkNotUsed(fbo))
{
}
///@}

//------------------------------------------------------------------------------
// Ask lights to load themselves into graphics pipeline.
int vtkWebGPURenderer::UpdateLights()
{
  vtkLightCollection* lc = this->GetLights();
  vtkLight* light;

  int lightingComplexity = LightingComplexityEnum::NoLighting;
  int lightsUsed = 0;

  vtkMTimeType ltime = lc->GetMTime();
  this->LightIDs.clear();

  vtkCollectionSimpleIterator sit;
  int lightID = 0;
  for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit)); ++lightID)
  {
    vtkTypeBool on = light->GetSwitch();
    if (on)
    {
      ltime = vtkMath::Max(ltime, light->GetMTime());
      this->LightIDs.emplace_back(lightID);
      light->Render(this, 0);
      lightsUsed++;
      if (lightingComplexity == LightingComplexityEnum::NoLighting)
      {
        lightingComplexity = LightingComplexityEnum::Headlight;
      }
    }

    if (lightingComplexity == LightingComplexityEnum::Headlight &&
      (lightsUsed > 1 || light->GetLightType() != VTK_LIGHT_TYPE_HEADLIGHT))
    {
      lightingComplexity = LightingComplexityEnum::Directional;
    }
    if (lightingComplexity < LightingComplexityEnum::Positional && (light->GetPositional()))
    {
      lightingComplexity = LightingComplexityEnum::Positional;
    }
  }

  if (this->GetUseImageBasedLighting() && this->GetEnvironmentTexture() && lightingComplexity == 0)
  {
    lightingComplexity = LightingComplexityEnum::Headlight;
  }

  // create alight if needed
  if (!lightsUsed)
  {
    if (this->AutomaticLightCreation)
    {
      vtkDebugMacro(<< "No lights are on, creating one.");
      this->CreateLight();
      lc->InitTraversal(sit);
      light = lc->GetNextLight(sit);
      ltime = lc->GetMTime();
      light->Render(this, 0);
      lightsUsed = 1;
      this->LightIDs.emplace_back(0);
      lightingComplexity = light->GetLightType() == VTK_LIGHT_TYPE_HEADLIGHT
        ? LightingComplexityEnum::Headlight
        : LightingComplexityEnum::Directional;
      ltime = vtkMath::Max(ltime, light->GetMTime());
    }
  }

  if (lightingComplexity != this->LightingComplexity || lightsUsed != this->NumberOfLightsUsed)
  {
    this->LightingComplexity = lightingComplexity;
    this->NumberOfLightsUsed = lightsUsed;
    this->LightingUpdateTime = ltime;
  }

  // for lighting complexity 2,3 camera has an impact
  vtkCamera* cam = this->GetActiveCamera();
  if (this->LightingComplexity > 1)
  {
    ltime = vtkMath::Max(ltime, cam->GetMTime());
  }

  if (ltime <= this->LightingUploadTimestamp.GetMTime())
  {
    return this->NumberOfLightsUsed;
  }

  this->LightingUpdateTime = ltime;
  return this->NumberOfLightsUsed;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::SetUserLightTransform(vtkTransform* transform)
{
  this->UserLightTransform = transform;
}

//------------------------------------------------------------------------------
vtkTransform* vtkWebGPURenderer::GetUserLightTransform()
{
  return this->UserLightTransform;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::SetEnvironmentTexture(vtkTexture* texture, bool isSRGB /*=false*/) {}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Bundles.clear();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::BeginEncoding()
{
  vtkDebugMacro(<< __func__);
  vtkRenderState state(this);
  state.SetPropArrayAndCount(this->PropArray, this->PropArrayCount);
  state.SetFrameBuffer(nullptr);
  this->Pass = vtkWebGPUClearPass::New();
  this->WGPURenderEncoder = vtkWebGPURenderPass::SafeDownCast(this->Pass)->Begin(&state);
#ifndef NDEBUG
  this->WGPURenderEncoder.PushDebugGroup("Renderer start encoding");
#endif
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::SetupBindGroupLayouts()
{
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  wgpu::Device device = wgpuRenWin->GetDevice();
  if (this->SceneBindGroupLayout.Get() == nullptr)
  {
    this->SceneBindGroupLayout = vtkWebGPUInternalsBindGroupLayout::MakeBindGroupLayout(device,
      {
        // clang-format off
      // SceneTransforms
      { 0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::Uniform },
      // SceneLights
      { 1, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage }
        // clang-format on
      });

    this->SceneBindGroupLayout.SetLabel("SceneBindGroupLayout");
  }

  if (this->ActorBindGroupLayout.Get() == nullptr)
  {
    this->ActorBindGroupLayout = vtkWebGPUInternalsBindGroupLayout::MakeBindGroupLayout(device,
      {
        // clang-format off
      // ActorBlocks
      { 0, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform, /*hasDynamicOffsets=*/true },
        // clang-format on
      });

    this->ActorBindGroupLayout.SetLabel("ActorBindGroupLayout");
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::SetupSceneBindGroup()
{
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  wgpu::Device device = wgpuRenWin->GetDevice();

  this->SceneBindGroup =
    vtkWebGPUInternalsBindGroup::MakeBindGroup(device, this->SceneBindGroupLayout,
      {
        // clang-format off
        { 0, this->SceneTransformBuffer },
        { 1, this->SceneLightsBuffer }
        // clang-format on
      });
  this->SceneBindGroup.SetLabel("SceneBindGroup");
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::SetupActorBindGroup()
{
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  wgpu::Device device = wgpuRenWin->GetDevice();
  this->ActorBindGroup =
    vtkWebGPUInternalsBindGroup::MakeBindGroup(device, this->ActorBindGroupLayout,
      {
        // clang-format off
        { 0, this->ActorBlocksBuffer, 0, vtkWGPUContext::Align(vtkWebGPUActor::GetCacheSizeBytes(), 256) },
        // clang-format on
      });
  this->ActorBindGroup.SetLabel("ActorBindGroup");
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::EndEncoding()
{
  vtkDebugMacro(<< __func__);
#ifndef NDEBUG
  this->WGPURenderEncoder.PopDebugGroup();
#endif
  this->WGPURenderEncoder.End();
  this->WGPURenderEncoder.Release();
  this->Pass->Delete();
  this->Pass = nullptr;
}

//------------------------------------------------------------------------------
wgpu::ShaderModule vtkWebGPURenderer::HasShaderCache(const std::string& source)
{
  return this->ShaderCache.count(source) > 0 ? this->ShaderCache.at(source) : nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::InsertShader(const std::string& source, wgpu::ShaderModule shader)
{
  this->ShaderCache.emplace(source, shader);
}

VTK_ABI_NAMESPACE_END
