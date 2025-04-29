// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "Private/vtkWebGPUComputePassInternals.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUShaderModuleInternals.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUCommandEncoderDebugGroup.h"
#include "vtkWebGPUComputePipeline.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputePassInternals);

//------------------------------------------------------------------------------
vtkWebGPUComputePassInternals::~vtkWebGPUComputePassInternals() = default;

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Initialized? : " << this->Initialized << std::endl;
  os << indent << "BindGroupOrLayoutsInvalidated? : " << this->BindGroupOrLayoutsInvalidated
     << std::endl;

  os << indent << "WGPUConfiguration: " << this->WGPUConfiguration << std::endl;

  os << indent << "Associated pipeline: ";
  this->AssociatedPipeline->PrintSelf(os, indent);

  os << indent << "ShaderModule: " << this->ShaderModule.Get() << std::endl;

  os << indent << this->BindGroups.size() << " binds groups: " << std::endl;
  for (const wgpu::BindGroup& bindGroup : this->BindGroups)
  {
    os << indent << "\t- " << bindGroup.Get() << std::endl;
  }

  os << indent << this->BindGroupEntries.size() << " binds group entries: " << std::endl;
  for (const auto& bindGroupEntry : this->BindGroupEntries)
  {
    os << indent << "\t Bind group " << bindGroupEntry.first << std::endl;
    os << indent << "\t (binding/buffer/offset/size)" << std::endl;
    for (wgpu::BindGroupEntry entry : bindGroupEntry.second)
    {
      os << indent << "\t- " << entry.binding << " / " << entry.buffer.Get() << " / "
         << entry.offset << " / " << entry.size << std::endl;
    }
  }

  os << indent << this->BindGroupLayouts.size() << " bind group layouts:" << std::endl;
  for (const wgpu::BindGroupLayout& bindGroupLayout : this->BindGroupLayouts)
  {
    os << indent << "\t- " << bindGroupLayout.Get() << std::endl;
  }

  os << indent << this->BindGroupLayoutEntries.size()
     << " binds group layouts entries: " << std::endl;
  for (const auto& bindLayoutGroupEntry : this->BindGroupLayoutEntries)
  {
    os << indent << "\t Bind group layout " << bindLayoutGroupEntry.first << std::endl;
    os << indent << "\t (binding/buffer type/visibility)" << std::endl;
    for (wgpu::BindGroupLayoutEntry entry : bindLayoutGroupEntry.second)
    {
      os << indent << "\t- " << entry.binding << " / " << static_cast<uint32_t>(entry.buffer.type)
         << " / " << static_cast<uint32_t>(entry.visibility) << std::endl;
    }
  }

  os << indent << "TextureStorage: ";
  this->TextureStorage->PrintSelf(os, indent);

  os << indent << "BufferStorage: ";
  this->BufferStorage->PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::SetParentPass(vtkWeakPointer<vtkWebGPUComputePass> parentPass)
{
  this->ParentPass = parentPass;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::SetWGPUConfiguration(vtkWebGPUConfiguration* config)
{
  vtkSetSmartPointerBodyMacro(WGPUConfiguration, vtkWebGPUConfiguration, config);
  this->TextureStorage->SetParentPassWGPUConfiguration(config);
  this->BufferStorage->SetParentPassWGPUConfiguration(config);
}

//------------------------------------------------------------------------------
vtkWeakPointer<vtkWebGPUComputePipeline> vtkWebGPUComputePassInternals::GetAssociatedPipeline()
{
  return this->AssociatedPipeline;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::SetAssociatedPipeline(
  vtkWeakPointer<vtkWebGPUComputePipeline> associatedPipeline)
{
  this->AssociatedPipeline = associatedPipeline;
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePassInternals::CheckTextureIndex(
  int textureIndex, const std::string& callerFunctionName)
{
  return this->TextureStorage->CheckTextureIndex(textureIndex, callerFunctionName);
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePassInternals::CheckTextureViewIndex(
  int textureViewIndex, const std::string& callerFunctionName)
{
  return this->TextureStorage->CheckTextureViewIndex(textureViewIndex, callerFunctionName);
}

//------------------------------------------------------------------------------
wgpu::TextureView vtkWebGPUComputePassInternals::CreateWebGPUTextureView(
  vtkSmartPointer<vtkWebGPUComputeTextureView> textureView, wgpu::Texture wgpuTexture)
{
  return this->TextureStorage->CreateWebGPUTextureView(textureView, wgpuTexture);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::UpdateWebGPUBuffer(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer wgpuBuffer)
{
  std::size_t bufferIndex;
  vtkWebGPUComputePassBufferStorageInternals::UpdateBufferStatusCode statusCode =
    this->BufferStorage->UpdateWebGPUBuffer(buffer, wgpuBuffer, bufferIndex);

  switch (statusCode)
  {
    case vtkWebGPUComputePassBufferStorageInternals::UpdateBufferStatusCode::SUCCESS:
      this->RecreateBufferBindGroup(bufferIndex);

      break;

    case vtkWebGPUComputePassBufferStorageInternals::UpdateBufferStatusCode::BUFFER_NOT_FOUND:
      // No buffer updated because the buffer was never added to the this compute pass
      vtkDebugMacro("UpdateWebGPUBuffer, buffer not found and not updated");

      return;

    case vtkWebGPUComputePassBufferStorageInternals::UpdateBufferStatusCode::UP_TO_DATE:
      // This means that the buffer was already up to date in the compute pass. This happens when a
      // buffer is recreated on a compute pass --> this triggers the update of the buffer within all
      // the passes of the compute pipeline but the pass that recreated the buffer already has the
      // right buffer up-to-date, it doesn't need an update. The buffer storage of this compute pass
      // will return -2
      //
      // We're thus returning early, no need to recreate the bind group
      vtkDebugMacro("UpdateWebGPUBuffer, buffer already up-to-date");

      return;

    default:
      vtkErrorWithObjectMacro(this,
        "UpdateBufferStatusCode: "
          << statusCode << " not handled in UpdateWebGPUBuffer(). This is an internal error.");

      return;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::UpdateComputeTextureAndViews(
  vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture newWgpuTexture)
{
  this->TextureStorage->UpdateComputeTextureAndViews(texture, newWgpuTexture);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::RecreateBuffer(int bufferIndex, vtkIdType newByteSize)
{
  this->BufferStorage->RecreateBuffer(bufferIndex, newByteSize);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::RecreateBufferBindGroup(int bufferIndex)
{
  vtkWebGPUComputeBuffer* buffer = this->BufferStorage->Buffers[bufferIndex];

  // We also need to recreate the bind group entry (and the bind group below) that corresponded to
  // this buffer.
  // We first need to find the bind group entry that corresponded to this buffer
  std::vector<wgpu::BindGroupEntry>& bgEntries = this->BindGroupEntries[buffer->GetGroup()];
  for (wgpu::BindGroupEntry& entry : bgEntries)
  {
    // We only need to check the binding because we already retrieved all the entries that
    // correspond to the group of the buffer
    if (entry.binding == static_cast<uint32_t>(buffer->GetBinding()))
    {
      // Replacing the buffer by the one we just recreated
      entry.buffer = this->BufferStorage->WebGPUBuffers[bufferIndex];

      break;
    }
  }

  // We need the bind group layout that the buffer belongs to to recreate the bind group.
  // The bind group layout is only created during a Dispatch().
  // If the user tries to resize the buffer before having called Dispatch(), we cannot recreate the
  // bind group because we don't have the bind group layout yet. This is why we're only recreating
  // the bind group if the group index can be found in the bind group layout vector.
  //
  // If the bind group layout doesn't exist yet and we cannot recreate the bind group, it's ok, the
  // Dispatch() call will do it. What matters in such a situation is that we recreated the buffer
  // with the right size so that the Dispatch() can create the right bind group
  const std::size_t group = buffer->GetGroup();
  if (group < this->BindGroupLayouts.size())
  {
    this->BindGroups[group] = vtkWebGPUBindGroupInternals::MakeBindGroup(
      this->WGPUConfiguration->GetDevice(), this->BindGroupLayouts[group], bgEntries);
  }

  this->BindGroupOrLayoutsInvalidated = true;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::RegisterBufferToPipeline(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer wgpuBuffer)
{
  this->AssociatedPipeline->RegisterBuffer(buffer, wgpuBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::RegisterTextureToPipeline(
  vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture wgpuTexture)
{
  this->AssociatedPipeline->RegisterTexture(texture, wgpuTexture);
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePassInternals::GetRegisteredBufferFromPipeline(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer& wgpuBuffer)
{
  return this->AssociatedPipeline->GetRegisteredBuffer(buffer, wgpuBuffer);
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePassInternals::GetRegisteredTextureFromPipeline(
  vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture& wgpuTexture)
{
  return this->AssociatedPipeline->GetRegisteredTexture(texture, wgpuTexture);
}

//------------------------------------------------------------------------------
wgpu::Buffer vtkWebGPUComputePassInternals::GetWGPUBuffer(std::size_t bufferIndex)
{
  return this->BufferStorage->GetWGPUBuffer(bufferIndex);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::RecreateTexture(int textureIndex)
{
  this->TextureStorage->RecreateTexture(textureIndex);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::RecreateTextureViews(int textureIndex)
{
  this->TextureStorage->RecreateTextureViews(textureIndex);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::RecreateTextureBindGroup(int textureIndex)
{
  if (!this->TextureStorage->CheckTextureIndex(textureIndex, "RecreateTextureBindGroup"))
  {
    return;
  }

  vtkSmartPointer<vtkWebGPUComputeTexture> texture = this->TextureStorage->Textures[textureIndex];

  // We're going to have to recreate the bind group entries for all the texture views that have been
  // created of this texture so we're getting all the views of this texture
  std::unordered_set<vtkSmartPointer<vtkWebGPUComputeTextureView>>& textureViews =
    this->TextureStorage->ComputeTextureToViews.find(texture)->second;

  for (const vtkSmartPointer<vtkWebGPUComputeTextureView>& textureView : textureViews)
  {
    // Finding the bind group entry of the texture view
    std::vector<wgpu::BindGroupEntry>& bgEntries = this->BindGroupEntries[textureView->GetGroup()];

    // Now iterating over all the entries of this group to find the one that has the same binding as
    // the texture view whose entry we're trying to recreate
    for (wgpu::BindGroupEntry& entry : bgEntries)
    {
      if (entry.binding == static_cast<uint32_t>(textureView->GetBinding()))
      {
        // Replacing the texture view by the new one (recreated by a previous call to
        // RecreateTexture())
        entry.textureView = this->TextureStorage->TextureViewsToWebGPUTextureViews[textureView];

        break;
      }
    }

    // Also recreating the bind group of this texture view. If we cannot find the bind group layout
    // of the current texture view, this means that the bind group layouts haven't been created yet.
    // This is probably because the user is trying to resize a texture before having call
    // Dispatch(): it is the Dispatch() call that creates the bind group layouts
    //
    // In this case, we have nothing to do and it is the Dispatch() call that will create the bind
    // group layouts for us
    //
    // Otherwise, if we could find the bind group layout, we need to recreate the bind group that
    // goes with it
    const std::size_t group = textureView->GetGroup();
    if (group < this->BindGroupLayouts.size())
    {
      this->BindGroups[group] = vtkWebGPUBindGroupInternals::MakeBindGroup(
        this->WGPUConfiguration->GetDevice(), this->BindGroupLayouts[group], bgEntries);
    }
  }

  this->BindGroupOrLayoutsInvalidated = true;
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayoutEntry vtkWebGPUComputePassInternals::CreateBindGroupLayoutEntry(
  uint32_t binding, vtkWebGPUComputeBuffer::BufferMode mode)
{
  wgpu::BufferBindingType bindingType =
    vtkWebGPUComputePassBufferStorageInternals::ComputeBufferModeToBufferBindingType(mode);

  vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper bglEntry{ binding,
    wgpu::ShaderStage::Compute, bindingType };

  return bglEntry;
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayoutEntry vtkWebGPUComputePassInternals::CreateBindGroupLayoutEntry(
  uint32_t binding, vtkSmartPointer<vtkWebGPUComputeTexture> computeTexture,
  vtkSmartPointer<vtkWebGPUComputeTextureView> textureView)
{
  wgpu::TextureViewDimension textureViewDimension =
    vtkWebGPUComputePassTextureStorageInternals::ComputeTextureDimensionToViewDimension(
      textureView->GetDimension());

  if (textureView->GetMode() == vtkWebGPUComputeTextureView::TextureViewMode::READ_ONLY)
  {
    // Not a storage texture

    vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper bglEntry(binding,
      wgpu::ShaderStage::Compute,
      vtkWebGPUComputePassTextureStorageInternals::ComputeTextureSampleTypeToWebGPU(
        computeTexture->GetSampleType()),
      textureViewDimension);

    return bglEntry;
  }
  else
  {
    // Storage texture
    wgpu::StorageTextureAccess storageAccess;
    wgpu::TextureFormat textureFormat;

    storageAccess =
      vtkWebGPUComputePassTextureStorageInternals::ComputeTextureViewModeToShaderStorage(
        textureView->GetMode(), textureView->GetLabel());
    textureFormat = vtkWebGPUComputePassTextureStorageInternals::ComputeTextureFormatToWebGPU(
      textureView->GetFormat());

    vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper bglEntry(
      binding, wgpu::ShaderStage::Compute, storageAccess, textureFormat, textureViewDimension);

    return bglEntry;
  }
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayoutEntry vtkWebGPUComputePassInternals::CreateBindGroupLayoutEntry(
  uint32_t binding, vtkSmartPointer<vtkWebGPUComputeTexture> computeTexture,
  wgpu::TextureViewDimension textureViewDimension)
{
  if (computeTexture->GetMode() == vtkWebGPUComputeTexture::TextureMode::READ_ONLY)
  {
    // Not a storage texture

    vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper bglEntry(binding,
      wgpu::ShaderStage::Compute,
      vtkWebGPUComputePassTextureStorageInternals::ComputeTextureSampleTypeToWebGPU(
        computeTexture->GetSampleType()),
      textureViewDimension);

    return bglEntry;
  }
  else
  {
    // Storage texture

    vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper bglEntry(binding,
      wgpu::ShaderStage::Compute,
      vtkWebGPUComputePassTextureStorageInternals::ComputeTextureModeToShaderStorage(
        computeTexture->GetMode(), computeTexture->GetLabel()),
      vtkWebGPUComputePassTextureStorageInternals::ComputeTextureFormatToWebGPU(
        computeTexture->GetFormat()),
      textureViewDimension);

    return bglEntry;
  }
}

//------------------------------------------------------------------------------
wgpu::BindGroupEntry vtkWebGPUComputePassInternals::CreateBindGroupEntry(wgpu::Buffer wgpuBuffer,
  uint32_t binding, vtkWebGPUComputeBuffer::BufferMode vtkNotUsed(mode), uint32_t offset)
{
  vtkWebGPUBindGroupInternals::BindingInitializationHelper bgEntry{ binding, wgpuBuffer, offset };

  return bgEntry.GetAsBinding();
}

//------------------------------------------------------------------------------
wgpu::BindGroupEntry vtkWebGPUComputePassInternals::CreateBindGroupEntry(
  uint32_t binding, wgpu::TextureView textureView)
{
  vtkWebGPUBindGroupInternals::BindingInitializationHelper bgEntry{ binding, textureView };

  return bgEntry.GetAsBinding();
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePassInternals::CheckBufferIndex(
  int bufferIndex, const std::string& callerFunctionName)
{
  return this->BufferStorage->CheckBufferIndex(bufferIndex, callerFunctionName);
}

//------------------------------------------------------------------------------
bool vtkWebGPUComputePassInternals::CheckBufferCorrectness(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer)
{
  return this->BufferStorage->CheckBufferCorrectness(buffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::WebGPUDispatch(
  unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ)
{
  if (groupsX * groupsY * groupsZ == 0)
  {
    vtkLogF(ERROR,
      "Invalid number of workgroups when dispatching compute pipeline \"%s\". Work groups sizes "
      "(X, Y, Z) were: (%u, %u, %u) but no dimensions can be 0.",
      this->ParentPass->Label.c_str(), groupsX, groupsY, groupsZ);

    return;
  }

  wgpu::CommandEncoder commandEncoder = this->CreateCommandEncoder();
  {
    vtkScopedEncoderDebugGroup(commandEncoder, this->ParentPass->GetLabel().c_str());
    wgpu::ComputePassEncoder computePassEncoder = CreateComputePassEncoder(commandEncoder);
    computePassEncoder.SetPipeline(this->ComputePipeline);
    for (std::size_t bindGroupIndex = 0; bindGroupIndex < this->BindGroups.size(); bindGroupIndex++)
    {
      computePassEncoder.SetBindGroup(bindGroupIndex, this->BindGroups[bindGroupIndex], 0, nullptr);
    }
    computePassEncoder.DispatchWorkgroups(groupsX, groupsY, groupsZ);
    computePassEncoder.End();
  }

  this->SubmitCommandEncoderToQueue(commandEncoder);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::CreateShaderModule()
{
  this->ShaderModule = vtkWebGPUShaderModuleInternals::CreateFromWGSL(
    this->WGPUConfiguration->GetDevice(), this->ParentPass->ShaderSource);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::CreateBindGroupsAndLayouts()
{
  this->BindGroupLayouts.clear();
  this->BindGroups.clear();

  this->BindGroupLayouts.resize(this->BindGroupLayoutEntries.size());
  this->BindGroups.resize(this->BindGroupLayoutEntries.size());

  for (const auto& mapEntry : this->BindGroupLayoutEntries)
  {
    int bindGroupIndex = mapEntry.first;

    const std::vector<wgpu::BindGroupLayoutEntry>& bglEntries =
      this->BindGroupLayoutEntries[bindGroupIndex];
    const std::vector<wgpu::BindGroupEntry>& bgEntries = this->BindGroupEntries[bindGroupIndex];

    this->BindGroupLayouts[bindGroupIndex] =
      CreateBindGroupLayout(this->WGPUConfiguration->GetDevice(), bglEntries);
    this->BindGroups[bindGroupIndex] = vtkWebGPUBindGroupInternals::MakeBindGroup(
      this->WGPUConfiguration->GetDevice(), BindGroupLayouts[bindGroupIndex], bgEntries);
  }
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUComputePassInternals::CreateBindGroupLayout(
  const wgpu::Device& device, const std::vector<wgpu::BindGroupLayoutEntry>& layoutEntries)
{
  wgpu::BindGroupLayout bgl =
    vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device, layoutEntries);
  return bgl;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::SetupRenderBuffer(
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer)
{
  this->BufferStorage->SetupRenderBuffer(renderBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::RecreateRenderTexture(
  vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture)
{
  this->TextureStorage->RecreateRenderTexture(renderTexture);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::CreateWebGPUComputePipeline()
{
  wgpu::ComputePipelineDescriptor computePipelineDescriptor;
  computePipelineDescriptor.compute.constantCount = 0;
  computePipelineDescriptor.compute.constants = nullptr;
  computePipelineDescriptor.compute.entryPoint = this->ParentPass->ShaderEntryPoint.c_str();
  computePipelineDescriptor.compute.module = this->ShaderModule;
  computePipelineDescriptor.compute.nextInChain = nullptr;
  computePipelineDescriptor.label = this->ParentPass->WGPUComputePipelineLabel.c_str();
  computePipelineDescriptor.layout = this->CreateWebGPUComputePipelineLayout();

  this->ComputePipeline =
    this->WGPUConfiguration->GetDevice().CreateComputePipeline(&computePipelineDescriptor);
}

//------------------------------------------------------------------------------
wgpu::PipelineLayout vtkWebGPUComputePassInternals::CreateWebGPUComputePipelineLayout()
{
  wgpu::PipelineLayoutDescriptor computePipelineLayoutDescriptor;
  computePipelineLayoutDescriptor.bindGroupLayoutCount = this->BindGroupLayouts.size();
  computePipelineLayoutDescriptor.bindGroupLayouts = this->BindGroupLayouts.data();
  computePipelineLayoutDescriptor.nextInChain = nullptr;

  return this->WGPUConfiguration->GetDevice().CreatePipelineLayout(
    &computePipelineLayoutDescriptor);
}

wgpu::CommandEncoder vtkWebGPUComputePassInternals::CreateCommandEncoder()
{
  wgpu::CommandEncoderDescriptor commandEncoderDescriptor;
  commandEncoderDescriptor.label = this->ParentPass->WGPUCommandEncoderLabel.c_str();

  return this->WGPUConfiguration->GetDevice().CreateCommandEncoder(&commandEncoderDescriptor);
}

//------------------------------------------------------------------------------
wgpu::ComputePassEncoder vtkWebGPUComputePassInternals::CreateComputePassEncoder(
  const wgpu::CommandEncoder& commandEncoder)
{
  wgpu::ComputePassDescriptor computePassDescriptor;
  computePassDescriptor.nextInChain = nullptr;
  computePassDescriptor.timestampWrites = 0;
  return commandEncoder.BeginComputePass(&computePassDescriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::SubmitCommandEncoderToQueue(
  const wgpu::CommandEncoder& commandEncoder)
{
  wgpu::CommandBuffer commandBuffer = commandEncoder.Finish();
  this->WGPUConfiguration->GetDevice().GetQueue().Submit(1, &commandBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePassInternals::ReleaseResources()
{
  this->Initialized = false;
  this->BindGroupOrLayoutsInvalidated = true;

  this->ShaderModule = nullptr;

  this->BindGroups.clear();
  this->BindGroupEntries.clear();
  this->BindGroupLayouts.clear();
  this->BindGroupLayoutEntries.clear();

  this->ComputePipeline = nullptr;

  this->TextureStorage->ReleaseResources();
  this->BufferStorage->ReleaseResources();
}

VTK_ABI_NAMESPACE_END
