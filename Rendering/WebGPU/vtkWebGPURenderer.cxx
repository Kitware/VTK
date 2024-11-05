// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPURenderer.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUBufferInternals.h"
#include "Private/vtkWebGPUComputePassInternals.h"
#include "vtkAbstractMapper.h"
#include "vtkFrameBufferObjectBase.h"
#include "vtkHardwareSelector.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkType.h"
#include "vtkWebGPUActor.h"
#include "vtkWebGPUCamera.h"
#include "vtkWebGPUClearDrawPass.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUComputeRenderBuffer.h"
#include "vtkWebGPUConfiguration.h"
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
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
  const auto size = vtkWebGPUCamera::GetCacheSizeBytes();
  const auto data =
    reinterpret_cast<vtkWebGPUCamera*>(this->ActiveCamera)->GetCachedSceneTransforms();
  wgpuConfiguration->WriteBuffer(this->SceneTransformBuffer, offset, data, size, "SceneTransforms");
  wroteBytes += size;
  return wroteBytes;
}

//------------------------------------------------------------------------------
std::size_t vtkWebGPURenderer::WriteLightsBuffer(std::size_t offset /*=0*/)
{
  std::size_t wroteBytes = 0;
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();

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
  wgpuConfiguration->WriteBuffer(
    this->SceneLightsBuffer, offset, stage.data(), wroteBytes, "LightInformation");
  return wroteBytes;
}

