// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUToneMappingPass.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUHandle.h"
#include "Private/vtkWebGPUShaderModuleInternals.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkRenderState.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include <algorithm>
#include <cstdint>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
constexpr WGPUTextureFormat ColorTextureFormat = WGPUTextureFormat_RGBA16Float;

// Matches the uniforms of the OpenGL implementation: a is the contrast, b the clipping point, c
// the toe speed and d the shoulder.
struct ToneMappingParameters
{
  float Exposure;
  float A;
  float B;
  float C;
  float D;
  float Padding[3];
};

const char* ShaderStart = R"(
struct ToneMappingParameters
{
  exposure: f32,
  a: f32,
  b: f32,
  c: f32,
  d: f32,
}

const acesInputMat = mat3x3<f32>(0.5972782409, 0.0760130499, 0.0284085382,
  0.3545713181, 0.9083220973, 0.1338243154,
  0.0482176639, 0.0156579968, 0.8375684636);
const acesOutputMat = mat3x3<f32>(1.6047539945, -0.1020831870, -0.0032670420,
  -0.5310794927, 1.1081322801, -0.0727552477,
  -0.0736720338, -0.0060518756, 1.0760219533);

@group(0) @binding(0) var source: texture_2d<f32>;
@group(0) @binding(1) var<uniform> parameters: ToneMappingParameters;

@vertex
fn vertexMain(@builtin(vertex_index) index: u32) -> @builtin(position) vec4<f32>
{
  let uv = vec2<f32>(f32((index << 1u) & 2u), f32(index & 2u));
  return vec4<f32>(uv * 2.0 - 1.0, 0.0, 1.0);
}

@fragment
fn fragmentMain(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32>
{
  let pixel = textureLoad(source, vec2<i32>(position.xy), 0);
  let color = pow(max(pixel.rgb, vec3<f32>(0.0)), vec3<f32>(2.2));
)";

const char* ShaderEnd = R"(
  toned = pow(toned, vec3<f32>(1.0 / 2.2));
  return mix(pixel, vec4<f32>(toned, pixel.a), pixel.a);
}
)";

std::string GetOperator(int type, bool useACES)
{
  switch (type)
  {
    case vtkToneMappingPass::Clamp:
      return "  var toned = min(color, vec3<f32>(1.0));\n";
    case vtkToneMappingPass::Reinhard:
      return "  var toned = color / (color + 1.0);\n";
    case vtkToneMappingPass::Exponential:
      return "  var toned = 1.0 - exp(-color * parameters.exposure);\n";
    case vtkToneMappingPass::GenericFilmic:
    {
      std::string source = "  var toned = color * parameters.exposure;\n";
      if (useACES)
      {
        source += "  toned = acesInputMat * toned;\n";
      }
      source += "  toned = pow(toned, vec3<f32>(parameters.a)) /\n"
                "    (pow(toned, vec3<f32>(parameters.a * parameters.d)) * parameters.b +\n"
                "      parameters.c);\n";
      if (useACES)
      {
        source += "  toned = acesOutputMat * toned;\n";
      }
      return source + "  toned = clamp(toned, vec3<f32>(0.0), vec3<f32>(1.0));\n";
    }
    case vtkToneMappingPass::NeutralPBR:
    default:
      // adapted from Khronos reference implementation:
      // https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/pbrNeutral.glsl
      return R"(
  let start_compression = 0.8 - 0.04;
  let desaturation = 0.15;
  let x = min(color.r, min(color.g, color.b));
  let offset = select(0.04, x - 6.25 * x * x, x < 0.08);
  var toned = color - vec3<f32>(offset);
  let peak = max(toned.r, max(toned.g, toned.b));
  if (peak >= start_compression)
  {
    let d = 1.0 - start_compression;
    let new_peak = 1.0 - d * d / (peak + d - start_compression);
    toned *= new_peak / peak;
    let g = 1.0 - 1.0 / (desaturation * (peak - new_peak) + 1.0);
    toned = mix(toned, vec3<f32>(new_peak), g);
  }
)";
  }
}
}

class vtkWebGPUToneMappingPass::vtkInternals
{
public:
  std::uint32_t Width = 0;
  std::uint32_t Height = 0;
  vtkWebGPU::Texture Texture;
  vtkWebGPU::TextureView View;

  int PipelineType = -1;
  bool PipelineUsesACES = false;
  WGPUTextureFormat PipelineFormat = WGPUTextureFormat_Undefined;
  vtkWebGPU::BindGroupLayout Layout;
  vtkWebGPU::RenderPipeline Pipeline;
  vtkWebGPU::Buffer Parameters;
  vtkWebGPU::BindGroup BindGroup;
};

