// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUPolyDataMapper_h
#define vtkWebGPUPolyDataMapper_h

#include "vtkPolyDataMapper.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtkType.h"                  // for types
#include "vtk_wgpu.h"                 // for webgpu

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkTypeFloat32Array;
class vtkWebGPURenderWindow;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkWebGPUPolyDataMapper* New();
  vtkTypeMacro(vtkWebGPUPolyDataMapper, vtkPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
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
  vtkWebGPUPolyDataMapper(const vtkWebGPUPolyDataMapper&) = delete;
  void operator=(const vtkWebGPUPolyDataMapper&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
