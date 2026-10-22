// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUEnvironmentMap.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUShaderModuleInternals.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkWebGPUConfiguration.h"

#include "IBLBRDFLookupTable.h"
#include "IBLCommon.h"
#include "IBLEnvironmentCube.h"
#include "IBLIrradiance.h"
#include "IBLPrefilter.h"
#include "IBLSphericalHarmonics.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
constexpr WGPUTextureFormat IBLFormat = WGPUTextureFormat_RGBA16Float;
constexpr std::uint32_t MaximumEnvironmentSize = 512;
constexpr std::uint32_t MinimumEnvironmentSize = 32;
constexpr std::uint32_t IrradianceSize = 32;
constexpr std::uint32_t HarmonicsSourceSize = 32;
constexpr std::uint32_t LookupTableSize = 512;
constexpr std::uint32_t LookupTableSamples = 1024;
constexpr std::uint32_t PrefilterMaxSamples = 512;
constexpr float IrradianceStep = 0.04908738521f;

struct EnvironmentParameters
{
  float Right[4];
  float Up[4];
  float Front[4];
  std::uint32_t Size;
  std::uint32_t ConvertToLinear;
  std::uint32_t Pad[2];
};

struct PrefilterParameters
{
  float Roughness;
  float TexelSolidAngle;
  float MaxSourceLevel;
  std::uint32_t Size;
  std::uint32_t SampleCount;
  std::uint32_t Pad[3];
};

struct IrradianceParameters
{
  float Step;
  float SourceLevel;
  std::uint32_t Size;
  std::uint32_t Pad;
};

struct SizeParameters
{
  std::uint32_t Size;
  std::uint32_t SampleCount;
  std::uint32_t Pad[2];
};

using LayoutEntry = vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper;
using BindingEntry = vtkWebGPUBindGroupInternals::BindingInitializationHelper;

std::uint32_t DispatchCount(std::uint32_t size)
{
  return (size + 7) / 8;
}

vtkWebGPU::ComputePipeline MakePipeline(WGPUDevice device, const std::string& source,
  const char* entryPoint, WGPUBindGroupLayout layout, const char* label)
{
  vtkWebGPU::ShaderModule module = vtkWebGPU::ShaderModule::Acquire(
    vtkWebGPUShaderModuleInternals::CreateFromWGSL(device, source));
  if (!module)
  {
    return nullptr;
  }
  WGPUPipelineLayoutDescriptor layoutDescriptor = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
  layoutDescriptor.bindGroupLayoutCount = 1;
  layoutDescriptor.bindGroupLayouts = &layout;
  layoutDescriptor.label = WGPUStringView{ label, WGPU_STRLEN };
  vtkWebGPU::PipelineLayout pipelineLayout =
    vtkWebGPU::PipelineLayout::Acquire(wgpuDeviceCreatePipelineLayout(device, &layoutDescriptor));

  WGPUComputePipelineDescriptor descriptor = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
  descriptor.label = WGPUStringView{ label, WGPU_STRLEN };
  descriptor.layout = pipelineLayout;
  descriptor.compute.module = module;
  descriptor.compute.entryPoint = WGPUStringView{ entryPoint, WGPU_STRLEN };
  return vtkWebGPU::ComputePipeline::Acquire(wgpuDeviceCreateComputePipeline(device, &descriptor));
}

vtkWebGPU::TextureView MakeView(WGPUTexture texture, WGPUTextureViewDimension dimension,
  std::uint32_t baseMipLevel, std::uint32_t mipLevelCount, std::uint32_t layerCount,
  const char* label)
{
  WGPUTextureViewDescriptor descriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
  descriptor.label = WGPUStringView{ label, WGPU_STRLEN };
  descriptor.format = IBLFormat;
  descriptor.dimension = dimension;
  descriptor.baseMipLevel = baseMipLevel;
  descriptor.mipLevelCount = mipLevelCount;
  descriptor.baseArrayLayer = 0;
  descriptor.arrayLayerCount = layerCount;
  return vtkWebGPU::TextureView::Acquire(wgpuTextureCreateView(texture, &descriptor));
}

vtkWebGPU::Texture MakeTexture(vtkWebGPUConfiguration* configuration, std::uint32_t size,
  std::uint32_t layers, std::uint32_t levels, const char* label)
{
  return vtkWebGPU::Texture::Acquire(
    configuration->CreateTexture({ size, size, layers }, WGPUTextureDimension_2D, IBLFormat,
      static_cast<WGPUTextureUsage>(
        WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding),
      static_cast<int>(levels), label));
}

