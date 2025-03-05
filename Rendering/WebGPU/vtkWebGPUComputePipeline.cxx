// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputePipeline.h"
#include "Private/vtkWebGPUComputePassInternals.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputePipeline);

//------------------------------------------------------------------------------
vtkWebGPUComputePipeline::vtkWebGPUComputePipeline() = default;

//------------------------------------------------------------------------------
vtkWebGPUComputePipeline::~vtkWebGPUComputePipeline()
{
  this->ReleaseResources();
}

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
  this->EnsureConfigured();

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
  this->EnsureConfigured();

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
  this->EnsureConfigured();

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
  this->EnsureConfigured();

  for (vtkWebGPUComputePass* computePass : this->ComputePasses)
  {
    computePass->Dispatch();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::Update()
{
  this->EnsureConfigured();

  wgpu::QueueWorkDoneStatus workStatus = wgpu::QueueWorkDoneStatus::Error;
  bool done = false;
  this->WGPUConfiguration->GetDevice().GetQueue().OnSubmittedWorkDone(
    wgpu::CallbackMode::AllowProcessEvents,
    [&workStatus, &done](wgpu::QueueWorkDoneStatus status)
    {
      workStatus = status;
      done = true;
    });
  while (!done)
  {
    this->WGPUConfiguration->ProcessEvents();
  }
  if (workStatus != wgpu::QueueWorkDoneStatus::Success)
  {
    vtkErrorMacro(<< "Submitted work did not complete!");
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::EnsureConfigured()
{
  if (this->WGPUConfiguration == nullptr)
  {
    this->WGPUConfiguration = vtk::TakeSmartPointer(vtkWebGPUConfiguration::New());
    this->WGPUConfiguration->Initialize();
  }
  else if (this->WGPUConfiguration->GetDevice() == nullptr)
  {
    this->WGPUConfiguration->Initialize();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::ReleaseResources()
{
  for (vtkWebGPUComputePass* computePass : this->ComputePasses)
  {
    computePass->ReleaseResources();
  }

  this->ComputePasses.clear();

  this->RegisteredBuffers.clear();
  this->RegisteredTextures.clear();
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
