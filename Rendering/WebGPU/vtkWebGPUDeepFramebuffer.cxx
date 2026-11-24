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

struct vtkWebGPUDeepFramebuffer::Readback
{
  bool InFlight = true;
  vtkTypeUInt64 Frame = 0;
  vtkTypeUInt64 Generation = 0;
  vtkTypeUInt32 Capacity = 0;
  vtkTypeUInt32 Count = 0;
  int Status = ReadbackFailed;
  WGPUBuffer Buffer = nullptr; // borrowed from the map request while InFlight
};

namespace
{
// Buffer creation may return a nonnull error object. Check both scopes before
// publishing new storage; this synchronization occurs only on reallocation.
bool CheckAllocation(vtkWebGPUConfiguration* config)
{
  bool success = true;
  for (int scope = 0; scope < 2; ++scope)
  {
    struct Result
    {
      bool Done = false;
      bool Success = false;
    } result;
    WGPUPopErrorScopeCallbackInfo callback = WGPU_POP_ERROR_SCOPE_CALLBACK_INFO_INIT;
    callback.mode = WGPUCallbackMode_AllowProcessEvents;
    callback.userdata1 = &result;
    callback.callback =
      [](WGPUPopErrorScopeStatus status, WGPUErrorType type, WGPUStringView, void* data, void*)
    {
      auto* output = static_cast<Result*>(data);
      output->Success = status == WGPUPopErrorScopeStatus_Success && type == WGPUErrorType_NoError;
      output->Done = true;
    };
    wgpuDevicePopErrorScope(config->GetDevice(), callback);
    while (!result.Done)
    {
      config->ProcessEvents();
    }
    success &= result.Success;
  }
  return success;
}
}

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
  const std::uint64_t pixelCount = static_cast<std::uint64_t>(width) * height;
  std::uint64_t nodeCount = pixelCount * this->AverageLayersPerPixel;

  // Consume feedback without waiting for the GPU. Retain grown capacity until
  // the dimensions or explicit budget changes, avoiding allocation oscillation.
  if (this->Statistics && this->Statistics->InFlight)
  {
    this->WGPUConfiguration->ProcessEvents();
  }
  this->UpdateStatistics();
  const bool sameSettings = width == this->Width && height == this->Height &&
    this->AllocatedAverageLayers == this->AverageLayersPerPixel &&
    this->AllocatedMemoryLimit == this->MaximumMemoryInBytes && this->HeadPointersBuffer != nullptr;
  if (!sameSettings)
  {
    this->FailedGrowthCapacity = 0;
  }
  if (this->AutoResize && sameSettings)
  {
    nodeCount = std::max<std::uint64_t>(nodeCount, this->FragmentNodeCapacity);
    if (this->Statistics && !this->Statistics->InFlight &&
      this->Statistics->Generation == this->AllocationGeneration &&
      (this->Statistics->Status & FragmentPoolOverflow) != 0)
    {
      const std::uint64_t requested = this->Statistics->Count;
      nodeCount = std::max(nodeCount, requested + requested / 4 + 1);
    }
  }

  std::uint64_t maxNodeCount = InvalidIndex - 1;

  if (WGPUDevice device = this->WGPUConfiguration->GetDevice())
  {
    WGPULimits limits = WGPU_LIMITS_INIT;
    if (wgpuDeviceGetLimits(device, &limits) != WGPUStatus_Success ||
      pixelCount * sizeof(std::uint32_t) > limits.maxStorageBufferBindingSize)
    {
      vtkErrorMacro(<< "Cannot bind the deep framebuffer head pointers.");
      return false;
    }
    maxNodeCount = std::min(maxNodeCount,
      std::min(limits.maxStorageBufferBindingSize, limits.maxBufferSize) / FragmentNodeSize);
  }
  else
  {
    vtkErrorMacro(<< "Cannot allocate a deep framebuffer without a device.");
    return false;
  }
  if (this->MaximumMemoryInBytes != 0)
  {
    maxNodeCount =
      std::min<std::uint64_t>(maxNodeCount, this->MaximumMemoryInBytes / FragmentNodeSize);
  }
  if (this->FailedGrowthCapacity != 0)
  {
    maxNodeCount = std::min<std::uint64_t>(maxNodeCount, this->FailedGrowthCapacity);
  }

  if (maxNodeCount == 0)
  {
    vtkWarningMacro(<< "The device or configured memory limit cannot accommodate even one "
                       "fragment node.");
    return false;
  }
  this->MaximumFragmentNodeCapacity = static_cast<vtkTypeUInt32>(maxNodeCount);

  if (nodeCount > maxNodeCount)
  {
    vtkDebugMacro(<< "Capping the deep framebuffer for " << width << "x" << height << " at "
                  << maxNodeCount << " fragment nodes, " << nodeCount << " requested.");
    nodeCount = maxNodeCount;
  }

  if (width == this->Width && height == this->Height && nodeCount == this->FragmentNodeCapacity &&
    this->HeadPointersBuffer != nullptr)
  {
    this->AllocatedAverageLayers = this->AverageLayersPerPixel;
    this->AllocatedMemoryLimit = this->MaximumMemoryInBytes;
    return true;
  }
  const bool automaticGrowth =
    this->AutoResize && sameSettings && nodeCount > this->FragmentNodeCapacity;

  // Keep working storage until the entire replacement has been validated.
  // Scene bindings and render bundles also retain the old buffers until the
  // renderer sees a new allocation generation and refreshes those references.
  wgpuDevicePushErrorScope(this->WGPUConfiguration->GetDevice(), WGPUErrorFilter_OutOfMemory);
  wgpuDevicePushErrorScope(this->WGPUConfiguration->GetDevice(), WGPUErrorFilter_Validation);
  const auto storage =
    static_cast<WGPUBufferUsage>(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);

  const std::string headLabel = "DeepFramebuffer-HeadPointers-" + this->GetObjectDescription();
  auto heads = vtkWebGPU::Buffer::Acquire(this->WGPUConfiguration->CreateBuffer(
    pixelCount * sizeof(std::uint32_t), storage, /*mappedAtCreation=*/false, headLabel.c_str()));

  const std::string nodeLabel = "DeepFramebuffer-FragmentNodes-" + this->GetObjectDescription();
  auto nodes = vtkWebGPU::Buffer::Acquire(this->WGPUConfiguration->CreateBuffer(
    nodeCount * FragmentNodeSize, storage, /*mappedAtCreation=*/false, nodeLabel.c_str()));

  const std::string counterLabel =
    "DeepFramebuffer-FragmentCounter-" + this->GetObjectDescription();
  auto counter = vtkWebGPU::Buffer::Acquire(this->WGPUConfiguration->CreateBuffer(
    2 * sizeof(std::uint32_t), storage | WGPUBufferUsage_CopySrc,
    /*mappedAtCreation=*/false, counterLabel.c_str()));

  const bool allocated = CheckAllocation(this->WGPUConfiguration);
  if (!allocated || !heads || !nodes || !counter)
  {
    if (automaticGrowth)
    {
      this->FailedGrowthCapacity = this->FragmentNodeCapacity;
      this->MaximumFragmentNodeCapacity = this->FragmentNodeCapacity;
      vtkWarningMacro(<< "Could not grow the deep framebuffer; retaining "
                      << this->FragmentNodeCapacity
                      << " nodes until its dimensions or budget change.");
      return true;
    }
    vtkWarningMacro(<< "Failed to allocate the deep framebuffer buffers for " << width << "x"
                    << height << ".");
    this->ReleaseGraphicsResources();
    return false;
  }

  this->ReleaseGraphicsResources();
  this->HeadPointersBuffer = heads.Release();
  this->FragmentNodesBuffer = nodes.Release();
  this->FragmentCounterBuffer = counter.Release();
  this->Width = width;
  this->Height = height;
  this->FragmentNodeCapacity = static_cast<std::uint32_t>(nodeCount);
  this->AllocatedAverageLayers = this->AverageLayersPerPixel;
  this->AllocatedMemoryLimit = this->MaximumMemoryInBytes;
  ++this->AllocationGeneration;

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

  const std::uint32_t zero[2] = { 0, 0 };
  this->WGPUConfiguration->WriteBuffer(
    this->FragmentCounterBuffer, 0, zero, sizeof(zero), "DeepFramebuffer counters reset");
  ++this->FrameNumber;
}