template <typename T>
vtkWebGPU::Buffer MakeUniform(
  vtkWebGPUConfiguration* configuration, const T& value, const char* label)
{
  vtkWebGPU::Buffer buffer = vtkWebGPU::Buffer::Acquire(configuration->CreateBuffer(sizeof(T),
    static_cast<WGPUBufferUsage>(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst), false, label));
  configuration->WriteBuffer(buffer, 0, &value, sizeof(T), label);
  return buffer;
}

vtkWebGPU::BindGroup MakeBindGroup(WGPUDevice device, WGPUBindGroupLayout layout,
  const std::vector<WGPUBindGroupEntry>& entries, const char* label)
{
  return vtkWebGPU::BindGroup::Acquire(
    vtkWebGPUBindGroupInternals::MakeBindGroup(device, layout, entries, label));
}

WGPUBindGroupEntry SamplerEntry(std::uint32_t binding, WGPUSampler sampler)
{
  WGPUBindGroupEntry entry = WGPU_BIND_GROUP_ENTRY_INIT;
  entry.binding = binding;
  entry.sampler = sampler;
  return entry;
}

WGPUBindGroupEntry ViewEntry(std::uint32_t binding, WGPUTextureView view)
{
  WGPUBindGroupEntry entry = WGPU_BIND_GROUP_ENTRY_INIT;
  entry.binding = binding;
  entry.textureView = view;
  return entry;
}

void Dispatch(WGPUCommandEncoder encoder, WGPUComputePipeline pipeline, WGPUBindGroup bindGroup,
  std::uint32_t x, std::uint32_t y, std::uint32_t z, const char* label)
{
  WGPUComputePassDescriptor passDescriptor = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
  passDescriptor.label = WGPUStringView{ label, WGPU_STRLEN };
  vtkWebGPU::ComputePassEncoder pass = vtkWebGPU::ComputePassEncoder::Acquire(
    wgpuCommandEncoderBeginComputePass(encoder, &passDescriptor));
  wgpuComputePassEncoderSetPipeline(pass, pipeline);
  wgpuComputePassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
  wgpuComputePassEncoderDispatchWorkgroups(pass, x, y, z);
  wgpuComputePassEncoderEnd(pass);
}
}

