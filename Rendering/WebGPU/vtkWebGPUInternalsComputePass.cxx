// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUInternalsComputePass.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUComputePipeline.h"
#include "vtkWebGPUInternalsShaderModule.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUInternalsComputePass);

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Initialized? : " << this->Initialized << std::endl;
  os << indent << "BindGroupOrLayoutsInvalidated? : " << this->BindGroupOrLayoutsInvalidated
     << std::endl;

  os << indent << "wgpuDevice: " << this->Device.Get() << std::endl;

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
void vtkWebGPUInternalsComputePass::SetParentPass(vtkWeakPointer<vtkWebGPUComputePass> parentPass)
{
  this->ParentPass = parentPass;
}

//------------------------------------------------------------------------------
wgpu::Device vtkWebGPUInternalsComputePass::GetDevice()
{
  return this->Device;
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::SetDevice(wgpu::Device device)
{
  this->Device = device;
  this->TextureStorage->SetParentDevice(device);
  this->BufferStorage->SetParentDevice(device);
}

//------------------------------------------------------------------------------
vtkWeakPointer<vtkWebGPUComputePipeline> vtkWebGPUInternalsComputePass::GetAssociatedPipeline()
{
  return this->AssociatedPipeline;
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::SetAssociatedPipeline(
  vtkWeakPointer<vtkWebGPUComputePipeline> associatedPipeline)
{
  this->AssociatedPipeline = associatedPipeline;
}

//------------------------------------------------------------------------------
bool vtkWebGPUInternalsComputePass::CheckTextureIndex(
  int textureIndex, const std::string& callerFunctionName)
{
  return this->TextureStorage->CheckTextureIndex(textureIndex, callerFunctionName);
}

//------------------------------------------------------------------------------
bool vtkWebGPUInternalsComputePass::CheckTextureViewIndex(
  int textureViewIndex, const std::string& callerFunctionName)
{
  return this->TextureStorage->CheckTextureViewIndex(textureViewIndex, callerFunctionName);
}

//------------------------------------------------------------------------------
wgpu::TextureView vtkWebGPUInternalsComputePass::CreateWebGPUTextureView(
  vtkSmartPointer<vtkWebGPUComputeTextureView> textureView, wgpu::Texture wgpuTexture)
{
  return this->TextureStorage->CreateWebGPUTextureView(textureView, wgpuTexture);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::UpdateWebGPUBuffer(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer wgpuBuffer)
{
  this->BufferStorage->UpdateWebGPUBuffer(buffer, wgpuBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::UpdateComputeTextureAndViews(
  vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture newWgpuTexture)
{
  this->TextureStorage->UpdateComputeTextureAndViews(texture, newWgpuTexture);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::RecreateBuffer(int bufferIndex, vtkIdType newByteSize)
{
  this->BufferStorage->RecreateBuffer(bufferIndex, newByteSize);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::RecreateBufferBindGroup(int bufferIndex)
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
    if (entry.binding == buffer->GetBinding())
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
  int group = buffer->GetGroup();
  if (group < this->BindGroupLayouts.size())
  {
    this->BindGroups[group] = vtkWebGPUInternalsBindGroup::MakeBindGroup(
      this->Device, this->BindGroupLayouts[group], bgEntries);
  }

  this->BindGroupOrLayoutsInvalidated = true;
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::RegisterBufferToPipeline(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer wgpuBuffer)
{
  this->AssociatedPipeline->RegisterBuffer(buffer, wgpuBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::RegisterTextureToPipeline(
  vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture wgpuTexture)
{
  this->AssociatedPipeline->RegisterTexture(texture, wgpuTexture);
}

//------------------------------------------------------------------------------
bool vtkWebGPUInternalsComputePass::GetRegisteredBufferFromPipeline(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer, wgpu::Buffer& wgpuBuffer)
{
  return this->AssociatedPipeline->GetRegisteredBuffer(buffer, wgpuBuffer);
}

//------------------------------------------------------------------------------
bool vtkWebGPUInternalsComputePass::GetRegisteredTextureFromPipeline(
  vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture& wgpuTexture)
{
  return this->AssociatedPipeline->GetRegisteredTexture(texture, wgpuTexture);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::RecreateTexture(int textureIndex)
{
  this->TextureStorage->RecreateTexture(textureIndex);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::RecreateTextureViews(int textureIndex)
{
  this->TextureStorage->RecreateTextureViews(textureIndex);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::RecreateTextureBindGroup(int textureIndex)
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
      if (entry.binding == textureView->GetBinding())
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
    int group = textureView->GetGroup();
    if (group < this->BindGroupLayouts.size())
    {
      this->BindGroups[group] = vtkWebGPUInternalsBindGroup::MakeBindGroup(
        this->Device, this->BindGroupLayouts[group], bgEntries);
    }
  }

  this->BindGroupOrLayoutsInvalidated = true;
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayoutEntry vtkWebGPUInternalsComputePass::CreateBindGroupLayoutEntry(
  uint32_t binding, vtkWebGPUComputeBuffer::BufferMode mode)
{
  wgpu::BufferBindingType bindingType =
    vtkWebGPUInternalsComputePassBufferStorage::ComputeBufferModeToBufferBindingType(mode);

  vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper bglEntry{ binding,
    wgpu::ShaderStage::Compute, bindingType };

  return bglEntry;
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayoutEntry vtkWebGPUInternalsComputePass::CreateBindGroupLayoutEntry(
  uint32_t binding, vtkSmartPointer<vtkWebGPUComputeTexture> computeTexture,
  vtkSmartPointer<vtkWebGPUComputeTextureView> textureView)
{
  wgpu::TextureViewDimension textureViewDimension =
    vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureDimensionToViewDimension(
      textureView->GetDimension());

  if (textureView->GetMode() == vtkWebGPUComputeTextureView::TextureViewMode::READ_ONLY)
  {
    // Not a storage texture

    vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper bglEntry(binding,
      wgpu::ShaderStage::Compute,
      vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureSampleTypeToWebGPU(
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
      vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureViewModeToShaderStorage(
        textureView->GetMode(), textureView->GetLabel());
    textureFormat = vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureFormatToWebGPU(
      textureView->GetFormat());

    vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper bglEntry(
      binding, wgpu::ShaderStage::Compute, storageAccess, textureFormat, textureViewDimension);

    return bglEntry;
  }
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayoutEntry vtkWebGPUInternalsComputePass::CreateBindGroupLayoutEntry(
  uint32_t binding, vtkSmartPointer<vtkWebGPUComputeTexture> computeTexture,
  wgpu::TextureViewDimension textureViewDimension)
{
  if (computeTexture->GetMode() == vtkWebGPUComputeTexture::TextureMode::READ_ONLY)
  {
    // Not a storage texture

    vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper bglEntry(binding,
      wgpu::ShaderStage::Compute,
      vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureSampleTypeToWebGPU(
        computeTexture->GetSampleType()),
      textureViewDimension);

    return bglEntry;
  }
  else
  {
    // Storage texture

    vtkWebGPUInternalsBindGroupLayout::LayoutEntryInitializationHelper bglEntry(binding,
      wgpu::ShaderStage::Compute,
      vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureModeToShaderStorage(
        computeTexture->GetMode(), computeTexture->GetLabel()),
      vtkWebGPUInternalsComputePassTextureStorage::ComputeTextureFormatToWebGPU(
        computeTexture->GetFormat()),
      textureViewDimension);

    return bglEntry;
  }
}

//------------------------------------------------------------------------------
wgpu::BindGroupEntry vtkWebGPUInternalsComputePass::CreateBindGroupEntry(wgpu::Buffer wgpuBuffer,
  uint32_t binding, vtkWebGPUComputeBuffer::BufferMode mode, uint32_t offset)
{
  wgpu::BufferBindingType bindingType =
    vtkWebGPUInternalsComputePassBufferStorage::ComputeBufferModeToBufferBindingType(mode);

  vtkWebGPUInternalsBindGroup::BindingInitializationHelper bgEntry{ binding, wgpuBuffer, offset };

  return bgEntry.GetAsBinding();
}

//------------------------------------------------------------------------------
wgpu::BindGroupEntry vtkWebGPUInternalsComputePass::CreateBindGroupEntry(
  uint32_t binding, wgpu::TextureView textureView)
{
  vtkWebGPUInternalsBindGroup::BindingInitializationHelper bgEntry{ binding, textureView };

  return bgEntry.GetAsBinding();
}

//------------------------------------------------------------------------------
bool vtkWebGPUInternalsComputePass::CheckBufferIndex(
  int bufferIndex, const std::string& callerFunctionName)
{
  return this->BufferStorage->CheckBufferIndex(bufferIndex, callerFunctionName);
}

//------------------------------------------------------------------------------
bool vtkWebGPUInternalsComputePass::CheckBufferCorrectness(
  vtkSmartPointer<vtkWebGPUComputeBuffer> buffer)
{
  return this->BufferStorage->CheckBufferCorrectness(buffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::WebGPUDispatch(
  unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ)
{
  if (groupsX * groupsY * groupsZ == 0)
  {
    vtkLogF(ERROR,
      "Invalid number of workgroups when dispatching compute pipeline \"%s\". Work groups sizes "
      "(X, Y, Z) were: (%d, %d, %d) but no dimensions can be 0.",
      this->ParentPass->Label.c_str(), groupsX, groupsY, groupsZ);

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
void vtkWebGPUInternalsComputePass::CreateShaderModule()
{
  this->ShaderModule =
    vtkWebGPUInternalsShaderModule::CreateFromWGSL(this->Device, this->ParentPass->ShaderSource);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::CreateBindGroupsAndLayouts()
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

    this->BindGroupLayouts[bindGroupIndex] = CreateBindGroupLayout(this->Device, bglEntries);
    this->BindGroups[bindGroupIndex] = vtkWebGPUInternalsBindGroup::MakeBindGroup(
      this->Device, BindGroupLayouts[bindGroupIndex], bgEntries);
  }
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUInternalsComputePass::CreateBindGroupLayout(
  const wgpu::Device& device, const std::vector<wgpu::BindGroupLayoutEntry>& layoutEntries)
{
  wgpu::BindGroupLayout bgl =
    vtkWebGPUInternalsBindGroupLayout::MakeBindGroupLayout(device, layoutEntries);
  return bgl;
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::SetupRenderBuffer(
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer)
{
  this->BufferStorage->SetupRenderBuffer(renderBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::SetupRenderTexture(
  vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture,
  wgpu::TextureViewDimension textureViewDimension, wgpu::TextureView textureView)
{
  this->TextureStorage->SetupRenderTexture(renderTexture, textureViewDimension, textureView);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::RecreateRenderTexture(
  vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture,
  wgpu::TextureViewDimension textureViewDimension, wgpu::TextureView textureView)
{
  this->TextureStorage->RecreateRenderTexture(renderTexture, textureViewDimension, textureView);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::CreateWebGPUComputePipeline()
{
  wgpu::ComputePipelineDescriptor computePipelineDescriptor;
  computePipelineDescriptor.compute.constantCount = 0;
  computePipelineDescriptor.compute.constants = nullptr;
  computePipelineDescriptor.compute.entryPoint = this->ParentPass->ShaderEntryPoint.c_str();
  computePipelineDescriptor.compute.module = this->ShaderModule;
  computePipelineDescriptor.compute.nextInChain = nullptr;
  computePipelineDescriptor.label = this->ParentPass->WGPUComputePipelineLabel.c_str();
  computePipelineDescriptor.layout = this->CreateWebGPUComputePipelineLayout();

  this->ComputePipeline = this->Device.CreateComputePipeline(&computePipelineDescriptor);
}

//------------------------------------------------------------------------------
wgpu::PipelineLayout vtkWebGPUInternalsComputePass::CreateWebGPUComputePipelineLayout()
{
  wgpu::PipelineLayoutDescriptor computePipelineLayoutDescriptor;
  computePipelineLayoutDescriptor.bindGroupLayoutCount = this->BindGroupLayouts.size();
  computePipelineLayoutDescriptor.bindGroupLayouts = this->BindGroupLayouts.data();
  computePipelineLayoutDescriptor.nextInChain = nullptr;

  return this->Device.CreatePipelineLayout(&computePipelineLayoutDescriptor);
}

wgpu::CommandEncoder vtkWebGPUInternalsComputePass::CreateCommandEncoder()
{
  wgpu::CommandEncoderDescriptor commandEncoderDescriptor;
  commandEncoderDescriptor.label = this->ParentPass->WGPUCommandEncoderLabel.c_str();

  return this->Device.CreateCommandEncoder(&commandEncoderDescriptor);
}

//------------------------------------------------------------------------------
wgpu::ComputePassEncoder vtkWebGPUInternalsComputePass::CreateComputePassEncoder(
  const wgpu::CommandEncoder& commandEncoder)
{
  wgpu::ComputePassDescriptor computePassDescriptor;
  computePassDescriptor.nextInChain = nullptr;
  computePassDescriptor.timestampWrites = 0;
  return commandEncoder.BeginComputePass(&computePassDescriptor);
}

//------------------------------------------------------------------------------
void vtkWebGPUInternalsComputePass::SubmitCommandEncoderToQueue(
  const wgpu::CommandEncoder& commandEncoder)
{
  wgpu::CommandBuffer commandBuffer = commandEncoder.Finish();
  this->Device.GetQueue().Submit(1, &commandBuffer);
}

VTK_ABI_NAMESPACE_END
