// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"

#include "Private/vtkWebGPUHelpersPrivate.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkWebGPURenderPipelineDescriptorInternals::vtkWebGPURenderPipelineDescriptorInternals()
  : WGPURenderPipelineDescriptor(WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT)
  , cFragment(WGPU_FRAGMENT_STATE_INIT)
  , cDepthStencil(WGPU_DEPTH_STENCIL_STATE_INIT)
{
  // The C structs carry no defaults of their own, so every member the rest of
  // VTK relies on is spelled out here. Start from the *_INIT macros rather than
  // zeroing, because the WebGPU defaults are not all zero.

  // Set defaults for the vertex state.
  {
    WGPUVertexState* dVertex = &this->vertex;
    dVertex->module = nullptr;
    dVertex->entryPoint = WGPUStringView{ "main", WGPU_STRLEN };
    dVertex->bufferCount = 0;

    // Fill the default values for vertexBuffers and vertexAttributes in buffers.
    for (uint32_t i = 0; i < kMaxVertexAttributes; ++i)
    {
      this->cAttributes[i] = WGPU_VERTEX_ATTRIBUTE_INIT;
      this->cAttributes[i].shaderLocation = 0;
      this->cAttributes[i].offset = 0;
      this->cAttributes[i].format = WGPUVertexFormat_Float32;
    }
    for (uint32_t i = 0; i < kMaxVertexBuffers; ++i)
    {
      this->cBuffers[i] = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
      this->cBuffers[i].arrayStride = 0;
      this->cBuffers[i].stepMode = WGPUVertexStepMode_Vertex;
      this->cBuffers[i].attributeCount = 0;
      this->cBuffers[i].attributes = nullptr;
    }
    // cBuffers[i].attributes points to somewhere in cAttributes.
    // cBuffers[0].attributes points to &cAttributes[0] by default. Assuming
    // cBuffers[0] has two attributes, then cBuffers[1].attributes should point to
    // &cAttributes[2]. Likewise, if cBuffers[1] has 3 attributes, then
    // cBuffers[2].attributes should point to &cAttributes[5].
    this->cBuffers[0].attributes = &this->cAttributes[0];
    dVertex->buffers = &this->cBuffers[0];
  }

  // Set the defaults for the primitive state
  {
    WGPUPrimitiveState* wPrimitive = &this->primitive;
    wPrimitive->topology = WGPUPrimitiveTopology_TriangleList;
    wPrimitive->stripIndexFormat = WGPUIndexFormat_Undefined;
    wPrimitive->frontFace = WGPUFrontFace_CCW;
    wPrimitive->cullMode = WGPUCullMode_None;
  }

  // Set the defaults for the depth-stencil state
  {
    WGPUStencilFaceState stencilFace = WGPU_STENCIL_FACE_STATE_INIT;
    stencilFace.compare = WGPUCompareFunction_Always;
    stencilFace.failOp = WGPUStencilOperation_Keep;
    stencilFace.depthFailOp = WGPUStencilOperation_Keep;
    stencilFace.passOp = WGPUStencilOperation_Keep;

    this->cDepthStencil.format = WGPUTextureFormat_Depth24PlusStencil8;
    this->cDepthStencil.depthWriteEnabled = WGPUOptionalBool_False;
    this->cDepthStencil.depthCompare = WGPUCompareFunction_Always;
    this->cDepthStencil.stencilBack = stencilFace;
    this->cDepthStencil.stencilFront = stencilFace;
    this->cDepthStencil.stencilReadMask = 0xff;
    this->cDepthStencil.stencilWriteMask = 0xff;
    this->cDepthStencil.depthBias = 0;
    this->cDepthStencil.depthBiasSlopeScale = 0.0;
    this->cDepthStencil.depthBiasClamp = 0.0;
  }

  // Set the defaults for the multisample state
  {
    WGPUMultisampleState* dMultisample = &this->multisample;
    dMultisample->count = 1;
    dMultisample->mask = 0xFFFFFFFF;
    dMultisample->alphaToCoverageEnabled = false;
  }

  // Set the defaults for the fragment state
  {
    this->cFragment.module = nullptr;
    this->cFragment.entryPoint = WGPUStringView{ "main", WGPU_STRLEN };
    this->cFragment.targetCount = 1;
    this->cFragment.targets = this->cTargets.data();
    this->fragment = &this->cFragment;

    WGPUBlendComponent colorBlendComponent = WGPU_BLEND_COMPONENT_INIT;
    colorBlendComponent.srcFactor = WGPUBlendFactor_SrcAlpha;
    colorBlendComponent.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    colorBlendComponent.operation = WGPUBlendOperation_Add;
    WGPUBlendComponent alphaBlendComponent = WGPU_BLEND_COMPONENT_INIT;
    alphaBlendComponent.srcFactor = WGPUBlendFactor_One;
    alphaBlendComponent.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    alphaBlendComponent.operation = WGPUBlendOperation_Add;

    for (uint32_t i = 0; i < kMaxColorAttachments; ++i)
    {
      this->cTargets[i] = WGPU_COLOR_TARGET_STATE_INIT;
      this->cTargets[i].format = WGPUTextureFormat_RGBA8Unorm;
      this->cTargets[i].writeMask = WGPUColorWriteMask_All;

      this->cBlends[i] = WGPU_BLEND_STATE_INIT;
      this->cBlends[i].color = colorBlendComponent;
      this->cBlends[i].alpha = alphaBlendComponent;
    }
  }
}

//------------------------------------------------------------------------------
WGPUDepthStencilState* vtkWebGPURenderPipelineDescriptorInternals::EnableDepthStencil(
  WGPUTextureFormat format)
{
  this->depthStencil = &this->cDepthStencil;
  this->cDepthStencil.format = format;
  return &this->cDepthStencil;
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineDescriptorInternals::DisableDepthStencil()
{
  this->depthStencil = nullptr;
}

//------------------------------------------------------------------------------
WGPUBlendState* vtkWebGPURenderPipelineDescriptorInternals::EnableBlending(
  std::size_t colorTargetId)
{
  this->cTargets[colorTargetId].blend = &this->cBlends[colorTargetId];
  return &this->cBlends[colorTargetId];
}

//------------------------------------------------------------------------------
void vtkWebGPURenderPipelineDescriptorInternals::DisableBlending(std::size_t colorTargetId)
{
  this->cTargets[colorTargetId].blend = nullptr;
}

VTK_ABI_NAMESPACE_END
