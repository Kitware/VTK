// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPURenderer.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUComputePassInternals.h"
#include "Private/vtkWebGPUHandle.h"
#include "Private/vtkWebGPUHelpersPrivate.h"
#include "Private/vtkWebGPURenderPassDescriptorInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"
#include "vtkAbstractMapper.h"
#include "vtkActor2D.h"
#include "vtkCuller.h"
#include "vtkCullerCollection.h"
#include "vtkFrameBufferObjectBase.h"
#include "vtkHardwareSelector.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
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
#include "vtkWebGPUPolyDataMapper2D.h"
#include "vtkWebGPURenderWindow.h"
#include <algorithm>
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

// Gradient background
// Unlike the flat background above, this shader writes the color directly
// instead of relying on the pipeline blend constant.
const char* gradientBackgroundShaderSource = R"(
    struct GradientOptions {
      stopColor0: vec4<f32>,
      stopColor1: vec4<f32>,
      // Maps the quad's local 0..1 coordinates onto the renderer's full extent.
      // These differ from (1,1,0,0) only when the window is rendered in tiles,
      // where the encoder viewport covers just the visible slice of the renderer.
      tcoordScaleOffset: vec4<f32>,
      // 0 = vertical, 1 = horizontal,
      // 2 = radial farthest side, 3 = radial farthest corner.
      mode: u32,
      dither: u32,
    }
    @group(0) @binding(0) var<uniform> gradient: GradientOptions;

    struct VertexOutput {
      @builtin(position) position: vec4<f32>,
      @location(0) tcoord: vec2<f32>,
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
      // Background is the first stop at t=0, so t must increase upwards.
      output.tcoord = coords[vertex_id].xy * 0.5 + 0.5;
      return output;
    }

    struct FragmentOutput {
      @location(0) color: vec4<f32>,
      @location(1) ids: vec4<u32>,
    };

    // Keep the dither granularity identical to the OpenGL backend.
    const DITHERING_GRANULARITY: f32 = 0.001960784313725;

    fn generateRandom(st: vec2<f32>) -> f32 {
      return fract(sin(dot(st, vec2<f32>(12.9898, 78.233))) * 43758.5453123);
    }

    @fragment
    fn fragmentMain(input: VertexOutput) -> FragmentOutput {
      let tcoord = input.tcoord * gradient.tcoordScaleOffset.xy
                   + gradient.tcoordScaleOffset.zw;

      var value: f32 = 0.0;
      if (gradient.mode == 0u) {
        value = tcoord.y;
      } else if (gradient.mode == 1u) {
        value = tcoord.x;
      } else if (gradient.mode == 2u) {
        value = clamp(length(tcoord - vec2<f32>(0.5, 0.5)) * 2.0, 0.0, 1.0);
      } else {
        value = length(tcoord - vec2<f32>(0.5, 0.5)) * sqrt(2.0);
      }

      var rgb: vec3<f32> = mix(gradient.stopColor0.rgb, gradient.stopColor1.rgb, value);
      if (gradient.dither != 0u) {
        let noise = mix(-DITHERING_GRANULARITY, DITHERING_GRANULARITY,
                        generateRandom(tcoord));
        rgb = rgb + vec3<f32>(noise);
      }

      var output: FragmentOutput;
      output.color = vec4<f32>(rgb, gradient.stopColor0.a);
      output.ids = vec4<u32>(0u);
      return output;
    }
  )";
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPURenderer);

//------------------------------------------------------------------------------
vtkWebGPURenderer::vtkWebGPURenderer()
{
  // vtkRenderer's constructor installs a vtkFrustumCoverageCuller. Remember it so that
  // WarnIfCullingWithRenderBundles() can tell it apart from a culler the application chose.
  vtkCollectionSimpleIterator cit;
  this->Cullers->InitTraversal(cit);
  this->DefaultCuller.Reset(this->Cullers->GetNextCuller(cit));
}