//------------------------------------------------------------------------------
std::size_t vtkWebGPURenderer::WriteActorBlocksBuffer(std::size_t offset /*=0*/)
{
  std::size_t wroteBytes = 0;
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();

  const auto size = vtkWebGPUConfiguration::Align(vtkWebGPUActor::GetCacheSizeBytes(), 256);
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
  wgpuConfiguration->WriteBuffer(
    this->ActorBlocksBuffer, offset, stage.data(), wroteBytes, "ActorInformation");
  return wroteBytes;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::CreateBuffers()
{
  const auto transformSize = vtkWebGPUCamera::GetCacheSizeBytes();
  const auto transformSizePadded = vtkWebGPUConfiguration::Align(transformSize, 32);

  const auto lightSize = sizeof(vtkTypeUInt32) // light count
    + this->LightIDs.size() * vtkWebGPULight::GetCacheSizeBytes();
  const auto lightSizePadded = vtkWebGPUConfiguration::Align(lightSize, 32);

  // use padded for actor because dynamic offsets are used.
  const auto actorBlkSize = vtkMath::Max<std::size_t>(this->Props->GetNumberOfItems() *
      vtkWebGPUConfiguration::Align(vtkWebGPUActor::GetCacheSizeBytes(), 256),
    256);

  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
  bool createSceneBindGroup = false;
  bool createActorBindGroup = false;

  if (this->SceneTransformBuffer == nullptr)
  {
    const std::string label = "SceneTransforms-" + this->GetObjectDescription();
    this->SceneTransformBuffer = wgpuConfiguration->CreateBuffer(transformSizePadded,
      wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, false, label.c_str());
    createSceneBindGroup = true;
  }

  if (this->SceneLightsBuffer == nullptr)
  {
    const std::string label = "LightInformation-" + this->GetObjectDescription();
    this->SceneLightsBuffer = wgpuConfiguration->CreateBuffer(lightSizePadded,
      wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, false, label.c_str());
    createSceneBindGroup = true;
  }

  if (actorBlkSize != this->LastActorBufferSize)
  {
    if (this->ActorBlocksBuffer != nullptr)
    {
      this->ActorBlocksBuffer.Destroy();
      this->ActorBlocksBuffer = nullptr;
    }
  }
  if (this->ActorBlocksBuffer == nullptr)
  {
    const std::string label = "ActorInformation-" + this->GetObjectDescription();
    this->ActorBlocksBuffer = wgpuConfiguration->CreateBuffer(
      actorBlkSize, wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, false, label.c_str());
    this->LastActorBufferSize = actorBlkSize;
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
    const auto size = vtkWebGPUConfiguration::Align(vtkWebGPUActor::GetCacheSizeBytes(), 256);
    int i = 0;
    for (this->Props->InitTraversal(piter); (aProp = this->Props->GetNextProp(piter)); i++)
    {
      vtkWGPUPropItem item;
      item.Bundle = nullptr;
      item.DynamicOffset = i * size;
      this->PropWGPUItems.emplace(aProp, item);
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::DeviceRender()
{
  vtkDebugMacro(<< __func__);

  // Rendering preparation (camera update, light update, ...) may already have been done by an
  // occlusion culling compute pass (or something else) when pre-rendering some props to fill the z
  // buffer
  if (this->RenderStage == RenderStageEnum::AwaitingPreparation)
  {
    this->UpdateBuffers();
  }

  this->ConfigureComputePipelines();
  this->PreRenderComputePipelines();

  this->BeginRecording(); // all pipelines execute in single render pass, for now.
  this->ActiveCamera->UpdateViewport(this);
  this->UpdateGeometry();
  this->EndRecording();

  this->DoClearPass = true;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::UpdateBuffers()
{
  this->RenderStage = RenderStageEnum::UpdatingBuffers;
  this->SetupBindGroupLayouts();
  this->UpdateCamera(); // brings the camera's transform matrices up-to-date.
  this->UpdateLightGeometry();
  this->UpdateLights();
  this->UpdateGeometry(); // mappers prepare geometry SSBO and pipeline layout.

  this->CreateBuffers();
  this->WriteSceneTransformsBuffer();
  this->WriteLightsBuffer();
  this->WriteActorBlocksBuffer();
}

//------------------------------------------------------------------------------
int vtkWebGPURenderer::UpdateGeometry(vtkFrameBufferObjectBase* /*fbo=nullptr*/)
{
  int i;

  if (this->DoClearPass)
  {
    this->PropsRendered.clear();
    this->NumberOfPropsRendered = 0;
  }

  if (this->PropArrayCount == 0)
  {
    return 0;
  }

  if (this->Selector)
  {
    // When selector is present, we are performing a selection,
    // so do the selection rendering pass instead of the normal passes.
    // Delegate the rendering of the props to the selector itself.

    // use pickfromprops ?
    if (this->PickFromProps)
    {
      vtkProp** pa;
      vtkProp* aProp;
      if (this->PickFromProps->GetNumberOfItems() > 0)
      {
        pa = new vtkProp*[this->PickFromProps->GetNumberOfItems()];
        int pac = 0;

        vtkCollectionSimpleIterator pit;
        for (this->PickFromProps->InitTraversal(pit);
             (aProp = this->PickFromProps->GetNextProp(pit));)
        {
          if (aProp->GetVisibility())
          {
            pa[pac++] = aProp;
          }
        }

        this->NumberOfPropsRendered = this->Selector->Render(this, pa, pac);
        delete[] pa;
      }
    }
    else
    {
      this->NumberOfPropsRendered =
        this->Selector->Render(this, this->PropArray, this->PropArrayCount);
    }

    this->RenderTime.Modified();
    vtkDebugMacro("Rendered " << this->NumberOfPropsRendered << " actors");
    return this->NumberOfPropsRendered;
  }

  // We can render everything because if it was
  // not visible it would not have been put in the
  // list in the first place, and if it was allocated
  // no time (culled) it would have been removed from
  // the list

  // Opaque geometry first:
  this->DeviceRenderOpaqueGeometry();

  // do the render library specific stuff about translucent polygonal geometry.
  // As it can be expensive, do a quick check if we can skip this step
  int hasTranslucentPolygonalGeometry = this->UseDepthPeelingForVolumes;
  for (i = 0; !hasTranslucentPolygonalGeometry && i < this->PropArrayCount; i++)
  {
    hasTranslucentPolygonalGeometry = this->PropArray[i]->HasTranslucentPolygonalGeometry();
  }
  if (hasTranslucentPolygonalGeometry)
  {
    this->DeviceRenderTranslucentPolygonalGeometry();
  }

  // loop through props and give them a chance to
  // render themselves as volumetric geometry.
  if (hasTranslucentPolygonalGeometry == 0 || !this->UseDepthPeelingForVolumes)
  {
    for (i = 0; i < this->PropArrayCount; i++)
    {
      this->NumberOfPropsRendered += this->PropArray[i]->RenderVolumetricGeometry(this);
    }
  }

  // loop through props and give them a chance to
  // render themselves as an overlay (or underlay)
  for (i = 0; i < this->PropArrayCount; i++)
  {
    this->NumberOfPropsRendered += this->PropArray[i]->RenderOverlay(this);
  }

  this->RenderTime.Modified();

  vtkDebugMacro(<< "Rendered " << this->NumberOfPropsRendered << " actors");

  return this->NumberOfPropsRendered;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderer::UpdateOpaquePolygonalGeometry()
{
  vtkDebugMacro(<< __func__ << " " << this->RenderStage);
  int result = 0;
  switch (this->RenderStage)
  {
    case RenderStageEnum::UpdatingBuffers:
    {
      for (int i = 0; i < this->PropArrayCount; i++)
      {
        auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(this->PropArray[i]);
        wgpuActor->RenderOpaqueGeometry(this);
        if (wgpuActor->GetBundleInvalidated())
        {
          // mapper's buffers and bind points have changed. bundle is outdated.
          auto& wgpuPropItem = this->PropWGPUItems[this->PropArray[i]];
          wgpuPropItem.Bundle = nullptr;
        }
      }
      result += this->PropArrayCount;
    }
    break;
    case RenderStageEnum::RecordingCommands:
    {
      for (int i = 0; i < this->PropArrayCount; i++)
      {
        if (auto* wgpuActor = vtkWebGPUActor::SafeDownCast(this->PropArray[i]))
        {
          int rendered = 0;
          auto& wgpuPropItem = this->PropWGPUItems[this->PropArray[i]];
          if (wgpuActor->SupportRenderBundles() && this->UseRenderBundles)
          {
            this->BundleCacheStats.TotalRequests++;
            if (wgpuPropItem.Bundle == nullptr)
            {
              const std::string label = this->GetObjectDescription();
              auto wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
              const auto colorFormat = wgpuRenderWindow->GetPreferredSurfaceTextureFormat();
              const int sampleCount =
                wgpuRenderWindow->GetMultiSamples() ? wgpuRenderWindow->GetMultiSamples() : 1;
              wgpu::RenderBundleEncoderDescriptor bundleEncDesc;
              bundleEncDesc.colorFormatCount = 1;
              bundleEncDesc.colorFormats = &colorFormat;
              bundleEncDesc.depthStencilFormat = wgpuRenderWindow->GetDepthStencilFormat();
              bundleEncDesc.sampleCount = sampleCount;
              bundleEncDesc.depthReadOnly = false;
              bundleEncDesc.stencilReadOnly = false;
              bundleEncDesc.label = label.c_str();
              bundleEncDesc.nextInChain = nullptr;
              this->WGPUBundleEncoder = wgpuRenderWindow->NewRenderBundleEncoder(bundleEncDesc);
              this->WGPUBundleEncoder.SetBindGroup(0, this->SceneBindGroup);
              this->WGPUBundleEncoder.SetBindGroup(
                1, this->ActorBindGroup, 1, &wgpuPropItem.DynamicOffset);
              rendered = wgpuActor->RenderOpaqueGeometry(this);
              auto bundle = this->WGPUBundleEncoder.Finish();
              this->WGPUBundleEncoder = nullptr;
              if (rendered > 0)
              {
                wgpuPropItem.Bundle = bundle;
                this->Bundles.emplace_back(wgpuPropItem.Bundle);
              }
              this->BundleCacheStats.Misses++;
            }
            else
            {
              // bundle gets reused for this prop.
              rendered = 1;
              this->Bundles.emplace_back(wgpuPropItem.Bundle);
              this->BundleCacheStats.Hits++;
            }
          }
          else
          {
            this->WGPURenderEncoder.SetBindGroup(
              1, this->ActorBindGroup, 1, &wgpuPropItem.DynamicOffset);
            rendered = wgpuActor->RenderOpaqueGeometry(this);
          }
          if (rendered > 0)
          {
            result += rendered;

            this->NumberOfPropsRendered += rendered;
            this->PropsRendered.insert(wgpuActor);
          }
        }
        else
        {
          vtkWarningMacro(<< "Prop " << this->PropArray[i]->GetObjectDescription()
                          << " is unrecognized in the vtkWebGPURenderer.");
        }
      }
    }
    break;
    default:
      break;
  }
  return result;
}

//------------------------------------------------------------------------------
int vtkWebGPURenderer::UpdateTranslucentPolygonalGeometry()
{
  vtkDebugMacro(<< __func__ << " " << this->RenderStage);
  int result = 0;
  switch (this->RenderStage)
  {
    case RenderStageEnum::UpdatingBuffers:
    {
      for (int i = 0; i < this->PropArrayCount; i++)
      {
        auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(this->PropArray[i]);
        wgpuActor->RenderTranslucentPolygonalGeometry(this);
        if (wgpuActor->GetBundleInvalidated())
        {
          // mapper's buffers and bind points have changed. bundle is outdated.
          auto& wgpuPropItem = this->PropWGPUItems[this->PropArray[i]];
          wgpuPropItem.Bundle = nullptr;
        }
      }
      result += this->PropArrayCount;
    }
    break;
    case RenderStageEnum::RecordingCommands:
    {
      for (int i = 0; i < this->PropArrayCount; i++)
      {
        if (auto* wgpuActor = vtkWebGPUActor::SafeDownCast(this->PropArray[i]))
        {
          int rendered = 0;
          auto& wgpuPropItem = this->PropWGPUItems[this->PropArray[i]];
          if (wgpuActor->SupportRenderBundles() && this->UseRenderBundles)
          {
            this->BundleCacheStats.TotalRequests++;
            if (wgpuPropItem.Bundle == nullptr)
            {
              const std::string label = this->GetObjectDescription();
              auto wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
              const auto colorFormat = wgpuRenderWindow->GetPreferredSurfaceTextureFormat();
              const int sampleCount =
                wgpuRenderWindow->GetMultiSamples() ? wgpuRenderWindow->GetMultiSamples() : 1;
              wgpu::RenderBundleEncoderDescriptor bundleEncDesc;
              bundleEncDesc.colorFormatCount = 1;
              bundleEncDesc.colorFormats = &colorFormat;
              bundleEncDesc.depthStencilFormat = wgpuRenderWindow->GetDepthStencilFormat();
              bundleEncDesc.sampleCount = sampleCount;
              bundleEncDesc.depthReadOnly = false;
              bundleEncDesc.stencilReadOnly = false;
              bundleEncDesc.label = label.c_str();
              bundleEncDesc.nextInChain = nullptr;
              this->WGPUBundleEncoder = wgpuRenderWindow->NewRenderBundleEncoder(bundleEncDesc);
              this->WGPUBundleEncoder.SetBindGroup(0, this->SceneBindGroup);
              this->WGPUBundleEncoder.SetBindGroup(
                1, this->ActorBindGroup, 1, &wgpuPropItem.DynamicOffset);
              rendered = wgpuActor->RenderTranslucentPolygonalGeometry(this);
              auto bundle = this->WGPUBundleEncoder.Finish();
              this->WGPUBundleEncoder = nullptr;
              if (rendered > 0)
              {
                wgpuPropItem.Bundle = bundle;
                this->Bundles.emplace_back(wgpuPropItem.Bundle);
              }
              this->BundleCacheStats.Misses++;
            }
            else
            {
              // bundle gets reused for this prop.
              rendered = 1;
              this->Bundles.emplace_back(wgpuPropItem.Bundle);
              this->BundleCacheStats.Hits++;
            }
          }
          else
          {
            this->WGPURenderEncoder.SetBindGroup(
              1, this->ActorBindGroup, 1, &wgpuPropItem.DynamicOffset);
            rendered = wgpuActor->RenderTranslucentPolygonalGeometry(this);
          }
          if (rendered > 0)
          {
            result += rendered;

            this->NumberOfPropsRendered += rendered;
            this->PropsRendered.insert(wgpuActor);
          }
        }
        else
        {
          vtkWarningMacro(<< "Prop " << this->PropArray[i]->GetObjectDescription()
                          << " is unrecognized in the vtkWebGPURenderer.");
        }
      }
    }
    break;
    default:
      break;
  }
  return result;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::ConfigureComputePipelines()
{
  vtkWebGPURenderWindow* webGPURenderWindow =
    vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());

  if (webGPURenderWindow == nullptr)
  {
    return;
  }

  for (const auto& computePipeline : this->NotSetupPreRenderComputePipelines)
  {
    this->ConfigureComputeRenderBuffers(computePipeline);
    this->SetupPreRenderComputePipelines.push_back(computePipeline);
  }

  for (const auto& computePipeline : this->NotSetupPostRenderComputePipelines)
  {
    this->ConfigureComputeRenderBuffers(computePipeline);
    this->SetupPostRenderComputePipelines.push_back(computePipeline);
  }

  // All the pipelines have been setup, we can clear the lists
  this->NotSetupPreRenderComputePipelines.clear();
  this->NotSetupPostRenderComputePipelines.clear();
}

//------------------------------------------------------------------------------
const std::vector<vtkSmartPointer<vtkWebGPUComputePipeline>>&
vtkWebGPURenderer::GetSetupPreRenderComputePipelines()
{
  return this->SetupPreRenderComputePipelines;
}

//------------------------------------------------------------------------------
const std::vector<vtkSmartPointer<vtkWebGPUComputePipeline>>&
vtkWebGPURenderer::GetSetupPostRenderComputePipelines()
{
  return this->SetupPostRenderComputePipelines;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::ConfigureComputeRenderBuffers(
  vtkSmartPointer<vtkWebGPUComputePipeline> computePipeline)
{
  vtkActorCollection* actors = this->GetActors();
  actors->InitTraversal();
  while (auto* actor = actors->GetNextItem())
  {
    vtkWebGPUActor* wgpuActor = vtkWebGPUActor::SafeDownCast(actor);
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
    // We're using an iterator here because we want to erase ComputeRenderBuffers from the
    // "NotSetup" list as we iterate through that same "NotSetupComputeRenderBuffers" list. Using an
    // iterator allows us to remove from a list we're iterating on thanks to the .erase() method
    // that returns an updated iterator on the next element of the vector after deletion
    for (auto it = wgpuMapper->NotSetupComputeRenderBuffers.begin();
         it != wgpuMapper->NotSetupComputeRenderBuffers.end();)
    {
      vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer = *it;
      vtkWeakPointer<vtkWebGPUComputePass> associatedPass = nullptr;

      for (const vtkSmartPointer<vtkWebGPUComputePass>& computePass :
        computePipeline->GetComputePasses())
      {
        const vtkSmartPointer<vtkWebGPUComputePass>& associatedComputePass =
          renderBuffer->GetAssociatedComputePass();
        if (computePass == associatedComputePass)
        {
          associatedPass = computePass;

          break;
        }
      }

      if (associatedPass == nullptr)
      {
        // The compute pass that uses the render buffer wasn't found. The render buffer must be used
        // in another compute pipeline
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

        renderBuffer->SetWebGPUBuffer(wgpuMapper->GetPointDataWGPUBuffer());

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

        renderBuffer->SetWebGPUBuffer(wgpuMapper->GetCellDataWGPUBuffer());

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

      associatedPass->Internals->SetupRenderBuffer(renderBuffer);
    }
  }
}

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
void vtkWebGPURenderer::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);
  this->Bundles.clear();
  this->PropWGPUItems.clear();
  this->WGPURenderEncoder = nullptr;
  this->SceneTransformBuffer = nullptr;
  this->SceneLightsBuffer = nullptr;
  this->ActorBlocksBuffer = nullptr;
  this->LastActorBufferSize = 0;
  this->SceneBindGroup = nullptr;
  this->SceneBindGroupLayout = nullptr;
  this->ActorBindGroup = nullptr;
  this->ActorBindGroupLayout = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::PreRenderComputePipelines()
{
  // Executing the compute pipelines before the rendering so that the
  // render can take the compute pipelines results into account
  for (vtkWebGPUComputePipeline* pipeline : this->SetupPreRenderComputePipelines)
  {
    pipeline->DispatchAllPasses();
    pipeline->Update();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::PostRenderComputePipelines()
{
  // Executing the compute pipelines before the rendering so that the
  // render can take the compute pipelines results into account
  for (const auto& pipeline : this->SetupPostRenderComputePipelines)
  {
    pipeline->DispatchAllPasses();
    pipeline->Update();
  }
}

//------------------------------------------------------------------------------
wgpu::CommandBuffer vtkWebGPURenderer::EncodePropListRenderCommand(
  vtkProp** propList, int listLength)
{
  this->UpdateBuffers();

  // Because all the command encoding / rendering function use the props of the this->PropArray
  // list, we're going to replace the list so that only the props we're interested in are rendered.
  // We need to backup the original list though to restore it afterwards
  vtkProp** propArrayBackup = this->PropArray;
  int propCountBackup = this->PropArrayCount;

  this->PropArray = propList;
  this->PropArrayCount = listLength;

  this->BeginRecording();
  this->UpdateGeometry();
  this->EndRecording();

  // Restoring
  this->PropArray = propArrayBackup;
  this->PropArrayCount = propCountBackup;

  vtkWebGPURenderWindow* renderWindow =
    vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  wgpu::CommandEncoder commandEncoder = renderWindow->GetCommandEncoder();
  wgpu::CommandBuffer commandBuffer = commandEncoder.Finish();

  // The command encoder of the render window has finished so we need to recreate a new one so that
  // it's ready to be used again by someone else
  renderWindow->CreateCommandEncoder();

  this->DoClearPass = false;
  return commandBuffer;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::BeginRecording()
{
  vtkDebugMacro(<< __func__);
  this->RenderStage = RenderStageEnum::RecordingCommands;
  vtkRenderState state(this);
  state.SetPropArrayAndCount(this->PropArray, this->PropArrayCount);
  state.SetFrameBuffer(nullptr);
  this->Pass = vtkWebGPUClearDrawPass::New();

  vtkWebGPUClearDrawPass::SafeDownCast(this->Pass)->SetDoClear(this->DoClearPass);

  this->WGPURenderEncoder = vtkWebGPURenderPass::SafeDownCast(this->Pass)->Begin(&state);
#ifndef NDEBUG
  this->WGPURenderEncoder.PushDebugGroup("Renderer start encoding");
#endif
  this->WGPURenderEncoder.SetBindGroup(0, this->SceneBindGroup);

  this->Bundles.clear();
  this->BundleCacheStats.TotalRequests = 0;
  this->BundleCacheStats.Hits = 0;
  this->BundleCacheStats.Misses = 0;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::SetupBindGroupLayouts()
{
  auto wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  wgpu::Device device = wgpuRenderWindow->GetDevice();
  if (this->SceneBindGroupLayout.Get() == nullptr)
  {
    this->SceneBindGroupLayout = vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
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
    this->ActorBindGroupLayout = vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
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
  auto wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  wgpu::Device device = wgpuRenderWindow->GetDevice();

  this->SceneBindGroup =
    vtkWebGPUBindGroupInternals::MakeBindGroup(device, this->SceneBindGroupLayout,
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
  auto wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  wgpu::Device device = wgpuRenderWindow->GetDevice();
  this->ActorBindGroup =
    vtkWebGPUBindGroupInternals::MakeBindGroup(device, this->ActorBindGroupLayout,
      {
        // clang-format off
        { 0, this->ActorBlocksBuffer, 0, vtkWebGPUConfiguration::Align(vtkWebGPUActor::GetCacheSizeBytes(), 256) },
        // clang-format on
      });
  this->ActorBindGroup.SetLabel("ActorBindGroup");
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::EndRecording()
{
  vtkDebugMacro(<< __func__);
  this->RenderStage = RenderStageEnum::Finished;
  if (this->UseRenderBundles)
  {
    if (!this->Bundles.empty() && this->BundleCacheStats.TotalRequests > 0)
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
  }
#ifndef NDEBUG
  this->WGPURenderEncoder.PopDebugGroup();
#endif
  this->WGPURenderEncoder.End();
  this->WGPURenderEncoder = nullptr;
  this->Pass->Delete();
  this->Pass = nullptr;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::PostRasterizationRender()
{
  this->RenderStage = RenderStageEnum::RenderPostRasterization;
  for (vtkActor* postRasterActor : this->PostRasterizationActors)
  {
    vtkWebGPUActor* wgpuActor = vtkWebGPUActor::SafeDownCast(postRasterActor);
    if (wgpuActor == nullptr)
    {
      vtkWarningWithObjectMacro(
        this, "This vtkWebGPURenderer was trying to render a nullptr actor.");

      continue;
    }

    postRasterActor->GetMapper()->Render(this, postRasterActor);
  }

  this->PostRasterizationActors.clear();
  this->RenderStage = RenderStageEnum::AwaitingPreparation; // for next frame.
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::AddPostRasterizationActor(vtkActor* actor)
{
  this->PostRasterizationActors.push_back(actor);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::AddPreRenderComputePipeline(
  vtkSmartPointer<vtkWebGPUComputePipeline> pipeline)
{
  this->NotSetupPreRenderComputePipelines.push_back(pipeline);

  this->InitComputePipeline(pipeline);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::AddPostRenderComputePipeline(
  vtkSmartPointer<vtkWebGPUComputePipeline> pipeline)
{
  this->NotSetupPostRenderComputePipelines.push_back(pipeline);

  this->InitComputePipeline(pipeline);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::InitComputePipeline(vtkSmartPointer<vtkWebGPUComputePipeline> pipeline)
{
  vtkWebGPURenderWindow* wgpuRenderWindow =
    vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  vtkWebGPUConfiguration* renderWindowConfiguration = wgpuRenderWindow->GetWGPUConfiguration();

  if (renderWindowConfiguration == nullptr)
  {
    vtkLog(ERROR,
      "Trying to add a compute pipeline to a vtkWebGPURenderer whose vtkWebGPURenderWindow wasn't "
      "initialized (or the renderer wasn't added to the render window.)");

    return;
  }

  pipeline->SetWGPUConfiguration(renderWindowConfiguration);
}

VTK_ABI_NAMESPACE_END
