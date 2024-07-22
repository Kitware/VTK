// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputePipeline.h"
#include "Private/vtkWebGPUComputePassInternals.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputePipeline);

//------------------------------------------------------------------------------
vtkWebGPUComputePipeline::vtkWebGPUComputePipeline()
{
  this->WGPUConfiguration = vtk::TakeSmartPointer(vtkWebGPUConfiguration::New());
  this->WGPUConfiguration->Initialize();
}

//------------------------------------------------------------------------------
vtkWebGPUComputePipeline::~vtkWebGPUComputePipeline() = default;

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Label: " << this->Label << '\n';
  os << indent << "WGPUConfiguration: " << this->WGPUConfiguration->GetObjectDescription();
  if (this->WGPUConfiguration)
  {
    this->WGPUConfiguration->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "(nullptr)\n";
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputePass> vtkWebGPUComputePipeline::CreateComputePass()
{
  vtkSmartPointer<vtkWebGPUComputePass> computePass = vtkSmartPointer<vtkWebGPUComputePass>::New();
  computePass->Internals->SetWGPUConfiguration(this->WGPUConfiguration);
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
  this->WGPUConfiguration->GetDevice().GetQueue().OnSubmittedWorkDone([](WGPUQueueWorkDoneStatus, void* userdata)
  { 
    *static_cast<bool*>(userdata) = true; 
  }, &workDone);
  // clang-format on

  // Waiting for the compute pipeline to complete all its work. The callback that will set workDone
  // to true will be called when all the work has been dispatched to the GPU and completed.
  while (!workDone)
  {
    this->WGPUConfiguration->ProcessEvents();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::SetWGPUConfiguration(vtkWebGPUConfiguration* config)
{
  vtkSetSmartPointerBodyMacro(WGPUConfiguration, vtkWebGPUConfiguration, config);
  for (const auto& computePass : this->ComputePasses)
  {
    computePass->Internals->SetWGPUConfiguration(config);
  }
}

VTK_ABI_NAMESPACE_END
