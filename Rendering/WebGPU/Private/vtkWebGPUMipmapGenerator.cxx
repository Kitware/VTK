// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUMipmapGenerator.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUHandle.h"
#include "Private/vtkWebGPUShaderModuleInternals.h"
#include "vtkWebGPUConfiguration.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{
const char* MipmapShaderSource = R"(
struct VertexOutput
{
  @builtin(position) position: vec4<f32>,
  @location(0) uv: vec2<f32>,
}

@group(0) @binding(0) var source_sampler: sampler;
@group(0) @binding(1) var source: texture_2d<f32>;

@vertex
fn vertexMain(@builtin(vertex_index) index: u32) -> VertexOutput
{
  var output: VertexOutput;
  let uv = vec2<f32>(f32((index << 1u) & 2u), f32(index & 2u));
  output.position = vec4<f32>(uv * vec2<f32>(2.0, -2.0) + vec2<f32>(-1.0, 1.0), 0.0, 1.0);
  output.uv = uv;
  return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32>
{
  return textureSampleLevel(source, source_sampler, input.uv, 0.0);
}
)";

vtkWebGPU::TextureView MakeLevelView(
  WGPUTexture texture, WGPUTextureFormat format, std::uint32_t level, std::uint32_t layer)
{
  WGPUTextureViewDescriptor descriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
  descriptor.label = WGPUStringView{ "MipmapLevelView", WGPU_STRLEN };
  descriptor.format = format;
  descriptor.dimension = WGPUTextureViewDimension_2D;
  descriptor.baseMipLevel = level;
  descriptor.mipLevelCount = 1;
  descriptor.baseArrayLayer = layer;
  descriptor.arrayLayerCount = 1;
  return vtkWebGPU::TextureView::Acquire(wgpuTextureCreateView(texture, &descriptor));
}
}

//------------------------------------------------------------------------------
void vtkWebGPUMipmapGenerator::Generate(vtkWebGPUConfiguration* configuration, WGPUTexture texture,
  WGPUTextureFormat format, std::uint32_t mipLevelCount, std::uint32_t layerCount)
{
  if (!configuration || !texture || mipLevelCount < 2)
  {
    return;
  }
  WGPUDevice device = configuration->GetDevice();

  vtkWebGPU::BindGroupLayout layout = vtkWebGPU::BindGroupLayout::Acquire(
    vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      { { 0, WGPUShaderStage_Fragment, WGPUSamplerBindingType_Filtering },
        { 1, WGPUShaderStage_Fragment, WGPUTextureSampleType_Float, WGPUTextureViewDimension_2D } },
      "MipmapBindGroupLayout"));
  WGPUBindGroupLayout rawLayout = layout;
  WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
  pipelineLayoutDescriptor.label = WGPUStringView{ "MipmapPipelineLayout", WGPU_STRLEN };
  pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
  pipelineLayoutDescriptor.bindGroupLayouts = &rawLayout;
  vtkWebGPU::PipelineLayout pipelineLayout = vtkWebGPU::PipelineLayout::Acquire(
    wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor));

  vtkWebGPU::ShaderModule module = vtkWebGPU::ShaderModule::Acquire(
    vtkWebGPUShaderModuleInternals::CreateFromWGSL(device, MipmapShaderSource));
  WGPUColorTargetState target = WGPU_COLOR_TARGET_STATE_INIT;
  target.format = format;
  WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
  fragment.module = module;
  fragment.entryPoint = WGPUStringView{ "fragmentMain", WGPU_STRLEN };
  fragment.targetCount = 1;
  fragment.targets = &target;
  WGPURenderPipelineDescriptor pipelineDescriptor = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
  pipelineDescriptor.label = WGPUStringView{ "MipmapPipeline", WGPU_STRLEN };
  pipelineDescriptor.layout = pipelineLayout;
  pipelineDescriptor.vertex.module = module;
  pipelineDescriptor.vertex.entryPoint = WGPUStringView{ "vertexMain", WGPU_STRLEN };
  pipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
  pipelineDescriptor.fragment = &fragment;
  vtkWebGPU::RenderPipeline pipeline =
    vtkWebGPU::RenderPipeline::Acquire(wgpuDeviceCreateRenderPipeline(device, &pipelineDescriptor));

  WGPUSamplerDescriptor samplerDescriptor = WGPU_SAMPLER_DESCRIPTOR_INIT;
  samplerDescriptor.label = WGPUStringView{ "MipmapSampler", WGPU_STRLEN };
  samplerDescriptor.magFilter = WGPUFilterMode_Linear;
  samplerDescriptor.minFilter = WGPUFilterMode_Linear;
  vtkWebGPU::Sampler sampler =
    vtkWebGPU::Sampler::Acquire(wgpuDeviceCreateSampler(device, &samplerDescriptor));

  WGPUCommandEncoderDescriptor encoderDescriptor = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
  encoderDescriptor.label = WGPUStringView{ "MipmapGeneration", WGPU_STRLEN };
  vtkWebGPU::CommandEncoder encoder =
    vtkWebGPU::CommandEncoder::Acquire(wgpuDeviceCreateCommandEncoder(device, &encoderDescriptor));
  for (std::uint32_t layer = 0; layer < layerCount; ++layer)
  {
    for (std::uint32_t level = 1; level < mipLevelCount; ++level)
    {
      vtkWebGPU::TextureView source = MakeLevelView(texture, format, level - 1, layer);
      vtkWebGPU::TextureView destination = MakeLevelView(texture, format, level, layer);
      vtkWebGPU::BindGroup bindGroup =
        vtkWebGPU::BindGroup::Acquire(vtkWebGPUBindGroupInternals::MakeBindGroup(
          device, layout, { { 0, sampler.Get() }, { 1, source.Get() } }, "MipmapBindGroup"));

      WGPURenderPassColorAttachment attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
      attachment.view = destination;
      attachment.loadOp = WGPULoadOp_Clear;
      attachment.storeOp = WGPUStoreOp_Store;
      WGPURenderPassDescriptor passDescriptor = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
      passDescriptor.label = WGPUStringView{ "MipmapLevel", WGPU_STRLEN };
      passDescriptor.colorAttachmentCount = 1;
      passDescriptor.colorAttachments = &attachment;
      vtkWebGPU::RenderPassEncoder pass = vtkWebGPU::RenderPassEncoder::Acquire(
        wgpuCommandEncoderBeginRenderPass(encoder, &passDescriptor));
      wgpuRenderPassEncoderSetPipeline(pass, pipeline);
      wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
      wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
      wgpuRenderPassEncoderEnd(pass);
    }
  }
  vtkWebGPU::CommandBuffer commands =
    vtkWebGPU::CommandBuffer::Acquire(wgpuCommandEncoderFinish(encoder, nullptr));
  vtkWebGPU::Queue queue = vtkWebGPU::Queue::Acquire(wgpuDeviceGetQueue(device));
  WGPUCommandBuffer rawCommands = commands;
  wgpuQueueSubmit(queue, 1, &rawCommands);
}

VTK_ABI_NAMESPACE_END
