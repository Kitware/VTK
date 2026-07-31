// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPURenderPassDescriptorInternals.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPURenderPassDescriptorInternals::~vtkWebGPURenderPassDescriptorInternals() = default;

//------------------------------------------------------------------------------
vtkWebGPURenderPassDescriptorInternals::vtkWebGPURenderPassDescriptorInternals(
  const std::vector<WGPUTextureView>& colorAttachmentInfo,
  WGPUTextureView depthStencil /*= nullptr*/, bool clearColor /*= true*/,
  bool clearDepth /*= true*/, bool clearStencil /*= true*/)
  : WGPURenderPassDescriptor(WGPU_RENDER_PASS_DESCRIPTOR_INIT)
{
  const WGPULoadOp colorLoadOp = clearColor ? WGPULoadOp_Clear : WGPULoadOp_Load;
  for (uint32_t i = 0; i < kMaxColorAttachments; ++i)
  {
    this->ColorAttachments[i] = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    this->ColorAttachments[i].loadOp = colorLoadOp;
    this->ColorAttachments[i].storeOp = WGPUStoreOp_Store;
    this->ColorAttachments[i].clearValue = { 0.0, 0.0, 0.0, 0.0 };
  }

  const WGPULoadOp depthLoadOp = clearDepth ? WGPULoadOp_Clear : WGPULoadOp_Load;
  const WGPULoadOp stencilLoadOp = clearStencil ? WGPULoadOp_Clear : WGPULoadOp_Load;
  this->DepthStencilAttachmentInfo.depthClearValue = 1.0f;
  this->DepthStencilAttachmentInfo.stencilClearValue = 0;
  this->DepthStencilAttachmentInfo.depthLoadOp = depthLoadOp;
  this->DepthStencilAttachmentInfo.depthStoreOp = WGPUStoreOp_Store;
  this->DepthStencilAttachmentInfo.stencilLoadOp = stencilLoadOp;
  this->DepthStencilAttachmentInfo.stencilStoreOp = WGPUStoreOp_Store;

  colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfo.size());
  uint32_t colorAttachmentIndex = 0;
  for (const WGPUTextureView& colorAttachment : colorAttachmentInfo)
  {
    if (colorAttachment != nullptr)
    {
      this->ColorAttachments[colorAttachmentIndex].view = colorAttachment;
    }
    ++colorAttachmentIndex;
  }

  if (!this->ColorAttachments.empty())
  {
    colorAttachments = this->ColorAttachments.data();
  }
  else
  {
    colorAttachments = nullptr;
  }

  if (depthStencil != nullptr)
  {
    this->DepthStencilAttachmentInfo.view = depthStencil;
    depthStencilAttachment = &this->DepthStencilAttachmentInfo;
  }
  else
  {
    depthStencilAttachment = nullptr;
  }
}

//------------------------------------------------------------------------------
vtkWebGPURenderPassDescriptorInternals::vtkWebGPURenderPassDescriptorInternals(
  const vtkWebGPURenderPassDescriptorInternals& other)
  : WGPURenderPassDescriptor(other)
{
  *this = other;
}

//------------------------------------------------------------------------------
const vtkWebGPURenderPassDescriptorInternals& vtkWebGPURenderPassDescriptorInternals::operator=(
  const vtkWebGPURenderPassDescriptorInternals& otherRenderPass)
{
  this->DepthStencilAttachmentInfo = otherRenderPass.DepthStencilAttachmentInfo;
  this->ColorAttachments = otherRenderPass.ColorAttachments;
  colorAttachmentCount = otherRenderPass.colorAttachmentCount;
  colorAttachments = this->ColorAttachments.data();
  if (otherRenderPass.depthStencilAttachment != nullptr)
  {
    // Assign desc.depthStencilAttachment to this->depthStencilAttachmentInfo;
    depthStencilAttachment = &(this->DepthStencilAttachmentInfo);
  }
  else
  {
    depthStencilAttachment = nullptr;
  }
  return *this;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPassDescriptorInternals::UnsetDepthStencilLoadStoreOpsForFormat(
  WGPUTextureFormat format)
{
  switch (format)
  {
    case WGPUTextureFormat_Depth24Plus:
    case WGPUTextureFormat_Depth32Float:
    case WGPUTextureFormat_Depth16Unorm:
      this->DepthStencilAttachmentInfo.stencilLoadOp = WGPULoadOp_Undefined;
      this->DepthStencilAttachmentInfo.stencilStoreOp = WGPUStoreOp_Undefined;
      break;
    case WGPUTextureFormat_Stencil8:
      this->DepthStencilAttachmentInfo.depthLoadOp = WGPULoadOp_Undefined;
      this->DepthStencilAttachmentInfo.depthStoreOp = WGPUStoreOp_Undefined;
      break;
    default:
      break;
  }
}
VTK_ABI_NAMESPACE_END
