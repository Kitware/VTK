// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUPolyDataMapper_h
#define vtkWebGPUPolyDataMapper_h

#include "vtkPolyDataMapper.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtkType.h"                  // for types
#include "vtk_wgpu.h"                 // for webgpu

#include <unordered_set> // for the not set compute render buffers

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkTypeFloat32Array;
class vtkWebGPURenderWindow;
class vtkWebGPURenderer;
class vtkWebGPUComputeRenderBuffer;

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
    CELL_EDGES,
    CELL_NB_ATTRIBUTES,
    CELL_UNDEFINED
  };

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  void RenderPiece(vtkRenderer* ren, vtkActor* act) override;
  void EncodeRenderCommands(
    vtkRenderer* renderer, vtkActor* act, const wgpu::RenderPassEncoder& passEncoder);
  void EncodeRenderCommands(
    vtkRenderer* renderer, vtkActor* act, const wgpu::RenderBundleEncoder& bundleEncoder);

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
   * Make a shallow copy of this mapper.
   */
  void ShallowCopy(vtkAbstractMapper* m) override;

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

  // This method will Map the specified data array for use as
  // a texture coordinate for texture tname. The actual
  // attribute will be named tname_coord so as to not
  // conflict with the texture sampler definition which will
  // be tname.
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
   * Sets up bindgroup layouts and creates a pipeline layout.
   */
  void SetupPipelineLayout(const wgpu::Device& device, vtkRenderer* renderer, vtkActor* actor);

  /**
   * Creates bind groups for all the buffers in the MeshGeometryBuffers struct.
   * This method should be invoked if the buffer sizes have changed.
   */
  void SetupBindGroups(const wgpu::Device& device, vtkRenderer* renderer);

  /**
   * Enqueues a write command on the device queue for all buffers whose data is outdated.
   * Internally, dawn uses a staging ring buffer, as a result, vtk arrays are copied
   * into that host-side buffer and kept alive until uploaded.
   */
  unsigned long GetExactPointBufferSize();
  unsigned long GetExactCellBufferSize();
  std::vector<unsigned long> GetExactConnecitivityBufferSizes();
  bool UpdateMeshGeometryBuffers(const wgpu::Device& device, vtkActor* actor);
  bool UpdateMeshIndexBuffers(const wgpu::Device& device);
  vtkTypeFloat32Array* ComputeEdgeArray(vtkCellArray* polys);

  /**
   * Creates the graphics pipeline. Rendering state is frozen after this point.
   */
  void SetupGraphicsPipeline(const wgpu::Device& device, vtkRenderer* renderer, vtkActor* actor);

  wgpu::PipelineLayout PipelineLayout;
  wgpu::ShaderModule Shader;

  struct MeshAttributeBuffers
  {
    struct
    {
      // point attributes.
      wgpu::Buffer Buffer;
    } Point;

    struct
    {
      // cell attributes.
      wgpu::Buffer Buffer;
    } Cell;
  };
  MeshAttributeBuffers MeshSSBO;

  struct AttributeDescriptor
  {
    vtkTypeUInt32 Start = 0;
    vtkTypeUInt32 NumTuples = 0;
    vtkTypeUInt32 NumComponents = 0;
    vtkTypeUInt32 Pad = 0; // for 16-byte alignment in MeshAttributeDescriptor
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
    AttributeDescriptor CellEdgeArray;
  };
  wgpu::Buffer AttributeDescriptorBuffer;

  vtkTimeStamp PointCellAttributesBuildTimestamp;
  vtkTimeStamp Primitive2CellIDsBuildTimestamp;

  bool InitializedPipeline = false;
  bool UpdatedPrimitiveSizes = false;
  bool HasPointColors = false;
  bool HasPointNormals = false;
  bool HasPointTangents = false;
  bool HasPointUVs = false;
  bool HasCellColors = false;
  bool HasCellNormals = false;
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
  // 1 bind group layout for the above bindgroup
  wgpu::BindGroupLayout MeshAttributeBindGroupLayout;

  struct PrimitiveBindGroupInfo
  {
    // 1 uniform buffer for the primitive size.
    wgpu::Buffer PrimitiveSizeBuffer;
    // 1 buffer for the primitive cell ids and point ids.
    wgpu::Buffer Buffer;
    // 1 bind group for the primitive size uniform.
    wgpu::BindGroup BindGroup;
    // each primitive-type gets a pipeline.
    wgpu::RenderPipeline Pipeline;
    // vertexCount for draw call.
    vtkTypeUInt32 VertexCount = 0;
  };
  // 1 bind group layout for the above bindgroups.
  wgpu::BindGroupLayout PrimitiveBindGroupLayout;

  PrimitiveBindGroupInfo PointPrimitiveBGInfo;
  PrimitiveBindGroupInfo LinePrimitiveBGInfo;
  PrimitiveBindGroupInfo TrianglePrimitiveBGInfo;
  unsigned long EdgeArrayCount = 0;

  // Cache these so that subsequent arrivals to UpdateMeshGeometry do not unnecessarily invoke
  // MapScalars().
  int LastScalarMode;
  bool LastScalarVisibility;
  vtkDataArray* LastColors = nullptr;

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
    CellDataAttributes::CELL_EDGES, CellDataAttributes::CELL_COLORS,
    CellDataAttributes::CELL_NORMALS
  };

  ///@{
  /**
   * Returns the size of the 'sub-buffer' within the whole point/cell data SSBO for the given
   * attribute
   */
  unsigned long GetPointAttributeByteSize(vtkWebGPUPolyDataMapper::PointDataAttributes attribute);
  unsigned long GetCellAttributeByteSize(vtkWebGPUPolyDataMapper::CellDataAttributes attribute);
  ///@}

  ///@{
  /**
   * Returns the size in bytes of one element of the given attribute.
   * 4 * sizeof(float) for an RGBA color attribute for example
   */
  unsigned long GetPointAttributeElementSize(
    vtkWebGPUPolyDataMapper::PointDataAttributes attribute);
  unsigned long GetCellAttributeElementSize(vtkWebGPUPolyDataMapper::CellDataAttributes attribute);
  ///@}

  ///@{
  /**
   * Returns the offset at which the 'sub-buffer' of 'attribute' starts within the mesh SSBO point
   * data buffer
   */
  vtkIdType GetPointAttributeByteOffset(PointDataAttributes attribute);
  vtkIdType GetCellAttributeByteOffset(CellDataAttributes attribute);
  ///@}

  vtkWebGPUPolyDataMapper(const vtkWebGPUPolyDataMapper&) = delete;
  void operator=(const vtkWebGPUPolyDataMapper&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