void vtkWebGPUDeepFramebuffer::RequestStatistics()
{
  if (!this->Statistics ||
    (!this->Statistics->InFlight &&
      (this->Statistics->Status != Complete ||
        this->FrameNumber - this->Statistics->Frame >=
          static_cast<vtkTypeUInt64>(this->StatisticsInterval))))
  {
    this->ReadStatistics();
  }
}

int vtkWebGPUDeepFramebuffer::ReadStatistics(bool wait)
{
  auto* config = this->WGPUConfiguration.GetPointer();
  if (!config || !this->FragmentCounterBuffer)
  {
    return ReadbackFailed;
  }
  config->ProcessEvents();
  if (wait)
  {
    while (this->Statistics && this->Statistics->InFlight)
    {
      config->ProcessEvents();
    }
  }
  if (this->Statistics && this->Statistics->InFlight)
  {
    return Pending;
  }
  this->UpdateStatistics();
  if (!this->Statistics || this->Statistics->Frame != this->FrameNumber)
  {
    auto state = std::make_shared<Readback>();
    state->Frame = this->FrameNumber;
    state->Generation = this->AllocationGeneration;
    state->Capacity = this->FragmentNodeCapacity;
    this->Statistics = state;
    if (!this->StatisticsReadbackBuffer)
    {
      this->StatisticsReadbackBuffer = config->CreateBuffer(
        8, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead, false, "DeepFramebuffer statistics");
    }
    auto buffer = vtkWebGPU::Buffer::Reference(this->StatisticsReadbackBuffer);
    auto encoder = vtkWebGPU::CommandEncoder::Acquire(
      wgpuDeviceCreateCommandEncoder(config->GetDevice(), nullptr));
    if (!buffer || !encoder)
    {
      state->InFlight = false;
      return ReadbackFailed;
    }
    wgpuCommandEncoderCopyBufferToBuffer(encoder, this->FragmentCounterBuffer, 0, buffer, 0, 8);
    auto commands = vtkWebGPU::CommandBuffer::Acquire(wgpuCommandEncoderFinish(encoder, nullptr));
    auto queue = vtkWebGPU::Queue::Acquire(wgpuDeviceGetQueue(config->GetDevice()));
    if (!commands || !queue)
    {
      state->InFlight = false;
      return ReadbackFailed;
    }
    WGPUCommandBuffer rawCommands = commands;
    wgpuQueueSubmit(queue, 1, &rawCommands);

    // The callback owns its state and configuration, never a renderer or framebuffer.
    struct Request
    {
      std::shared_ptr<Readback> State;
      vtkSmartPointer<vtkWebGPUConfiguration> Configuration;
      WGPUBuffer Buffer;
    };
    auto* request = new Request{ state, config, buffer.Release() };
    state->Buffer = request->Buffer;
    config->IncrementActiveBufferMapCount();
    WGPUBufferMapCallbackInfo callback = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
    callback.mode = WGPUCallbackMode_AllowProcessEvents;
    callback.userdata1 = request;
    callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* data, void*)
    {
      auto* requestData = static_cast<Request*>(data);
      auto& result = *requestData->State;
      if (status == WGPUMapAsyncStatus_Success)
      {
        const auto* values = static_cast<const std::uint32_t*>(
          wgpuBufferGetConstMappedRange(requestData->Buffer, 0, 8));
        if (values)
        {
          result.Count = values[0];
          result.Status = static_cast<int>(values[1]);
          if (result.Count > result.Capacity)
          {
            result.Status |= FragmentPoolOverflow;
          }
        }
        wgpuBufferUnmap(requestData->Buffer);
      }
      result.InFlight = false;
#ifdef __EMSCRIPTEN__
      wgpuBufferRelease(requestData->Buffer);
#endif
      requestData->Configuration->DecrementActiveBufferMapCount();
      requestData->Configuration->DeferBufferRelease(requestData->Buffer);
      delete requestData;
    };
#ifdef __EMSCRIPTEN__
    // Match the extra reference used by compute readback for browser map callbacks.
    wgpuBufferAddRef(request->Buffer);
#endif
    wgpuBufferMapAsync(request->Buffer, WGPUMapMode_Read, 0, 8, callback);
  }
  if (wait)
  {
    while (this->Statistics->InFlight)
    {
      config->ProcessEvents();
    }
  }
  if (this->Statistics->InFlight)
  {
    return Pending;
  }
  this->UpdateStatistics();
  return this->LastFrameStatus;
}

