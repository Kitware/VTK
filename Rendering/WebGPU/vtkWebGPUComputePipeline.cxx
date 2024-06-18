// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputePipeline.h"
#include "vtkWebGPUInternalsBindGroup.h"       // for bind group utilitary methods
#include "vtkWebGPUInternalsBindGroupLayout.h" // for bind group layouts utilitary methods
#include "vtkWebGPUInternalsBuffer.h"          // for internal buffer utils
#include "vtkWebGPUInternalsCallbacks.h"
#include "vtkWebGPUInternalsComputeBuffer.h"
#include "vtkWebGPUInternalsShaderModule.h" // for wgpu::ShaderModule
#include "vtksys/FStream.hxx"

VTK_ABI_NAMESPACE_BEGIN

namespace
{
/**
 * Structure used to pass data to the asynchronous callback of wgpu::Buffer.MapAsync()
 */
struct InternalMapBufferAsyncData
{
  // Buffer currently being mapped
  wgpu::Buffer buffer = nullptr;
  // Label of the buffer currently being mapped. Used for printing errors
  std::string bufferLabel;
  // Size of the buffer being mapped in bytes
  vtkIdType byteSize = -1;

  // The callback given by the user that will be called once the buffer is mapped. The user will
  // usually use their callback to copy the data from the mapped buffer into a CPU-side buffer
  // that will use the result of the compute shader in the rest of the application
  vtkWebGPUComputePipeline::MapAsyncCallback userCallback;
  // Userdata passed to userCallback. This is typically the structure that contains the CPU-side
  // buffer into which the data of the mapped buffer will be copied
  void* userdata = nullptr;
};
}

vtkStandardNewMacro(vtkWebGPUComputePipeline);