//------------------------------------------------------------------------------
bool vtkWebGPUEnvironmentMap::CreatePipelines(vtkWebGPUConfiguration* configuration)
{
  if (this->LookupTablePipeline)
  {
    return true;
  }
  WGPUDevice device = configuration->GetDevice();
  const std::string common = IBLCommon;
  constexpr auto compute = WGPUShaderStage_Compute;
  constexpr auto storageWrite = WGPUStorageTextureAccess_WriteOnly;

  this->EnvironmentLayout = vtkWebGPU::BindGroupLayout::Acquire(
    vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      { LayoutEntry{ 0, compute, WGPUBufferBindingType_Uniform },
        LayoutEntry{ 1, compute, WGPUSamplerBindingType_Filtering },
        LayoutEntry{ 2, compute, WGPUTextureSampleType_Float, WGPUTextureViewDimension_2D },
        LayoutEntry{ 4, compute, storageWrite, IBLFormat, WGPUTextureViewDimension_2DArray } },
      "IBLEquirectangularLayout"));
  this->CubeLayout = vtkWebGPU::BindGroupLayout::Acquire(
    vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      { LayoutEntry{ 0, compute, WGPUBufferBindingType_Uniform },
        LayoutEntry{ 1, compute, WGPUSamplerBindingType_Filtering },
        LayoutEntry{ 3, compute, WGPUTextureSampleType_Float, WGPUTextureViewDimension_Cube },
        LayoutEntry{ 4, compute, storageWrite, IBLFormat, WGPUTextureViewDimension_2DArray } },
      "IBLCubeLayout"));
  this->DownsampleLayout = vtkWebGPU::BindGroupLayout::Acquire(
    vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      { LayoutEntry{ 0, compute, WGPUBufferBindingType_Uniform },
        LayoutEntry{ 4, compute, storageWrite, IBLFormat, WGPUTextureViewDimension_2DArray },
        LayoutEntry{
          5, compute, WGPUTextureSampleType_UnfilterableFloat, WGPUTextureViewDimension_2DArray } },
      "IBLDownsampleLayout"));
  const auto cubeSamplingLayout = [&](const char* label)
  {
    return vtkWebGPU::BindGroupLayout::Acquire(
      vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
        { LayoutEntry{ 0, compute, WGPUBufferBindingType_Uniform },
          LayoutEntry{ 1, compute, WGPUSamplerBindingType_Filtering },
          LayoutEntry{ 2, compute, WGPUTextureSampleType_Float, WGPUTextureViewDimension_Cube },
          LayoutEntry{ 3, compute, storageWrite, IBLFormat, WGPUTextureViewDimension_2DArray } },
        label));
  };
  this->PrefilterLayout = cubeSamplingLayout("IBLPrefilterLayout");
  this->IrradianceLayout = cubeSamplingLayout("IBLIrradianceLayout");
  this->HarmonicsLayout = vtkWebGPU::BindGroupLayout::Acquire(
    vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      { LayoutEntry{ 0, compute, WGPUBufferBindingType_Uniform },
        LayoutEntry{
          1, compute, WGPUTextureSampleType_UnfilterableFloat, WGPUTextureViewDimension_2DArray },
        LayoutEntry{ 2, compute, WGPUBufferBindingType_Storage } },
      "IBLHarmonicsLayout"));
  this->LookupTableLayout = vtkWebGPU::BindGroupLayout::Acquire(
    vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      { LayoutEntry{ 0, compute, WGPUBufferBindingType_Uniform },
        LayoutEntry{ 1, compute, storageWrite, IBLFormat, WGPUTextureViewDimension_2D } },
      "IBLLookupTableLayout"));

  const std::string environment = common + IBLEnvironmentCube;
  this->EquirectangularPipeline = MakePipeline(device, environment, "equirectangularToCube",
    this->EnvironmentLayout, "IBLEquirectangularToCube");
  this->CubePipeline =
    MakePipeline(device, environment, "cubeToCube", this->CubeLayout, "IBLCubeToCube");
  this->DownsamplePipeline =
    MakePipeline(device, environment, "downsample", this->DownsampleLayout, "IBLDownsample");
  this->PrefilterPipeline =
    MakePipeline(device, common + IBLPrefilter, "prefilter", this->PrefilterLayout, "IBLPrefilter");
  this->IrradiancePipeline = MakePipeline(
    device, common + IBLIrradiance, "irradiance", this->IrradianceLayout, "IBLIrradiance");
  this->HarmonicsPipeline = MakePipeline(device, common + IBLSphericalHarmonics,
    "sphericalHarmonics", this->HarmonicsLayout, "IBLSphericalHarmonics");
  this->LookupTablePipeline = MakePipeline(device, common + IBLBRDFLookupTable, "brdfLookupTable",
    this->LookupTableLayout, "IBLBRDFLookupTable");

  const bool valid = this->EquirectangularPipeline && this->CubePipeline &&
    this->DownsamplePipeline && this->PrefilterPipeline && this->IrradiancePipeline &&
    this->HarmonicsPipeline && this->LookupTablePipeline;
  if (!valid)
  {
    vtkLog(ERROR, "Failed to create the image based lighting compute pipelines.");
    this->LookupTablePipeline = nullptr;
  }
  return valid;
}

