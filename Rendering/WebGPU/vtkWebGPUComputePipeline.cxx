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

vtkStandardNewMacro(vtkWebGPUComputePipeline);

namespace
{
//------------------------------------------------------------------------------
wgpu::BufferUsage ComputeBufferModeToBufferUsage(vtkWebGPUComputeBuffer::BufferMode mode)
{
  switch (mode)
  {
    case vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE:
    case vtkWebGPUComputeBuffer::READ_WRITE_COMPUTE_STORAGE:
      return wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage;

    case vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE:
      return wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Storage;

    case vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER:
      return wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;

    default:
      return wgpu::BufferUsage::None;
  }
}

//------------------------------------------------------------------------------
wgpu::BufferBindingType ComputeBufferModeToBufferBindingType(
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
  // usually use their callback to copy the data from the mapped buffer into a CPU-side buffer that
  // will use the result of the compute shader in the rest of the application
  vtkWebGPUComputePipeline::MapAsyncCallback userCallback;
  // Userdata passed to userCallback. This is typically the structure that contains the CPU-side
  // buffer into which the data of the mapped buffer will be copied
  void* userdata = nullptr;
};
}

class vtkWebGPUComputePipeline::ComputePipelineInternals
{
public:
  ComputePipelineInternals(vtkWebGPUComputePipeline* self)
    : Self(self)
  {
  }

  /**
   * Given a buffer, create the associated bind group layout entry
   * that will be used when creating the bind group layouts
   */
  void AddBindGroupLayoutEntry(
    uint32_t bindGroup, uint32_t binding, vtkWebGPUComputeBuffer::BufferMode mode)
  {
    wgpu::BufferBindingType bindingType = ComputeBufferModeToBufferBindingType(mode);

    vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper bglEntry{ binding,
      wgpu::ShaderStage::Compute, bindingType };

    this->BindGroupLayoutEntries[bindGroup].push_back(bglEntry);
  }

  /**
   * Given a buffer, create the associated bind group entry
   * that will be used when creating the bind groups
   */
  void AddBindGroupEntry(wgpu::Buffer wgpuBuffer, uint32_t bindGroup, uint32_t binding,
    vtkWebGPUComputeBuffer::BufferMode mode, uint32_t offset)
  {
    wgpu::BufferBindingType bindingType = ComputeBufferModeToBufferBindingType(mode);

    vtkWebGPUInternalsBindGroup::BindingInitializationHelper bgEntry{ binding, wgpuBuffer, offset };

    this->BindGroupEntries[bindGroup].push_back(bgEntry.GetAsBinding());
  }

  /**
   * Initializes the adapter of the compute pipeline
   */
  void CreateAdapter()
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

  /**
   * Initializes the device of the compute pipeline
   */
  void CreateDevice()
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

  /**
   * Compiles the shader source given into a WGPU shader module
   */
  void CreateShaderModule()
  {
    this->ShaderModule =
      vtkWebGPUInternalsShaderModule::CreateFromWGSL(this->Device, Self->ShaderSource);
  }

