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
#include "vtkWebGPUActor.h"
#include "vtkWebGPUCamera.h"
#include "vtkWebGPUClearPass.h"
#include "vtkWebGPUInternalsBindGroup.h"
#include "vtkWebGPUInternalsBindGroupLayout.h"
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
void vtkWebGPURenderer::DeviceRender()
{
  vtkDebugMacro(<< __func__);
  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  this->UpdateCamera(); // sets up scene transform buffer.
  this->UpdateLightGeometry();
  this->UpdateLights(); // peeks at the collection of lights in the renderer.

  auto wgpuCamera = vtkWebGPUCamera::SafeDownCast(this->ActiveCamera);
  assert(wgpuCamera != nullptr);

  this->SetupBindGroups(); // provides shader modules with scene transform and lights.
  this->UpdateGeometry();  // mappers prepare geometry SSBO and pipeline layout.
  this->BeginEncoding();   // all pipelines execute in single render pass, for now.
  this->WGPURenderEncoder.SetBindGroup(0, this->SceneBindGroup);
  wgpuCamera->UpdateViewport(this, this->WGPURenderEncoder);
  // encode draw commands for each unique pipeline.
  for (this->CurrentPipelineID = 0; this->CurrentPipelineID < this->RenderPipelineBatches.size();
       ++this->CurrentPipelineID)
  {
    const auto& batch = this->RenderPipelineBatches[this->CurrentPipelineID];
    this->WGPURenderEncoder.SetPipeline(batch.Pipeline);
    this->RenderGeometry();
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
  return this->UpdateOpaquePolygonalGeometry();
}

//------------------------------------------------------------------------------
int vtkWebGPURenderer::UpdateOpaquePolygonalGeometry()
{
  int result = 0;
  for (int i = 0; i < this->PropArrayCount; i++)
  {
    auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(this->PropArray[i]);
    result += wgpuActor->Update(this, wgpuActor->GetMapper());
  }
  this->NumberOfPropsUpdated += result;
  return result;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::DeviceRenderOpaqueGeometry(vtkFrameBufferObjectBase* vtkNotUsed(fbo))
{
  int result = 0;
  for (int i = 0; i < this->PropArrayCount; i++)
  {
    auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(this->PropArray[i]);
    wgpuActor->Render(this, wgpuActor->GetMapper());
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
  std::vector<int> lightIDs;

  vtkCollectionSimpleIterator sit;
  int lightID = 0;
  for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit)); ++lightID)
  {
    vtkTypeBool on = light->GetSwitch();
    if (on)
    {
      ltime = vtkMath::Max(ltime, light->GetMTime());
      lightIDs.emplace_back(lightID);
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
      lightsUsed = 1;
      lightIDs.emplace_back(0);
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
  auto wgpuRenWin = reinterpret_cast<vtkWebGPURenderWindow*>(this->GetRenderWindow());
  wgpu::Device device = wgpuRenWin->GetDevice();

  std::vector<vtkWebGPULight::LightInfo> lightInfos;
  switch (this->LightingComplexity)
  {
    case NoLighting:
      break;
    case Headlight:
    {
      vtkWebGPULight* wgpuLight = reinterpret_cast<vtkWebGPULight*>(lc->GetItemAsObject(0));
      assert(wgpuLight != nullptr);
      assert(this->ActiveCamera != nullptr);
      lightInfos.emplace_back(wgpuLight->GetLightInfo(this, this->ActiveCamera));
      break;
    }
    case Directional:
    case Positional:
      for (const auto& lightID : lightIDs)
      {
        vtkWebGPULight* wgpuLight = reinterpret_cast<vtkWebGPULight*>(lc->GetItemAsObject(lightID));
        assert(wgpuLight != nullptr);
        assert(this->ActiveCamera != nullptr);
        lightInfos.emplace_back(wgpuLight->GetLightInfo(this, this->ActiveCamera));
      }
      break;
  }

  vtkTypeUInt32 count = lightInfos.size();
  wgpu::BufferDescriptor desc;
  desc.size = count + lightInfos.size() * sizeof(lightInfos[0]);
  desc.size = (desc.size + 31) & (-32);
  desc.label = "LightsInfoBuffer";
  desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
  desc.nextInChain = nullptr;
  desc.mappedAtCreation = false;
  this->LightsInfoBuffer = device.CreateBuffer(&desc);
  // upload number of lights.
  device.GetQueue().WriteBuffer(this->LightsInfoBuffer, 0, &count, sizeof(count));
  // upload lights.
  device.GetQueue().WriteBuffer(
    this->LightsInfoBuffer, 16, lightInfos.data(), count * sizeof(lightInfos[0]));
  this->LightingUploadTimestamp.Modified();

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
void vtkWebGPURenderer::ReleaseGraphicsResources(vtkWindow* w) {}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::BeginEncoding()
{
  vtkDebugMacro(<< __func__);
  vtkRenderState state(this);
  state.SetPropArrayAndCount(this->PropArray, this->PropArrayCount);
  state.SetFrameBuffer(nullptr);
  this->Pass = vtkWebGPUClearPass::New();
  this->WGPURenderEncoder = vtkWebGPURenderPass::SafeDownCast(this->Pass)->Begin(&state);
  this->WGPURenderEncoder.PushDebugGroup("Renderer start encoding");
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::SetupBindGroups()
{
  if (this->SceneBindGroup.Get() != nullptr)
  {
    // already setup.
    return;
  }

  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  wgpu::Device device = wgpuRenWin->GetDevice();
  // scene transform includes a view and projection matrix.
  auto sceneTransformBuffer =
    reinterpret_cast<vtkWebGPUCamera*>(this->ActiveCamera)->GetSceneTransformBuffer();
  assert(sceneTransformBuffer != nullptr);

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

  this->SceneBindGroup =
    vtkWebGPUInternalsBindGroup::MakeBindGroup(device, this->SceneBindGroupLayout,
      {
        // clang-format off
      // SceneTransforms
      { 0, sceneTransformBuffer, 0 },
      // SceneLights
      { 1, this->LightsInfoBuffer, 0 }
        // clang-format on
      });
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::EndEncoding()
{
  vtkDebugMacro(<< __func__);
  this->WGPURenderEncoder.PopDebugGroup();
  this->WGPURenderEncoder.End();
  this->WGPURenderEncoder.Release();
  this->Pass->Delete();
  this->Pass = nullptr;
}

//------------------------------------------------------------------------------
bool vtkWebGPURenderer::HasRenderPipeline(
  vtkAbstractMapper* mapper, const std::string& additionalInfo /*=""*/)
{
  const std::string key = mapper->GetClassName() + additionalInfo;
  return this->MapperPipelineTable.count(key) > 0;
}

//------------------------------------------------------------------------------
std::size_t vtkWebGPURenderer::InsertRenderPipeline(vtkAbstractMapper* mapper, vtkProp* prop,
  const wgpu::RenderPipelineDescriptor& pipelineDescriptor,
  const std::string& additionalInfo /*=""*/)
{
  const std::string key = mapper->GetClassName() + additionalInfo;
  auto iter = this->MapperPipelineTable.find(key);
  if (iter != this->MapperPipelineTable.end())
  {
    // reuse cached pipeline instance.
    const auto& pipelineID = iter->second;
    const auto& batch = this->RenderPipelineBatches[pipelineID];
    // add prop to pipeline's collection of props.
    batch.Props->AddItem(prop);
    vtkDebugMacro(<< "Using cached pipeline (" << pipelineID << ") for key - " << key
                  << ", batch_size=" << batch.Props->GetNumberOfItems());
    return pipelineID;
  }
  else
  {
    auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
    const wgpu::Device device = wgpuRenWin->GetDevice();
    // create a new render pipeline.
    RenderPipelineBatch batch;
    batch.Pipeline = device.CreateRenderPipeline(&pipelineDescriptor);
    batch.Props = vtk::TakeSmartPointer(vtkPropCollection::New());
    // add prop to pipeline's collection of props.
    batch.Props->AddItem(prop);
    this->RenderPipelineBatches.emplace_back(batch);
    // keep track of pipelineID in the mapper pipeline table.
    const auto& pipelineID = this->RenderPipelineBatches.size() - 1;
    this->MapperPipelineTable.emplace(key, pipelineID);
    vtkDebugMacro(<< "Inserted new render pipeline batch (" << this->RenderPipelineBatches.size()
                  << ") for key - " << key << ", batch_size=" << batch.Props->GetNumberOfItems());
    return pipelineID;
  }
}

VTK_ABI_NAMESPACE_END