//------------------------------------------------------------------------------
void vtkWebGPUEnvironmentMap::CreateTextures(
  vtkWebGPUConfiguration* configuration, std::uint32_t size)
{
  if (!this->Sampler)
  {
    WGPUSamplerDescriptor descriptor = WGPU_SAMPLER_DESCRIPTOR_INIT;
    descriptor.label = WGPUStringView{ "IBLSampler", WGPU_STRLEN };
    descriptor.addressModeU = WGPUAddressMode_ClampToEdge;
    descriptor.addressModeV = WGPUAddressMode_ClampToEdge;
    descriptor.addressModeW = WGPUAddressMode_ClampToEdge;
    descriptor.magFilter = WGPUFilterMode_Linear;
    descriptor.minFilter = WGPUFilterMode_Linear;
    descriptor.mipmapFilter = WGPUMipmapFilterMode_Linear;
    this->Sampler =
      vtkWebGPU::Sampler::Acquire(wgpuDeviceCreateSampler(configuration->GetDevice(), &descriptor));
    descriptor.label = WGPUStringView{ "IBLSourceSampler", WGPU_STRLEN };
    descriptor.addressModeU = WGPUAddressMode_Repeat;
    this->SourceSampler =
      vtkWebGPU::Sampler::Acquire(wgpuDeviceCreateSampler(configuration->GetDevice(), &descriptor));
  }
  if (!this->LookupTable)
  {
    this->LookupTable = MakeTexture(configuration, LookupTableSize, 1, 1, "IBLBRDFLookupTable");
    this->LookupTableView =
      MakeView(this->LookupTable, WGPUTextureViewDimension_2D, 0, 1, 1, "IBLBRDFLookupTableView");
  }
  if (!this->HarmonicsBuffer)
  {
    this->HarmonicsBuffer =
      vtkWebGPU::Buffer::Acquire(configuration->CreateBuffer(GetHarmonicsBufferSize(),
        static_cast<WGPUBufferUsage>(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc), false,
        "IBLSphericalHarmonics"));
  }
  if (!this->IrradianceCube)
  {
    this->IrradianceCube = MakeTexture(configuration, IrradianceSize, 6, 1, "IBLIrradiance");
    this->IrradianceCubeView =
      MakeView(this->IrradianceCube, WGPUTextureViewDimension_Cube, 0, 1, 6, "IBLIrradianceView");
  }
  if (this->EnvironmentSize == size && this->EnvironmentCube)
  {
    return;
  }
  this->EnvironmentSize = size;
  this->EnvironmentLevels = static_cast<std::uint32_t>(std::log2(size)) + 1;
  this->EnvironmentCube =
    MakeTexture(configuration, size, 6, this->EnvironmentLevels, "IBLEnvironment");
  this->EnvironmentCubeView = MakeView(this->EnvironmentCube, WGPUTextureViewDimension_Cube, 0,
    this->EnvironmentLevels, 6, "IBLEnvironmentView");
  this->PrefilterCube = MakeTexture(configuration, size, 6, this->PrefilterLevels, "IBLPrefilter");
  this->PrefilterCubeView = MakeView(this->PrefilterCube, WGPUTextureViewDimension_Cube, 0,
    this->PrefilterLevels, 6, "IBLPrefilterView");
}

//------------------------------------------------------------------------------
void vtkWebGPUEnvironmentMap::ComputeLookupTable(
  vtkWebGPUConfiguration* configuration, WGPUCommandEncoder encoder)
{
  const SizeParameters parameters{ LookupTableSize, LookupTableSamples, { 0, 0 } };
  vtkWebGPU::Buffer uniform =
    MakeUniform(configuration, parameters, "IBLBRDFLookupTableParameters");
  vtkWebGPU::BindGroup bindGroup =
    MakeBindGroup(configuration->GetDevice(), this->LookupTableLayout,
      { BindingEntry{ 0, uniform, 0, sizeof(parameters) }.GetAsBinding(),
        ViewEntry(1, this->LookupTableView) },
      "IBLBRDFLookupTableBindGroup");
  Dispatch(encoder, this->LookupTablePipeline, bindGroup, DispatchCount(LookupTableSize),
    DispatchCount(LookupTableSize), 1, "IBLBRDFLookupTable");
}

