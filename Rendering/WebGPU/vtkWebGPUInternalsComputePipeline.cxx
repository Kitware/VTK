// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUInternalsComputePipeline.h"
#include "vtkWebGPUComputePipeline.h"
#include "vtkWebGPUInternalsShaderModule.h" // for shader compilation internals

//------------------------------------------------------------------------------
wgpu::BufferUsage vtkWebGPUInternalsComputePipeline::ComputeBufferModeToBufferUsage(
  vtkWebGPUComputeBuffer::BufferMode mode)
{
  switch (mode)
  {
    case vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE:
    case vtkWebGPUComputeBuffer::READ_WRITE_COMPUTE_STORAGE:
      return wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage;

    case vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE:
      return wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage;

    case vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER:
      return wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;

    default:
      return wgpu::BufferUsage::None;
  }
}

//------------------------------------------------------------------------------
wgpu::BufferBindingType vtkWebGPUInternalsComputePipeline::ComputeBufferModeToBufferBindingType(
  vtkWebGPUComputeBuffer::BufferMode mode)
{
  switch (mode)
  {
    case vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE:
      return wgpu::BufferBindingType::ReadOnlyStorage;

    case vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_COMPUTE_STORAGE:
    case vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE:
      return wgpu::BufferBindingType::Storage;

    case vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER:
      return wgpu::BufferBindingType::Uniform;

    default:
      return wgpu::BufferBindingType::Undefined;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePipeline::AddBindGroupLayoutEntry(
  uint32_t bindGroup, uint32_t binding, vtkWebGPUComputeBuffer::BufferMode mode)
{
  wgpu::BufferBindingType bindingType = this->ComputeBufferModeToBufferBindingType(mode);

  vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper bglEntry{ binding,
    wgpu::ShaderStage::Compute, bindingType };

  this->BindGroupLayoutEntries[bindGroup].push_back(bglEntry);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePipeline::AddBindGroupEntry(wgpu::Buffer wgpuBuffer,
  uint32_t bindGroup, uint32_t binding, vtkWebGPUComputeBuffer::BufferMode mode, uint32_t offset)
{
  wgpu::BufferBindingType bindingType = this->ComputeBufferModeToBufferBindingType(mode);

  vtkWebGPUInternalsBindGroup::BindingInitializationHelper bgEntry{ binding, wgpuBuffer, offset };

  this->BindGroupEntries[bindGroup].push_back(bgEntry.GetAsBinding());
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePipeline::CreateDevice()
{
  if (this->Device != nullptr)
  {
    // The device already exists, it must have been given by SetDevice()
    return;
  }

  wgpu::DeviceDescriptor deviceDescriptor;
  deviceDescriptor.nextInChain = nullptr;
  deviceDescriptor.deviceLostCallback = &vtkWebGPUInternalsCallbacks::DeviceLostCallback;
  deviceDescriptor.label = Self->Label.c_str();
  this->Device = vtkWGPUContext::RequestDevice(this->Adapter, deviceDescriptor);
  this->Device.SetUncapturedErrorCallback(
    &vtkWebGPUInternalsCallbacks::UncapturedErrorCallback, nullptr);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePipeline::CreateAdapter()
{
  if (this->Adapter != nullptr)
  {
    // The adapter already exists, it must have been given by SetAdapter()
    return;
  }

#if defined(__APPLE__)
  wgpu::BackendType backendType = wgpu::BackendType::Metal;
#elif defined(_WIN32)
  wgpu::BackendType backendType = wgpu::BackendType::D3D12;
#else
  wgpu::BackendType backendType = wgpu::BackendType::Undefined;
#endif

  wgpu::RequestAdapterOptions adapterOptions;
  adapterOptions.backendType = backendType;
  adapterOptions.powerPreference = wgpu::PowerPreference::HighPerformance;
  this->Adapter = vtkWGPUContext::RequestAdapter(adapterOptions);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePipeline::CreateShaderModule()
{
  this->ShaderModule =
    vtkWebGPUInternalsShaderModule::CreateFromWGSL(this->Device, Self->ShaderSource);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePipeline::CreateBindGroupsAndLayouts()
{
  this->BindGroupLayouts.clear();
  this->BindGroups.clear();

  for (const auto& mapEntry : this->BindGroupLayoutEntries)
  {
    int bindGroup = mapEntry.first;

    const std::vector<wgpu::BindGroupLayoutEntry>& bglEntries =
      this->BindGroupLayoutEntries[mapEntry.first];
    const std::vector<wgpu::BindGroupEntry>& bgEntries = this->BindGroupEntries[mapEntry.first];

    this->BindGroupLayouts.push_back(CreateBindGroupLayout(this->Device, bglEntries));
    this->BindGroupsOrder.push_back(bindGroup);
    this->BindGroups.push_back(
      vtkWebGPUInternalsBindGroup::MakeBindGroup(this->Device, BindGroupLayouts.back(), bgEntries));
  }
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUInternalsComputePipeline::CreateBindGroupLayout(
  const wgpu::Device& device, const std::vector<wgpu::BindGroupLayoutEntry>& layoutEntries)
{
  wgpu::BindGroupLayout bgl =
    vtkWebGPUInternalsBindGroupLayout::MakeBindGroupLayout(device, layoutEntries);
  return bgl;
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePipeline::CreateComputePipeline()
{
  wgpu::ComputePipelineDescriptor computePipelineDescriptor;
  computePipelineDescriptor.compute.constantCount = 0;
  computePipelineDescriptor.compute.constants = nullptr;
  computePipelineDescriptor.compute.entryPoint = Self->ShaderEntryPoint.c_str();
  computePipelineDescriptor.compute.module = this->ShaderModule;
  computePipelineDescriptor.compute.nextInChain = nullptr;
  computePipelineDescriptor.label = this->WGPUComputePipelineLabel.c_str();
  computePipelineDescriptor.layout = CreateComputePipelineLayout();

  this->ComputePipeline = this->Device.CreateComputePipeline(&computePipelineDescriptor);
}

//------------------------------------------------------------------------------
wgpu::PipelineLayout vtkWebGPUInternalsComputePipeline::CreateComputePipelineLayout()
{
  wgpu::PipelineLayoutDescriptor computePipelineLayoutDescriptor;
  computePipelineLayoutDescriptor.bindGroupLayoutCount = this->BindGroupLayouts.size();
  computePipelineLayoutDescriptor.bindGroupLayouts = this->BindGroupLayouts.data();
  computePipelineLayoutDescriptor.nextInChain = nullptr;

  return this->Device.CreatePipelineLayout(&computePipelineLayoutDescriptor);
}

//------------------------------------------------------------------------------
wgpu::CommandEncoder vtkWebGPUInternalsComputePipeline::CreateCommandEncoder()
{
  wgpu::CommandEncoderDescriptor commandEncoderDescriptor;
  commandEncoderDescriptor.label = this->WGPUCommandEncoderLabel.c_str();

  return this->Device.CreateCommandEncoder(&commandEncoderDescriptor);
}

//------------------------------------------------------------------------------
wgpu::ComputePassEncoder vtkWebGPUInternalsComputePipeline::CreateComputePassEncoder(
  const wgpu::CommandEncoder& commandEncoder)
{
  wgpu::ComputePassDescriptor computePassDescriptor;
  computePassDescriptor.nextInChain = nullptr;
  computePassDescriptor.timestampWrites = 0;
  return commandEncoder.BeginComputePass(&computePassDescriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePipeline::DispatchComputePass(
  unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ)
{
  if (groupsX * groupsY * groupsZ == 0)
  {
    vtkLogF(ERROR,
      "Invalid number of workgroups when dispatching compute pipeline \"%s\". Work groups "
      "sizes "
      "(X, Y, Z) were: (%d, %d, %d) but no dimensions can be 0.",
      Self->Label.c_str(), groupsX, groupsY, groupsZ);

    return;
  }

  wgpu::CommandEncoder commandEncoder = this->CreateCommandEncoder();

  wgpu::ComputePassEncoder computePassEncoder = CreateComputePassEncoder(commandEncoder);
  computePassEncoder.SetPipeline(this->ComputePipeline);
  for (int bindGroupIndex = 0; bindGroupIndex < this->BindGroups.size(); bindGroupIndex++)
  {
    computePassEncoder.SetBindGroup(bindGroupIndex, this->BindGroups[bindGroupIndex], 0, nullptr);
  }
  computePassEncoder.DispatchWorkgroups(groupsX, groupsY, groupsZ);
  computePassEncoder.End();

  this->SubmitCommandEncoderToQueue(commandEncoder);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePipeline::SubmitCommandEncoderToQueue(
  const wgpu::CommandEncoder& commandEncoder)
{
  wgpu::CommandBuffer commandBuffer = commandEncoder.Finish();
  this->Device.GetQueue().Submit(1, &commandBuffer);
}