//------------------------------------------------------------------------------
vtkWebGPURenderer::~vtkWebGPURenderer() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPURenderer::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

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
  // WGSL SceneLights layout: count (u32, 4 bytes) + implicit padding (12 bytes) +
  // values array (count * 80 bytes). The 12-byte padding aligns 'values' to 16 bytes
  // after the 4-byte count. We write exactly this layout.
  constexpr std::size_t kLightArrayOffset = 16; // offset of values[] in SceneLights
  std::vector<uint8_t> stage;
  stage.resize(kLightArrayOffset + count * size, 0);

  // number of lights.
  const uint8_t* countu8 = reinterpret_cast<const uint8_t*>(&count);
  std::memcpy(stage.data(), countu8, sizeof(vtkTypeUInt32));
  wroteBytes = kLightArrayOffset; // skip count + 12-byte alignment padding

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
    this->SceneLightsBuffer, offset, stage.data(), stage.size(), "LightInformation");
  return stage.size();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::CreateBuffers()
{
  const auto transformSize = vtkWebGPUCamera::GetCacheSizeBytes();
  const auto transformSizePadded = vtkWebGPUConfiguration::Align(transformSize, 32);

  // Match WriteLightsBuffer: count (4) + padding (12) + N * 80 bytes per light.
  // Ensure we have space for at least 1 light to match shader expectations
  const auto lightCount = std::max(std::size_t(1), this->LightIDs.size());
  const auto lightSize = 16 + lightCount * vtkWebGPULight::GetCacheSizeBytes();
  const auto lightSizePadded = vtkWebGPUConfiguration::Align(lightSize, 32);

  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
  bool createSceneBindGroup = false;

  if (this->SceneTransformBuffer == nullptr)
  {
    const std::string label = "SceneTransforms-" + this->GetObjectDescription();
    this->SceneTransformBuffer = wgpuConfiguration->CreateBuffer(transformSizePadded,
      static_cast<WGPUBufferUsage>(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst), false,
      label.c_str());
    createSceneBindGroup = true;
  }

  bool recreateLightsBuffer = false;
  if (this->SceneLightsBuffer == nullptr)
  {
    recreateLightsBuffer = true;
    createSceneBindGroup = true;
  }
  else if (this->AllocatedLightsBufferSize < lightSizePadded)
  {
    // Buffer exists but is too small for the number of lights, need to recreate it
    vtkDebugMacro(<< "Recreating lights buffer: " << this->AllocatedLightsBufferSize << " < "
                  << lightSizePadded);
    recreateLightsBuffer = true;
    createSceneBindGroup = true;
  }

  if (recreateLightsBuffer)
  {
    const std::string label = "LightInformation-" + this->GetObjectDescription();
    this->SceneLightsBuffer = wgpuConfiguration->CreateBuffer(lightSizePadded,
      static_cast<WGPUBufferUsage>(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst), false,
      label.c_str());
    this->AllocatedLightsBufferSize = lightSizePadded;
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

  if (this->GradientBackground)
  {
    this->ClearGradientBackground();
    return;
  }
  vtkWebGPURenderPipelineDescriptorInternals bkgPipelineDescriptor;
  bkgPipelineDescriptor.vertex.entryPoint = WGPUStringView{ "vertexMain", WGPU_STRLEN };
  bkgPipelineDescriptor.vertex.bufferCount = 0;
  bkgPipelineDescriptor.cFragment.entryPoint = WGPUStringView{ "fragmentMain", WGPU_STRLEN };
  bkgPipelineDescriptor.cTargets[0].format = wgpuRenderWindow->GetPreferredSurfaceTextureFormat();

  auto depthState =
    bkgPipelineDescriptor.EnableDepthStencil(wgpuRenderWindow->GetDepthStencilFormat());
  depthState->depthWriteEnabled =
    this->PreserveDepthBuffer ? WGPUOptionalBool_False : WGPUOptionalBool_True;
  depthState->depthCompare = WGPUCompareFunction_Always;

  bkgPipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
  bkgPipelineDescriptor.primitive.cullMode = WGPUCullMode_Front;
  bkgPipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleStrip;

  for (int i = 0; i < vtkWebGPURenderPipelineDescriptorInternals::kMaxColorAttachments; ++i)
  {
    auto* blendState = bkgPipelineDescriptor.EnableBlending(i);
    if (this->Transparent())
    {
      blendState->color.srcFactor = WGPUBlendFactor_Zero;
      blendState->color.dstFactor = WGPUBlendFactor_One;
      blendState->alpha.srcFactor = WGPUBlendFactor_Zero;
      blendState->alpha.dstFactor = WGPUBlendFactor_One;
    }
    else
    {
      blendState->color.srcFactor = WGPUBlendFactor_Constant;
      blendState->color.dstFactor = WGPUBlendFactor_Zero;
      blendState->alpha.srcFactor = WGPUBlendFactor_Constant;
      blendState->alpha.dstFactor = WGPUBlendFactor_Zero;
    }
  }
  // Prepare selection ids output.
  bkgPipelineDescriptor.cTargets[1].format =
    wgpuRenderWindow->GetPreferredSelectorIdsTextureFormat();
  bkgPipelineDescriptor.cFragment.targetCount++;
  bkgPipelineDescriptor.DisableBlending(1);
  const auto pipelineKey =
    wgpuPipelineCache->GetPipelineKey((&bkgPipelineDescriptor), backgroundShaderSource);
  wgpuPipelineCache->CreateRenderPipeline(
    (&bkgPipelineDescriptor), wgpuRenderWindow, backgroundShaderSource);
  auto pipeline = wgpuPipelineCache->GetRenderPipeline(pipelineKey);

  WGPURenderPassEncoder encoder(this->WGPURenderEncoder);
  wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
  WGPUColor bkgColor = { this->Background[0], this->Background[1], this->Background[2],
    this->BackgroundAlpha };
  wgpuRenderPassEncoderSetBlendConstant(encoder, &bkgColor);
  wgpuRenderPassEncoderDraw(encoder, 4, 1, 0, 0);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::WarnIfCullingWithRenderBundles()
{
  if (this->WarnedAboutCullingWithRenderBundles || !this->UseRenderBundles)
  {
    return;
  }
  vtkCollectionSimpleIterator cit;
  vtkCuller* culler = nullptr;
  for (this->Cullers->InitTraversal(cit); (culler = this->Cullers->GetNextCuller(cit));)
  {
    // The culler installed by vtkRenderer's constructor is not the application's choice, so
    // silently overriding it is what everyone already expects.
    if (culler == this->DefaultCuller.Lock())
    {
      continue;
    }
    this->WarnedAboutCullingWithRenderBundles = true;
    vtkWarningMacro(<< culler->GetClassName()
                    << " has no effect while render bundles are enabled. A render bundle is a "
                       "fixed list of draw commands replayed across frames, so it cannot be driven "
                       "by a prop list that culling resizes every frame, and DeviceRender() "
                       "restores the props that the cullers removed. Call UseRenderBundlesOff() on "
                       "this renderer for culling to take effect.");
    break;
  }
}

//------------------------------------------------------------------------------
vtkProp* vtkWebGPURenderer::GetPropWithId(vtkTypeUInt32 id)
{
  if (id >= this->PropArray.size())
  {
    return nullptr;
  }
  return this->PropArray[id];
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::ClearGradientBackground()
{
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->RenderWindow);
  auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();

  // std140 layout: two vec4 stop colors followed by two u32 scalars.
  struct GradientOptions
  {
    float StopColor0[4];
    float StopColor1[4];
    float TCoordScaleOffset[4];
    std::uint32_t Mode;
    std::uint32_t Dither;
    std::uint32_t Padding[2];
  };
  GradientOptions options = {};
  for (int i = 0; i < 3; ++i)
  {
    options.StopColor0[i] = static_cast<float>(this->Background[i]);
    options.StopColor1[i] = static_cast<float>(this->Background2[i]);
  }
  options.StopColor0[3] = static_cast<float>(this->BackgroundAlpha);
  options.StopColor1[3] = static_cast<float>(this->BackgroundAlpha);
  options.Mode = static_cast<std::uint32_t>(this->GradientMode);
  options.Dither = this->DitherGradient ? 1u : 0u;

  // vtkWebGPUCamera::UpdateViewport() sets the encoder viewport to the part of
  // this renderer visible in the current tile, so the quad only covers that
  // slice. Rescale the texture coordinates so the gradient is still evaluated
  // over the renderer's whole extent and stays continuous across tiles.
  options.TCoordScaleOffset[0] = 1.0f;
  options.TCoordScaleOffset[1] = 1.0f;
  options.TCoordScaleOffset[2] = 0.0f;
  options.TCoordScaleOffset[3] = 0.0f;
  if (auto* window = this->GetVTKWindow())
  {
    const double* viewport = this->GetViewport();
    const double* tile = window->GetTileViewport();
    for (int axis = 0; axis < 2; ++axis)
    {
      const double vMin = viewport[axis];
      const double vMax = viewport[axis + 2];
      const double extent = vMax - vMin;
      if (extent <= 0.0)
      {
        continue;
      }
      const double visibleMin = std::max(vMin, tile[axis]);
      const double visibleMax = std::min(vMax, tile[axis + 2]);
      options.TCoordScaleOffset[axis] = static_cast<float>((visibleMax - visibleMin) / extent);
      options.TCoordScaleOffset[axis + 2] = static_cast<float>((visibleMin - vMin) / extent);
    }
  }

  if (this->BackgroundGradientBuffer == nullptr)
  {
    const std::string label = "BackgroundGradient-" + this->GetObjectDescription();
    this->BackgroundGradientBuffer = wgpuConfiguration->CreateBuffer(sizeof(GradientOptions),
      static_cast<WGPUBufferUsage>(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst), false,
      label.c_str());
  }
  wgpuConfiguration->WriteBuffer(
    this->BackgroundGradientBuffer, 0, &options, sizeof(options), "BackgroundGradient");

  vtkWebGPURenderPipelineDescriptorInternals bkgPipelineDescriptor;
  bkgPipelineDescriptor.vertex.entryPoint = WGPUStringView{ "vertexMain", WGPU_STRLEN };
  bkgPipelineDescriptor.vertex.bufferCount = 0;
  bkgPipelineDescriptor.cFragment.entryPoint = WGPUStringView{ "fragmentMain", WGPU_STRLEN };
  bkgPipelineDescriptor.cTargets[0].format = wgpuRenderWindow->GetPreferredSurfaceTextureFormat();

  auto depthState =
    bkgPipelineDescriptor.EnableDepthStencil(wgpuRenderWindow->GetDepthStencilFormat());
  depthState->depthWriteEnabled =
    this->PreserveDepthBuffer ? WGPUOptionalBool_False : WGPUOptionalBool_True;
  depthState->depthCompare = WGPUCompareFunction_Always;

  bkgPipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
  bkgPipelineDescriptor.primitive.cullMode = WGPUCullMode_Front;
  bkgPipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleStrip;

  // The shader writes the final color, so pass it through untouched rather than
  // going through the blend constant used by the flat background path.
  bkgPipelineDescriptor.DisableBlending(0);

  // Prepare selection ids output.
  bkgPipelineDescriptor.cTargets[1].format =
    wgpuRenderWindow->GetPreferredSelectorIdsTextureFormat();
  bkgPipelineDescriptor.cFragment.targetCount++;
  bkgPipelineDescriptor.DisableBlending(1);

  const auto pipelineKey =
    wgpuPipelineCache->GetPipelineKey((&bkgPipelineDescriptor), gradientBackgroundShaderSource);
  if (wgpuPipelineCache->GetRenderPipeline(pipelineKey) == nullptr)
  {
    wgpuPipelineCache->CreateRenderPipeline(
      (&bkgPipelineDescriptor), wgpuRenderWindow, gradientBackgroundShaderSource);
  }
  auto pipeline = wgpuPipelineCache->GetRenderPipeline(pipelineKey);
  WGPURenderPipeline wgpuPipeline = pipeline;

  // The pipeline uses an automatic layout, so take the bind group layout from it
  // rather than declaring one separately.
  vtkWebGPU::BindGroupLayout autoLayout =
    vtkWebGPU::BindGroupLayout::Acquire(wgpuRenderPipelineGetBindGroupLayout(wgpuPipeline, 0));
  vtkWebGPU::BindGroup bindGroup = vtkWebGPU::BindGroup::Acquire(
    vtkWebGPUBindGroupInternals::MakeBindGroup(wgpuConfiguration->GetDevice(), autoLayout,
      { { 0, this->BackgroundGradientBuffer, 0, sizeof(GradientOptions) } },
      "BackgroundGradientBindGroup"));

  WGPURenderPassEncoder encoder(this->WGPURenderEncoder);
  wgpuRenderPassEncoderSetPipeline(encoder, wgpuPipeline);
  wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindGroup, 0, nullptr);
  wgpuRenderPassEncoderDraw(encoder, 4, 1, 0, 0);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::DeviceRender()
{
  vtkDebugMacro(<< __func__);

  // Rendering preparation (camera update, light update, ...) may already have been done by an
  // occlusion culling compute pass (or something else) when pre-rendering some props to fill the z
  // buffer
  if (this->UseRenderBundles)
  {
    this->WarnIfCullingWithRenderBundles();
    // Undo culling for the duration of this frame.
    this->PropArray.clear();
    vtkCollectionSimpleIterator pit;
    vtkProp* prop = nullptr;
    for (this->Props->InitTraversal(pit); (prop = this->Props->GetNextProp(pit));)
    {
      // AllocateTime() diverts a blurred skybox to BackgroundProp instead of
      // PropArray. Leave it there, or it would be drawn twice.
      if (prop->GetVisibility() && prop != this->BackgroundProp)
      {
        this->PropArray.push_back(prop);
      }
    }
  }

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
  if (auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->RenderWindow))
  {
    vtkWebGPURenderPassDescriptorInternals renderPassDescriptor(
      { wgpuRenderWindow->GetOffscreenColorAttachmentView(),
        wgpuRenderWindow->GetHardwareSelectorAttachmentView() },
      wgpuRenderWindow->GetDepthStencilView(),
      /*clearColor=*/false, /*clearDepth=*/false, /*clearStencil=*/false);
    renderPassDescriptor.label =
      WGPUStringView{ "vtkWebGPURenderer::RecordRenderCommands", WGPU_STRLEN };
    vtkWebGPU::ReleaseAndNull(this->WGPURenderEncoder, wgpuRenderPassEncoderRelease);
    this->WGPURenderEncoder =
      wgpuRenderWindow->NewRenderPass(static_cast<WGPURenderPassDescriptor&>(renderPassDescriptor));
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
bool vtkWebGPURenderer::VisiblePropSetChanged()
{
  this->VisibleProps.clear();
  vtkCollectionSimpleIterator pit;
  vtkProp* prop = nullptr;
  for (this->Props->InitTraversal(pit); (prop = this->Props->GetNextProp(pit));)
  {
    if (prop->GetVisibility())
    {
      this->VisibleProps.push_back(prop);
    }
  }
  if (this->VisibleProps == this->LastVisibleProps)
  {
    return false;
  }
  this->LastVisibleProps.swap(this->VisibleProps);
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPURenderer::PropSupportsReusingRenderBundle(vtkProp* prop) const
{
  if (!this->CanReuseRenderBundle())
  {
    return false;
  }
  if (auto* webgpuActor = vtkWebGPUActor::SafeDownCast(prop))
  {
    return webgpuActor->SupportRenderBundles();
  }
  else if (auto* actor2D = vtkActor2D::SafeDownCast(prop))
  {
    if (auto* webgpuMapper2D = vtkWebGPUPolyDataMapper2D::SafeDownCast(actor2D->GetMapper()))
    {
      (void)webgpuMapper2D;
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::UpdateBuffers()
{
  this->RenderStage = RenderStageEnum::SyncDeviceResources;
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
    // Invalidate the bundle when the set of visible props has changed.
    // This handles actor visibility toggles, additions, and removals.
    //
    // This deliberately looks at every prop the renderer owns rather than at
    // PropArray. PropArray is the *culled* list: vtkRenderer installs a
    // vtkFrustumCoverageCuller by default, so AllocateTime() resizes it every
    // frame according to what the camera can currently see. Comparing that
    // against the previous frame's rendered set means any camera motion at all
    // reports a changed prop set and re-records the whole bundle, which is
    // exactly the work render bundles exist to avoid. The visible prop set,
    // by contrast, only changes when the application actually changes it.
    if (this->VisiblePropSetChanged())
    {
      this->InvalidateBundle();
    }
    // Invalidate the bundle if the number of lights has changed
    if (this->LightIDs.size() != this->PreviousLightCount)
    {
      this->InvalidateBundle();
      this->PreviousLightCount = this->LightIDs.size();
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
  if (this->DrawBackgroundInClearPass)
  {
    this->PropsRendered.clear();
    this->NumberOfPropsRendered = 0;
  }

  // Render background prop (skybox) before all other geometry.
  // vtkRenderer stores skybox actors as BackgroundProp rather than in PropArray.
  if (this->BackgroundProp)
  {
    this->BackgroundProp->RenderOpaqueGeometry(this);
  }

  if (this->PropArray.size() == 0)
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
  for (std::size_t i = 0; !hasTranslucentPolygonalGeometry && i < this->PropArray.size(); i++)
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
    for (std::size_t i = 0; i < this->PropArray.size(); i++)
    {
      this->NumberOfPropsRendered += this->PropArray[i]->RenderVolumetricGeometry(this);
    }
  }

  // loop through props and give them a chance to
  // render themselves as an overlay (or underlay)
  for (std::size_t i = 0; i < this->PropArray.size(); i++)
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
    case RenderStageEnum::SyncDeviceResources:
    {
      for (std::size_t i = 0; i < this->PropArray.size(); i++)
      {
        if (auto* wgpuActor = vtkWebGPUActor::SafeDownCast(this->PropArray[i]))
        {
          wgpuActor->SetId(i);
        }
        this->PropArray[i]->RenderOpaqueGeometry(this);
      }
      result += this->PropArray.size();
    }
    break;
    case RenderStageEnum::RecordingCommands:
    {
      for (std::size_t i = 0; i < this->PropArray.size(); i++)
      {
        int rendered = 0;
        if (this->PropSupportsReusingRenderBundle(this->PropArray[i]))
        {
          rendered = 1;
        }
        else
        {
          rendered = this->PropArray[i]->RenderOpaqueGeometry(this);
        }
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
    case RenderStageEnum::SyncDeviceResources:
    {
      for (std::size_t i = 0; i < this->PropArray.size(); i++)
      {
        if (auto* wgpuActor = vtkWebGPUActor::SafeDownCast(this->PropArray[i]))
        {
          wgpuActor->SetId(static_cast<vtkTypeUInt32>(i));
        }
        this->PropArray[i]->RenderTranslucentPolygonalGeometry(this);
      }
      result += this->PropArray.size();
    }
    break;
    case RenderStageEnum::RecordingCommands:
    {
      for (std::size_t i = 0; i < this->PropArray.size(); i++)
      {
        int rendered = 0;
        if (this->PropSupportsReusingRenderBundle(this->PropArray[i]))
        {
          rendered = 1;
        }
        else
        {
          rendered = this->PropArray[i]->RenderTranslucentPolygonalGeometry(this);
        }
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
        renderBuffer->SetRenderBufferOffset(0);
        renderBuffer->SetRenderBufferElementCount(
          wgpuMapper->GetPointAttributeByteSize(bufferAttribute) /
          wgpuMapper->GetPointAttributeElementSize(bufferAttribute));

        renderBuffer->SetWebGPUBuffer(wgpuMapper->GetPointDataWGPUBuffer(bufferAttribute));

        it = wgpuMapper->NotSetupComputeRenderBuffers.erase(it);
        erased = true;
      }
      else if (renderBuffer->GetCellBufferAttribute() != vtkWebGPUPolyDataMapper::CELL_UNDEFINED)
      {
        // Cell data attribute

        vtkWebGPUPolyDataMapper::CellDataAttributes bufferAttribute =
          renderBuffer->GetCellBufferAttribute();

        renderBuffer->SetByteSize(wgpuMapper->GetCellAttributeByteSize(bufferAttribute));
        renderBuffer->SetRenderBufferOffset(0);
        renderBuffer->SetRenderBufferElementCount(
          wgpuMapper->GetCellAttributeByteSize(bufferAttribute) /
          wgpuMapper->GetCellAttributeElementSize(bufferAttribute));

        renderBuffer->SetWebGPUBuffer(wgpuMapper->GetCellDataWGPUBuffer(bufferAttribute));

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
  // These are owned references. Dropping the pointer does not release them; the
  // wgpu:: wrapper members these replaced did that in their destructor.
  vtkWebGPU::ReleaseAndNull(this->Bundle, wgpuRenderBundleRelease);
  vtkWebGPU::ReleaseAndNull(this->WGPUBundleEncoder, wgpuRenderBundleEncoderRelease);
  vtkWebGPU::ReleaseAndNull(this->WGPURenderEncoder, wgpuRenderPassEncoderRelease);
  vtkWebGPU::ReleaseAndNull(this->SceneTransformBuffer, wgpuBufferRelease);
  vtkWebGPU::ReleaseAndNull(this->SceneLightsBuffer, wgpuBufferRelease);
  vtkWebGPU::ReleaseAndNull(this->BackgroundGradientBuffer, wgpuBufferRelease);
  vtkWebGPU::ReleaseAndNull(this->SceneBindGroup, wgpuBindGroupRelease);
  vtkWebGPU::ReleaseAndNull(this->SceneBindGroupLayout, wgpuBindGroupLayoutRelease);
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
WGPUCommandBuffer vtkWebGPURenderer::EncodePropListRenderCommand(vtkProp** propList, int listLength)
{
  this->UpdateBuffers();

  // Because all the command encoding / rendering function use the props of the this->PropArray
  // list, we're going to replace the list so that only the props we're interested in are rendered.
  // We need to backup the original list though to restore it afterwards
  std::vector<vtkProp*> propArrayBackup = this->PropArray;

  this->PropArray.resize(listLength);
  for (int i = 0; i < listLength; i++)
  {
    this->PropArray[i] = propList[i];
  }

  this->RecordRenderCommands();

  // Restoring
  this->PropArray = propArrayBackup;

  vtkWebGPURenderWindow* renderWindow =
    vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  WGPUCommandEncoder commandEncoder = renderWindow->GetCommandEncoder();
  vtkWebGPU::CommandBuffer commandBuffer =
    vtkWebGPU::CommandBuffer::Acquire(wgpuCommandEncoderFinish(commandEncoder, nullptr));

  // The command encoder of the render window has finished so we need to recreate a new one so that
  // it's ready to be used again by someone else
  renderWindow->CreateCommandEncoder();

  this->DrawBackgroundInClearPass = false;
  return commandBuffer.Release();
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::BeginRecording()
{
  vtkDebugMacro(<< __func__);
  this->RenderStage = RenderStageEnum::RecordingCommands;
  assert(this->WGPURenderEncoder != nullptr);

  WGPURenderPassEncoder renderEncoder(this->WGPURenderEncoder);
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
  wgpuRenderPassEncoderPushDebugGroup(
    renderEncoder, WGPUStringView{ "Renderer start encoding", WGPU_STRLEN });
#endif
  wgpuRenderPassEncoderSetBindGroup(renderEncoder, 0, this->SceneBindGroup, 0, nullptr);
  if (this->RebuildRenderBundle)
  {
    // destroy previous bundle.
    vtkWebGPU::ReleaseAndNull(this->Bundle, wgpuRenderBundleRelease);
    // create a new bundle encoder.
    const std::string label = this->GetObjectDescription();
    auto wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
    const std::vector<WGPUTextureFormat> colorFormats = {
      wgpuRenderWindow->GetPreferredSurfaceTextureFormat(),
      wgpuRenderWindow->GetPreferredSelectorIdsTextureFormat()
    };
    WGPURenderBundleEncoderDescriptor bundleEncDesc = WGPU_RENDER_BUNDLE_ENCODER_DESCRIPTOR_INIT;
    bundleEncDesc.colorFormatCount = colorFormats.size();
    bundleEncDesc.colorFormats = colorFormats.data();
    bundleEncDesc.depthStencilFormat = wgpuRenderWindow->GetDepthStencilFormat();
    bundleEncDesc.sampleCount =
      1; // multi-sampling only works for 1 or 4 samples on some implementations.
    bundleEncDesc.depthReadOnly = false;
    bundleEncDesc.stencilReadOnly = false;
    bundleEncDesc.label = vtkWebGPUMakeStringView(label);
    bundleEncDesc.nextInChain = nullptr;
    vtkWebGPU::ReleaseAndNull(this->WGPUBundleEncoder, wgpuRenderBundleEncoderRelease);
    this->WGPUBundleEncoder = wgpuRenderWindow->NewRenderBundleEncoder(bundleEncDesc);
    wgpuRenderBundleEncoderSetBindGroup(
      this->WGPUBundleEncoder, 0, this->SceneBindGroup, 0, nullptr);
  }
  else
  {
    vtkWebGPU::ReleaseAndNull(this->WGPUBundleEncoder, wgpuRenderBundleEncoderRelease);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::SetupBindGroupLayouts()
{
  auto wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  WGPUDevice device = wgpuRenderWindow->GetDevice();
  if (this->SceneBindGroupLayout == nullptr)
  {
    vtkWebGPU::BindGroupLayout layout = vtkWebGPU::BindGroupLayout::Acquire(
      vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
        {
          // clang-format off
      // SceneTransforms
      { 0, WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, WGPUBufferBindingType_Uniform },
      // SceneLights
      { 1, WGPUShaderStage_Fragment, WGPUBufferBindingType_ReadOnlyStorage }
          // clang-format on
        },
        "SceneBindGroupLayout"));
    this->SceneBindGroupLayout = layout.Release();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::SetupSceneBindGroup()
{
  auto wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(this->GetRenderWindow());
  WGPUDevice device = wgpuRenderWindow->GetDevice();

  // Calculate current buffer sizes to bind
  const auto transformSize = vtkWebGPUCamera::GetCacheSizeBytes();
  const auto transformSizePadded = vtkWebGPUConfiguration::Align(transformSize, 32);
  const auto lightCount = std::max(std::size_t(1), this->LightIDs.size());
  const auto lightSize = 16 + lightCount * vtkWebGPULight::GetCacheSizeBytes();
  const auto lightSizePadded = vtkWebGPUConfiguration::Align(lightSize, 32);

  std::vector<WGPUBindGroupEntry> entries;
  WGPUBindGroupEntry entry0 = WGPU_BIND_GROUP_ENTRY_INIT;
  entry0.binding = 0;
  entry0.buffer = this->SceneTransformBuffer;
  entry0.offset = 0;
  entry0.size = transformSizePadded;
  entries.push_back(entry0);

  WGPUBindGroupEntry entry1 = WGPU_BIND_GROUP_ENTRY_INIT;
  entry1.binding = 1;
  entry1.buffer = this->SceneLightsBuffer;
  entry1.offset = 0;
  entry1.size = lightSizePadded;
  entries.push_back(entry1);

  WGPUBindGroupDescriptor descriptor = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
  descriptor.label = WGPUStringView{ "SceneBindGroup", WGPU_STRLEN };
  descriptor.layout = this->SceneBindGroupLayout;
  descriptor.entryCount = static_cast<uint32_t>(entries.size());
  descriptor.entries = entries.data();

  this->SceneBindGroup = wgpuDeviceCreateBindGroup(device, &descriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPURenderer::EndRecording()
{
  vtkDebugMacro(<< __func__);
  this->RenderStage = RenderStageEnum::Finished;
  WGPURenderPassEncoder renderEncoder(this->WGPURenderEncoder);
  if (this->UseRenderBundles)
  {
    if (this->WGPUBundleEncoder)
    {
      vtkWebGPU::ReleaseAndNull(this->Bundle, wgpuRenderBundleRelease);
      this->Bundle = wgpuRenderBundleEncoderFinish(this->WGPUBundleEncoder, nullptr);
    }
    if (this->Bundle != nullptr)
    {
      wgpuRenderPassEncoderExecuteBundles(renderEncoder, 1, &this->Bundle);
    }
  }
#if !defined(NDEBUG) && !defined(__EMSCRIPTEN__)
  wgpuRenderPassEncoderPopDebugGroup(renderEncoder);
#endif
  wgpuRenderPassEncoderEnd(renderEncoder);
  vtkWebGPU::ReleaseAndNull(this->WGPURenderEncoder, wgpuRenderPassEncoderRelease);
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