//------------------------------------------------------------------------------
void vtkWebGPUEnvironmentMap::ComputeEnvironment(
  vtkWebGPUConfiguration* configuration, WGPUCommandEncoder encoder, const Source& source)
{
  WGPUDevice device = configuration->GetDevice();
  const std::uint32_t size = this->EnvironmentSize;

  EnvironmentParameters environment{};
  std::copy(source.Right, source.Right + 3, environment.Right);
  std::copy(source.Up, source.Up + 3, environment.Up);
  std::copy(source.Front, source.Front + 3, environment.Front);
  environment.Size = size;
  environment.ConvertToLinear = source.ConvertToLinear ? 1 : 0;
  vtkWebGPU::Buffer environmentUniform =
    MakeUniform(configuration, environment, "IBLEnvironmentParameters");
  {
    vtkWebGPU::TextureView level0 = MakeView(
      this->EnvironmentCube, WGPUTextureViewDimension_2DArray, 0, 1, 6, "IBLEnvironmentLevel0");
    std::vector<WGPUBindGroupEntry> entries = {
      BindingEntry{ 0, environmentUniform, 0, sizeof(environment) }.GetAsBinding(),
      SamplerEntry(1, this->SourceSampler), ViewEntry(source.IsCubeMap ? 3 : 2, source.View),
      ViewEntry(4, level0)
    };
    vtkWebGPU::BindGroup bindGroup =
      MakeBindGroup(device, source.IsCubeMap ? this->CubeLayout : this->EnvironmentLayout, entries,
        "IBLEnvironmentBindGroup");
    Dispatch(encoder, source.IsCubeMap ? this->CubePipeline : this->EquirectangularPipeline,
      bindGroup, DispatchCount(size), DispatchCount(size), 6, "IBLEnvironmentToCube");
  }

  for (std::uint32_t level = 1; level < this->EnvironmentLevels; ++level)
  {
    const std::uint32_t levelSize = std::max(1u, size >> level);
    EnvironmentParameters downsample = environment;
    downsample.Size = levelSize;
    vtkWebGPU::Buffer uniform = MakeUniform(configuration, downsample, "IBLDownsampleParameters");
    vtkWebGPU::TextureView input = MakeView(this->EnvironmentCube, WGPUTextureViewDimension_2DArray,
      level - 1, 1, 6, "IBLDownsampleInput");
    vtkWebGPU::TextureView output = MakeView(
      this->EnvironmentCube, WGPUTextureViewDimension_2DArray, level, 1, 6, "IBLDownsampleOutput");
    vtkWebGPU::BindGroup bindGroup = MakeBindGroup(device, this->DownsampleLayout,
      { BindingEntry{ 0, uniform, 0, sizeof(downsample) }.GetAsBinding(), ViewEntry(4, output),
        ViewEntry(5, input) },
      "IBLDownsampleBindGroup");
    Dispatch(encoder, this->DownsamplePipeline, bindGroup, DispatchCount(levelSize),
      DispatchCount(levelSize), 6, "IBLDownsample");
  }

  const float texelSolidAngle =
    static_cast<float>(vtkMath::Pi()) / (6.0f * static_cast<float>(size * size));
  for (std::uint32_t level = 0; level < this->PrefilterLevels; ++level)
  {
    const std::uint32_t levelSize = std::max(1u, size >> level);
    const float roughness = static_cast<float>(level) / static_cast<float>(this->PrefilterLevels);
    constexpr float a = 0.65f;
    constexpr float b = 1.0f - a;
    PrefilterParameters prefilter{};
    prefilter.Roughness = roughness;
    prefilter.TexelSolidAngle = texelSolidAngle;
    prefilter.MaxSourceLevel = static_cast<float>(this->EnvironmentLevels - 1);
    prefilter.Size = levelSize;
    prefilter.SampleCount =
      static_cast<std::uint32_t>(roughness * PrefilterMaxSamples / (a * roughness + b)) + 1;
    vtkWebGPU::Buffer uniform = MakeUniform(configuration, prefilter, "IBLPrefilterParameters");
    vtkWebGPU::TextureView output = MakeView(
      this->PrefilterCube, WGPUTextureViewDimension_2DArray, level, 1, 6, "IBLPrefilterOutput");
    vtkWebGPU::BindGroup bindGroup = MakeBindGroup(device, this->PrefilterLayout,
      { BindingEntry{ 0, uniform, 0, sizeof(prefilter) }.GetAsBinding(),
        SamplerEntry(1, this->Sampler), ViewEntry(2, this->EnvironmentCubeView),
        ViewEntry(3, output) },
      "IBLPrefilterBindGroup");
    Dispatch(encoder, this->PrefilterPipeline, bindGroup, DispatchCount(levelSize),
      DispatchCount(levelSize), 6, "IBLPrefilter");
  }

  {
    const std::uint32_t sourceSize = std::min(size, 16u);
    IrradianceParameters irradiance{};
    irradiance.Step = IrradianceStep;
    irradiance.SourceLevel = std::log2(static_cast<float>(size / sourceSize));
    irradiance.Size = IrradianceSize;
    vtkWebGPU::Buffer uniform = MakeUniform(configuration, irradiance, "IBLIrradianceParameters");
    vtkWebGPU::TextureView output = MakeView(
      this->IrradianceCube, WGPUTextureViewDimension_2DArray, 0, 1, 6, "IBLIrradianceOutput");
    vtkWebGPU::BindGroup bindGroup = MakeBindGroup(device, this->IrradianceLayout,
      { BindingEntry{ 0, uniform, 0, sizeof(irradiance) }.GetAsBinding(),
        SamplerEntry(1, this->Sampler), ViewEntry(2, this->EnvironmentCubeView),
        ViewEntry(3, output) },
      "IBLIrradianceBindGroup");
    Dispatch(encoder, this->IrradiancePipeline, bindGroup, DispatchCount(IrradianceSize),
      DispatchCount(IrradianceSize), 6, "IBLIrradiance");
  }

  {
    const std::uint32_t harmonicsSize = std::min(size, HarmonicsSourceSize);
    const auto level = static_cast<std::uint32_t>(std::log2(size / harmonicsSize));
    const SizeParameters parameters{ harmonicsSize, 0, { 0, 0 } };
    vtkWebGPU::Buffer uniform = MakeUniform(configuration, parameters, "IBLHarmonicsParameters");
    vtkWebGPU::TextureView input = MakeView(
      this->EnvironmentCube, WGPUTextureViewDimension_2DArray, level, 1, 6, "IBLHarmonicsInput");
    vtkWebGPU::BindGroup bindGroup = MakeBindGroup(device, this->HarmonicsLayout,
      { BindingEntry{ 0, uniform, 0, sizeof(parameters) }.GetAsBinding(), ViewEntry(1, input),
        BindingEntry{ 2, this->HarmonicsBuffer, 0, GetHarmonicsBufferSize() }.GetAsBinding() },
      "IBLHarmonicsBindGroup");
    Dispatch(encoder, this->HarmonicsPipeline, bindGroup, 1, 1, 1, "IBLSphericalHarmonics");
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUEnvironmentMap::Update(vtkWebGPUConfiguration* configuration, const Source& source)
{
  if (!configuration || !source.View)
  {
    this->Ready = false;
    return false;
  }
  float basis[9];
  std::copy(source.Right, source.Right + 3, basis);
  std::copy(source.Up, source.Up + 3, basis + 3);
  std::copy(source.Front, source.Front + 3, basis + 6);
  const bool sourceChanged = !this->Ready || source.ModifiedTime != this->SourceTime ||
    source.View != this->SourceView || source.ConvertToLinear != this->SourceConvertToLinear ||
    source.IsCubeMap != this->SourceIsCubeMap ||
    (source.IsCubeMap && std::memcmp(basis, this->SourceBasis, sizeof(basis)) != 0);
  if (!sourceChanged)
  {
    return false;
  }
  if (!this->CreatePipelines(configuration))
  {
    this->Ready = false;
    return false;
  }

  std::uint32_t size = 1;
  const std::uint32_t target =
    std::clamp(source.Height, MinimumEnvironmentSize, MaximumEnvironmentSize);
  while (size < target)
  {
    size <<= 1;
  }
  const bool firstTime = !this->LookupTable;
  this->CreateTextures(configuration, size);

  WGPUDevice device = configuration->GetDevice();
  WGPUCommandEncoderDescriptor encoderDescriptor = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
  encoderDescriptor.label = WGPUStringView{ "IBLGeneration", WGPU_STRLEN };
  vtkWebGPU::CommandEncoder encoder =
    vtkWebGPU::CommandEncoder::Acquire(wgpuDeviceCreateCommandEncoder(device, &encoderDescriptor));
  if (firstTime)
  {
    this->ComputeLookupTable(configuration, encoder);
  }
  this->ComputeEnvironment(configuration, encoder, source);
  vtkWebGPU::CommandBuffer commands =
    vtkWebGPU::CommandBuffer::Acquire(wgpuCommandEncoderFinish(encoder, nullptr));
  vtkWebGPU::Queue queue = vtkWebGPU::Queue::Acquire(wgpuDeviceGetQueue(device));
  WGPUCommandBuffer rawCommands = commands;
  wgpuQueueSubmit(queue, 1, &rawCommands);

  this->SourceTime = source.ModifiedTime;
  this->SourceView = source.View;
  this->SourceConvertToLinear = source.ConvertToLinear;
  this->SourceIsCubeMap = source.IsCubeMap;
  std::copy(basis, basis + 9, this->SourceBasis);
  this->Ready = true;
  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPUEnvironmentMap::ReleaseGraphicsResources()
{
  *this = vtkWebGPUEnvironmentMap();
}

VTK_ABI_NAMESPACE_END
