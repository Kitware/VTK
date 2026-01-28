// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUPolyDataMapper_h
#define vtkWebGPUPolyDataMapper_h

#include "vtkPolyDataMapper.h"

#include "vtkProperty.h"                       // for VTK_SURFACE constants
#include "vtkRenderingWebGPUModule.h"          // for export macro
#include "vtkWebGPUCellToPrimitiveConverter.h" // for TopologySourceType
#include "vtkWebGPUComputePipeline.h"          // for ivar
#include "vtkWrappingHints.h"                  // For VTK_MARSHALAUTO
#include "vtk_wgpu.h"                          // for webgpu

#include <array>         // for ivar
#include <unordered_set> // for the not set compute render buffers

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkOverrideAttribute;
class vtkWebGPUActor;
class vtkWebGPURenderWindow;
class vtkWebGPURenderer;
class vtkWebGPUComputeRenderBuffer;
class vtkWebGPUTexture;
class vtkWebGPUConfiguration;

class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkWebGPUPolyDataMapper* New();
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();
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
    POINT_COLOR_UVS,
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
    // Pipeline that renders points, is best suitable for rendering 1-pixel wide points.
    GFX_PIPELINE_POINTS = 0,
    GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE,
    // Pipeline that renders points using a square or circle shape.
    GFX_PIPELINE_POINTS_SHAPED,
    GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE,
    // Pipeline that is best suitable for rendering 1-pixel thick line segments
    GFX_PIPELINE_LINES,
    GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE,
    // Pipeline that can render lines thicker than 1-pixel. This pipeline does not
    // create joining geometry between contiguous line segments in a polyline.
    GFX_PIPELINE_LINES_THICK,
    GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE,
    // Pipeline that renders lines with rounded caps and rounded joins.
    GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN,
    GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE,
    // Pipeline that renders lines with miter joins.
    GFX_PIPELINE_LINES_MITER_JOIN,
    GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE,
    // Pipeline that renders triangles
    GFX_PIPELINE_TRIANGLES,
    GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE,
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

  virtual std::vector<wgpu::BindGroupLayoutEntry> GetMeshBindGroupLayoutEntries();

  /**
   * Create a bind group layout for the mesh attribute bind group.
   */
  wgpu::BindGroupLayout CreateMeshAttributeBindGroupLayout(
    const wgpu::Device& device, const std::string& label);

  /**
   * Create a bind group layout for the `TopologyRenderInfo::BindGroup`
   */
  wgpu::BindGroupLayout CreateTopologyBindGroupLayout(const wgpu::Device& device,
    const std::string& label, bool homogeneousCellSize, bool useEdgeArray);

  virtual std::vector<wgpu::BindGroupEntry> GetMeshBindGroupEntries();

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
   * Calculates the size of a buffer that is large enough to contain
   * all the values from the point attributes. See vtkWebGPUPolyDataMapper::PointDataAttributes
   * for the kinds of attributes.
   */
  unsigned long GetExactPointBufferSize(PointDataAttributes attribute);

  /**
   * Calculates the size of a buffer that is large enough to contain
   * all the values from the cell attributes. See vtkWebGPUPolyDataMapper::PointDataAttributes
   * for the kinds of attributes.
   */
  unsigned long GetExactCellBufferSize(CellDataAttributes attribute);

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
   * Updates the clipping planes buffer with the current clipping planes data.
   */
  void UpdateClippingPlanesBuffer(vtkWebGPUConfiguration* wgpuConfiguration, vtkActor* actor);

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
   * Generates vertex and fragment shader code
   */
  virtual void ApplyShaderReplacements(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss, std::string& fss);

  virtual void ReplaceShaderConstantsDef(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss, std::string& fss);
  virtual void ReplaceShaderActorDef(GraphicsPipelineType pipelineType, vtkWebGPURenderer* renderer,
    vtkWebGPUActor* actor, std::string& vss, std::string& fss);
  virtual void ReplaceShaderClippingPlanesDef(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss, std::string& fss);
  virtual void ReplaceShaderCustomDef(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss, std::string& fss);

  virtual void ReplaceShaderRendererBindings(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss, std::string& fss);
  virtual void ReplaceShaderActorBindings(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss, std::string& fss);
  virtual void ReplaceShaderClippingPlanesBindings(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss, std::string& fss);
  virtual void ReplaceShaderMeshAttributeBindings(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss, std::string& fss);
  virtual void ReplaceShaderCustomBindings(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss, std::string& fss);
  virtual void ReplaceShaderTopologyBindings(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss, std::string& fss);
  virtual void ReplaceShaderVertexOutputDef(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss, std::string& fss);

  virtual void ReplaceVertexShaderInputDef(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderMainStart(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderCamera(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderNormalTransform(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderVertexId(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderPrimitiveId(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderCellId(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderPosition(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderClippingPlanes(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderPositionVC(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderPicking(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderColors(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderUVs(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderEdges(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderNormals(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderTangents(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);
  virtual void ReplaceVertexShaderMainEnd(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& vss);

  virtual void ReplaceFragmentShaderOutputDef(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& fss);

  virtual void ReplaceFragmentShaderMainStart(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& fss);
  virtual void ReplaceFragmentShaderClippingPlanes(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& fss);
  virtual void ReplaceFragmentShaderColors(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& fss);
  virtual void ReplaceFragmentShaderNormals(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& fss);
  virtual void ReplaceFragmentShaderEdges(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& fss);
  virtual void ReplaceFragmentShaderLights(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& fss);
  virtual void ReplaceFragmentShaderPicking(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& fss);
  virtual void ReplaceFragmentShaderMainEnd(GraphicsPipelineType pipelineType,
    vtkWebGPURenderer* renderer, vtkWebGPUActor* actor, std::string& fss);

  /**
   * Whether shaders must be built to target the specific pipeline.
   * If true, shaders will be built and draw commands will be recorded for the pipeline.
   * If false, shaders will not be built and no draw commands will be recorded for the pipeline.
   *
   * Subclasses may override to return false for pipelines that they do not wish to
   * support.
   */
  virtual bool IsPipelineSupported(GraphicsPipelineType vtkNotUsed(pipelineType)) { return true; }
  static bool IsPipelineForHomogeneousCellSize(GraphicsPipelineType pipelineType);

  /**
   * Get the primitive topology type that should be used for the given pipeline.
   */
  virtual wgpu::PrimitiveTopology GetPrimitiveTopologyForPipeline(
    GraphicsPipelineType pipelineType);

  struct DrawCallArgs
  {
    std::uint32_t VertexCount = 0;
    std::uint32_t InstanceCount = 0;
  };
  virtual DrawCallArgs GetDrawCallArgs(GraphicsPipelineType pipelineType,
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType);
  virtual DrawCallArgs GetDrawCallArgsForDrawingVertices(
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType);

  /**
   * Get whether the graphics pipeline needs rebuilt.
   * This method checks MTime of the vtkActor's vtkProperty instance against the build timestamp of
   * the graphics pipeline.
   */
  bool GetNeedToRebuildGraphicsPipelines(vtkActor* actor, vtkRenderer* renderer);
  struct AttributeBuffer
  {
    // point attributes.
    wgpu::Buffer Buffer;
    uint64_t Size = 0;
  };
  AttributeBuffer PointBuffers[POINT_NB_ATTRIBUTES];
  AttributeBuffer CellBuffers[CELL_NB_ATTRIBUTES];
  struct
  {
    vtkTypeFloat32 PlaneEquations[6][4];
    vtkTypeUInt32 PlaneCount = 0;
  } ClippingPlanesData;
  wgpu::Buffer ClippingPlanesBuffer;

  ///@{ Timestamps help reuse previous resources as much as possible.
  vtkTimeStamp CellAttributesBuildTimestamp[CELL_NB_ATTRIBUTES];
  vtkTimeStamp PointAttributesBuildTimestamp[POINT_NB_ATTRIBUTES];
  vtkTimeStamp ClippingPlanesBuildTimestamp;
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
  vtkSmartPointer<vtkWebGPUTexture> ColorTextureHostResource;

  // 1 bind group for this polydata mesh
  wgpu::BindGroup MeshAttributeBindGroup;

  struct TopologyBindGroupInfo
  {
    // buffer for point ids.
    wgpu::Buffer ConnectivityBuffer;
    // buffer for the cell ids.
    wgpu::Buffer CellIdBuffer;
    // buffer for edge array. this lets fragment shader hide internal edges of a polygon
    // when edge visibility is turned on.
    wgpu::Buffer EdgeArrayBuffer;
    // uniform buffer for cell id offset.
    wgpu::Buffer CellIdOffsetUniformBuffer;
    // // buffer for indirect draw command
    // wgpu::Buffer IndirectDrawBuffer;
    // bind group for the primitive size uniform.
    wgpu::BindGroup BindGroup;
    // maximum number of vertices in a cell
    vtkTypeUInt32 MaxCellSize = 0;
    // vertexCount for draw call.
    vtkTypeUInt32 VertexCount = 0;
  };

  enum BindingGroup : int
  {
    GROUP_RENDERER = 0,
    GROUP_ACTOR = 1,
    GROUP_TEXTURES = GROUP_ACTOR,
    GROUP_MESH = 2,
    // Clipping planes are bound to the same group as the mesh attributes
    // because they vary based on the mapper's shift/scale and the actor's
    // transformation matrix.
    GROUP_CLIPPING_PLANES = GROUP_MESH,
    GROUP_TOPOLOGY = 3,
    GROUP_NB_BINDGROUPS = 4
  };
  static_assert(GROUP_NB_BINDGROUPS <= 4,
    "Number of bind groups exceeds 4! Most devices can support only up to 4 bind groups");
  std::array<std::uint32_t, GROUP_NB_BINDGROUPS> NumberOfBindings = {};
  vtkNew<vtkWebGPUCellToPrimitiveConverter> CellConverter;
  TopologyBindGroupInfo
    TopologyBindGroupInfos[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES] = {};
  std::string GraphicsPipelineKeys[GFX_PIPELINE_NB_TYPES] = {};

  // Cache these so that subsequent executions of UpdateMeshGeometryBuffers() do not unnecessarily
  // invoke MapScalars().
  int LastScalarMode = -1;
  bool LastScalarVisibility = false;
  vtkTypeUInt32 LastNumClipPlanes = 0;
  struct ActorState
  {
    bool LastActorBackfaceCulling = false;
    bool LastActorFrontfaceCulling = false;
    bool LastVertexVisibility = false;
    int LastRepresentation = VTK_SURFACE;
    bool LastHasRenderingTranslucentGeometry = false;
    int LastPointSize = 1;
    int LastLineWidth = 1;
  };

private:
  friend class vtkWebGPUComputeRenderBuffer;
  friend class vtkWebGPURenderer;

  /**
   * Returns the wgpu::Buffer containing the point data attributes of this mapper
   */
  wgpu::Buffer GetPointDataWGPUBuffer(PointDataAttributes attribute)
  {
    return this->PointBuffers[attribute].Buffer;
  }

  /**
   * Returns the wgpu::Buffer containing the cell data attributes of this mapper
   */
  wgpu::Buffer GetCellDataWGPUBuffer(CellDataAttributes attribute)
  {
    return this->CellBuffers[attribute].Buffer;
  }

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
    PointDataAttributes::POINT_UVS, PointDataAttributes::POINT_COLOR_UVS
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
#define vtkWebGPUPolyDataMapper_OVERRIDE_ATTRIBUTES                                                \
  vtkWebGPUPolyDataMapper::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
