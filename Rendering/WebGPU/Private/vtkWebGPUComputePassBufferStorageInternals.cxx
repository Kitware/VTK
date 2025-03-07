// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkWebGPUComputePassBufferStorageInternals.h"
#include "Private/vtkWebGPUComputeBufferInternals.h"
#include "Private/vtkWebGPUComputePassInternals.h"
#include "vtkWebGPUHelpers.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputePassBufferStorageInternals);

/**
 * Structure used to pass data to the asynchronous callback of wgpu::Buffer.MapAsync()
 */
struct InternalMapBufferAsyncData
{
  // Buffer currently being mapped
  wgpu::Buffer buffer;
  // Label of the buffer currently being mapped. Used for printing errors
  std::string bufferLabel;
  // Size of the buffer being mapped in bytes
  vtkIdType byteSize;

  // Userdata passed to userCallback. This is typically the structure that contains the CPU-side
  // buffer into which the data of the mapped buffer will be copied
  void* userdata;

  // The callback given by the user that will be called once the buffer is mapped. The user will
  // usually use their callback to copy the data from the mapped buffer into a CPU-side buffer that
  // will then use the result of the compute shader in the rest of the application
  vtkWebGPUComputePass::BufferMapAsyncCallback userCallback;
};

//------------------------------------------------------------------------------
vtkWebGPUComputePassBufferStorageInternals::~vtkWebGPUComputePassBufferStorageInternals()
{
  this->ReleaseResources();
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassBufferStorageInternals::SetComputePass(
  vtkWeakPointer<vtkWebGPUComputePass> parentComputePass)
{
  this->ParentComputePass = parentComputePass;
  if (parentComputePass)
  {
    this->ParentPassWGPUConfiguration = parentComputePass->Internals->WGPUConfiguration;
  }
}

//------------------------------------------------------------------------------
int vtkWebGPUComputePassBufferStorageInternals::AddBuffer(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer)
{
  // Giving the buffer a default label if it doesn't have one already
  if (buffer->GetLabel().empty())
  {
    buffer->SetLabel("Buffer " + std::to_string(this->Buffers.size()));
  }

  std::string bufferLabel = buffer->GetLabel();
  const char* bufferLabelCStr = bufferLabel.c_str();

  if (!this->CheckBufferCorrectness(buffer))
  {
    return -1;
  }

  wgpu::Buffer wgpuBuffer;
  vtkWebGPUComputeBuffer::BufferMode mode = buffer->GetMode();
  if (!this->ParentComputePass->Internals->GetRegisteredBufferFromPipeline(buffer, wgpuBuffer))
  {
    // If this buffer wasn't already registered in the pipeline by another compute pass, creating
    // the buffer. Otherwise, wgpuBuffer has been set to the already existing buffer

    wgpu::BufferUsage bufferUsage =
      vtkWebGPUComputePassBufferStorageInternals::ComputeBufferModeToBufferUsage(mode);
    vtkIdType byteSize = buffer->GetByteSize();

    wgpuBuffer = this->ParentPassWGPUConfiguration->CreateBuffer(
      byteSize, bufferUsage, false, bufferLabelCStr);

    // The buffer is read only by the shader if it doesn't have CopySrc (meaning that we would be
    // mapping the buffer from the GPU to read its results on the CPU meaning that the shader writes
    // to the buffer)
    bool bufferReadOnly = !(bufferUsage | wgpu::BufferUsage::CopySrc);
    // Uploading from std::vector or vtkDataArray if one of the two is present
    switch (buffer->GetDataType())
    {
      case vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR:
        if (buffer->GetDataPointer() != nullptr)
        {
          this->ParentPassWGPUConfiguration->WriteBuffer(
            wgpuBuffer, 0, buffer->GetDataPointer(), buffer->GetByteSize(), bufferLabelCStr);
        }
        else if (bufferReadOnly)
        {
          // Only warning if we're using a read only buffer without uploading data to initialize it

          vtkLog(WARNING,
            "The buffer with label \"" << bufferLabel
                                       << "\" has data type STD_VECTOR but no std::vector data was "
                                          "given. No data uploaded.");
        }
        break;

      case vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY:
        if (buffer->GetDataArray() != nullptr)
        {
          vtkWebGPUComputeBufferInternals::UploadFromDataArray(
            this->ParentPassWGPUConfiguration, wgpuBuffer, buffer->GetDataArray(), bufferLabelCStr);
        }
        else if (bufferReadOnly)
        {
          // Only warning if we're using a read only buffer without uploading data to initialize it

          vtkLog(WARNING,
            "The buffer with label \""
              << bufferLabel
              << "\" has data type VTK_DATA_ARRAY but no vtkDataArray data "
                 "was given. No data uploaded.");
        }
        break;

      default:
        break;
    }

    this->ParentComputePass->Internals->RegisterBufferToPipeline(buffer, wgpuBuffer);
  }

  // Adding the buffer to the lists
  this->Buffers.push_back(buffer);
  this->WebGPUBuffers.push_back(wgpuBuffer);

  // Creating the layout entry and the bind group entry for this buffer. These entries will be used
  // later when creating the bind groups / bind group layouts
  uint32_t group = buffer->GetGroup();
  uint32_t binding = buffer->GetBinding();

  wgpu::BindGroupLayoutEntry bglEntry =
    this->ParentComputePass->Internals->CreateBindGroupLayoutEntry(binding, mode);
  wgpu::BindGroupEntry bgEntry =
    this->ParentComputePass->Internals->CreateBindGroupEntry(wgpuBuffer, binding, mode, 0);

  this->ParentComputePass->Internals->BindGroupLayoutEntries[group].push_back(bglEntry);
  this->ParentComputePass->Internals->BindGroupEntries[group].push_back(bgEntry);

  // Returning the index of the buffer
  return this->Buffers.size() - 1;
}

//------------------------------------------------------------------------------
wgpu::Buffer vtkWebGPUComputePassBufferStorageInternals::GetWGPUBuffer(std::size_t bufferIndex)
{
  if (!this->CheckBufferIndex(bufferIndex, "GetWGPUBuffer"))
  {
    return nullptr;
  }

  return this->WebGPUBuffers[bufferIndex];
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassBufferStorageInternals::AddRenderBuffer(
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer)
{
  renderBuffer->SetAssociatedComputePass(this->ParentComputePass);

  this->Buffers.push_back(renderBuffer);
}

//------------------------------------------------------------------------------
unsigned int vtkWebGPUComputePassBufferStorageInternals::GetBufferByteSize(std::size_t bufferIndex)
{
  if (!this->CheckBufferIndex(bufferIndex, "GetBufferByteSize"))
  {
    return 0;
  }

  return this->WebGPUBuffers[bufferIndex].GetSize();
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassBufferStorageInternals::ResizeBuffer(
  std::size_t bufferIndex, vtkIdType newByteSize)
{
  if (!this->CheckBufferIndex(bufferIndex, "ResizeBuffer"))
  {
    return;
  }

  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer = this->Buffers[bufferIndex];

  this->RecreateBuffer(bufferIndex, newByteSize);
  this->ParentComputePass->Internals->RecreateBufferBindGroup(bufferIndex);

  this->ParentComputePass->Internals->RegisterBufferToPipeline(
    buffer, this->WebGPUBuffers[bufferIndex]);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassBufferStorageInternals::RecreateBuffer(
  std::size_t bufferIndex, vtkIdType newByteSize)
{
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer = this->Buffers[bufferIndex];

  // Updating the byte size
  buffer->SetByteSize(newByteSize);
  wgpu::BufferUsage bufferUsage =
    vtkWebGPUComputePassBufferStorageInternals::ComputeBufferModeToBufferUsage(buffer->GetMode());

  // Recreating the buffer
  std::string label = buffer->GetLabel();
  const char* bufferLabel = label.c_str();
  this->WebGPUBuffers[bufferIndex] =
    this->ParentPassWGPUConfiguration->CreateBuffer(newByteSize, bufferUsage, false, bufferLabel);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassBufferStorageInternals::ReadBufferFromGPU(
  std::size_t bufferIndex, vtkWebGPUComputePass::BufferMapAsyncCallback callback, void* userdata)
{
  if (!this->CheckBufferIndex(bufferIndex, "ReadBufferFromGPU"))
  {
    return;
  }

  // We need a buffer that will hold the mapped data.
  // We cannot directly map the output buffer of the compute shader because
  // wgpu::BufferUsage::Storage is incompatible with wgpu::BufferUsage::MapRead. This is a
  // restriction of WebGPU. This means that we have to create a new buffer with the MapRead flag
  // that is not a Storage buffer, copy the storage buffer that we actually want to this new buffer
  // (that has the MapRead usage flag) and then map this buffer to the CPU.
  vtkIdType byteSize = this->Buffers[bufferIndex]->GetByteSize();
  wgpu::Buffer mappedBuffer = this->ParentPassWGPUConfiguration->CreateBuffer(
    byteSize, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, false, nullptr);

  // If we were to allocate this callbackData locally on the stack, it would be destroyed when going
  // out of scope (at the end of this function). The callback, called asynchronously would then be
  // referring to data that has been destroyed (since it was allocated locally). This is why we're
  // allocating it dynamically with a new
  InternalMapBufferAsyncData* internalCallbackData = new InternalMapBufferAsyncData;
  internalCallbackData->buffer = mappedBuffer;
  internalCallbackData->bufferLabel = this->ParentComputePass->Label;
  internalCallbackData->byteSize = byteSize;
  internalCallbackData->userCallback = callback;
  internalCallbackData->userdata = userdata;

  wgpu::CommandEncoder commandEncoder = this->ParentComputePass->Internals->CreateCommandEncoder();
  commandEncoder.CopyBufferToBuffer(
    this->WebGPUBuffers[bufferIndex], 0, internalCallbackData->buffer, 0, byteSize);
  this->ParentComputePass->Internals->SubmitCommandEncoderToQueue(commandEncoder);

  auto internalCallback =
    [](wgpu::MapAsyncStatus status, wgpu::StringView message, void* wgpuUserData)
  {
    InternalMapBufferAsyncData* callbackData =
      reinterpret_cast<InternalMapBufferAsyncData*>(wgpuUserData);

    if (status == wgpu::MapAsyncStatus::Success)
    {
      const void* mappedRange = callbackData->buffer.GetConstMappedRange(0, callbackData->byteSize);
      callbackData->userCallback(mappedRange, callbackData->userdata);

      callbackData->buffer.Unmap();
    }
    else
    {
      vtkLog(
        WARNING, << "Failed to map [Buffer \'"
                 << (callbackData->bufferLabel.empty() ? "(nolabel)" : callbackData->bufferLabel)
                 << "\'] with error status: " << static_cast<std::uint32_t>(status) << " "
                 << vtkWebGPUHelpers::StringViewToStdString(message));
    }
#if defined(__EMSCRIPTEN__)
    wgpuBufferRelease(callbackData->buffer.Get());
#endif
    // Freeing the callbackData structure as it was dynamically allocated
    delete callbackData;
  };

#if defined(__EMSCRIPTEN__)
  // keep buffer alive for map.
  // See https://issues.chromium.org/issues/399131918
  wgpuBufferAddRef(internalCallbackData->buffer.Get());
#endif
  internalCallbackData->buffer.MapAsync(wgpu::MapMode::Read, 0, byteSize,
    wgpu::CallbackMode::AllowProcessEvents, internalCallback,
    static_cast<void*>(internalCallbackData));
}

//------------------------------------------------------------------------------
vtkWebGPUComputePassBufferStorageInternals::UpdateBufferStatusCode
vtkWebGPUComputePassBufferStorageInternals::UpdateWebGPUBuffer(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer wgpuBuffer,
  std::size_t& outBufferIndex)
{
  outBufferIndex = 0;
  for (vtkSmartPointer<vtkWebGPUComputeBuffer> computeBuffer : this->Buffers)
  {
    if (computeBuffer == buffer)
    {
      if (this->WebGPUBuffers[outBufferIndex].Get() == wgpuBuffer.Get())
      {
        return vtkWebGPUComputePassBufferStorageInternals::UpdateBufferStatusCode::UP_TO_DATE;
      }
      else
      {
        this->WebGPUBuffers[outBufferIndex] = wgpuBuffer;

        return vtkWebGPUComputePassBufferStorageInternals::UpdateBufferStatusCode::SUCCESS;
      }
    }

    outBufferIndex++;
  }

  return vtkWebGPUComputePassBufferStorageInternals::UpdateBufferStatusCode::BUFFER_NOT_FOUND;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassBufferStorageInternals::UpdateBufferData(
  std::size_t bufferIndex, vtkDataArray* newData)
{
  if (!this->CheckBufferIndex(bufferIndex, std::string("UpdateBufferData")))
  {
    return;
  }

  vtkWebGPUComputeBuffer* buffer = this->Buffers[bufferIndex];
  vtkIdType byteSize = buffer->GetByteSize();
  vtkIdType givenSize = newData->GetNumberOfValues() * newData->GetDataTypeSize();

  if (givenSize > byteSize)
  {
    vtkLog(ERROR,
      "vtkDataArray data given to UpdateBufferData with index "
        << bufferIndex << " is too big. " << givenSize << "bytes were given but the buffer is only "
        << byteSize << " bytes long. No data was updated by this call.");

    return;
  }

  wgpu::Buffer wgpuBuffer = this->WebGPUBuffers[bufferIndex];

  const std::string bufferLabel = buffer->GetLabel();
  const char* bufferLabelCStr = bufferLabel.c_str();
  vtkWebGPUComputeBufferInternals::UploadFromDataArray(
    this->ParentPassWGPUConfiguration, wgpuBuffer, newData, bufferLabelCStr);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassBufferStorageInternals::UpdateBufferData(
  std::size_t bufferIndex, vtkIdType byteOffset, vtkDataArray* newData)
{
  if (!this->CheckBufferIndex(bufferIndex, std::string("UpdateBufferData with offset")))
  {
    return;
  }

  vtkWebGPUComputeBuffer* buffer = this->Buffers[bufferIndex];
  vtkIdType byteSize = buffer->GetByteSize();
  vtkIdType givenSize = newData->GetNumberOfValues() * newData->GetDataTypeSize();

  if (givenSize + byteOffset > byteSize)
  {
    vtkLog(ERROR,
      "vtkDataArray data given to UpdateBufferData with index "
        << bufferIndex << " and offset " << byteOffset << " is too big. " << givenSize
        << "bytes and offset " << byteOffset << " were given but the buffer is only " << byteSize
        << " bytes long. No data was updated by this call.");

    return;
  }

  wgpu::Buffer wgpuBuffer = this->WebGPUBuffers[bufferIndex];

  const std::string bufferLabel = buffer->GetLabel();
  const char* bufferLabelCStr = bufferLabel.c_str();
  vtkWebGPUComputeBufferInternals::UploadFromDataArray(
    this->ParentPassWGPUConfiguration, wgpuBuffer, byteOffset, newData, bufferLabelCStr);
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePassBufferStorageInternals::CheckBufferIndex(
  std::size_t bufferIndex, const std::string& callerFunctionName)
{
  if (bufferIndex >= this->Buffers.size())
  {
    vtkLog(ERROR,
      "Invalid bufferIndex given to "
        << callerFunctionName << ". Index was '" << bufferIndex << "' while there are "
        << this->Buffers.size()
        << " available buffers. Make sure to use an index that was returned by AddBuffer().");

    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePassBufferStorageInternals::CheckBufferCorrectness(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer)
{
  const auto& bufferLabel = buffer->GetLabel();

  if (buffer->GetGroup() == -1)
  {
    vtkLogF(ERROR, "The group of the buffer with label \"%s\" hasn't been initialized",
      bufferLabel.c_str());
    return false;
  }
  else if (buffer->GetBinding() == -1)
  {
    vtkLogF(ERROR, "The binding of the buffer with label \"%s\" hasn't been initialized",
      bufferLabel.c_str());
    return false;
  }
  else if (buffer->GetByteSize() == 0)
  {
    vtkLogF(ERROR, "The buffer with label \"%s\" has a size of 0. Did you forget to set its size?",
      bufferLabel.c_str());
    return false;
  }
  else
  {
    // Checking that the buffer isn't already used
    for (vtkWebGPUComputeBuffer* existingBuffer : this->Buffers)
    {
      if (buffer->GetBinding() == existingBuffer->GetBinding() &&
        buffer->GetGroup() == existingBuffer->GetGroup())
      {
        vtkLog(ERROR,
          "The buffer with label" << bufferLabel << " is bound to binding " << buffer->GetBinding()
                                  << " but that binding is already used by buffer with label \""
                                  << existingBuffer->GetLabel() << "\" in bind group "
                                  << buffer->GetGroup());

        return false;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassBufferStorageInternals::SetupRenderBuffer(
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer)
{
  if (renderBuffer->GetWebGPUBuffer() == nullptr)
  {
    vtkLog(ERROR,
      "The given render buffer with label \""
        << renderBuffer->GetLabel()
        << "\" does not have an assigned WebGPUBuffer meaning that it will not reuse an existing "
           "buffer of the render pipeline. The issue probably is that SetWebGPUBuffer() wasn't "
           "called.");

    return;
  }

  this->WebGPUBuffers.push_back(renderBuffer->GetWebGPUBuffer());

  // Creating the entries for this existing buffer
  vtkIdType group = renderBuffer->GetGroup();
  vtkIdType binding = renderBuffer->GetBinding();
  vtkWebGPUComputeBuffer::BufferMode mode = renderBuffer->GetMode();

  wgpu::BindGroupLayoutEntry bglEntry;
  wgpu::BindGroupEntry bgEntry;
  bglEntry = this->ParentComputePass->Internals->CreateBindGroupLayoutEntry(binding, mode);
  bgEntry = this->ParentComputePass->Internals->CreateBindGroupEntry(
    renderBuffer->GetWebGPUBuffer(), binding, mode, 0);

  this->ParentComputePass->Internals->BindGroupLayoutEntries[group].push_back(bglEntry);
  this->ParentComputePass->Internals->BindGroupEntries[group].push_back(bgEntry);

  // Creating the uniform buffer that will contain the offset and the length of the data held by
  // the render buffer
  std::vector<unsigned int> uniformData = { renderBuffer->GetRenderBufferOffset(),
    renderBuffer->GetRenderBufferElementCount() };
  vtkNew<vtkWebGPUComputeBuffer> offsetSizeUniform;
  offsetSizeUniform->SetMode(vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER);
  offsetSizeUniform->SetGroup(renderBuffer->GetRenderUniformsGroup());
  offsetSizeUniform->SetBinding(renderBuffer->GetRenderUniformsBinding());
  offsetSizeUniform->SetData(uniformData);

  this->AddBuffer(offsetSizeUniform);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassBufferStorageInternals::ReleaseResources()
{
  this->ParentComputePass = nullptr;
  this->ParentPassWGPUConfiguration = nullptr;

  this->Buffers.clear();
  this->WebGPUBuffers.clear();
}

//------------------------------------------------------------------------------
wgpu::BufferUsage vtkWebGPUComputePassBufferStorageInternals::ComputeBufferModeToBufferUsage(
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
      vtkLog(ERROR, "Unhandled compute buffer mode in ComputeBufferModeToBufferUsage: " << mode);
      return wgpu::BufferUsage::None;
  }
}

//------------------------------------------------------------------------------
wgpu::BufferBindingType
vtkWebGPUComputePassBufferStorageInternals::ComputeBufferModeToBufferBindingType(
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
      vtkLog(
        ERROR, "Unhandled compute buffer mode in ComputeBufferModeToBufferBindingType: " << mode);
      return wgpu::BufferBindingType::Undefined;
  }
}

VTK_ABI_NAMESPACE_END
