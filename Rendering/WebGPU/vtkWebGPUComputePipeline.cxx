// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputePipeline.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUInternalsCallbacks.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputePipeline);

//------------------------------------------------------------------------------
vtkWebGPUComputePipeline::vtkWebGPUComputePipeline()
{
  this->CreateAdapter();
  this->CreateDevice();
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Adapter: " << this->Adapter.Get() << std::endl;
  os << indent << "Device: " << this->Device.Get() << std::endl;
  os << indent << "Label: " << this->Label << std::endl;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputePass> vtkWebGPUComputePipeline::CreateComputePass()
{
  vtkSmartPointer<vtkWebGPUComputePass> computePass = vtkSmartPointer<vtkWebGPUComputePass>::New();
  computePass->Internals->SetDevice(this->Device);
  computePass->Internals->SetAssociatedPipeline(this);

  this->ComputePasses.push_back(computePass);

  return computePass;
}

//------------------------------------------------------------------------------
const std::vector<vtkSmartPointer<vtkWebGPUComputePass>>&
vtkWebGPUComputePipeline::GetComputePasses() const
{
  return this->ComputePasses;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::RegisterBuffer(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer wgpuBuffer)
{
  if (this->RegisteredBuffers.find(buffer) != this->RegisteredBuffers.end())
  {
    // If we're registering a new wgpu::Buffer for an existing (already registered)
    // vtkWebGPUComputeBuffer, we're going to have to make sure that all compute passes that are
    // using this vtkWebGPUComputeBuffer now use the new wgpu::Buffer that we're registering

    for (vtkSmartPointer<vtkWebGPUComputePass> computePass : this->ComputePasses)
    {
      int computePassBufferIndex = 0;

      computePass->Internals->UpdateWebGPUBuffer(buffer, wgpuBuffer);
    }
  }

  this->RegisteredBuffers[buffer] = wgpuBuffer;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::RegisterTexture(
  vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture wgpuTexture)
{
  if (this->RegisteredTextures.find(texture) != this->RegisteredTextures.end())
  {
    // If we're registering a new wgpu::Texture for an existing (already registered)
    // vtkWebGPUComputeTexture, we're going to have to make sure that all compute passes that are
    // using this vtkWebGPUComputeTexture now use the new wgpu::Texture that we're registering

    for (vtkSmartPointer<vtkWebGPUComputePass> computePass : this->ComputePasses)
    {
      computePass->Internals->UpdateComputeTextureAndViews(texture, wgpuTexture);
    }
  }

  this->RegisteredTextures[texture] = wgpuTexture;
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePipeline::GetRegisteredBuffer(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer& wgpuBuffer)
{
  auto find = this->RegisteredBuffers.find(buffer);
  if (find != this->RegisteredBuffers.end())
  {
    wgpuBuffer = find->second;

    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePipeline::GetRegisteredTexture(
  vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture& wgpuTexture)
{
  auto find = this->RegisteredTextures.find(texture);
  if (find != this->RegisteredTextures.end())
  {
    wgpuTexture = find->second;

    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::DispatchAllPasses()
{
  for (vtkWebGPUComputePass* computePass : this->ComputePasses)
  {
    computePass->Dispatch();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::Update()
{
  bool workDone = false;

  // clang-format off
  this->Device.GetQueue().OnSubmittedWorkDone([](WGPUQueueWorkDoneStatus, void* userdata)
  { 
    *static_cast<bool*>(userdata) = true; 
  }, &workDone);
  // clang-format on

  // Waiting for the compute pipeline to complete all its work. The callback that will set workDone
  // to true will be called when all the work has been dispatched to the GPU and completed.
  while (!workDone)
  {
    vtkWGPUContext::WaitABit();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::CreateAdapter()
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
  wgpu::BackendType backendType = wgpu::BackendType::Vulkan;
#endif

  wgpu::RequestAdapterOptions adapterOptions;
  adapterOptions.backendType = backendType;
  adapterOptions.powerPreference = wgpu::PowerPreference::HighPerformance;
  this->Adapter = vtkWGPUContext::RequestAdapter(adapterOptions);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::CreateDevice()
{
  if (this->Device != nullptr)
  {
    // The device already exists, it must have been given by SetDevice()
    return;
  }

  wgpu::DeviceDescriptor deviceDescriptor;
  deviceDescriptor.nextInChain = nullptr;
  deviceDescriptor.deviceLostCallback = &vtkWebGPUInternalsCallbacks::DeviceLostCallback;
  deviceDescriptor.label = this->Label.c_str();
  this->Device = vtkWGPUContext::RequestDevice(this->Adapter, deviceDescriptor);
  this->Device.SetUncapturedErrorCallback(
    &vtkWebGPUInternalsCallbacks::UncapturedErrorCallback, nullptr);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::SetDevice(wgpu::Device device)
{
  this->Device = device;

  for (vtkWebGPUComputePass* computePass : this->ComputePasses)
  {
    computePass->Internals->SetDevice(device);
  }
}

VTK_ABI_NAMESPACE_END
