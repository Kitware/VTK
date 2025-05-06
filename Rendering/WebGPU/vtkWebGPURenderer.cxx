// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPURenderer.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUComputePassInternals.h"
#include "Private/vtkWebGPURenderPassDescriptorInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"
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
#include "vtkWebGPUActor.h"
#include "vtkWebGPUCamera.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUComputeRenderBuffer.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPULight.h"
#include "vtkWebGPUPolyDataMapper.h"
#include "vtkWebGPURenderWindow.h"

#include <cstring>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
const char* backgroundShaderSource = R"(
    struct VertexOutput {
      @builtin(position) position: vec4<f32>,
    }

    @vertex
    fn vertexMain(@builtin(vertex_index) vertex_id: u32) -> VertexOutput {
      var output: VertexOutput;
      var coords: array<vec2<f32>, 4> = array<vec2<f32>, 4>(
        vec2<f32>(-1, -1), // bottom-left
        vec2<f32>(-1,  1), // top-left
        vec2<f32>( 1, -1), // bottom-right
        vec2<f32>( 1,  1)  // top-right
      );
      output.position = vec4<f32>(coords[vertex_id].xy, 1.0, 1.0);
      return output;
    }

    struct FragmentInput {
      @builtin(position) position: vec4<f32>
    };
    struct FragmentOutput {
      @location(0) color: vec4<f32>,
      @location(1) ids: vec4<u32>,
    };

    @fragment
    fn fragmentMain() -> FragmentOutput {
      var output: FragmentOutput;
      output.color = vec4<f32>(1, 1, 1, 1);
      output.ids = vec4<u32>(0u);
      return output;
    }
  )";
}

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
void vtkWebGPURenderer::CreateBuffers()
{
  const auto transformSize = vtkWebGPUCamera::GetCacheSizeBytes();
  const auto transformSizePadded = vtkWebGPUConfiguration::Align(transformSize, 32);

  const auto lightSize = sizeof(vtkTypeUInt32) // light count
    + this->LightIDs.size() * vtkWebGPULight::GetCacheSizeBytes();
  const auto lightSizePadded = vtkWebGPUConfiguration::Align(lightSize, 32);

  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
  bool createSceneBindGroup = false;

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

  if (createSceneBindGroup)
  {
    this->SetupSceneBindGroup();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::Clear()
{
  if (!this->DrawBackgroundInClearPass)
  {
    return;
  }

  // Draw a quad as big as viewport and colored by the background color.
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->RenderWindow);
  auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();
  vtkWebGPURenderPipelineDescriptorInternals bkgPipelineDescriptor;
  bkgPipelineDescriptor.vertex.entryPoint = "vertexMain";
  bkgPipelineDescriptor.vertex.bufferCount = 0;
  bkgPipelineDescriptor.cFragment.entryPoint = "fragmentMain";
  bkgPipelineDescriptor.cTargets[0].format = wgpuRenderWindow->GetPreferredSurfaceTextureFormat();

  auto depthState =
    bkgPipelineDescriptor.EnableDepthStencil(wgpuRenderWindow->GetDepthStencilFormat());
  depthState->depthWriteEnabled = !this->PreserveDepthBuffer;
  depthState->depthCompare = wgpu::CompareFunction::Always;

  bkgPipelineDescriptor.primitive.frontFace = wgpu::FrontFace::CCW;
  bkgPipelineDescriptor.primitive.cullMode = wgpu::CullMode::Front;
  bkgPipelineDescriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;

  for (int i = 0; i < vtkWebGPURenderPipelineDescriptorInternals::kMaxColorAttachments; ++i)
  {
    auto* blendState = bkgPipelineDescriptor.EnableBlending(i);
    if (this->Transparent())
    {
      blendState->color.srcFactor = wgpu::BlendFactor::Zero;
      blendState->color.dstFactor = wgpu::BlendFactor::One;
      blendState->alpha.srcFactor = wgpu::BlendFactor::Zero;
      blendState->alpha.dstFactor = wgpu::BlendFactor::One;
    }
    else
    {
      blendState->color.srcFactor = wgpu::BlendFactor::Constant;
      blendState->color.dstFactor = wgpu::BlendFactor::Zero;
      blendState->alpha.srcFactor = wgpu::BlendFactor::Constant;
      blendState->alpha.dstFactor = wgpu::BlendFactor::Zero;
    }
  }
  // Prepare selection ids output.
  bkgPipelineDescriptor.cTargets[1].format =
    wgpuRenderWindow->GetPreferredSelectorIdsTextureFormat();
  bkgPipelineDescriptor.cFragment.targetCount++;
  bkgPipelineDescriptor.DisableBlending(1);
  const auto pipelineKey =
    wgpuPipelineCache->GetPipelineKey(&bkgPipelineDescriptor, backgroundShaderSource);
  wgpuPipelineCache->CreateRenderPipeline(&bkgPipelineDescriptor, this, backgroundShaderSource);
  auto pipeline = wgpuPipelineCache->GetRenderPipeline(pipelineKey);

  this->WGPURenderEncoder.SetPipeline(pipeline);
  wgpu::Color bkgColor = { this->Background[0], this->Background[1], this->Background[2],
    this->BackgroundAlpha };
  this->WGPURenderEncoder.SetBlendConstant(&bkgColor);
  this->WGPURenderEncoder.Draw(4);
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

  this->RecordRenderCommands();

  this->DrawBackgroundInClearPass = true;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::RecordRenderCommands()
{
  vtkRenderState state(this);
  state.SetPropArrayAndCount(this->PropArray, this->PropArrayCount);
  state.SetFrameBuffer(nullptr);

  if (auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->RenderWindow))
  {
    vtkWebGPURenderPassDescriptorInternals renderPassDescriptor(
      { wgpuRenderWindow->GetOffscreenColorAttachmentView(),
        wgpuRenderWindow->GetHardwareSelectorAttachmentView() },
      wgpuRenderWindow->GetDepthStencilView(),
      /*clearColor=*/false, /*clearDepth=*/false, /*clearStencil=*/false);
    renderPassDescriptor.label = "vtkWebGPURenderer::RecordRenderCommands";
    this->WGPURenderEncoder = wgpuRenderWindow->NewRenderPass(renderPassDescriptor);
    this->BeginRecording();
    // 1. Draw the background color/texture.
    // updates viewport and scissor rectangles on the render pass encoder.
    this->ActiveCamera->UpdateViewport(this);
    // clear the viewport rectangle to background color.
    if (this->RenderWindow->GetErase() && this->Erase)
    {
      this->Clear();
    }
    // 2. Now render all opaque and translucent props.
    this->UpdateGeometry();
    this->EndRecording();
  }
  else
  {
    vtkErrorMacro(
      << "Cannot record render commands because RenderWindow is not a vtkWebGPURenderWindow!");
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::UpdateBuffers()
{
  this->RenderStage = RenderStageEnum::UpdatingBuffers;
  this->SetupBindGroupLayouts();
  this->UpdateCamera(); // brings the camera's transform matrices up-to-date.
  this->UpdateLightGeometry();
  this->UpdateLights();

  // Render bundle is rebuilt if any mapper needs to re-record render commands.
  if (this->UseRenderBundles)
  {
    if (this->Bundle != nullptr)
    {
      this->RebuildRenderBundle = false;
    }
    else
    {
      this->RebuildRenderBundle = true;
    }
  }
  this->UpdateGeometry(); // mappers prepare geometry SSBO and pipeline layout.

  this->CreateBuffers();
  this->WriteSceneTransformsBuffer();
  this->WriteLightsBuffer();
}

//------------------------------------------------------------------------------
int vtkWebGPURenderer::UpdateGeometry(vtkFrameBufferObjectBase* /*fbo=nullptr*/)
{
  int i;

  if (this->DrawBackgroundInClearPass)
  {
    this->PropsRendered.clear();
    this->NumberOfPropsRendered = 0;
  }

  if (this->PropArrayCount == 0)
  {
    return 0;
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
        if (auto* wgpuActor = vtkWebGPUActor::SafeDownCast(this->PropArray[i]))
        {
          wgpuActor->SetId(i);
        }
        this->PropArray[i]->RenderOpaqueGeometry(this);
      }
      result += this->PropArrayCount;
    }
    break;
    case RenderStageEnum::RecordingCommands:
    {
      for (int i = 0; i < this->PropArrayCount; i++)
      {
        const int rendered = this->PropArray[i]->RenderOpaqueGeometry(this);
        if (rendered > 0)
        {
          result += rendered;
          this->NumberOfPropsRendered += rendered;
          this->PropsRendered.insert(this->PropArray[i]);
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
        if (auto* wgpuActor = vtkWebGPUActor::SafeDownCast(this->PropArray[i]))
        {
          wgpuActor->SetId(i);
        }
        this->PropArray[i]->RenderTranslucentPolygonalGeometry(this);
      }
      result += this->PropArrayCount;
    }
    break;
    case RenderStageEnum::RecordingCommands:
    {
      for (int i = 0; i < this->PropArrayCount; i++)
      {
        const int rendered = this->PropArray[i]->RenderTranslucentPolygonalGeometry(this);
        if (rendered > 0)
        {
          result += rendered;
          this->NumberOfPropsRendered += rendered;
          this->PropsRendered.insert(this->PropArray[i]);
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
  this->Bundle = nullptr;
  this->WGPUBundleEncoder = nullptr;
  this->WGPURenderEncoder = nullptr;
  this->SceneTransformBuffer = nullptr;
  this->SceneLightsBuffer = nullptr;
  this->SceneBindGroup = nullptr;
  this->SceneBindGroupLayout = nullptr;
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

  this->RecordRenderCommands();

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

  this->DrawBackgroundInClearPass = false;
  return commandBuffer;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::BeginRecording()
{
  vtkDebugMacro(<< __func__);
  this->RenderStage = RenderStageEnum::RecordingCommands;
  assert(this->WGPURenderEncoder != nullptr);

#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
  this->WGPURenderEncoder.PushDebugGroup("Renderer start encoding");
#endif
  this->WGPURenderEncoder.SetBindGroup(0, this->SceneBindGroup);
  if (this->RebuildRenderBundle)
  {
    // destroy previous bundle.
    this->Bundle = nullptr;
    // create a new bundle encoder.
    const std::string label = this->GetObjectDescription();
    auto wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
    const std::vector<wgpu::TextureFormat> colorFormats = {
      wgpuRenderWindow->GetPreferredSurfaceTextureFormat(),
      wgpuRenderWindow->GetPreferredSelectorIdsTextureFormat()
    };
    const int sampleCount =
      wgpuRenderWindow->GetMultiSamples() ? wgpuRenderWindow->GetMultiSamples() : 1;
    wgpu::RenderBundleEncoderDescriptor bundleEncDesc;
    bundleEncDesc.colorFormatCount = colorFormats.size();
    bundleEncDesc.colorFormats = colorFormats.data();
    bundleEncDesc.depthStencilFormat = wgpuRenderWindow->GetDepthStencilFormat();
    bundleEncDesc.sampleCount = sampleCount;
    bundleEncDesc.depthReadOnly = false;
    bundleEncDesc.stencilReadOnly = false;
    bundleEncDesc.label = label.c_str();
    bundleEncDesc.nextInChain = nullptr;
    this->WGPUBundleEncoder = wgpuRenderWindow->NewRenderBundleEncoder(bundleEncDesc);
    this->WGPUBundleEncoder.SetBindGroup(0, this->SceneBindGroup);
  }
  else
  {
    this->WGPUBundleEncoder = nullptr;
  }
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
      { 0, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform },
      // SceneLights
      { 1, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage }
        // clang-format on
      });

    this->SceneBindGroupLayout.SetLabel("SceneBindGroupLayout");
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
void vtkWebGPURenderer::EndRecording()
{
  vtkDebugMacro(<< __func__);
  this->RenderStage = RenderStageEnum::Finished;
  if (this->UseRenderBundles)
  {
    if (this->WGPUBundleEncoder)
    {
      this->Bundle = this->WGPUBundleEncoder.Finish();
    }
    if (this->Bundle != nullptr)
    {
      this->WGPURenderEncoder.ExecuteBundles(1, &this->Bundle);
    }
  }
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
  this->WGPURenderEncoder.PopDebugGroup();
#endif
  this->WGPURenderEncoder.End();
  this->WGPURenderEncoder = nullptr;
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
