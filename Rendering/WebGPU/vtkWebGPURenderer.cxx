// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkWebGPUPolyDataMapper.h"
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
void vtkWebGPURenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

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
  stage.resize(this->Props->GetNumberOfItems() * size);
  vtkProp* aProp = nullptr;
  vtkCollectionSimpleIterator piter;
  for (this->Props->InitTraversal(piter); (aProp = this->Props->GetNextProp(piter));)
  {
    vtkWebGPUActor* wgpuActor = reinterpret_cast<vtkWebGPUActor*>(aProp);
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
  const auto actorBlkSize = vtkMath::Max<std::size_t>(this->Props->GetNumberOfItems() *
      vtkWGPUContext::Align(vtkWebGPUActor::GetCacheSizeBytes(), 256),
    256);

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
    // delete outdated info.
    this->PropWGPUItems.clear();
    this->SetupActorBindGroup();
    // build new cache for bundles and dynamic offsets.
    vtkProp* aProp = nullptr;
    vtkCollectionSimpleIterator piter;
    const auto size = vtkWGPUContext::Align(vtkWebGPUActor::GetCacheSizeBytes(), 256);
    int i = 0;
    for (this->Props->InitTraversal(piter); (aProp = this->Props->GetNextProp(piter)); i++)
    {
      vtkWGPUPropItem item;
      item.Bundle = nullptr;
      item.DynamicOffsets = vtk::TakeSmartPointer(vtkTypeUInt32Array::New());
      item.DynamicOffsets->InsertNextValue(i * size);
      this->PropWGPUItems.emplace(aProp, item);
    }
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

  this->SetupBindGroupLayouts();
  this->UpdateCamera(); // brings the camera's transform matrices up-to-date.
  this->UpdateLightGeometry();
  this->UpdateLights();
  this->UpdateGeometry(); // mappers prepare geometry SSBO and pipeline layout.

  this->CreateBuffers();
  this->UpdateBufferData();

  this->UpdateComputePipelines();
  this->ComputePass();

  this->BeginEncoding(); // all pipelines execute in single render pass, for now.
  this->ActiveCamera->UpdateViewport(this);

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
    this->RenderGeometry();
    if (!this->Bundles.empty())
    {
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
    this->Bundles.clear();
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
int vtkWebGPURenderer::UpdateGeometry(vtkFrameBufferObjectBase* vtkNotUsed(fbo) /*=nullptr*/)
{
  this->NumberOfPropsUpdated = 0;
  if (this->PropArrayCount == 0)
  {
    return 0;
  }
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
    if (wgpuActor->Update(this, wgpuActor->GetMapper()))
    {
      // mapper's buffers and bind points have changed. bundle is outdated.
      auto& wgpuPropItem = this->PropWGPUItems[this->PropArray[i]];
      wgpuPropItem.Bundle = nullptr;
    }
  }
  this->NumberOfPropsUpdated += result;
  return result;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::UpdateComputePipelines()
{
  vtkWebGPURenderWindow* webGPURenderWindow;
  webGPURenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  if (webGPURenderWindow == nullptr)
  {
    return;
  }

  for (vtkSmartPointer<vtkWebGPUComputePipeline> computePipeline : this->NotSetupComputePipelines)
  {
    computePipeline->SetAdapter(webGPURenderWindow->GetAdapter());
    computePipeline->SetDevice(webGPURenderWindow->GetDevice());

    this->UpdateComputeBuffers(computePipeline);
    this->SetupComputePipelines.push_back(computePipeline);
  }

  this->NotSetupComputePipelines.clear();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::UpdateComputeBuffers(vtkSmartPointer<vtkWebGPUComputePipeline> pipeline)
{
  for (int i = 0; i < this->PropArrayCount; i++)
  {
    vtkWebGPUActor* wgpuActor = reinterpret_cast<vtkWebGPUActor*>(this->PropArray[i]);
    if (wgpuActor == nullptr)
    {
      continue;
    }

    vtkWebGPUPolyDataMapper* wgpuMapper =
      vtkWebGPUPolyDataMapper::SafeDownCast(wgpuActor->GetMapper());
    if (wgpuMapper == nullptr)
    {
      continue;
    }

    std::vector<vtkSmartPointer<vtkWebGPUComputeRenderBuffer>> renderBufferToRemove;
    for (auto it = wgpuMapper->NotSetupComputeRenderBuffers.begin();
         it != wgpuMapper->NotSetupComputeRenderBuffers.end();)
    {
      vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer;
      vtkWeakPointer<vtkWebGPUComputePipeline> associatedPipeline;

      renderBuffer = *it;
      associatedPipeline = renderBuffer->GetAssociatedPipeline();
      if (associatedPipeline.Get() != pipeline.Get())
      {
        // Not the right compute pipeline
        it++;
        continue;
      }

      renderBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_COMPUTE_STORAGE);

      bool erased = false;
      if (renderBuffer->GetPointBufferAttribute() !=
        vtkWebGPUPolyDataMapper::PointDataAttributes::POINT_UNDEFINED)
      {
        // Point data attribute

        vtkWebGPUPolyDataMapper::PointDataAttributes bufferAttribute =
          renderBuffer->GetPointBufferAttribute();

        renderBuffer->SetByteSize(wgpuMapper->GetPointAttributeByteSize(bufferAttribute));
        renderBuffer->SetRenderBufferOffset(
          wgpuMapper->GetPointAttributeByteOffset(bufferAttribute) / sizeof(float));
        renderBuffer->SetRenderBufferElementCount(
          wgpuMapper->GetPointAttributeByteSize(bufferAttribute) /
          wgpuMapper->GetPointAttributeElementSize(bufferAttribute));

        renderBuffer->SetWGPUBuffer(wgpuMapper->GetPointDataWGPUBuffer());

        it = wgpuMapper->NotSetupComputeRenderBuffers.erase(it);
        erased = true;
      }
      else if (renderBuffer->GetCellBufferAttribute() != vtkWebGPUPolyDataMapper::CELL_UNDEFINED)
      {
        // Cell data attribute

        vtkWebGPUPolyDataMapper::CellDataAttributes bufferAttribute =
          renderBuffer->GetCellBufferAttribute();

        renderBuffer->SetByteSize(wgpuMapper->GetCellAttributeByteSize(bufferAttribute));
        renderBuffer->SetRenderBufferOffset(
          wgpuMapper->GetCellAttributeByteOffset(bufferAttribute) / sizeof(float));
        renderBuffer->SetRenderBufferElementCount(
          wgpuMapper->GetCellAttributeByteSize(bufferAttribute) /
          wgpuMapper->GetCellAttributeElementSize(bufferAttribute));

        renderBuffer->SetWGPUBuffer(wgpuMapper->GetCellDataWGPUBuffer());

        // Erasing the element. erase() returns the iterator on the next element after removal
        it = wgpuMapper->NotSetupComputeRenderBuffers.erase(it);
        erased = true;
      }
      else
      {
        vtkLog(ERROR,
          "Could not determine the attribute represented by the render buffer with label "
            << renderBuffer->GetLabel());
      }

      if (!erased)
      {
        // We only want to ++ the iterator if we didn't erase an element. If we erased an element,
        // we already got the next iterator with the value returned by erase()
        it++;
      }

      associatedPipeline->SetupRenderBuffer(renderBuffer);
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::DeviceRenderOpaqueGeometry(vtkFrameBufferObjectBase* vtkNotUsed(fbo))
{
  int result = 0;

  for (int i = 0; i < this->PropArrayCount; i++)
  {
    auto& wgpuPropItem = this->PropWGPUItems[this->PropArray[i]];
    auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(this->PropArray[i]);
    if (this->UseRenderBundles)
    {
      this->BundleCacheStats.TotalRequests++;
      if (wgpuPropItem.Bundle == nullptr)
      {
        wgpuActor->SetDynamicOffsets(wgpuPropItem.DynamicOffsets);
        wgpuPropItem.Bundle = wgpuActor->RenderToBundle(this, wgpuActor->GetMapper());
        this->Bundles.emplace_back(wgpuPropItem.Bundle);
        this->BundleCacheStats.Misses++;
      }
      else
      {
        // bundle gets reused for this prop.
        this->Bundles.emplace_back(wgpuPropItem.Bundle);
        this->BundleCacheStats.Hits++;
      }
    }
    else
    {
      wgpuActor->SetDynamicOffsets(wgpuPropItem.DynamicOffsets);
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
  std::size_t lightsUsed = 0;

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
void vtkWebGPURenderer::SetEnvironmentTexture(vtkTexture*, bool vtkNotUsed(isSRGB) /*=false*/) {}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::ReleaseGraphicsResources(vtkWindow* vtkNotUsed(w))
{
  this->PropWGPUItems.clear();
  this->Bundles.clear();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::ComputePass()
{
  // Executing the compute pipelines before the rendering so that the
  // render can take the compute pipelines results into account
  for (vtkSmartPointer<vtkWebGPUComputePipeline> pipeline : this->SetupComputePipelines)
  {
    pipeline->Dispatch();
  }
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
  this->WGPURenderEncoder = nullptr;
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

//------------------------------------------------------------------------------
void vtkWebGPURenderer::AddComputePipeline(vtkSmartPointer<vtkWebGPUComputePipeline> pipeline)
{
  this->NotSetupComputePipelines.push_back(pipeline);
}

VTK_ABI_NAMESPACE_END