vtkStandardNewMacro(vtkWebGPUToneMappingPass);

//------------------------------------------------------------------------------
vtkWebGPUToneMappingPass::vtkWebGPUToneMappingPass()
  : Internals(new vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkWebGPUToneMappingPass::~vtkWebGPUToneMappingPass() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPUToneMappingPass::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkWebGPUToneMappingPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColorTextureSize: " << this->Internals->Width << "x" << this->Internals->Height
     << endl;
}

//------------------------------------------------------------------------------
void vtkWebGPUToneMappingPass::Render(const vtkRenderState* s)
{
  this->NumberOfRenderedProps = 0;
  if (this->DelegatePass == nullptr)
  {
    vtkWarningMacro("no delegate in vtkWebGPUToneMappingPass.");
    return;
  }
  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(s->GetRenderer());
  if (wgpuRenderer &&
    wgpuRenderer->GetRenderStage() == vtkWebGPURenderer::RenderStageEnum::ConfiguringPasses)
  {
    wgpuRenderer->SetToneMappingPass(this);
  }
  this->DelegatePass->Render(s);
  this->NumberOfRenderedProps = this->DelegatePass->GetNumberOfRenderedProps();
}

//------------------------------------------------------------------------------
WGPUTextureFormat vtkWebGPUToneMappingPass::GetColorTextureFormat() const
{
  return ColorTextureFormat;
}

//------------------------------------------------------------------------------
WGPUTextureView vtkWebGPUToneMappingPass::GetColorTextureView(vtkWebGPURenderer* renderer)
{
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  if (!wgpuRenderWindow)
  {
    return nullptr;
  }
  const int* size = wgpuRenderWindow->GetActualSize();
  const auto width = static_cast<std::uint32_t>(std::max(1, size[0]));
  const auto height = static_cast<std::uint32_t>(std::max(1, size[1]));
  auto& internals = *this->Internals;
  if (internals.View && width == internals.Width && height == internals.Height)
  {
    return internals.View;
  }
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
  internals.BindGroup = vtkWebGPU::BindGroup();
  internals.Texture = vtkWebGPU::Texture::Acquire(wgpuConfiguration->CreateTexture(
    { width, height, 1 }, WGPUTextureDimension_2D, ColorTextureFormat,
    static_cast<WGPUTextureUsage>(
      WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding),
    1, "ToneMappingColorTexture"));
  internals.View = vtkWebGPU::TextureView::Acquire(
    wgpuConfiguration->CreateView(internals.Texture, WGPUTextureViewDimension_2D,
      WGPUTextureAspect_All, ColorTextureFormat, 0, 1, "ToneMappingColorTextureView"));
  internals.Width = width;
  internals.Height = height;
  return internals.View;
}

