// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUBatchedLabeledDataMapperInternals_h
#define vtkWebGPUBatchedLabeledDataMapperInternals_h

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWebGPUPolyDataMapper.h"

#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkWebGPUBatchedLabeledDataMapper;

class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUBatchedLabeledDataMapperInternals
  : public vtkWebGPUPolyDataMapper
{
public:
  static vtkWebGPUBatchedLabeledDataMapperInternals* New();
  vtkTypeMacro(vtkWebGPUBatchedLabeledDataMapperInternals, vtkWebGPUPolyDataMapper);

  vtkWebGPUBatchedLabeledDataMapper* Parent = nullptr;

  uint32_t NumberOfLabels = 0;

  wgpu::Texture GlyphsTexture;
  wgpu::TextureView GlyphsTextureView;
  wgpu::Sampler GlyphsSampler;
  wgpu::Buffer LabelUniformBuffer;

  enum InstanceAttrib : int
  {
    GLYPH_EXTENTS = 0,
    COFF_PROPID = 1,
    FRAME_COLORS = 2,
    NUM_INSTANCE_ATTRIBS = 3
  };
  wgpu::Buffer InstanceBuffers[NUM_INSTANCE_ATTRIBS];
  uint64_t InstanceBufferSizes[NUM_INSTANCE_ATTRIBS] = { 0, 0, 0 };
  wgpu::VertexAttribute InstanceAttributes[3];
  bool LabelCountChanged = false;

  void InvalidatePipelines() { this->RebuildGraphicsPipelines = true; }

  void RenderPiece(vtkRenderer* renderer, vtkActor* actor) override;
  void ReleaseGraphicsResources(vtkWindow* window) override;

protected:
  vtkWebGPUBatchedLabeledDataMapperInternals() = default;
  ~vtkWebGPUBatchedLabeledDataMapperInternals() override = default;

  bool IsPipelineSupported(GraphicsPipelineType pipelineType) override;
  DrawCallArgs GetDrawCallArgs(GraphicsPipelineType pipelineType,
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType) override;
  DrawCallArgs GetDrawCallArgsForDrawingVertices(
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType) override;
  std::vector<wgpu::VertexBufferLayout> GetVertexBufferLayouts() override;
  void SetVertexBuffers(const wgpu::RenderPassEncoder& encoder) override;
  void SetVertexBuffers(const wgpu::RenderBundleEncoder& encoder) override;
  std::vector<wgpu::BindGroupLayoutEntry> GetMeshBindGroupLayoutEntries() override;
  std::vector<wgpu::BindGroupEntry> GetMeshBindGroupEntries() override;

  void ReplaceShaderConstantsDef(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss, std::string& fss) override;
  void ReplaceShaderCustomDef(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss, std::string& fss) override;
  void ReplaceShaderCustomBindings(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss, std::string& fss) override;
  void ReplaceVertexShaderInputDef(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss) override;
  void ReplaceVertexShaderVertexId(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss) override;
  void ReplaceVertexShaderPosition(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss) override;
  void ReplaceVertexShaderColors(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss) override;
  void ReplaceVertexShaderUVs(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss) override;
  void ReplaceVertexShaderNormals(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss) override;
  void ReplaceVertexShaderTangents(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss) override;
  void ReplaceFragmentShaderColors(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& fss) override;
  void ReplaceFragmentShaderNormals(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& fss) override;
  void ReplaceFragmentShaderLights(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& fss) override;
  void ReplaceFragmentShaderCoincidentOffset(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& fss) override;

private:
  vtkWebGPUBatchedLabeledDataMapperInternals(
    const vtkWebGPUBatchedLabeledDataMapperInternals&) = delete;
  void operator=(const vtkWebGPUBatchedLabeledDataMapperInternals&) = delete;

  void UpdateInstanceBuffers(vtkWebGPUConfiguration* wgpuConfiguration);
  void UpdateUniformBuffer(vtkRenderer* renderer, vtkWebGPUConfiguration* wgpuConfiguration);
};
VTK_ABI_NAMESPACE_END

#endif
