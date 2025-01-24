// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUPolyDataMapper2DInternals_h
#define vtkWebGPUPolyDataMapper2DInternals_h

#include "vtkNew.h"
#include "vtkRenderingWebGPUModule.h"
#include "vtkSmartPointer.h"
#include "vtkTimeStamp.h"
#include "vtkWebGPUCellToPrimitiveConverter.h"

#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkPoints;
class vtkMatrix4x4;
class vtkViewport;
class vtkActor2D;
class vtkWebGPUPolyDataMapper2D;

/**
 * Internal implementation details of vtkWebGPUPolyDataMapper2D
 */
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUPolyDataMapper2DInternals
{
  /**
   * This mapper uses different `wgpu::RenderPipeline` to render
   * a list of primitives. Each pipeline uses an appropriate
   * shader module, bindgroup and primitive type.
   */
  enum GraphicsPipeline2DType : int
  {
    // Pipeline that renders points
    GFX_PIPELINE_2D_POINTS = 0,
    // Pipeline that renders lines
    GFX_PIPELINE_2D_LINES,
    // Pipeline that renders triangles
    GFX_PIPELINE_2D_TRIANGLES,
    NUM_GFX_PIPELINE_2D_NB_TYPES
  };

  struct Mapper2DState
  {
    vtkTypeFloat32 WCVCMatrix[4][4];
    vtkTypeFloat32 Color[4];
    vtkTypeFloat32 PointSize;
    vtkTypeFloat32 LineWidth;
    vtkTypeUInt32 Flags;
    vtkTypeUInt32 Padding;
  };

  struct MeshAttributeArrayDescriptor
  {
    vtkTypeUInt32 Start = 0;
    vtkTypeUInt32 NumTuples = 0;
    vtkTypeUInt32 NumComponents = 0;
  };

  struct MeshDescriptor
  {
    MeshAttributeArrayDescriptor Positions;
    MeshAttributeArrayDescriptor UVs;
    MeshAttributeArrayDescriptor Colors;
  };

  struct ShaderSSBO
  {
    wgpu::Buffer Buffer;
    std::size_t Size;
    vtkTimeStamp BuildTimeStamp;
  };

  struct TopologyBindGroupInfo
  {
    // buffer for the primitive cell ids and point ids.
    wgpu::Buffer TopologyBuffer;
    // // buffer for indirect draw command
    // wgpu::Buffer IndirectDrawBuffer;
    // bind group for the primitive size uniform.
    wgpu::BindGroup BindGroup;
    // vertexCount for draw call.
    vtkTypeUInt32 VertexCount = 0;
  };

  bool RebuildGraphicsPipelines = false;
  bool UseCellScalarMapping = false;
  bool UsePointScalarMapping = false;

  std::map<GraphicsPipeline2DType, vtkWebGPUCellToPrimitiveConverter::TopologySourceType>
    PipelineBindGroupCombos = {
      { GFX_PIPELINE_2D_POINTS, vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS },
      { GFX_PIPELINE_2D_LINES, vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES },
      { GFX_PIPELINE_2D_TRIANGLES, vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS },
    };

  const std::array<wgpu::PrimitiveTopology, NUM_GFX_PIPELINE_2D_NB_TYPES>
    GraphicsPipeline2DPrimitiveTypes = { wgpu::PrimitiveTopology::TriangleStrip,
      wgpu::PrimitiveTopology::TriangleStrip, wgpu::PrimitiveTopology::TriangleList };
  const std::array<std::string, NUM_GFX_PIPELINE_2D_NB_TYPES> VertexShaderEntryPoints = {
    "pointVertexMain", "lineVertexMain", "polygonVertexMain"
  };

  std::string GraphicsPipeline2DKeys[NUM_GFX_PIPELINE_2D_NB_TYPES];

  vtkSmartPointer<vtkPoints> TransformedPoints;

  vtkNew<vtkMatrix4x4> WCVCMatrix;
  Mapper2DState State;
  MeshDescriptor MeshArraysDescriptor;

  ShaderSSBO Mapper2DStateData;
  ShaderSSBO AttributeDescriptorData;
  ShaderSSBO MeshData;

  TopologyBindGroupInfo
    TopologyBindGroupInfos[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES] = {};

  wgpu::BindGroup MeshAttributeBindGroup;

  vtkNew<vtkWebGPUCellToPrimitiveConverter> CellConverter;

  /**
   * Create a bind group layout for the mesh attribute bind group.
   */
  static wgpu::BindGroupLayout CreateMeshAttributeBindGroupLayout(
    const wgpu::Device& device, const std::string& label);

  /**
   * Create a bind group layout for the `TopologyRenderInfo::BindGroup`
   */
  static wgpu::BindGroupLayout CreateTopologyBindGroupLayout(
    const wgpu::Device& device, const std::string& label);

  /**
   * Get the name of the graphics pipeline type as a string.
   */
  static const char* GetGraphicsPipelineTypeAsString(GraphicsPipeline2DType graphicsPipelineType);

public:
  vtkWebGPUPolyDataMapper2DInternals();
  ~vtkWebGPUPolyDataMapper2DInternals();

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* w);

  /**
   * Convert cells to primitives, update topology and mesh data in webgpu buffers used by shader
   * program.
   */
  void UpdateBuffers(vtkViewport* viewport, vtkActor2D* actor, vtkWebGPUPolyDataMapper2D* mapper);

  /**
   * Record draw calls in the render pass encoder. It also sets the bind group, graphics pipeline to
   * use before making the draw calls.
   */
  void RecordDrawCommands(vtkViewport* viewport, const wgpu::RenderPassEncoder& encoder);
  void RecordDrawCommands(vtkViewport* viewport, const wgpu::RenderBundleEncoder& encoder);
};

VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUPolyDataMapper2DInternals.h