//------------------------------------------------------------------------------
vtkWebGPUComputePipeline::vtkWebGPUComputePipeline()
  : Internals(new vtkWebGPUInternalsComputePipeline(this))
{
  this->Internals->CreateAdapter();
  this->Internals->CreateDevice();
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  auto& internals = *this->Internals;

  os << indent << "Initialized: " << internals.Initialized << std::endl;

  os << indent << "Adapter: " << internals.Adapter.Get() << std::endl;
  os << indent << "Device: " << internals.Device.Get() << std::endl;
  os << indent << "ShaderModule: " << internals.ShaderModule.Get() << std::endl;

  os << indent << internals.BindGroups.size() << " binds groups: " << std::endl;
  for (const wgpu::BindGroup& bindGroup : internals.BindGroups)
  {
    os << indent << "\t- " << bindGroup.Get() << std::endl;
  }

  os << indent << internals.BindGroupEntries.size() << " binds group entries: " << std::endl;
  for (const auto& bindGroupEntry : internals.BindGroupEntries)
  {
    os << indent << "\t Bind group " << bindGroupEntry.first << std::endl;
    os << indent << "\t (binding/buffer/offset/size)" << std::endl;
    for (wgpu::BindGroupEntry entry : bindGroupEntry.second)
    {
      os << indent << "\t- " << entry.binding << " / " << entry.buffer.Get() << " / "
         << entry.offset << " / " << entry.size << std::endl;
    }
  }

  os << indent << internals.BindGroupLayouts.size() << " bind group layouts:" << std::endl;
  for (const wgpu::BindGroupLayout& bindGroupLayout : internals.BindGroupLayouts)
  {
    os << indent << "\t- " << bindGroupLayout.Get() << std::endl;
  }

  os << indent << internals.BindGroupLayoutEntries.size()
     << " binds group layouts entries: " << std::endl;
  for (const auto& bindLayoutGroupEntry : internals.BindGroupLayoutEntries)
  {
    os << indent << "\t Bind group layout " << bindLayoutGroupEntry.first << std::endl;
    os << indent << "\t (binding/buffer type/visibility)" << std::endl;
    for (wgpu::BindGroupLayoutEntry entry : bindLayoutGroupEntry.second)
    {
      os << indent << "\t- " << entry.binding << " / " << static_cast<uint32_t>(entry.buffer.type)
         << " / " << static_cast<uint32_t>(entry.visibility) << std::endl;
    }
  }

  os << indent << "WGPU Compute pipeline: " << internals.ComputePipeline.Get() << std::endl;

  os << indent << internals.Buffers.size() << "buffers: " << std::endl;
  for (vtkSmartPointer<vtkWebGPUComputeBuffer> buffer : internals.Buffers)
  {
    buffer->PrintSelf(os, indent);
  }

  os << indent << internals.RenderBuffers.size() << " render buffers: " << std::endl;
  for (vtkWebGPUComputeRenderBuffer* renderBuffer : internals.RenderBuffers)
  {
    renderBuffer->PrintSelf(os, indent);
  }

  os << indent << internals.WGPUBuffers.size() << "WGPU Buffers:" << std::endl;
  for (wgpu::Buffer buffer : internals.WGPUBuffers)
  {
    os << indent << "\t- " << buffer.Get() << std::endl;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::SetShaderSourceFromPath(const std::string& shaderFilePath)
{
  if (!vtksys::SystemTools::FileExists(shaderFilePath))
  {
    vtkLogF(ERROR, "Given shader file path '%s' doesn't exist", shaderFilePath.c_str());

    return;
  }

  vtksys::ifstream inputFileStream(shaderFilePath);
  assert(inputFileStream);
  std::string source(
    (std::istreambuf_iterator<char>(inputFileStream)), std::istreambuf_iterator<char>());

  this->SetShaderSource(source);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::SetLabel(const std::string& label)
{
  auto& internals = *this->Internals;

  this->Label = label;

  internals.WGPUComputePipelineLabel = std::string("Compute pipeline of \"" + this->Label + "\"");
  internals.WGPUCommandEncoderLabel = std::string("Command encoder of \"" + this->Label + "\"");
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::SetAdapter(wgpu::Adapter adapter)
{
  this->Internals->Adapter = adapter;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::SetDevice(wgpu::Device device)
{
  this->Internals->Device = device;
}

//------------------------------------------------------------------------------
int vtkWebGPUComputePipeline::AddBuffer(vtkSmartPointer<vtkWebGPUComputeBuffer> buffer)
{
  auto& internals = *this->Internals;

  // Giving the buffer a default label if it doesn't have one already
  if (buffer->GetLabel().empty())
  {
    buffer->SetLabel("Buffer " + std::to_string(internals.Buffers.size()));
  }

  std::string bufferLabel = buffer->GetLabel();

  bool bufferCorrect = this->IsBufferValid(buffer, bufferLabel.c_str());
  if (!bufferCorrect)
  {
    return -1;
  }

  wgpu::BufferUsage bufferUsage =
    vtkWebGPUInternalsComputePipeline::ComputeBufferModeToBufferUsage(buffer->GetMode());
  wgpu::Buffer wgpuBuffer = vtkWebGPUInternalsBuffer::CreateABuffer(
    internals.Device, buffer->GetByteSize(), bufferUsage, false, bufferLabel.c_str());

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
        internals.Device.GetQueue().WriteBuffer(
          wgpuBuffer, 0, buffer->GetDataPointer(), buffer->GetByteSize());
      }
      else if (bufferReadOnly)
      {
        // Only warning if we're using a read only buffer without uploading data to initialize it

        vtkLog(WARNING,
          "The buffer with label \""
            << bufferLabel
            << "\" has data type STD_VECTOR but no std::vector data was given. No data uploaded.");
      }
      break;

    case vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY:
      if (buffer->GetDataArray() != nullptr)
      {
        vtkWebGPUInternalsComputeBuffer::UploadFromDataArray(
          internals.Device, wgpuBuffer, buffer->GetDataArray());
      }
      else if (bufferReadOnly)
      {
        // Only warning if we're using a read only buffer without uploading data to initialize it

        vtkLog(WARNING,
          "The buffer with label \"" << bufferLabel
                                     << "\" has data type VTK_DATA_ARRAY but no vtkDataArray data "
                                        "was given. No data uploaded.");
      }
      break;

    default:
      break;
  }

  // Adding the buffer to the list
  internals.Buffers.push_back(buffer);
  internals.WGPUBuffers.push_back(wgpuBuffer);

  // Creating the layout entry and the bind group entry for this buffer. These entries will be
  // used later when creating the bind groups / bind group layouts
  internals.AddBindGroupLayoutEntry(buffer->GetGroup(), buffer->GetBinding(), buffer->GetMode());
  internals.AddBindGroupEntry(
    wgpuBuffer, buffer->GetGroup(), buffer->GetBinding(), buffer->GetMode(), 0);

  // Returning the index of the buffer
  return internals.Buffers.size() - 1;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::AddRenderBuffer(
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer)
{
  auto& internals = *this->Internals;

  renderBuffer->SetAssociatedPipeline(this);

  internals.Buffers.push_back(renderBuffer);
  internals.WGPUBuffers.push_back(renderBuffer->GetWGPUBuffer());
  internals.RenderBuffers.push_back(renderBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::ResizeBuffer(int bufferIndex, vtkIdType newByteSize)
{
  if (!this->CheckBufferIndex(bufferIndex, std::string("ResizeBuffer")))
  {
    return;
  }

  vtkWebGPUComputeBuffer* buffer = this->Internals->Buffers[bufferIndex];

  this->RecreateBuffer(bufferIndex, newByteSize);
  this->RecreateBufferBindGroup(bufferIndex);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::RecreateBuffer(int bufferIndex, vtkIdType newByteSize)
{
  auto& internals = *this->Internals;

  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer = internals.Buffers[bufferIndex];

  // Updating the byte size
  buffer->SetByteSize(newByteSize);
  const char* bufferLabel = buffer->GetLabel().c_str();
  wgpu::BufferUsage bufferUsage =
    vtkWebGPUInternalsComputePipeline::ComputeBufferModeToBufferUsage(buffer->GetMode());

  // Recreating the buffer
  internals.WGPUBuffers[bufferIndex] = vtkWebGPUInternalsBuffer::CreateABuffer(
    internals.Device, newByteSize, bufferUsage, false, bufferLabel);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::RecreateBufferBindGroup(int bufferIndex)
{
  auto& internals = *this->Internals;

  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer = internals.Buffers[bufferIndex];

  // We also need to recreate the bind group entry (and the bind group below) that corresponded to
  // this buffer.
  // We first need to find the bind group entry that corresponded to this buffer
  std::vector<wgpu::BindGroupEntry>& bgEntries = internals.BindGroupEntries[buffer->GetGroup()];
  for (wgpu::BindGroupEntry& entry : bgEntries)
  {
    // We only need to check the binding because we already retrieved all the entries that
    // correspond to the group of the buffer
    if (entry.binding == buffer->GetBinding())
    {
      // Replacing the buffer by the one we just recreated
      entry.buffer = internals.WGPUBuffers[bufferIndex];

      break;
    }
  }

  // Finding which bind group is the one to recreate
  int bindGroupIndex = -1;
  for (int i = 0; i < internals.BindGroupsOrder.size(); i++)
  {
    if (internals.BindGroupsOrder[i] == buffer->GetGroup())
    {
      bindGroupIndex = i;

      break;
    }
  }

  if (bindGroupIndex == -1)
  {
    // We couldn't find the bind group, something went wrong
    vtkLog(ERROR,
      "Unable to find the bind group to which the buffer of index" << bufferIndex << " belongs.");

    return;
  }

  // Recreating the right bind group
  internals.BindGroups[bindGroupIndex] = vtkWebGPUInternalsBindGroup::MakeBindGroup(
    internals.Device, internals.BindGroupLayouts[bindGroupIndex], bgEntries);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::SetupRenderBuffer(vtkWebGPUComputeRenderBuffer* renderBuffer)
{
  auto& internals = *this->Internals;

  internals.AddBindGroupLayoutEntry(
    renderBuffer->GetGroup(), renderBuffer->GetBinding(), renderBuffer->GetMode());
  internals.AddBindGroupEntry(renderBuffer->GetWGPUBuffer(), renderBuffer->GetGroup(),
    renderBuffer->GetBinding(), renderBuffer->GetMode(), 0);

  std::vector<unsigned int> uniformData = { renderBuffer->GetRenderBufferOffset(),
    renderBuffer->GetRenderBufferElementCount() };
  vtkNew<vtkWebGPUComputeBuffer> offsetSizeUniform;
  offsetSizeUniform->SetMode(vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER);
  offsetSizeUniform->SetGroup(renderBuffer->GetRenderUniformsGroup());
  offsetSizeUniform->SetBinding(renderBuffer->GetRenderUniformsBinding());
  offsetSizeUniform->SetData(uniformData);
  offsetSizeUniform->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);
  offsetSizeUniform->SetLabel(
    "Uniform buffer of render buffer \"" + renderBuffer->GetLabel() + "\"");

  this->AddBuffer(offsetSizeUniform);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::ReadBufferFromGPU(
  int bufferIndex, vtkWebGPUComputePipeline::MapAsyncCallback callback, void* userdata)
{
  auto& internals = *this->Internals;

  // We need a buffer that will hold the mapped data. We cannot directly map the output
  // buffer of the compute shader. This is a restriction of WebGPU.
  vtkIdType byteSize = internals.Buffers[bufferIndex]->GetByteSize();
  wgpu::Buffer mappedBuffer = vtkWebGPUInternalsBuffer::CreateABuffer(internals.Device, byteSize,
    wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, false, nullptr);

  // If we were to allocate this callbackData locally on the stack, it would be destroyed when going
  // out of scope (at the end of this function). The callback, called asynchronously would then be
  // refering to data that has been destroyed (since it was allocated locally). This is why we're
  // allocating it dynamically with a new
  InternalMapBufferAsyncData* internalCallbackData = new InternalMapBufferAsyncData;
  internalCallbackData->buffer = wgpu::Buffer::Acquire(mappedBuffer.Get());
  internalCallbackData->bufferLabel = this->Label;
  internalCallbackData->byteSize = byteSize;
  internalCallbackData->userCallback = callback;
  internalCallbackData->userdata = userdata;

  wgpu::CommandEncoder commandEncoder = internals.CreateCommandEncoder();
  commandEncoder.CopyBufferToBuffer(
    internals.WGPUBuffers[bufferIndex], 0, internalCallbackData->buffer, 0, byteSize);
  internals.SubmitCommandEncoderToQueue(commandEncoder);

  auto internalCallback = [](WGPUBufferMapAsyncStatus status, void* wgpuUserData) {
    InternalMapBufferAsyncData* callbackData =
      reinterpret_cast<InternalMapBufferAsyncData*>(wgpuUserData);

    if (status == WGPUBufferMapAsyncStatus::WGPUBufferMapAsyncStatus_Success)
    {
      const void* mappedRange = callbackData->buffer.GetConstMappedRange(0, callbackData->byteSize);
      callbackData->userCallback(mappedRange, callbackData->userdata);

      callbackData->buffer.Unmap();
      // Freeing the callbackData structure as it was dynamically allocated
      delete callbackData;
    }
    else
    {
      vtkLogF(WARNING, "Could not map buffer '%s' with error status: %d",
        callbackData->bufferLabel.length() > 0 ? callbackData->bufferLabel.c_str() : "(nolabel)",
        status);

      delete callbackData;
    }
  };

  internalCallbackData->buffer.MapAsync(
    wgpu::MapMode::Read, 0, byteSize, internalCallback, internalCallbackData);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::UpdateBufferData(int bufferIndex, vtkDataArray* newData)
{
  auto& internals = *this->Internals;

  if (!this->CheckBufferIndex(bufferIndex, std::string("UpdateBufferData")))
  {
    return;
  }

  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer = internals.Buffers[bufferIndex];
  vtkIdType byteSize = buffer->GetByteSize();
  vtkIdType givenSize = newData->GetNumberOfValues() * newData->GetDataTypeSize();

  if (givenSize > byteSize)
  {
    vtkLog(ERROR,
      "std::vector data given to UpdateBufferData with index "
        << bufferIndex << " is too big. " << givenSize << "bytes were given but the buffer is only "
        << byteSize << " bytes long. No data was updated by this call.");

    return;
  }

  wgpu::Buffer wgpuBuffer = internals.WGPUBuffers[bufferIndex];

  vtkWebGPUInternalsComputeBuffer::UploadFromDataArray(internals.Device, wgpuBuffer, newData);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::UpdateBufferData(
  int bufferIndex, vtkIdType byteOffset, vtkDataArray* newData)
{
  auto& internals = *this->Internals;

  if (!this->CheckBufferIndex(bufferIndex, std::string("UpdateBufferData with offset")))
  {
    return;
  }

  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer = internals.Buffers[bufferIndex];
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

  wgpu::Buffer wgpuBuffer = internals.WGPUBuffers[bufferIndex];

  vtkWebGPUInternalsComputeBuffer::UploadFromDataArray(
    internals.Device, wgpuBuffer, byteOffset, newData);
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePipeline::CheckBufferIndex(
  int bufferIndex, const std::string& callerFunctionName)
{
  auto& internals = *this->Internals;

  if (bufferIndex < 0)
  {
    vtkLog(ERROR,
      "Negative bufferIndex given to "
        << callerFunctionName << ". Make sure to use an index that was returned by AddBuffer().");

    return false;
  }

  if (bufferIndex >= internals.Buffers.size())
  {
    vtkLog(ERROR,
      "Invalid bufferIndex given to "
        << callerFunctionName << ". Index was '" << bufferIndex << "' while there are "
        << internals.Buffers.size()
        << " available buffers. Make sure to use an index that was returned by AddBuffer().");

    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePipeline::IsBufferValid(
  vtkWebGPUComputeBuffer* buffer, const char* bufferLabel)
{
  auto& internals = *this->Internals;

  if (buffer->GetGroup() == -1)
  {
    vtkLogF(
      ERROR, "The group of the buffer with label \"%s\" hasn't been initialized", bufferLabel);
    return false;
  }
  else if (buffer->GetBinding() == -1)
  {
    vtkLogF(
      ERROR, "The binding of the buffer with label \"%s\" hasn't been initialized", bufferLabel);
    return false;
  }
  else if (buffer->GetByteSize() == 0)
  {
    vtkLogF(ERROR, "The buffer with label \"%s\" has a size of 0. Did you forget to set its size?",
      bufferLabel);
    return false;
  }
  else
  {
    // Checking that the buffer isn't already used
    for (vtkSmartPointer<vtkWebGPUComputeBuffer> existingBuffer : internals.Buffers)
    {
      if (buffer->GetBinding() == existingBuffer->GetBinding() &&
        buffer->GetGroup() == existingBuffer->GetGroup())
      {
        vtkLog(ERROR,
          "The buffer with label" << bufferLabel << " is bound to binding " << buffer->GetBinding()
                                  << " but that binding is already used by buffer with label \""
                                  << buffer->GetLabel().c_str() << "\" in bind group "
                                  << buffer->GetGroup());

        return false;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::SetWorkgroups(int groupsX, int groupsY, int groupsZ)
{
  auto& internals = *this->Internals;

  internals.GroupsX = groupsX;
  internals.GroupsY = groupsY;
  internals.GroupsZ = groupsZ;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::Dispatch()
{
  auto& internals = *this->Internals;

  if (!internals.Initialized)
  {
    internals.CreateShaderModule();
    internals.CreateBindGroupsAndLayouts();
    internals.CreateComputePipeline();

    internals.Initialized = true;
  }

  internals.DispatchComputePass(internals.GroupsX, internals.GroupsY, internals.GroupsZ);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePipeline::Update()
{
  bool workDone = false;

  // clang-format off
  this->Internals->Device.GetQueue().OnSubmittedWorkDone([](WGPUQueueWorkDoneStatus, void* userdata)
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

VTK_ABI_NAMESPACE_END