void vtkWebGPUDeepFramebuffer::UpdateStatistics()
{
  if (!this->Statistics || this->Statistics->InFlight ||
    this->Statistics->Generation != this->AllocationGeneration)
  {
    return;
  }
  this->LastFrameStatus = this->Statistics->Status;
  this->LastFragmentCount = this->Statistics->Count;
  if ((this->LastFrameStatus & FragmentPoolOverflow & ~this->WarnedStatus) != 0)
  {
    vtkWarningMacro(<< "Deep framebuffer overflow: " << this->LastFragmentCount << " fragments for "
                    << this->Statistics->Capacity << " nodes. Increase the budget or use "
                    << "RenderWithCheckedDeepFramebuffer() to check recovery before capture.");
    this->WarnedStatus |= FragmentPoolOverflow;
  }
}

void vtkWebGPUDeepFramebuffer::ReleaseGraphicsResources()
{
  if (this->Statistics && this->Statistics->InFlight)
  {
    // Cancel and deliver the callback before the configuration/device can be
    // finalized. Deferred buffer release must run outside the map callback.
    wgpuBufferUnmap(this->Statistics->Buffer);
    while (this->Statistics->InFlight)
    {
      this->WGPUConfiguration->ProcessEvents();
    }
  }
  vtkWebGPU::ReleaseAndNull(this->HeadPointersBuffer, wgpuBufferRelease);
  vtkWebGPU::ReleaseAndNull(this->FragmentNodesBuffer, wgpuBufferRelease);
  vtkWebGPU::ReleaseAndNull(this->FragmentCounterBuffer, wgpuBufferRelease);
  vtkWebGPU::ReleaseAndNull(this->StatisticsReadbackBuffer, wgpuBufferRelease);

  this->Width = 0;
  this->Height = 0;
  this->FragmentNodeCapacity = 0;
  this->Statistics.reset();
  this->LastFrameStatus = Pending;
  this->LastFragmentCount = 0;
  this->FailedGrowthCapacity = 0;
}

void vtkWebGPUDeepFramebuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Width: " << this->Width << '\n';
  os << indent << "Height: " << this->Height << '\n';
  os << indent << "AverageLayersPerPixel: " << this->AverageLayersPerPixel << '\n';
  os << indent << "FragmentNodeCapacity: " << this->FragmentNodeCapacity << '\n';
  os << indent << "MaximumMemoryInBytes: " << this->MaximumMemoryInBytes << '\n';
  os << indent << "AutoResize: " << this->AutoResize << '\n';
  os << indent << "StatisticsInterval: " << this->StatisticsInterval << '\n';
  os << indent << "LastFrameStatus: " << this->LastFrameStatus << '\n';
  os << indent << "LastFragmentCount: " << this->LastFragmentCount << '\n';
  os << indent << "HeadPointersBuffer: " << this->HeadPointersBuffer << '\n';
  os << indent << "FragmentNodesBuffer: " << this->FragmentNodesBuffer << '\n';
  os << indent << "FragmentCounterBuffer: " << this->FragmentCounterBuffer << '\n';
}
VTK_ABI_NAMESPACE_END