  /**
   * Creates all the bind groups and bind group layouts of this compute pipeline from the buffers
   * that have been added so far.
   */
  void CreateBindGroupsAndLayouts()
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
      this->BindGroups.push_back(vtkWebGPUInternalsBindGroup::MakeBindGroup(
        this->Device, BindGroupLayouts.back(), bgEntries));
    }
  }

  /**
   * Creates the bind group layout of a given list of buffers (that must all belong to the same bind
   * group)
   */
  wgpu::BindGroupLayout CreateBindGroupLayout(
    const wgpu::Device& device, const std::vector<wgpu::BindGroupLayoutEntry>& layoutEntries)
  {
    wgpu::BindGroupLayout bgl =
      vtkWebGPUInternalsBindGroupLayout::MakeBindGroupLayout(device, layoutEntries);
    return bgl;
  }

  /**
   * Creates the compute pipeline that will be used to dispatch the compute shader
   */
  void CreateComputePipeline()
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

  /**
   * Creates the compute pipeline layout associated with the bind group layouts of this compute
   * pipeline
   *
   * @warning: The bind group layouts must have been created by CreateBindGroups() prior to calling
   * this function
   */
  wgpu::PipelineLayout CreateComputePipelineLayout()
  {
    wgpu::PipelineLayoutDescriptor computePipelineLayoutDescriptor;
    computePipelineLayoutDescriptor.bindGroupLayoutCount = this->BindGroupLayouts.size();
    computePipelineLayoutDescriptor.bindGroupLayouts = this->BindGroupLayouts.data();
    computePipelineLayoutDescriptor.nextInChain = nullptr;

    return this->Device.CreatePipelineLayout(&computePipelineLayoutDescriptor);
  }

  /**
   * Creates and returns a command encoder
   */
  wgpu::CommandEncoder CreateCommandEncoder()
  {
    wgpu::CommandEncoderDescriptor commandEncoderDescriptor;
    commandEncoderDescriptor.label = this->WGPUCommandEncoderLabel.c_str();

    return this->Device.CreateCommandEncoder(&commandEncoderDescriptor);
  }

  /**
   * Creates a compute pass encoder from a command encoder
   */
  wgpu::ComputePassEncoder CreateComputePassEncoder(const wgpu::CommandEncoder& commandEncoder)
  {
    wgpu::ComputePassDescriptor computePassDescriptor;
    computePassDescriptor.nextInChain = nullptr;
    computePassDescriptor.timestampWrites = 0;
    return commandEncoder.BeginComputePass(&computePassDescriptor);
  }

  /**
   * Encodes the compute pass and dispatches the workgroups
   *
   * @warning: The bind groups and the compute pipeline must have been created prior to calling this
   * function
   */
  void DispatchComputePass(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ)
  {
    if (groupsX * groupsY * groupsZ == 0)
    {
      vtkLogF(ERROR,
        "Invalid number of workgroups when dispatching compute pipeline \"%s\". Work groups sizes "
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

  /**
   * Finishes the encoding of a command encoder and submits the resulting command buffer
   * to the queue
   */
  void SubmitCommandEncoderToQueue(const wgpu::CommandEncoder& commandEncoder)
  {
    wgpu::CommandBuffer commandBuffer = commandEncoder.Finish();
    this->Device.GetQueue().Submit(1, &commandBuffer);
  }

  bool Initialized = false;

  wgpu::Adapter Adapter = nullptr;
  wgpu::Device Device = nullptr;
  wgpu::ShaderModule ShaderModule;
  std::vector<wgpu::BindGroup> BindGroups;
  // Maps a bind group index to to the list of bind group entries for this group. These
  // entries will be used at the creation of the bind groups
  std::unordered_map<int, std::vector<wgpu::BindGroupEntry>> BindGroupEntries;
  std::vector<wgpu::BindGroupLayout> BindGroupLayouts;
  // Maps a bind group index to to the list of bind group layout entries for this group.
  // These layout entries will be used at the creation of the bind group layouts
  std::unordered_map<int, std::vector<wgpu::BindGroupLayoutEntry>> BindGroupLayoutEntries;
  wgpu::ComputePipeline ComputePipeline;

  std::vector<vtkWebGPUComputeBuffer*> Buffers;
  std::vector<wgpu::Buffer> WGPUBuffers;

  /**
   * Render buffers use already existing wgpu buffers (those of poly data mappers for example) and
   * thus need to be handled differently
   */
  std::vector<vtkSmartPointer<vtkWebGPUComputeRenderBuffer>> RenderBuffers;

  // How many groups to launch when dispatching the compute
  unsigned int GroupsX = 0, GroupsY = 0, GroupsZ = 0;

  // Label used for the wgpu compute pipeline of this VTK compute pipeline
  std::string WGPUComputePipelineLabel = "WebGPU compute pipeline of \"VTK Compute pipeline\"";
  // Label used for the wgpu command encoders created and used by this VTK compute pipeline
  std::string WGPUCommandEncoderLabel = "WebGPU command encoder of \"VTK Compute pipeline\"";

  // Main class
  vtkWebGPUComputePipeline* Self;
};

//------------------------------------------------------------------------------
vtkWebGPUComputePipeline::vtkWebGPUComputePipeline()
  : Internals(new ComputePipelineInternals(this))
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
  for (vtkWebGPUComputeBuffer* buffer : internals.Buffers)
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
int vtkWebGPUComputePipeline::AddBuffer(vtkWebGPUComputeBuffer* buffer)
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

  wgpu::Buffer wgpuBuffer =
    vtkWebGPUInternalsBuffer::CreateABuffer(internals.Device, buffer->GetByteSize(),
      ComputeBufferModeToBufferUsage(buffer->GetMode()), false, bufferLabel.c_str());

  // Uploading from std::vector or vtkDataArray if one of the two is present
  switch (buffer->GetDataType())
  {
    case vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR:
      if (buffer->GetDataPointer() != nullptr)
      {
        internals.Device.GetQueue().WriteBuffer(
          wgpuBuffer, 0, buffer->GetDataPointer(), buffer->GetByteSize());
      }
      else
      {
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
      else
      {
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
void vtkWebGPUComputePipeline::AddRenderBuffer(vtkWebGPUComputeRenderBuffer* renderBuffer)
{
  auto& internals = *this->Internals;

  renderBuffer->SetAssociatedPipeline(this);

  internals.Buffers.push_back(renderBuffer);
  internals.WGPUBuffers.push_back(renderBuffer->GetWGPUBuffer());
  internals.RenderBuffers.push_back(renderBuffer);
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
    for (vtkWebGPUComputeBuffer* existingBuffer : internals.Buffers)
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

VTK_ABI_NAMESPACE_END
