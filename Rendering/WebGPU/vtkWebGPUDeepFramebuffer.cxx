// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUDeepFramebuffer.h"

#include "Private/vtkWebGPUHandle.h"
#include "vtkObjectFactory.h"

#include "vtkWebGPUConfiguration.h"
#include <algorithm> // for std::min

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebGPUDeepFramebuffer);

vtkWebGPUDeepFramebuffer::vtkWebGPUDeepFramebuffer() = default;

vtkWebGPUDeepFramebuffer::~vtkWebGPUDeepFramebuffer()
{
  this->ReleaseGraphicsResources();
}

void vtkWebGPUDeepFramebuffer::SetWGPUConfiguration(vtkWebGPUConfiguration* config)
{
  if (this->WGPUConfiguration != config)
  {
    this->ReleaseGraphicsResources();
    this->WGPUConfiguration = config;
    this->Modified();
  }
}

bool vtkWebGPUDeepFramebuffer::Resize(int width, int height)
{
  if (width <= 0 || height <= 0)
  {
    vtkErrorMacro(<< "Cannot size a deep framebuffer to " << width << "x" << height);
    return false;
  }
  if (this->WGPUConfiguration == nullptr)
  {
    vtkErrorMacro(<< "Cannot allocate a deep framebuffer without a vtkWebGPUConfiguration.");
    return false;
  }
  if (width == this->Width && height == this->Height && this->HeadPointersBuffer != nullptr)
  {
    return true;
  }

  this->ReleaseGraphicsResources();

  const std::uint64_t pixelCount = static_cast<std::uint64_t>(width) * height;
  std::uint64_t nodeCount = pixelCount * this->AverageLayersPerPixel;

  std::uint64_t maxNodeCount = InvalidIndex - 1;

  if (WGPUDevice device = this->WGPUConfiguration->GetDevice())
  {
    WGPULimits limits = WGPU_LIMITS_INIT;
    wgpuDeviceGetLimits(device, &limits);
    if (limits.maxStorageBufferBindingSize > 0)
    {
      maxNodeCount = std::min(maxNodeCount, limits.maxStorageBufferBindingSize / FragmentNodeSize);
    }
  }

  if (maxNodeCount == 0)
  {
    vtkWarningMacro(<< "This device cannot bind a storage buffer large enough for even one "
                       "fragment node.");
    return false;
  }

  if (nodeCount > maxNodeCount)
  {
    vtkDebugMacro(<< "Capping the deep framebuffer for " << width << "x" << height << " at "
                  << maxNodeCount << " fragment nodes, " << nodeCount << " requested.");
    nodeCount = maxNodeCount;
  }

  const auto storage =
    static_cast<WGPUBufferUsage>(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);

  const std::string headLabel = "DeepFramebuffer-HeadPointers-" + this->GetObjectDescription();
  this->HeadPointersBuffer = this->WGPUConfiguration->CreateBuffer(
    pixelCount * sizeof(std::uint32_t), storage, /*mappedAtCreation=*/false, headLabel.c_str());

  const std::string nodeLabel = "DeepFramebuffer-FragmentNodes-" + this->GetObjectDescription();
  this->FragmentNodesBuffer = this->WGPUConfiguration->CreateBuffer(
    nodeCount * FragmentNodeSize, storage, /*mappedAtCreation=*/false, nodeLabel.c_str());

  const std::string counterLabel =
    "DeepFramebuffer-FragmentCounter-" + this->GetObjectDescription();
  this->FragmentCounterBuffer = this->WGPUConfiguration->CreateBuffer(
    sizeof(std::uint32_t), storage, /*mappedAtCreation=*/false, counterLabel.c_str());

  if (this->HeadPointersBuffer == nullptr || this->FragmentNodesBuffer == nullptr ||
    this->FragmentCounterBuffer == nullptr)
  {
    vtkWarningMacro(<< "Failed to allocate the deep framebuffer buffers for " << width << "x"
                    << height << ".");
    this->ReleaseGraphicsResources();
    return false;
  }

  this->Width = width;
  this->Height = height;
  this->FragmentNodeCapacity = static_cast<std::uint32_t>(nodeCount);

  this->Clear();
  this->Modified();
  return true;
}

void vtkWebGPUDeepFramebuffer::Clear()
{
  if (this->HeadPointersBuffer == nullptr || this->WGPUConfiguration == nullptr)
  {
    return;
  }

  const std::size_t pixelCount = static_cast<std::size_t>(this->Width) * this->Height;
  const std::vector<std::uint32_t> emptyHeads(pixelCount, InvalidIndex);
  this->WGPUConfiguration->WriteBuffer(this->HeadPointersBuffer, 0, emptyHeads.data(),
    emptyHeads.size() * sizeof(std::uint32_t), "DeepFramebuffer head pointers reset");

  const std::uint32_t zero = 0;
  this->WGPUConfiguration->WriteBuffer(
    this->FragmentCounterBuffer, 0, &zero, sizeof(zero), "DeepFramebuffer counter reset");
}

void vtkWebGPUDeepFramebuffer::ReleaseGraphicsResources()
{
  vtkWebGPU::ReleaseAndNull(this->HeadPointersBuffer, wgpuBufferRelease);
  vtkWebGPU::ReleaseAndNull(this->FragmentNodesBuffer, wgpuBufferRelease);
  vtkWebGPU::ReleaseAndNull(this->FragmentCounterBuffer, wgpuBufferRelease);

  this->Width = 0;
  this->Height = 0;
  this->FragmentNodeCapacity = 0;
}

void vtkWebGPUDeepFramebuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Width: " << this->Width << '\n';
  os << indent << "Height: " << this->Height << '\n';
  os << indent << "AverageLayersPerPixel: " << this->AverageLayersPerPixel << '\n';
  os << indent << "FragmentNodeCapacity: " << this->FragmentNodeCapacity << '\n';
  os << indent << "HeadPointersBuffer: " << this->HeadPointersBuffer << '\n';
  os << indent << "FragmentNodesBuffer: " << this->FragmentNodesBuffer << '\n';
  os << indent << "FragmentCounterBuffer: " << this->FragmentCounterBuffer << '\n';
}
VTK_ABI_NAMESPACE_END
