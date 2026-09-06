// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkWebGPUBatchedPolyDataMapper
 * @brief A WebGPU mapper for batched rendering of vtkPolyData.
 *
 * @sa vtkWebGPUPolyDataMapper vtkWebGPUCompositePolyDataMapperDelegator
 */

#ifndef vtkWebGPUBatchedPolyDataMapper_h
#define vtkWebGPUBatchedPolyDataMapper_h

#include "vtkWebGPUPolyDataMapper.h"

#include "vtkRenderingWebGPUModule.h"                  // for export macro
#include "vtkWebGPUCompositePolyDataMapperDelegator.h" // for struct BatchElement

#include <cstdint> // for std::uintptr_t
#include <memory>  // for unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositePolyDataMapper;
class vtkPolyData;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUBatchedPolyDataMapper : public vtkWebGPUPolyDataMapper
{
public:
  static vtkWebGPUBatchedPolyDataMapper* New();
  vtkTypeMacro(vtkWebGPUBatchedPolyDataMapper, vtkWebGPUPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * API to add and query a BatchElement instance per vtkPolyData.
   */
  using BatchElement = vtkWebGPUCompositePolyDataMapperDelegator::BatchElement;
  void AddBatchElement(unsigned int flatIndex, BatchElement&& batchElement);
  BatchElement* GetBatchElement(vtkPolyData* polydata);
  void ClearBatchElements();
  ///@}

  /**
   * Accessor to the ordered list of PolyData that we last drew.
   */
  std::vector<vtkPolyData*> GetRenderedList() const;
  void SetParent(vtkCompositePolyDataMapper* parent);

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  void RenderPiece(vtkRenderer* renderer, vtkActor* actor) override;
  void UnmarkBatchElements();
  void ClearUnmarkedBatchElements();

  /**
   * Returns the maximum of our and Parent vtkCompositePolyDataMapper's MTime
   */
  vtkMTimeType GetMTime() override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkWebGPUBatchedPolyDataMapper();
  ~vtkWebGPUBatchedPolyDataMapper() override;

  /**
   * Override to detect changes in the polydata batch.
   */
  bool ShouldReleaseGraphicsResourcesOnSync() override;

  /**
   * Override to ensure batch is not empty.
   */
  bool EnsureInput() override;

  /**
   * Override to call super class for each polydata in batch.
   */
  void DeducePointCellAttributeAvailability() override;

  ///@{
  /**
   * Override to upload all the polydata from the batch.
   */
  void UpdateMeshGeometryBuffers(vtkWebGPUConfiguration* wgpuConfiguration) override;
  void UpdateMeshTopologyBuffers(
    vtkWebGPUConfiguration* wgpuConfiguration, vtkProperty* displayProperty) override;
  ///@}

  ///@{
  /**
   * Override to accumulate the sizes of the given `attribute` from all
   * the polydata in the batch.
   */
  unsigned long GetPointAttributeByteSize(
    vtkWebGPUPolyDataMapper::PointDataAttributes attribute) override;
  unsigned long GetCellAttributeByteSize(
    vtkWebGPUPolyDataMapper::CellDataAttributes attribute) override;
  ///@}

  ///@{
  /**
   * Override to add another binding for CompositeDataPropertyStorage
   */
  std::vector<WGPUBindGroupLayoutEntry> GetMeshBindGroupLayoutEntries() override;
  std::vector<WGPUBindGroupEntry> GetMeshBindGroupEntries() override;
  ///@}

  ///@{
  /**
   * Override to customize bindings when homogeneousCellSize=true
   */
  std::vector<WGPUBindGroupLayoutEntry> GetTopologyBindGroupLayoutEntries(
    bool homogeneousCellSize, bool useEdgeArray) override;
  std::vector<WGPUBindGroupEntry> GetTopologyBindGroupEntries(
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType,
    bool homogeneousCellSize, bool useEdgeArray) override;
  ///@}

  void ReplaceShaderCustomDef(GraphicsPipelineType pipelineType, vtkWebGPURenderer* wgpuRenderer,
    vtkWebGPUActor* wgpuActor, std::string& vss, std::string& fss) override;
  void ReplaceShaderCustomBindings(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* wgpuRenderer, vtkWebGPUActor* wgpuActor, std::string& vss,
    std::string& fss) override;

  void ReplaceVertexShaderCellId(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss) override;
  void ReplaceVertexShaderPicking(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* wgpuRenderer, vtkWebGPUActor* wgpuActor, std::string& vss) override;

  void ReplaceFragmentShaderColors(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* wgpuRenderer, vtkWebGPUActor* wgpuActor, std::string& fss) override;
  void ReplaceFragmentShaderPicking(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* wgpuRenderer, vtkWebGPUActor* wgpuActor, std::string& fss) override;

  ///@{
  /**
   * Override to return the correct vertex offset and vertex count for the mesh currently being
   * drawn within our batch.
   */
  DrawCallArgs GetDrawCallArgs(GraphicsPipelineType pipelineType,
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType) override;
  DrawCallArgs GetDrawCallArgsForDrawingVertices(
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType) override;
  ///@}

  int CanUseTextureMapForColoring(vtkDataObject*) override;

private:
  vtkWebGPUBatchedPolyDataMapper(const vtkWebGPUBatchedPolyDataMapper&) = delete;
  void operator=(const vtkWebGPUBatchedPolyDataMapper&) = delete;

  // Reference to CPDM
  vtkCompositePolyDataMapper* Parent = nullptr;
  // Maps an address of a vtkPolyData to its rendering attributes.
  std::map<std::uintptr_t, std::unique_ptr<BatchElement>> VTKPolyDataToBatchElement;
  std::map<unsigned int, std::uintptr_t> FlatIndexToPolyData;

  struct CompositeDataProperties
  {
    vtkTypeUInt32 ApplyOverrideColors = 0;
    vtkTypeFloat32 Opacity = 0;
    vtkTypeUInt32 CompositeId = 0;
    vtkTypeUInt32 Pickable = 1;
    vtkTypeFloat32 Ambient[3] = {};
    vtkTypeUInt32 CellIdOffsetForVerts = 0;
    vtkTypeFloat32 Diffuse[3] = {};
    vtkTypeUInt32 CellIdOffsetForLines = 0;
    vtkTypeUInt32 CellIdOffsetForPolys = 0;
    vtkTypeUInt32 CellIdOffsetForSelector = 0;
  };
  struct StorageBuffer
  {
    WGPUBuffer Buffer = nullptr;
    std::uint64_t Size = 0;
    std::uint32_t BindingSize = 0;
  };
  StorageBuffer CompositeDataPropertyStorage;
  vtkTimeStamp ResourcesSyncTimeStamp;

  std::size_t CurrentDrawMeshId = 0; // 0 <= CurrentDrawMeshId < BatchSize
  std::uint32_t MinStorageBufferOffsetAlignment = 0;

  bool AllocateCompositeDataPropertyStorageBuffer(vtkWebGPUConfiguration* wgpuConfiguration);
};

VTK_ABI_NAMESPACE_END
#endif
