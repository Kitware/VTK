// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUPolyDataMapper_h
#define vtkWebGPUPolyDataMapper_h

#include "vtkPolyDataMapper.h"

#include "vtkProperty.h"                       // for VTK_SURFACE constants
#include "vtkRenderingWebGPUModule.h"          // for export macro
#include "vtkWeakPointer.h"                    // for vtkWeakPointer
#include "vtkWebGPUCellToPrimitiveConverter.h" // for TopologySourceType
#include "vtkWebGPUComputePipeline.h"          // for ivar
#include "vtk_wgpu.h"                          // for webgpu

#include <unordered_set> // for the not set compute render buffers

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkWebGPURenderWindow;
class vtkWebGPURenderer;
class vtkWebGPUComputeRenderBuffer;
class vtkWebGPUConfiguration;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkWebGPUPolyDataMapper* New();
  vtkTypeMacro(vtkWebGPUPolyDataMapper, vtkPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * All the attributes supported by the point data buffer
   */
  enum PointDataAttributes : int
  {
    POINT_POSITIONS = 0,
    POINT_COLORS,
    POINT_NORMALS,
    POINT_TANGENTS,
    POINT_UVS,
    POINT_NB_ATTRIBUTES,
    POINT_UNDEFINED
  };

  /**
   * All the attributes supported by the cell data buffer
   */
  enum CellDataAttributes : int
  {
    CELL_COLORS = 0,
    CELL_NORMALS,
    CELL_NB_ATTRIBUTES,
    CELL_UNDEFINED
  };

  /**
   * This mapper uses different `wgpu::RenderPipeline` to render
   * a list of primitives. Each pipeline uses an appropriate
   * shader module, bindgroup and primitive type.
   */
  enum GraphicsPipelineType : int
  {
    // Pipeline that renders points
    GFX_PIPELINE_POINTS = 0,
    // Pipeline that renders lines with rounded caps and rounded joins.
    GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN,
    // Pipeline that renders lines with miter joins.
    GFX_PIPELINE_LINES_MITER_JOIN,
    // Pipeline that renders triangles
    GFX_PIPELINE_TRIANGLES,
    GFX_PIPELINE_NB_TYPES
  };

  vtkPolyDataMapper::MapperHashType GenerateHash(vtkPolyData* polydata) override;

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  void RenderPiece(vtkRenderer* renderer, vtkActor* act) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  /**
   * @warning: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Used by vtkHardwareSelector to determine if the prop supports hardware
   * selection.
   */
  bool GetSupportsSelection() override { return false; }

  /**
   * Select a data array from the point/cell data
   * and map it to a generic vertex attribute.
   * vertexAttributeName is the name of the vertex attribute.
   * dataArrayName is the name of the data array.
   * fieldAssociation indicates when the data array is a point data array or
   * cell data array (vtkDataObject::FIELD_ASSOCIATION_POINTS or
   * (vtkDataObject::FIELD_ASSOCIATION_CELLS).
   * componentno indicates which component from the data array must be passed as
   * the attribute. If -1, then all components are passed.
   */
  void MapDataArrayToVertexAttribute(const char* vertexAttributeName, const char* dataArrayName,
    int fieldAssociation, int componentno = -1) override;

  /**
   * This method will Map the specified data array for use as
   * a texture coordinate for texture tname. The actual
   * attribute will be named tname_coord so as to not
   * conflict with the texture sampler definition which will
   * be tname.
   */
  void MapDataArrayToMultiTextureAttribute(const char* tname, const char* dataArrayName,
    int fieldAssociation, int componentno = -1) override;

  /**
   * Remove a vertex attribute mapping.
   */
  void RemoveVertexAttributeMapping(const char* vertexAttributeName) override;

  /**
   * Remove all vertex attributes.
   */
  void RemoveAllVertexAttributeMappings() override;

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(
    vtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, vtkProp* prop) override;

  /**
   * Returns an already configured (ready to be added to a vtkWebGPUComputePipeline) buffer bound to
   * the given group and binding.
   *
   * All point data (positions, normals, colors, ...) of this mapper is contained within the single
   * returned buffer. To access the requested (specified by 'attribute') part of the buffer, an
   * offset and a length are automatically bound as uniforms on the 'uniformsGroups' and
   * 'uniformsBinding' given.
   * The offset can then be used in the shader to access the relevant part of the buffer while the
   * length can be used for bounds checking
   *
   * @warning: The returned buffer is already configured and should immediately be inserted into a
   * compute pipeline via vtkWebGPUComputePipeline::AddBuffer() without further modifications
   * through vtkWebGPUComputeBuffer setter methods (other than SetLabel())
   */
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> AcquirePointAttributeComputeRenderBuffer(
    PointDataAttributes attribute, int bufferGroup, int bufferBinding, int uniformsGroup,
    int uniformsBinding);

  /**
   * Same as AcquirePointAttributeComputeRenderBuffer but for cell data attributes
   */
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> AcquireCellAttributeComputeRenderBuffer(
    CellDataAttributes attribute, int bufferGroup, int bufferBinding, int uniformsGroup,
    int uniformsBinding);

protected:
  vtkWebGPUPolyDataMapper();
  ~vtkWebGPUPolyDataMapper() override;

  /**
   * Called in GetBounds(). When this method is called, the consider the input
   * to be updated depending on whether this->Static is set or not. This method
   * simply obtains the bounds from the data-object and returns it.
   */
  void ComputeBounds() override;

  /**
   * This method keeps track of few properties of the actor which when changed,
   * require rebuilding a render bundle. For example, if representation changed
   * from wireframe to surface, the last set of draw commands were recorded using
   * the SurfaceMesh pipeline. In order to draw wireframes, the render bundle
   * will need to be rebuilt using the wireframe pipeline instead.
   *
   * This method returns true if the cached properties have changed or the properties of the actor
   * are cached for the first time, false otherwise.
   */
  bool CacheActorRendererProperties(vtkActor* actor, vtkRenderer* renderer);

  /**
   * Record draw calls in the render pass encoder. It also sets the bind group, graphics pipeline to
   * use before making the draw calls.
   */
  void RecordDrawCommands(
    vtkRenderer* renderer, vtkActor* actor, const wgpu::RenderPassEncoder& passEncoder);
  void RecordDrawCommands(
    vtkRenderer* renderer, vtkActor* actor, const wgpu::RenderBundleEncoder& bundleEncoder);

  /**
   * Looks at the point/cell data of `vtkPolyData` object and determines
   * which attributes are available. Scalars should have been mapped if required.
   */
  void DeducePointCellAttributeAvailability(vtkPolyData* mesh);

  /**
   * Reset the internal `Has{Point,Cell}Attribute` booleans to `false`.
   */
  void ResetPointCellAttributeState();

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
   * Create a render pipeline.
   */
  static wgpu::RenderPipeline CreateRenderPipeline(const wgpu::Device& device,
    wgpu::RenderPipelineDescriptor* pipelineDescriptor, const wgpu::ShaderModule& shaderModule,
    wgpu::PrimitiveTopology primitiveTopology);

  /**
   * Create a bind group for the point and cell attributes of a mesh. It has three bindings.
   *
   * 0: [storage] AttributeDescriptorBuffer
   *      tells where different attributes start and end for each
   *      sub-buffer in the point/cell buffers.
   *
   * 1: [storage] MeshSSBO.Point.Buffer
   *      all point attributes
   *      @sa vtkWebGPUPolyDataMapper::PointDataAttributes
   *
   * 2: [storage] MeshSSBO.Cell.Buffer
   *      all cell attributes
   *      @sa vtkWebGPUPolyDataMapper::CellDataAttributes
   */
  wgpu::BindGroup CreateMeshAttributeBindGroup(
    const wgpu::Device& device, const std::string& label);

  /**
   * Create a bind group for the primitives of a mesh. It has 2 bindings.
   *
   * 0: [storage] TopologyRenderInfo.TopologyBuffer
   *      sequence of cell_id,point_id for all vertices
   *      @sa vtkWebGPUPolyDataMapper::TopologyRenderInfo
   *
   * 1: [storage] TopologyRenderInfo.EdgeArrayBuffer
   *      sequence of edge_value for all triangles
   *      @sa vtkWebGPUPolyDataMapper::TopologyRenderInfo
   */
  wgpu::BindGroup CreateTopologyBindGroup(const wgpu::Device& device, const std::string& label,
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType);

  /**
   * Returns the size of the 'sub-buffer' within the whole point data SSBO for the given attribute
   */
  unsigned long GetPointAttributeByteSize(vtkWebGPUPolyDataMapper::PointDataAttributes attribute);

  /**
   * Returns the size of the 'sub-buffer' within the whole cell data SSBO for the given attribute
   */
  unsigned long GetCellAttributeByteSize(vtkWebGPUPolyDataMapper::CellDataAttributes attribute);

  /**
   * Returns the size in bytes of one element of the given attribute.
   * 4 * sizeof(float) for an RGBA color attribute for example
   */
  unsigned long GetPointAttributeElementSize(
    vtkWebGPUPolyDataMapper::PointDataAttributes attribute);

  /**
   * Returns the size in bytes of one element of the given attribute.
   * 4 * sizeof(float) for an RGBA color attribute for example
   */
  unsigned long GetCellAttributeElementSize(vtkWebGPUPolyDataMapper::CellDataAttributes attribute);

  /**
   * Returns the offset at which the 'sub-buffer' of 'attribute' starts within the mesh SSBO point
   * data buffer
   */
  vtkIdType GetPointAttributeByteOffset(PointDataAttributes attribute);

  /**
   * Returns the offset at which the 'sub-buffer' of 'attribute' starts within the mesh SSBO cell
   * data buffer
   */
  vtkIdType GetCellAttributeByteOffset(CellDataAttributes attribute);

  /**
   * Calculates the size of a buffer that is large enough to contain
   * all the values from the point attributes. See vtkWebGPUPolyDataMapper::PointDataAttributes
   * for the kinds of attributes.
   */
  unsigned long GetExactPointBufferSize();

  /**
   * Calculates the size of a buffer that is large enough to contain
   * all the values from the cell attributes. See vtkWebGPUPolyDataMapper::PointDataAttributes
   * for the kinds of attributes.
   */
  unsigned long GetExactCellBufferSize();

  ///@{
  /**
   * Creates buffers as needed and updates them with point/cell attributes,
   * topology, draw parameters.
   *
   * This function enqueues a `BufferWrite` command on the device queue for all buffers
   * whose data is outdated.
   * Note that internally, dawn uses a staging ring buffer, as a result, vtk arrays are copied
   * into that host-side buffer and kept alive until uploaded.
   */
  void UpdateMeshGeometryBuffers(vtkWebGPURenderWindow* wgpuRenderWindow);
  ///@}

  /**
   * Get the name of the graphics pipeline type as a string.
   */
  const char* GetGraphicsPipelineTypeAsString(GraphicsPipelineType graphicsPipelineType);

  /**
   * Creates the graphics pipeline. Rendering state is frozen after this point.
   * The build timestamp is recorded in `GraphicsPipelineBuildTimestamp`.
   */
  void SetupGraphicsPipelines(const wgpu::Device& device, vtkRenderer* renderer, vtkActor* actor);

  /**
   * Get whether the graphics pipeline needs rebuilt.
   * This method checks MTime of the vtkActor's vtkProperty instance against the build timestamp of
   * the graphics pipeline.
   */
  bool GetNeedToRebuildGraphicsPipelines(vtkActor* actor, vtkRenderer* renderer);

  struct MeshAttributeBuffers
  {
    struct
    {
      // point attributes.
      wgpu::Buffer Buffer;
      uint64_t Size = 0;
    } Point;

    struct
    {
      // cell attributes.
      wgpu::Buffer Buffer;
      uint64_t Size = 0;
    } Cell;
  };
  MeshAttributeBuffers MeshSSBO;

  struct AttributeDescriptor
  {
    vtkTypeUInt32 Start = 0;
    vtkTypeUInt32 NumTuples = 0;
    vtkTypeUInt32 NumComponents = 0;
  };
  struct MeshAttributeDescriptor
  {
    AttributeDescriptor Positions;
    AttributeDescriptor Colors;
    AttributeDescriptor Normals;
    AttributeDescriptor Tangents;
    AttributeDescriptor UVs;
    AttributeDescriptor CellColors;
    AttributeDescriptor CellNormals;
    vtkTypeUInt32 ApplyOverrideColors = 0;
    vtkTypeFloat32 Opacity = 0;
    vtkTypeUInt32 CompositeId = 0;
    vtkTypeFloat32 Ambient[3] = {};
    vtkTypeUInt32 ProcessId = 0;
    vtkTypeFloat32 Diffuse[3] = {};
    vtkTypeUInt32 Pickable = false;
  };
  wgpu::Buffer AttributeDescriptorBuffer;

  ///@{ Timestamps help reuse previous resources as much as possible.
  vtkTimeStamp CellAttributesBuildTimestamp[CELL_NB_ATTRIBUTES];
  vtkTimeStamp PointAttributesBuildTimestamp[POINT_NB_ATTRIBUTES];
  vtkTimeStamp
    IndirectDrawBufferUploadTimeStamp[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
  ///@}

  bool HasPointAttributes[POINT_NB_ATTRIBUTES];
  bool HasCellAttributes[CELL_NB_ATTRIBUTES];
  bool RebuildGraphicsPipelines = true;
  // used by RenderPiece and functions it calls to reduce
  // calls to get the input and allow for rendering of
  // other polydata (not the input)
  vtkPolyData* CurrentInput = nullptr;
  // vtkRenderer culls props to frustum. At that point, it requests
  // mappers for bounds of the geometry. We cache the vtkAlgorithm output
  // so that `UpdateMeshGeometryBuffers` can reuse it without climbing up
  // vtkAlgorithm pipeline.
  vtkPolyData* CachedInput = nullptr;

  // 1 bind group for this polydata mesh
  wgpu::BindGroup MeshAttributeBindGroup;

  struct TopologyBindGroupInfo
  {
    // buffer for the primitive cell ids and point ids.
    wgpu::Buffer TopologyBuffer;
    // buffer for edge array. this lets fragment shader hide internal edges of a polygon
    // when edge visibility is turned on.
    wgpu::Buffer EdgeArrayBuffer;
    // // buffer for indirect draw command
    // wgpu::Buffer IndirectDrawBuffer;
    // bind group for the primitive size uniform.
    wgpu::BindGroup BindGroup;
    // vertexCount for draw call.
    vtkTypeUInt32 VertexCount = 0;
  };

  vtkNew<vtkWebGPUCellToPrimitiveConverter> CellConverter;
  TopologyBindGroupInfo
    TopologyBindGroupInfos[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES] = {};
  std::string GraphicsPipelineKeys[GFX_PIPELINE_NB_TYPES] = {};

  // Cache these so that subsequent executions of UpdateMeshGeometryBuffers() do not unnecessarily
  // invoke MapScalars().
  int LastScalarMode = -1;
  bool LastScalarVisibility = false;
  struct ActorState
  {
    bool LastActorBackfaceCulling = false;
    bool LastActorFrontfaceCulling = false;
    bool LastVertexVisibility = false;
    int LastRepresentation = VTK_SURFACE;
    bool LastHasRenderingTranslucentGeometry = false;
  };

private:
  friend class vtkWebGPUComputeRenderBuffer;
  friend class vtkWebGPURenderer;

  /**
   * Returns the wgpu::Buffer containing the point data attributes of this mapper
   */
  wgpu::Buffer GetPointDataWGPUBuffer() { return this->MeshSSBO.Point.Buffer; }

  /**
   * Returns the wgpu::Buffer containing the cell data attributes of this mapper
   */
  wgpu::Buffer GetCellDataWGPUBuffer() { return this->MeshSSBO.Cell.Buffer; }

  /**
   * List of the RenderBuffers created by calls to AcquirePointAttributeComputeRenderBuffer(). This
   * list is used in the vtkWebGPURenderer where these render buffers are actually going to be set
   * up / created on the device.
   * This list only contains buffers that have been set up.
   */
  std::vector<vtkSmartPointer<vtkWebGPUComputeRenderBuffer>> SetupComputeRenderBuffers;

  /**
   * Compute render buffers that yet have to be set up on their compute pipelines
   */
  std::unordered_set<vtkSmartPointer<vtkWebGPUComputeRenderBuffer>> NotSetupComputeRenderBuffers;

  /**
   * Order in which the point data attributes are concatenated into the mapper mesh SSBO
   */
  const PointDataAttributes PointDataAttributesOrder[PointDataAttributes::POINT_NB_ATTRIBUTES] = {
    PointDataAttributes::POINT_POSITIONS, PointDataAttributes::POINT_COLORS,
    PointDataAttributes::POINT_NORMALS, PointDataAttributes::POINT_TANGENTS,
    PointDataAttributes::POINT_UVS
  };

  /**
   * Order in which the cell data attributes are concatenated into the mapper mesh SSBO
   */
  const CellDataAttributes CellDataAttributesOrder[CellDataAttributes::CELL_NB_ATTRIBUTES] = {
    CellDataAttributes::CELL_COLORS, CellDataAttributes::CELL_NORMALS
  };

  std::map<std::pair<vtkActor*, vtkRenderer*>, ActorState> CachedActorRendererProperties;

  vtkWebGPUPolyDataMapper(const vtkWebGPUPolyDataMapper&) = delete;
  void operator=(const vtkWebGPUPolyDataMapper&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