//------------------------------------------------------------------------------
void vtkWebGPUToneMappingPass::Resolve(vtkWebGPURenderer* renderer)
{
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  auto& internals = *this->Internals;
  if (!wgpuRenderWindow || !internals.View)
  {
    return;
  }
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
  WGPUDevice device = wgpuConfiguration->GetDevice();
  const WGPUTextureFormat destinationFormat = wgpuRenderWindow->GetOffscreenColorTextureFormat();

  if (!internals.Layout)
  {
    internals.Layout = vtkWebGPU::BindGroupLayout::Acquire(
      vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
        { { 0, WGPUShaderStage_Fragment, WGPUTextureSampleType_UnfilterableFloat,
            WGPUTextureViewDimension_2D },
          { 1, WGPUShaderStage_Fragment, WGPUBufferBindingType_Uniform } },
        "ToneMappingBindGroupLayout"));
    internals.Parameters =
      vtkWebGPU::Buffer::Acquire(wgpuConfiguration->CreateBuffer(sizeof(ToneMappingParameters),
        static_cast<WGPUBufferUsage>(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst), false,
        "ToneMappingParameters"));
  }
  const bool useACES = this->ToneMappingType == GenericFilmic && this->UseACES;
  if (!internals.Pipeline || internals.PipelineType != this->ToneMappingType ||
    internals.PipelineUsesACES != useACES || internals.PipelineFormat != destinationFormat)
  {
    WGPUBindGroupLayout layout = internals.Layout;
    WGPUPipelineLayoutDescriptor layoutDescriptor = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    layoutDescriptor.label = WGPUStringView{ "ToneMappingPipelineLayout", WGPU_STRLEN };
    layoutDescriptor.bindGroupLayoutCount = 1;
    layoutDescriptor.bindGroupLayouts = &layout;
    vtkWebGPU::PipelineLayout pipelineLayout =
      vtkWebGPU::PipelineLayout::Acquire(wgpuDeviceCreatePipelineLayout(device, &layoutDescriptor));
    const std::string source =
      ShaderStart + GetOperator(this->ToneMappingType, useACES) + ShaderEnd;
    vtkWebGPU::ShaderModule module = vtkWebGPU::ShaderModule::Acquire(
      vtkWebGPUShaderModuleInternals::CreateFromWGSL(device, source));
    WGPUColorTargetState target = WGPU_COLOR_TARGET_STATE_INIT;
    target.format = destinationFormat;
    WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
    fragment.module = module;
    fragment.entryPoint = WGPUStringView{ "fragmentMain", WGPU_STRLEN };
    fragment.targetCount = 1;
    fragment.targets = &target;
    WGPURenderPipelineDescriptor descriptor = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    descriptor.label = WGPUStringView{ "ToneMappingPipeline", WGPU_STRLEN };
    descriptor.layout = pipelineLayout;
    descriptor.vertex.module = module;
    descriptor.vertex.entryPoint = WGPUStringView{ "vertexMain", WGPU_STRLEN };
    descriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    descriptor.fragment = &fragment;
    internals.Pipeline =
      vtkWebGPU::RenderPipeline::Acquire(wgpuDeviceCreateRenderPipeline(device, &descriptor));
    internals.PipelineType = this->ToneMappingType;
    internals.PipelineUsesACES = useACES;
    internals.PipelineFormat = destinationFormat;
  }
  if (!internals.Pipeline)
  {
    return;
  }

  this->UpdateAnchorCurveGenericFilmic();
  const ToneMappingParameters parameters = { this->Exposure, this->Contrast, this->ClippingPoint,
    this->ToeSpeed, this->Shoulder, { 0.0f, 0.0f, 0.0f } };
  wgpuConfiguration->WriteBuffer(internals.Parameters, 0, &parameters, sizeof(parameters));
  if (!internals.BindGroup)
  {
    internals.BindGroup = vtkWebGPU::BindGroup::Acquire(
      vtkWebGPUBindGroupInternals::MakeBindGroup(device, internals.Layout,
        { { 0, internals.View.Get() }, { 1, internals.Parameters.Get(), 0, sizeof(parameters) } },
        "ToneMappingBindGroup"));
  }

  WGPURenderPassColorAttachment attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
  attachment.view = wgpuRenderWindow->GetOffscreenColorAttachmentView();
  attachment.loadOp = WGPULoadOp_Load;
  attachment.storeOp = WGPUStoreOp_Store;
  WGPURenderPassDescriptor passDescriptor = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
  passDescriptor.label = WGPUStringView{ "vtkWebGPUToneMappingPass::Resolve", WGPU_STRLEN };
  passDescriptor.colorAttachmentCount = 1;
  passDescriptor.colorAttachments = &attachment;
  vtkWebGPU::RenderPassEncoder pass =
    vtkWebGPU::RenderPassEncoder::Acquire(wgpuRenderWindow->NewRenderPass(passDescriptor));

  int width, height, originX, originY;
  renderer->GetTiledSizeAndOrigin(&width, &height, &originX, &originY);
  originY = wgpuRenderWindow->GetActualSize()[1] - originY - height;
  if (width <= 0 || height <= 0)
  {
    wgpuRenderPassEncoderEnd(pass);
    return;
  }
  wgpuRenderPassEncoderSetViewport(pass, static_cast<float>(originX), static_cast<float>(originY),
    static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
  wgpuRenderPassEncoderSetScissorRect(pass, static_cast<std::uint32_t>(originX),
    static_cast<std::uint32_t>(originY), static_cast<std::uint32_t>(width),
    static_cast<std::uint32_t>(height));
  wgpuRenderPassEncoderSetPipeline(pass, internals.Pipeline);
  wgpuRenderPassEncoderSetBindGroup(pass, 0, internals.BindGroup, 0, nullptr);
  wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
  wgpuRenderPassEncoderEnd(pass);
}

//------------------------------------------------------------------------------
void vtkWebGPUToneMappingPass::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);
  *this->Internals = vtkInternals();
}
VTK_ABI_NAMESPACE_END
