// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUCellToPrimitiveConverter
 * @brief   Converts VTK cell connectivity arrays into webgpu primitives using compute shaders.
 *
 * When given only vertices, lines and triangles and using 32-bit integer IDs, this class opts into
 * low memory code paths, i.e, does not copy indices into new arrays.
 *
 * When input has poly-vertices, poly-lines and polygons or triangle strips or uses 64-bit integer
 * IDs, this class converts the underlying cell array storage to 32-bit. Itmakes an additional copy
 * of the indices.
 *
 * This class can process millions of polygons, lines, vertices very quickly in parallel.
 *
 * @sa
 * vtkPolyDataMapper2D
 */

#ifndef vtkWebGPUCellToPrimitiveConverter_h
#define vtkWebGPUCellToPrimitiveConverter_h

#include "vtkObject.h"

#include "vtkProperty.h"              // for VTK_SURFACE
#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtkSmartPointer.h"          // for vtkSmartPointer

#include "vtk_wgpu.h" // for wgpu::Buffer

#include <utility> // for std::pair

VTK_ABI_NAMESPACE_BEGIN
class vtkWebGPUConfiguration;
class vtkPolyData;
class vtkCellArray;
class vtkWebGPUComputePass;
class vtkWebGPUComputePipeline;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUCellToPrimitiveConverter : public vtkObject
{
public:
  static vtkWebGPUCellToPrimitiveConverter* New();
  vtkTypeMacro(vtkWebGPUCellToPrimitiveConverter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*);

  /**
   * All supported types of topology. These describe the kinds of cells
   * found in a `vtkPolyData`.
   */
  enum TopologySourceType : int
  {
    // Used to draw VTK_VERTEX and VTK_POLY_VERTEX cell types
    TOPOLOGY_SOURCE_VERTS = 0,
    // Used to draw VTK_LINE and VTK_POLY_LINE cell types.
    TOPOLOGY_SOURCE_LINES,
    // Used to draw only the points of each line segment.
    // Activated when `vtkProperty::Representation` == `VTK_POINTS`
    TOPOLOGY_SOURCE_LINE_POINTS,
    // Used to draw VTK_QUAD, VTK_TRIANGLE and VTK_POLYGON cell types.
    TOPOLOGY_SOURCE_POLYGONS,
    // Used to draw only the corner points of each face.
    // Activated when `vtkProperty::Representation` == `VTK_POINTS`
    TOPOLOGY_SOURCE_POLYGON_POINTS,
    // Used to draw only the edges of each face.
    // Activated when `vtkProperty::Representation` == `VTK_WIREFRAME`
    TOPOLOGY_SOURCE_POLYGON_EDGES,
    NUM_TOPOLOGY_SOURCE_TYPES
  };

  /**
   * Tessellates the cells in a mesh into graphics primitives.
   * This function calls DispatchCellToPrimitiveComputePipeline() for
   * - vtkPolyData::GetVerts()
   * - vtkPolyData::GetLines()
   * - vtkPolyData::GetPolys()
   * This method will initialize the vertexCounts, topologyBuffers and edgeArrayBuffers
   * after dispatching the compute pipelines.
   * Returns false if no buffers have changed, else returns true.
   */
  bool DispatchMeshToPrimitiveComputePipeline(vtkWebGPUConfiguration* wgpuConfiguration,
    vtkPolyData* mesh, int representation, vtkTypeUInt32* vertexCounts[NUM_TOPOLOGY_SOURCE_TYPES],
    wgpu::Buffer* topologyBuffers[NUM_TOPOLOGY_SOURCE_TYPES],
    wgpu::Buffer* edgeArrayBuffers[NUM_TOPOLOGY_SOURCE_TYPES]);

  /**
   * Tessellates each cell into primitives.
   * This function splits polygons, quads and triangle-strips into separate triangles.
   * It splits polylines into line segments and polyvertices into individual vertices.
   * This method will initialize the vertexCounts, topologyBuffers and edgeArrayBuffers
   * after dispatching the compute pipelines.
   * Returns false if no buffers have changed, else returns true.
   */
  bool DispatchCellToPrimitiveComputePipeline(vtkWebGPUConfiguration* wgpuConfiguration,
    vtkCellArray* cells, int representation, int cellType, vtkIdType cellIdOffset,
    vtkTypeUInt32* vertexCounts[NUM_TOPOLOGY_SOURCE_TYPES],
    wgpu::Buffer* topologyBuffers[NUM_TOPOLOGY_SOURCE_TYPES],
    wgpu::Buffer* edgeArrayBuffers[NUM_TOPOLOGY_SOURCE_TYPES]);

  /**
   * Get whether the Cell-to-Primitive compute pipeline needs rebuilt.
   * This method checks MTime of the vtkCellArray against the build timestamp of the relevant
   * compute pipeline.
   */
  bool GetNeedToRebuildCellToPrimitiveComputePipeline(
    vtkCellArray* cells, vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType);

  /**
   * Brings the build timestamp of the compute pipeline associated with `cellType` up to date.
   */
  void UpdateCellToPrimitiveComputePipelineTimestamp(
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType);

  /**
   * Query the integer that, when subtracted from the no. of vertices of a polygon gives
   * the number of sub primitives.
   *
   * Example: there are n - 2 triangles in a n-sided polygon, so this function returns '2' for
   * `VTK_POLYGON`
   */
  static vtkIdType GetTessellatedPrimitiveSizeOffsetForCellType(int cellType);

  /**
   * Get the name of the topology source type as a string.
   */
  static const char* GetTopologySourceTypeAsString(TopologySourceType topologySourceType);

  /**
   * Get the name of the VTK cell type as a string.
   */
  static const char* GetCellTypeAsString(int cellType);

  /**
   * Get the name of the sub primitive of a VTK cell type as a string. Ex: "vertex", "line",
   * "triangle"
   */
  static const char* GetTessellatedPrimitiveTypeAsString(TopologySourceType topologySourceType);

  /**
   * Get the number of points in the sub primitive of a VTK cell type.
   */
  static std::size_t GetTessellatedPrimitiveSize(TopologySourceType topologySourceType);

  /**
   * A convenient method to get the relevant `TopologyRenderInfo` instance for a `cellType`.
   */
  static TopologySourceType GetTopologySourceTypeForCellType(
    int cellType, int representation = VTK_SURFACE);

protected:
  vtkWebGPUCellToPrimitiveConverter();
  ~vtkWebGPUCellToPrimitiveConverter() override;

  // Timestamps help reuse previous resources as much as possible.
  vtkTimeStamp TopologyBuildTimestamp[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  // compute pass speeds up cell to primitive conversions.
  vtkSmartPointer<vtkWebGPUComputePass>
    ComputePasses[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  // compute pipeline to execute the compute pass.
  vtkSmartPointer<vtkWebGPUComputePipeline>
    ComputePipelines[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];

private:
  vtkWebGPUCellToPrimitiveConverter(const vtkWebGPUCellToPrimitiveConverter&) = delete;
  void operator=(const vtkWebGPUCellToPrimitiveConverter&) = delete;

  /**
   * This method creates a compute pass and a compute pipeline for breaking down composite
   * VTK cells into graphics primitives. It selects the correct shader entry point
   * based on the `cellType`
   */
  std::pair<vtkSmartPointer<vtkWebGPUComputePass>, vtkSmartPointer<vtkWebGPUComputePipeline>>
  CreateCellToPrimitiveComputePassForCellType(
    vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration,
    TopologySourceType topologySourceType);
};

VTK_ABI_NAMESPACE_END

#endif
