// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUGlyph3DMapper.h"
#include "vtkActor.h"
#include "vtkArrayDispatch.h"
#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkColor.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkGlyph3DMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkQuaternion.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"
#include "vtkWebGPUActor.h"
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "vtkWebGPUCommandEncoderDebugGroup.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include "Private/vtkWebGPUActorInternals.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUPipelineLayoutInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"

#include "LineGlyphShader.h"
#include "PointGlyphShader.h"
#include "SurfaceMeshGlyphShader.h"

#include <stack>

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPUGlyph3DMapperHelper : public vtkPolyDataMapper
{
  /**
   * All the attributes supported by the point data buffer
   */
  enum PointDataAttributes : int
  {
    POINT_POSITIONS = 0,
    POINT_NORMALS,
    POINT_TANGENTS,
    POINT_UVS,
    NUM_POINT_ATTRIBUTES,
    POINT_UNDEFINED
  };

  /**
   * All the attributes supported by the cell data buffer
   */
  enum CellDataAttributes : int
  {
    CELL_NORMALS = 0,
    NUM_CELL_ATTRIBUTES,
    CELL_UNDEFINED
  };

  /**
   * All the attributes supported by the instance data buffer
   */
  enum InstanceDataAttributes : int
  {
    INSTANCE_COLORS = 0,
    INSTANCE_TRANSFORMS,
    INSTANCE_NORMAL_TRANSFORMS,
    NUM_INSTANCE_ATTRIBUTES,
    INSTANCE_UNDEFINED
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
    // Pipeline that renders lines.
    GFX_PIPELINE_LINES,
    // Pipeline that renders triangles
    GFX_PIPELINE_TRIANGLES,
    NUM_GFX_PIPELINE_TYPES
  };

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

    struct
    {
      // instance attributes.
      wgpu::Buffer Buffer;
      uint64_t Size = 0;
    } Instance;
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
    AttributeDescriptor Normals;
    AttributeDescriptor Tangents;
    AttributeDescriptor UVs;
    AttributeDescriptor CellNormals;
    AttributeDescriptor InstanceColors;
    AttributeDescriptor InstanceTransforms;
    AttributeDescriptor InstanceNormalTransforms;
    vtkTypeUInt32 CompositeId;
    vtkTypeUInt32 ProcessId;
    vtkTypeUInt32 Pickable;
  };
  wgpu::Buffer AttributeDescriptorBuffer;
  wgpu::BindGroup MeshAttributeBindGroup;
  bool HasPointAttributes[NUM_POINT_ATTRIBUTES];
  bool HasCellAttributes[NUM_CELL_ATTRIBUTES];
  ///@{ Timestamps help reuse previous resources as much as possible.
  vtkTimeStamp CellAttributesBuildTimestamp[NUM_CELL_ATTRIBUTES];
  vtkTimeStamp PointAttributesBuildTimestamp[NUM_POINT_ATTRIBUTES];
  vtkTimeStamp InstanceAttributesBuildTimestamp[NUM_INSTANCE_ATTRIBUTES];
  ///@}

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
  std::string GraphicsPipelineKeys[NUM_GFX_PIPELINE_TYPES] = {};

  bool RebuildGraphicsPipelines = true;
  // used by RenderPiece and functions it calls to reduce
  // calls to get the input and allow for rendering of
  // other polydata (not the input)
  vtkPolyData* CurrentInput = nullptr;
  int NumberOfGlyphPoints = 0;
  std::vector<vtkTypeFloat32>* InstanceColors;
  std::vector<vtkTypeFloat32>* InstanceTransforms;
  std::vector<vtkTypeFloat32>* InstanceNormalTransforms;
  vtkTypeUInt32 FlatIndex = 0;
  bool Pickable = false;
  bool PickingAttributesModified = false;
  vtkMTimeType GlyphStructuresBuildTime = 0;

  struct ActorState
  {
    bool LastActorBackfaceCulling = false;
    bool LastActorFrontfaceCulling = false;
    bool LastVertexVisibility = false;
    int LastRepresentation = VTK_SURFACE;
    bool LastHasRenderingTranslucentGeometry = false;
  };
  std::map<std::pair<vtkWeakPointer<vtkActor>, vtkWeakPointer<vtkRenderer>>, ActorState>
    CachedActorRendererProperties;

  const std::array<const char**, vtkWebGPUGlyph3DMapperHelper::NUM_GFX_PIPELINE_TYPES>
    GraphicsPipelineShaderSources = { &PointGlyphShader, &LineGlyphShader,
      &SurfaceMeshGlyphShader };

  const std::array<wgpu::PrimitiveTopology, vtkWebGPUGlyph3DMapperHelper::NUM_GFX_PIPELINE_TYPES>
    GraphicsPipelinePrimitiveTypes = { wgpu::PrimitiveTopology::TriangleList,
      wgpu::PrimitiveTopology::TriangleList, wgpu::PrimitiveTopology::TriangleList };

  std::map<vtkWebGPUGlyph3DMapperHelper::GraphicsPipelineType,
    std::vector<vtkWebGPUCellToPrimitiveConverter::TopologySourceType>>
    PipelineBindGroupCombos[VTK_SURFACE + 1] = { // VTK_POINTS
      { { vtkWebGPUGlyph3DMapperHelper::GFX_PIPELINE_POINTS,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS } } },
      // VTK_WIREFRAME
      { { vtkWebGPUGlyph3DMapperHelper::GFX_PIPELINE_LINES,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES } } },
      // VTK_SURFACE
      {
        { vtkWebGPUGlyph3DMapperHelper::GFX_PIPELINE_POINTS,
          { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS } },
        { vtkWebGPUGlyph3DMapperHelper::GFX_PIPELINE_LINES,
          { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES } },
        { vtkWebGPUGlyph3DMapperHelper::GFX_PIPELINE_TRIANGLES,
          { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS } },
      }
    };

  /**
   * Order in which the point data attributes are concatenated into the mapper mesh SSBO
   */
  const PointDataAttributes PointDataAttributesOrder[PointDataAttributes::NUM_POINT_ATTRIBUTES] = {
    PointDataAttributes::POINT_POSITIONS, PointDataAttributes::POINT_NORMALS,
    PointDataAttributes::POINT_TANGENTS, PointDataAttributes::POINT_UVS
  };

  /**
   * Order in which the cell data attributes are concatenated into the mapper mesh SSBO
   */
  const CellDataAttributes CellDataAttributesOrder[CellDataAttributes::NUM_CELL_ATTRIBUTES] = {
    CellDataAttributes::CELL_NORMALS
  };

  /**
   * Order in which the instance data attributes are concatenated into the mapper mesh SSBO
   */
  const InstanceDataAttributes
    InstanceDataAttributesOrder[InstanceDataAttributes::NUM_INSTANCE_ATTRIBUTES] = {
      InstanceDataAttributes::INSTANCE_COLORS, InstanceDataAttributes::INSTANCE_TRANSFORMS,
      InstanceDataAttributes::INSTANCE_NORMAL_TRANSFORMS
    };

  template <typename DestT>
  struct WriteTypedArray
  {
    std::size_t ByteOffset = 0;
    const wgpu::Buffer& DstBuffer;
    vtkSmartPointer<vtkWebGPUConfiguration> WGPUConfiguration;
    float Denominator = 1.0;

    template <typename SrcArrayT>
    void operator()(SrcArrayT* array, const char* description)
    {
      if (array == nullptr || this->DstBuffer.Get() == nullptr)
      {
        return;
      }
      const auto values = vtk::DataArrayValueRange(array);
      vtkNew<vtkAOSDataArrayTemplate<DestT>> data;
      for (const auto& value : values)
      {
        data->InsertNextValue(value / this->Denominator);
      }
      const std::size_t nbytes = data->GetNumberOfValues() * sizeof(DestT);
      this->WGPUConfiguration->WriteBuffer(
        this->DstBuffer, this->ByteOffset, data->GetPointer(0), nbytes, description);
      this->ByteOffset += nbytes;
    }

    void operator()(std::vector<vtkTypeFloat32>& inputVector, const char* description)
    {
      if (inputVector.empty())
      {
        return;
      }
      const std::size_t nbytes = inputVector.size() * sizeof(vtkTypeFloat32);
      this->WGPUConfiguration->WriteBuffer(
        this->DstBuffer, this->ByteOffset, inputVector.data(), nbytes, description);
      this->ByteOffset += nbytes;
    }

    /**
     * Seek into the buffer by the number of bytes in `array`.
     * This is useful for partial updates to the point/cell attribute buffer.
     * if `array` doesn't need to be updated, then the `ByteOffset` will be
     * correctly setup when writing the next attribute.
     *
     * Assume that a webgpu buffer is packed with arrays A, B and C, whose values are
     *
     * |a1,a2,a3,a4,a5|b1,b2,b3,b4,b5,b6|c1,c2,c3|
     *
     * When array 'B' has the same size as previous upload, but it's values have changed and values
     * of array 'A' have not changed, the mapper is designed to partially update only the portion of
     * the buffer which has values corresponding to array 'B'. To facilitate such partial updates,
     * use `Advance` for array 'A' to seek forward all the way to the end of the array 'A' in the
     * webgpu buffer before writing values of 'B'
     *
     * \|/
     *  |a1,a2,a3,a4,a5|b1,b2,b3,b4,b5,b6|c1,c2,c3|
     * -> Advance(A)
     *                \|/
     *  |a1,a2,a3,a4,a5|b1,b2,b3,b4,b5,b6|c1,c2,c3|
     * -> operator()(B)
     */
    void Advance(vtkDataArray* array)
    {
      if (!array)
      {
        return;
      }
      const std::size_t nbytes = array->GetNumberOfValues() * sizeof(DestT);
      this->ByteOffset += nbytes;
    }

    void Advance(std::vector<vtkTypeFloat32>& inputVector)
    {
      if (inputVector.empty())
      {
        return;
      }
      const std::size_t nbytes = inputVector.size() * sizeof(vtkTypeFloat32);
      this->ByteOffset += nbytes;
    }
  };

public:
  vtkTypeMacro(vtkWebGPUGlyph3DMapperHelper, vtkPolyDataMapper);

  //------------------------------------------------------------------------------
  static vtkWebGPUGlyph3DMapperHelper* New()
  {
    VTK_STANDARD_NEW_BODY(vtkWebGPUGlyph3DMapperHelper);
  }

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override
  {
    this->Superclass::ReleaseGraphicsResources(window);

    // Release mesh buffers, bind groups and reset the attribute build timestamps.
    this->MeshSSBO.Cell.Buffer = nullptr;
    this->MeshSSBO.Cell.Size = 0;
    for (int attributeIndex = 0; attributeIndex < NUM_CELL_ATTRIBUTES; ++attributeIndex)
    {
      this->CellAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
    }
    this->MeshSSBO.Point.Buffer = nullptr;
    this->MeshSSBO.Point.Size = 0;
    for (int attributeIndex = 0; attributeIndex < NUM_POINT_ATTRIBUTES; ++attributeIndex)
    {
      this->PointAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
    }
    this->MeshSSBO.Instance.Buffer = nullptr;
    this->MeshSSBO.Instance.Size = 0;
    for (int attributeIndex = 0; attributeIndex < NUM_INSTANCE_ATTRIBUTES; ++attributeIndex)
    {
      this->InstanceAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
    }
    this->AttributeDescriptorBuffer = nullptr;
    this->MeshAttributeBindGroup = nullptr;

    // Release topology conversion pipelines and reset their build timestamps.
    for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
    {
      this->TopologyBindGroupInfos[i] = TopologyBindGroupInfo{};
    }
    this->CellConverter->ReleaseGraphicsResources(window);
    this->RebuildGraphicsPipelines = true;
    this->CachedActorRendererProperties.clear();
  }

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
  bool CacheActorRendererProperties(vtkActor* actor, vtkRenderer* renderer)
  {
    const auto key = std::make_pair(actor, renderer);
    auto it = this->CachedActorRendererProperties.find(key);
    auto* displayProperty = actor->GetProperty();
    bool hasTranslucentPolygonalGeometry = false;
    if (actor)
    {
      hasTranslucentPolygonalGeometry = actor->HasTranslucentPolygonalGeometry();
    }
    if (it == this->CachedActorRendererProperties.end())
    {
      ActorState state = {};
      state.LastActorBackfaceCulling = displayProperty->GetBackfaceCulling();
      state.LastActorFrontfaceCulling = displayProperty->GetFrontfaceCulling();
      state.LastRepresentation = displayProperty->GetRepresentation();
      state.LastVertexVisibility = displayProperty->GetVertexVisibility();
      state.LastHasRenderingTranslucentGeometry = hasTranslucentPolygonalGeometry;
      this->CachedActorRendererProperties[key] = state;
      return true;
    }
    else
    {
      auto& state = it->second;
      bool cacheChanged = false;
      if (state.LastActorBackfaceCulling != displayProperty->GetBackfaceCulling())
      {
        cacheChanged = true;
      }
      state.LastActorBackfaceCulling = displayProperty->GetBackfaceCulling();
      if (state.LastActorFrontfaceCulling != displayProperty->GetFrontfaceCulling())
      {
        cacheChanged = true;
      }
      state.LastActorFrontfaceCulling = displayProperty->GetFrontfaceCulling();
      if (state.LastRepresentation != displayProperty->GetRepresentation())
      {
        cacheChanged = true;
      }
      state.LastRepresentation = displayProperty->GetRepresentation();
      if (state.LastVertexVisibility != displayProperty->GetVertexVisibility())
      {
        cacheChanged = true;
      }
      state.LastVertexVisibility = displayProperty->GetVertexVisibility();
      if (state.LastHasRenderingTranslucentGeometry != hasTranslucentPolygonalGeometry)
      {
        cacheChanged = true;
      }
      state.LastHasRenderingTranslucentGeometry = hasTranslucentPolygonalGeometry;
      return cacheChanged;
    }
  }

  /**
   * Looks at the point/cell data of `vtkPolyData` object and determines
   * which attributes are available. Scalars should have been mapped if required.
   */
  void DeducePointCellAttributeAvailability(vtkPolyData* mesh)
  {
    this->ResetPointCellAttributeState();
    if (mesh == nullptr)
    {
      return;
    }

    vtkPointData* pointData = mesh->GetPointData();
    vtkCellData* cellData = mesh->GetCellData();

    this->HasPointAttributes[POINT_POSITIONS] = true;
    this->HasPointAttributes[POINT_NORMALS] = pointData->GetNormals() != nullptr;
    this->HasPointAttributes[POINT_TANGENTS] = pointData->GetTangents();
    this->HasPointAttributes[POINT_UVS] = pointData->GetTCoords();
    // check for cell normals
    this->HasCellAttributes[CELL_NORMALS] = cellData->GetNormals() != nullptr;
  }

  /**
   * Reset the internal `Has{Point,Cell}Attribute` booleans to `false`.
   */
  void ResetPointCellAttributeState()
  {
    for (int i = 0; i < NUM_POINT_ATTRIBUTES; ++i)
    {
      this->HasPointAttributes[i] = false;
    }
    for (int i = 0; i < NUM_CELL_ATTRIBUTES; ++i)
    {
      this->HasCellAttributes[i] = false;
    }
  }

  /**
   * Create a bind group layout for the mesh attribute bind group.
   */
  static wgpu::BindGroupLayout CreateMeshAttributeBindGroupLayout(
    const wgpu::Device& device, const std::string& label)
  {
    return vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      {
        // clang-format off
        // MeshAttributeArrayDescriptor
        { 0, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage },
        // point_data
        { 1, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // cell_data
        { 2, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // instance_data
        { 3, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // clang-format on
      },
      label);
  }

  /**
   * Create a bind group layout for the `TopologyRenderInfo::BindGroup`
   */
  static wgpu::BindGroupLayout CreateTopologyBindGroupLayout(
    const wgpu::Device& device, const std::string& label)
  {
    return vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      {
        // clang-format off
        // topology
        { 0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // edge_array
        { 1, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // clang-format on
      },
      label);
  }

  /**
   * Create a bind group for the point and cell attributes of a mesh. It has four bindings.
   *
   * 0: [storage] AttributeDescriptorBuffer
   *      tells where different attributes start and end for each
   *      sub-buffer in the point/cell buffers.
   *
   * 1: [storage] MeshSSBO.Point.Buffer
   *      all point attributes
   *      @sa vtkWebGPUGlyph3DMapperHelper::PointDataAttributes
   *
   * 2: [storage] MeshSSBO.Cell.Buffer
   *      all cell attributes
   *      @sa vtkWebGPUGlyph3DMapperHelper::CellDataAttributes
   *
   * 3: [storage] MeshSSBO.Cell.Buffer
   *      all instance attributes
   *      @sa vtkWebGPUGlyph3DMapperHelper::InstanceDataAttributes
   */
  wgpu::BindGroup CreateMeshAttributeBindGroup(const wgpu::Device& device, const std::string& label)
  {
    auto layout = this->CreateMeshAttributeBindGroupLayout(device, label + "_LAYOUT");
    return vtkWebGPUBindGroupInternals::MakeBindGroup(device, layout,
      {
        // clang-format off
        { 0, this->AttributeDescriptorBuffer, 0 },
        { 1, this->MeshSSBO.Point.Buffer, 0 },
        { 2, this->MeshSSBO.Cell.Buffer, 0 },
        { 3, this->MeshSSBO.Instance.Buffer, 0 }
        // clang-format on
      },
      label);
  }

  /**
   * Create a bind group for the primitives of a mesh. It has 2 bindings.
   *
   * 0: [storage] TopologyRenderInfo.TopologyBuffer
   *      sequence of cell_id,point_id for all vertices
   *      @sa vtkWebGPUGlyph3DMapperHelper::TopologyRenderInfo
   *
   * 1: [storage] TopologyRenderInfo.EdgeArrayBuffer
   *      sequence of edge_value for all triangles
   *      @sa vtkWebGPUGlyph3DMapperHelper::TopologyRenderInfo
   */
  wgpu::BindGroup CreateTopologyBindGroup(const wgpu::Device& device, const std::string& label,
    vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType)
  {
    const auto& info = this->TopologyBindGroupInfos[topologySourceType];
    {
      auto layout = this->CreateTopologyBindGroupLayout(device, label + "_LAYOUT");
      return vtkWebGPUBindGroupInternals::MakeBindGroup(device, layout,
        {
          { 0, info.TopologyBuffer, 0 },
          { 1, info.EdgeArrayBuffer, 0 },
        },
        label);
    }
  }

  /**
   * Returns the size of the 'sub-buffer' within the whole point data SSBO for the given attribute
   */
  unsigned long GetPointAttributeByteSize(
    vtkWebGPUGlyph3DMapperHelper::PointDataAttributes attribute)
  {
    switch (attribute)
    {
      case PointDataAttributes::POINT_POSITIONS:
        return this->CurrentInput->GetNumberOfPoints() * 3 * sizeof(vtkTypeFloat32);

      case PointDataAttributes::POINT_NORMALS:
        if (this->HasPointAttributes[attribute])
        {
          return this->CurrentInput->GetPointData()->GetNormals()->GetNumberOfValues() *
            sizeof(vtkTypeFloat32);
        }

        break;

      case PointDataAttributes::POINT_TANGENTS:
        if (this->HasPointAttributes[attribute])
        {
          return this->CurrentInput->GetPointData()->GetTangents()->GetNumberOfValues() *
            sizeof(vtkTypeFloat32);
        }

        break;

      case PointDataAttributes::POINT_UVS:
        if (this->HasPointAttributes[attribute])
        {
          return this->CurrentInput->GetPointData()->GetTCoords()->GetNumberOfValues() *
            sizeof(vtkTypeFloat32);
        }

        break;

      default:
        break;
    }

    return 0;
  }

  /**
   * Returns the size of the 'sub-buffer' within the whole cell data SSBO for the given attribute
   */
  unsigned long GetCellAttributeByteSize(vtkWebGPUGlyph3DMapperHelper::CellDataAttributes attribute)
  {
    switch (attribute)
    {
      case CellDataAttributes::CELL_NORMALS:
        if (this->HasCellAttributes[attribute])
        {
          return this->CurrentInput->GetCellData()->GetNormals()->GetDataSize() *
            sizeof(vtkTypeFloat32);
        }

        break;

      default:
        break;
    }

    return 0;
  }

  /**
   * Returns the size of the 'sub-buffer' within the whole cell data SSBO for the given attribute
   */
  unsigned long GetInstanceAttributeByteSize(
    vtkWebGPUGlyph3DMapperHelper::InstanceDataAttributes attribute)
  {
    switch (attribute)
    {
      case InstanceDataAttributes::INSTANCE_COLORS:
        if (this->InstanceColors)
        {
          return this->InstanceColors->size() * sizeof(vtkTypeFloat32);
        }

        break;

      case InstanceDataAttributes::INSTANCE_TRANSFORMS:
        if (this->InstanceTransforms)
        {
          return this->InstanceTransforms->size() * sizeof(vtkTypeFloat32);
        }

        break;

      case InstanceDataAttributes::INSTANCE_NORMAL_TRANSFORMS:
        if (this->InstanceNormalTransforms)
        {
          return this->InstanceNormalTransforms->size() * sizeof(vtkTypeFloat32);
        }

        break;

      default:
        break;
    }

    return 0;
  }

  /**
   * Calculates the size of a buffer that is large enough to contain
   * all the values from the point attributes. See vtkWebGPUGlyph3DMapperHelper::PointDataAttributes
   * for the kinds of attributes.
   */
  unsigned long GetExactPointBufferSize()
  {
    unsigned long result = 0;

    result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_POSITIONS);
    result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_NORMALS);
    result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_TANGENTS);
    result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_UVS);

    result = vtkWebGPUConfiguration::Align(result, 32);
    return result;
  }

  /**
   * Calculates the size of a buffer that is large enough to contain
   * all the values from the cell attributes. See vtkWebGPUGlyph3DMapperHelper::CellDataAttributes
   * for the kinds of attributes.
   */
  unsigned long GetExactCellBufferSize()
  {
    unsigned long result = 0;

    result += this->GetCellAttributeByteSize(CellDataAttributes::CELL_NORMALS);

    result = vtkWebGPUConfiguration::Align(result, 32);
    return result;
  }

  /**
   * Calculates the size of a buffer that is large enough to contain
   * all the values from the cell attributes. See
   * vtkWebGPUGlyph3DMapperHelper::InstanceDataAttributes for the kinds of attributes.
   */
  unsigned long GetExactInstanceBufferSize()
  {
    unsigned long result = 0;

    result += this->GetInstanceAttributeByteSize(InstanceDataAttributes::INSTANCE_COLORS);
    result += this->GetInstanceAttributeByteSize(InstanceDataAttributes::INSTANCE_TRANSFORMS);
    result +=
      this->GetInstanceAttributeByteSize(InstanceDataAttributes::INSTANCE_NORMAL_TRANSFORMS);

    result = vtkWebGPUConfiguration::Align(result, 32);
    return result;
  }

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
  void UpdateMeshGeometryBuffers(vtkWebGPURenderWindow* wgpuRenderWindow)
  {
    this->DeducePointCellAttributeAvailability(this->CurrentInput);
    MeshAttributeDescriptor meshAttrDescriptor = {};
    meshAttrDescriptor.CompositeId = this->FlatIndex;
    meshAttrDescriptor.Pickable = this->Pickable ? 1u : 0u;
    meshAttrDescriptor.ProcessId = 0;

    vtkPointData* pointData = this->CurrentInput->GetPointData();
    vtkDataArray* pointPositions = this->CurrentInput->GetPoints()->GetData();
    vtkDataArray* pointNormals = pointData->GetNormals();
    vtkDataArray* pointTangents = pointData->GetTangents();
    vtkDataArray* pointUvs = pointData->GetTCoords();

    using DispatchT = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;

    // Realloc WGPUBuffer to fit all point attributes.
    auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
    bool updatePointDescriptor = false;
    uint64_t currentPointBufferSize = 0;
    uint64_t requiredPointBufferSize = this->GetExactPointBufferSize();
    if (this->MeshSSBO.Point.Buffer)
    {
      currentPointBufferSize = this->MeshSSBO.Point.Size;
    }
    if (currentPointBufferSize != requiredPointBufferSize)
    {
      if (this->MeshSSBO.Point.Buffer)
      {
        this->MeshSSBO.Point.Buffer.Destroy();
        this->MeshSSBO.Point.Size = 0;
      }
      wgpu::BufferDescriptor pointBufDescriptor{};
      pointBufDescriptor.size = requiredPointBufferSize;
      const auto label = "pointdata-" + this->GetObjectDescription() + "-" +
        this->CurrentInput->GetObjectDescription();
      pointBufDescriptor.label = label.c_str();
      pointBufDescriptor.mappedAtCreation = false;
      pointBufDescriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
      this->MeshSSBO.Point.Buffer = wgpuConfiguration->CreateBuffer(pointBufDescriptor);
      this->MeshSSBO.Point.Size = requiredPointBufferSize;
      for (int attributeIndex = 0; attributeIndex < PointDataAttributes::NUM_POINT_ATTRIBUTES;
           attributeIndex++)
      {
        this->PointAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
      }
      updatePointDescriptor = true;
    }

    WriteTypedArray<vtkTypeFloat32> pointDataWriter{ 0, this->MeshSSBO.Point.Buffer,
      wgpuConfiguration, 1. };

    pointDataWriter.Denominator = 1.0;
    pointDataWriter.ByteOffset = 0;
    for (int attributeIndex = 0; attributeIndex < PointDataAttributes::NUM_POINT_ATTRIBUTES;
         attributeIndex++)
    {
      switch (PointDataAttributesOrder[attributeIndex])
      {
        case PointDataAttributes::POINT_POSITIONS:
          meshAttrDescriptor.Positions.Start = pointDataWriter.ByteOffset / sizeof(vtkTypeFloat32);

          if (pointPositions->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
          {
            if (!DispatchT::Execute(pointPositions, pointDataWriter, "Positions"))
            {
              pointDataWriter(pointPositions, "Positions");
            }
            this->PointAttributesBuildTimestamp[attributeIndex].Modified();
          }
          else
          {
            pointDataWriter.Advance(pointPositions);
          }
          meshAttrDescriptor.Positions.NumComponents = pointPositions->GetNumberOfComponents();
          meshAttrDescriptor.Positions.NumTuples = pointPositions->GetNumberOfTuples();

          break;

        case PointDataAttributes::POINT_NORMALS:
          meshAttrDescriptor.Normals.Start = pointDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
          if (pointNormals &&
            pointNormals->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
          {
            if (!DispatchT::Execute(pointNormals, pointDataWriter, "Normals"))
            {
              pointDataWriter(pointNormals, "Normals");
            }
            this->PointAttributesBuildTimestamp[attributeIndex].Modified();
          }
          else
          {
            pointDataWriter.Advance(pointNormals);
          }
          meshAttrDescriptor.Normals.NumComponents =
            pointNormals ? pointNormals->GetNumberOfComponents() : 0;
          meshAttrDescriptor.Normals.NumTuples =
            pointNormals ? pointNormals->GetNumberOfTuples() : 0;
          break;

        case PointDataAttributes::POINT_TANGENTS:
          meshAttrDescriptor.Tangents.Start = pointDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
          if (pointTangents &&
            pointTangents->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
          {
            if (!DispatchT::Execute(pointTangents, pointDataWriter, "Tangents"))
            {
              pointDataWriter(pointTangents, "Tangents");
            }
            this->PointAttributesBuildTimestamp[attributeIndex].Modified();
          }
          else
          {
            pointDataWriter.Advance(pointTangents);
          }
          meshAttrDescriptor.Tangents.NumComponents =
            pointTangents ? pointTangents->GetNumberOfComponents() : 0;
          meshAttrDescriptor.Tangents.NumTuples =
            pointTangents ? pointTangents->GetNumberOfTuples() : 0;
          break;

        case PointDataAttributes::POINT_UVS:
          meshAttrDescriptor.UVs.Start = pointDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
          if (pointUvs &&
            pointUvs->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
          {
            if (!DispatchT::Execute(pointUvs, pointDataWriter, "UVs"))
            {
              pointDataWriter(pointUvs, "UVs");
            }
            this->PointAttributesBuildTimestamp[attributeIndex].Modified();
          }
          else
          {
            pointDataWriter.Advance(pointUvs);
          }
          meshAttrDescriptor.UVs.NumComponents = pointUvs ? pointUvs->GetNumberOfComponents() : 0;
          meshAttrDescriptor.UVs.NumTuples = pointUvs ? pointUvs->GetNumberOfTuples() : 0;
          break;

        default:
          break;
      }
    }

    WriteTypedArray<vtkTypeFloat32> cellDataWriter{ 0, this->MeshSSBO.Cell.Buffer,
      wgpuConfiguration, 1. };

    vtkCellData* cellData = this->CurrentInput->GetCellData();
    vtkDataArray* cellNormals =
      this->HasCellAttributes[CELL_NORMALS] ? cellData->GetNormals() : nullptr;

    // Realloc WGPUBuffer to fit all cell attributes.
    bool updateCellArrayDescriptor = false;
    uint64_t currentCellBufferSize = 0;
    uint64_t requiredCellBufferSize = this->GetExactCellBufferSize();
    if (requiredCellBufferSize == 0)
    {
      requiredCellBufferSize = 4; // placeholder
    }
    if (this->MeshSSBO.Cell.Buffer)
    {
      currentCellBufferSize = this->MeshSSBO.Cell.Size;
    }
    if (currentCellBufferSize != requiredCellBufferSize)
    {
      if (this->MeshSSBO.Cell.Buffer)
      {
        this->MeshSSBO.Cell.Buffer.Destroy();
        this->MeshSSBO.Cell.Size = 0;
      }
      wgpu::BufferDescriptor cellBufDescriptor{};
      cellBufDescriptor.size = requiredCellBufferSize;
      const auto label = "celldata-" + this->GetObjectDescription() + "-" +
        this->CurrentInput->GetObjectDescription();
      cellBufDescriptor.label = label.c_str();
      cellBufDescriptor.mappedAtCreation = false;
      cellBufDescriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
      this->MeshSSBO.Cell.Buffer = wgpuConfiguration->CreateBuffer(cellBufDescriptor);
      this->MeshSSBO.Cell.Size = requiredCellBufferSize;
      for (int attributeIndex = 0; attributeIndex < CellDataAttributes::NUM_CELL_ATTRIBUTES;
           attributeIndex++)
      {
        this->CellAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
      }
      updateCellArrayDescriptor = true;
    }

    for (int attributeIndex = 0; attributeIndex < CellDataAttributes::NUM_CELL_ATTRIBUTES;
         attributeIndex++)
    {
      switch (CellDataAttributesOrder[attributeIndex])
      {
        case CellDataAttributes::CELL_NORMALS:
        {
          meshAttrDescriptor.CellNormals.Start = cellDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
          if (cellNormals &&
            cellNormals->GetMTime() > this->CellAttributesBuildTimestamp[attributeIndex])
          {
            if (!DispatchT::Execute(cellNormals, cellDataWriter, "Cell normals"))
            {
              cellDataWriter(cellNormals, "Cell normals");
            }
            this->CellAttributesBuildTimestamp[attributeIndex].Modified();
          }
          else
          {
            cellDataWriter.Advance(cellNormals);
          }
          meshAttrDescriptor.CellNormals.NumComponents =
            cellNormals ? cellNormals->GetNumberOfComponents() : 0;
          meshAttrDescriptor.CellNormals.NumTuples =
            cellNormals ? cellNormals->GetNumberOfTuples() : 0;

          break;
        }

        default:
          break;
      }
    }

    // Realloc WGPUBuffer to fit all cell attributes.
    WriteTypedArray<vtkTypeFloat32> instanceDataWriter{ 0, this->MeshSSBO.Instance.Buffer,
      wgpuConfiguration, 1. };
    bool updateInstanceArrayDescriptor = false;
    uint64_t currentInstanceBufferSize = 0;
    uint64_t requiredInstanceBufferSize = this->GetExactInstanceBufferSize();
    if (requiredInstanceBufferSize == 0)
    {
      requiredInstanceBufferSize = 4; // placeholder
    }
    if (this->MeshSSBO.Instance.Buffer)
    {
      currentInstanceBufferSize = this->MeshSSBO.Instance.Size;
    }
    if (currentInstanceBufferSize != requiredInstanceBufferSize)
    {
      if (this->MeshSSBO.Instance.Buffer)
      {
        this->MeshSSBO.Instance.Buffer.Destroy();
        this->MeshSSBO.Instance.Size = 0;
      }
      wgpu::BufferDescriptor instanceBufDescriptor{};
      instanceBufDescriptor.size = requiredInstanceBufferSize;
      const auto label = "InstanceAttributes-" + this->GetObjectDescription() + "-" +
        this->CurrentInput->GetObjectDescription();
      instanceBufDescriptor.label = label.c_str();
      instanceBufDescriptor.mappedAtCreation = false;
      instanceBufDescriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
      this->MeshSSBO.Instance.Buffer = wgpuConfiguration->CreateBuffer(instanceBufDescriptor);
      this->MeshSSBO.Instance.Size = requiredInstanceBufferSize;
      for (int attributeIndex = 0; attributeIndex < InstanceDataAttributes::NUM_INSTANCE_ATTRIBUTES;
           attributeIndex++)
      {
        this->InstanceAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
      }
      updateInstanceArrayDescriptor = true;
    }

    for (int attributeIndex = 0; attributeIndex < InstanceDataAttributes::NUM_INSTANCE_ATTRIBUTES;
         attributeIndex++)
    {
      switch (InstanceDataAttributesOrder[attributeIndex])
      {
        case InstanceDataAttributes::INSTANCE_COLORS:
        {
          meshAttrDescriptor.InstanceColors.Start =
            instanceDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
          if (this->InstanceColors &&
            this->GlyphStructuresBuildTime > this->InstanceAttributesBuildTimestamp[attributeIndex])
          {
            instanceDataWriter(*this->InstanceColors, "Instance colors");
            this->InstanceAttributesBuildTimestamp[attributeIndex].Modified();
          }
          else
          {
            instanceDataWriter.Advance(*this->InstanceColors);
          }
          meshAttrDescriptor.InstanceColors.NumComponents = 1;
          meshAttrDescriptor.InstanceColors.NumTuples = this->InstanceColors->size();

          break;
        }
        case InstanceDataAttributes::INSTANCE_TRANSFORMS:
        {
          meshAttrDescriptor.InstanceTransforms.Start =
            instanceDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
          if (this->InstanceTransforms &&
            this->GlyphStructuresBuildTime > this->InstanceAttributesBuildTimestamp[attributeIndex])
          {
            instanceDataWriter(*this->InstanceTransforms, "Instance transforms");
            this->InstanceAttributesBuildTimestamp[attributeIndex].Modified();
          }
          else
          {
            instanceDataWriter.Advance(*this->InstanceTransforms);
          }
          meshAttrDescriptor.InstanceTransforms.NumComponents = 1;
          meshAttrDescriptor.InstanceTransforms.NumTuples = this->InstanceTransforms->size();

          break;
        }
        case InstanceDataAttributes::INSTANCE_NORMAL_TRANSFORMS:
        {
          meshAttrDescriptor.InstanceNormalTransforms.Start =
            instanceDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
          if (this->InstanceNormalTransforms &&
            this->GlyphStructuresBuildTime > this->InstanceAttributesBuildTimestamp[attributeIndex])
          {
            instanceDataWriter(*this->InstanceNormalTransforms, "Instance normal transforms");
            this->InstanceAttributesBuildTimestamp[attributeIndex].Modified();
          }
          else
          {
            instanceDataWriter.Advance(*this->InstanceNormalTransforms);
          }
          meshAttrDescriptor.InstanceNormalTransforms.NumComponents = 1;
          meshAttrDescriptor.InstanceNormalTransforms.NumTuples =
            this->InstanceNormalTransforms->size();

          break;
        }

        default:
          break;
      }
    }

    // handle partial updates
    if (updatePointDescriptor || updateCellArrayDescriptor || updateInstanceArrayDescriptor)
    {
      const std::string meshAttrDescriptorLabel = "MeshAttributeDescriptor-" +
        this->GetObjectDescription() + "-" + this->CurrentInput->GetObjectDescription();
      if (this->AttributeDescriptorBuffer == nullptr)
      {
        this->AttributeDescriptorBuffer = wgpuConfiguration->CreateBuffer(
          sizeof(meshAttrDescriptor), wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
          /*mappedAtCreation=*/false, meshAttrDescriptorLabel.c_str());
      }
      wgpuConfiguration->WriteBuffer(this->AttributeDescriptorBuffer, 0, &meshAttrDescriptor,
        sizeof(meshAttrDescriptor), meshAttrDescriptorLabel.c_str());
      // Create bind group for the point/cell attribute buffers.
      this->MeshAttributeBindGroup = this->CreateMeshAttributeBindGroup(
        wgpuConfiguration->GetDevice(), "MeshAttributeBindGroup");
      this->RebuildGraphicsPipelines = true;
    }
    else if (this->PickingAttributesModified)
    {
      // update only the portion of the buffer relevant to picking attributes.
      const auto* data = reinterpret_cast<void*>(&meshAttrDescriptor.CompositeId);
      constexpr auto offset = offsetof(MeshAttributeDescriptor, CompositeId);
      constexpr auto size = sizeof(MeshAttributeDescriptor) - offset;
      wgpuConfiguration->WriteBuffer(
        this->AttributeDescriptorBuffer, offset, data, size, "MeshAttributeDescriptor.CompositeId");
    }
  }
  ///@}

  /**
   * Get the name of the graphics pipeline type as a string.
   */
  const char* GetGraphicsPipelineTypeAsString(GraphicsPipelineType graphicsPipelineType)
  {
    switch (graphicsPipelineType)
    {
      case GFX_PIPELINE_POINTS:
        return "GFX_PIPELINE_GLYPH_POINTS";
      case GFX_PIPELINE_LINES:
        return "GFX_PIPELINE_GLYPH_LINES";
      case GFX_PIPELINE_TRIANGLES:
        return "GFX_PIPELINE_GLYPH_TRIANGLES";
      default:
        return "";
    }
  }

  /**
   * Creates the graphics pipeline. Rendering state is frozen after this point.
   * The build timestamp is recorded in `GraphicsPipelineBuildTimestamp`.
   */
  void SetupGraphicsPipelines(const wgpu::Device& device, vtkRenderer* renderer, vtkActor* actor)
  {
    auto* wgpuActor = vtkWebGPUActor::SafeDownCast(actor);
    auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
    auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
    auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

    vtkWebGPURenderPipelineDescriptorInternals descriptor;
    descriptor.vertex.entryPoint = "vertexMain";
    descriptor.vertex.bufferCount = 0;
    descriptor.cFragment.entryPoint = "fragmentMain";
    descriptor.EnableBlending(0);
    descriptor.cTargets[0].format = wgpuRenderWindow->GetPreferredSurfaceTextureFormat();
    ///@{ TODO: Only for valid depth stencil formats
    auto depthState = descriptor.EnableDepthStencil(wgpuRenderWindow->GetDepthStencilFormat());
    depthState->depthWriteEnabled = true;
    depthState->depthCompare = wgpu::CompareFunction::Less;
    ///@}
    // Prepare selection ids output.
    descriptor.cTargets[1].format = wgpuRenderWindow->GetPreferredSelectorIdsTextureFormat();
    descriptor.cFragment.targetCount++;
    descriptor.DisableBlending(1);

    // Update local parameters that decide whether a pipeline must be rebuilt.
    this->RebuildGraphicsPipelines = false;

    if (actor->GetProperty()->GetBackfaceCulling())
    {
      descriptor.primitive.cullMode = wgpu::CullMode::Back;
    }
    else if (actor->GetProperty()->GetFrontfaceCulling())
    {
      descriptor.primitive.cullMode = wgpu::CullMode::Front;
    }

    std::vector<wgpu::BindGroupLayout> bgls;
    wgpuRenderer->PopulateBindgroupLayouts(bgls);
    wgpuActor->Internals->PopulateBindgroupLayouts(bgls);
    bgls.emplace_back(
      this->CreateMeshAttributeBindGroupLayout(device, "MeshAttributeBindGroupLayout"));
    bgls.emplace_back(this->CreateTopologyBindGroupLayout(device, "TopologyBindGroupLayout"));
    descriptor.layout =
      vtkWebGPUPipelineLayoutInternals::MakePipelineLayout(device, bgls, "pipelineLayout");

    for (int i = 0; i < NUM_GFX_PIPELINE_TYPES; ++i)
    {
      descriptor.label =
        this->GetGraphicsPipelineTypeAsString(static_cast<GraphicsPipelineType>(i));
      descriptor.primitive.topology = GraphicsPipelinePrimitiveTypes[i];
      // generate a unique key for the pipeline descriptor and shader source pointer
      this->GraphicsPipelineKeys[i] =
        wgpuPipelineCache->GetPipelineKey(&descriptor, *(GraphicsPipelineShaderSources[i]));
      // create a pipeline if it does not already exist
      if (wgpuPipelineCache->GetRenderPipeline(this->GraphicsPipelineKeys[i]) == nullptr)
      {
        wgpuPipelineCache->CreateRenderPipeline(
          &descriptor, wgpuRenderer, *(GraphicsPipelineShaderSources[i]));
      }
    }
  }

  /**
   * Get whether the graphics pipeline needs rebuilt.
   * This method checks MTime of the vtkActor's vtkProperty instance against the build timestamp of
   * the graphics pipeline.
   */
  bool GetNeedToRebuildGraphicsPipelines(vtkActor* actor, vtkRenderer* renderer)
  {
    if (this->RebuildGraphicsPipelines)
    {
      return true;
    }
    const auto key = std::make_pair(actor, renderer);
    auto it = this->CachedActorRendererProperties.find(key);
    if (it == this->CachedActorRendererProperties.end())
    {
      return true;
    }
    auto* displayProperty = actor->GetProperty();
    if (it->second.LastActorBackfaceCulling != displayProperty->GetBackfaceCulling())
    {
      return true;
    }
    if (it->second.LastActorFrontfaceCulling != displayProperty->GetFrontfaceCulling())
    {
      return true;
    }
    return false;
  }

  //------------------------------------------------------------------------------
  void RecordDrawCommands(vtkRenderer* renderer, vtkActor* actor, int numPoints,
    const wgpu::RenderPassEncoder& passEncoder)
  {
    passEncoder.SetBindGroup(2, this->MeshAttributeBindGroup);

    auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
    auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

    auto* displayProperty = actor->GetProperty();
    const int representation = displayProperty->GetRepresentation();
    const bool showVertices = displayProperty->GetVertexVisibility();

    for (const auto& pipelineMapping : PipelineBindGroupCombos[representation])
    {
      const auto& pipelineKey = this->GraphicsPipelineKeys[pipelineMapping.first];
      passEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(pipelineMapping.first);
      vtkScopedEncoderDebugGroup(passEncoder, pipelineLabel);
      for (const auto& bindGroupType : pipelineMapping.second)
      {
        const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
        if (bgInfo.VertexCount == 0)
        {
          continue;
        }
        passEncoder.SetBindGroup(3, bgInfo.BindGroup);
        const auto topologyBGInfoName =
          vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
        vtkScopedEncoderDebugGroup(passEncoder, topologyBGInfoName);
        switch (bindGroupType)
        {
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS:
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS:
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS:
            passEncoder.Draw(/*vertexCount=*/6 * bgInfo.VertexCount, /*instanceCount=*/numPoints);
            break;
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES:
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES:
            passEncoder.Draw(/*vertexCount=*/3 * bgInfo.VertexCount, /*instanceCount=*/numPoints);
            break;
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS:
            passEncoder.Draw(/*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/numPoints);
            break;
          case vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES:
          default:
            break;
        }
      }
    }
    if (showVertices && (representation != VTK_POINTS)) // Don't draw vertices on top of points.
    {
      const auto& pipelineKey = this->GraphicsPipelineKeys[GFX_PIPELINE_POINTS];
      const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(GFX_PIPELINE_POINTS);
      passEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      vtkScopedEncoderDebugGroup(passEncoder, pipelineLabel);
      passEncoder.Draw(
        /*vertexCount=*/6 * static_cast<std::uint32_t>(this->CurrentInput->GetNumberOfPoints()),
        /*instanceCount=*/numPoints);
      for (const auto& bindGroupType : { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS,
             vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS,
             vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS })
      {
        const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
        if (bgInfo.VertexCount == 0)
        {
          continue;
        }
        passEncoder.SetBindGroup(3, bgInfo.BindGroup);
        const auto topologyBGInfoName =
          vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
        vtkScopedEncoderDebugGroup(passEncoder, topologyBGInfoName);
        passEncoder.Draw(/*vertexCount=*/6 * bgInfo.VertexCount, /*instanceCount=*/numPoints);
      }
    }
  }

  //------------------------------------------------------------------------------
  void RecordDrawCommands(vtkRenderer* renderer, vtkActor* actor, int numPoints,
    const wgpu::RenderBundleEncoder& bundleEncoder)
  {
    bundleEncoder.SetBindGroup(2, this->MeshAttributeBindGroup);

    auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
    auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

    auto* displayProperty = actor->GetProperty();
    const int representation = displayProperty->GetRepresentation();
    const bool showVertices = displayProperty->GetVertexVisibility();

    for (const auto& pipelineMapping : PipelineBindGroupCombos[representation])
    {
      const auto& pipelineKey = this->GraphicsPipelineKeys[pipelineMapping.first];
      bundleEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(pipelineMapping.first);
      vtkScopedEncoderDebugGroup(bundleEncoder, pipelineLabel);
      for (const auto& bindGroupType : pipelineMapping.second)
      {
        const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
        if (bgInfo.VertexCount == 0)
        {
          continue;
        }
        bundleEncoder.SetBindGroup(3, bgInfo.BindGroup);
        const auto topologyBGInfoName =
          vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
        vtkScopedEncoderDebugGroup(bundleEncoder, topologyBGInfoName);
        switch (bindGroupType)
        {
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS:
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS:
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS:
            bundleEncoder.Draw(/*vertexCount=*/6 * bgInfo.VertexCount, /*instanceCount=*/numPoints);
            break;
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES:
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES:
            bundleEncoder.Draw(/*vertexCount=*/3 * bgInfo.VertexCount, /*instanceCount=*/numPoints);
            break;
          case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS:
            bundleEncoder.Draw(/*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/numPoints);
            break;
          case vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES:
          default:
            break;
        }
      }
    }
    if (showVertices && (representation != VTK_POINTS)) // Don't draw vertices on top of points.
    {
      const auto& pipelineKey = this->GraphicsPipelineKeys[GFX_PIPELINE_POINTS];
      const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(GFX_PIPELINE_POINTS);
      bundleEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      vtkScopedEncoderDebugGroup(bundleEncoder, pipelineLabel);
      bundleEncoder.Draw(/*vertexCount=*/4,
        /*instanceCount=*/static_cast<std::uint32_t>(this->CurrentInput->GetNumberOfPoints()));
      for (const auto& bindGroupType : { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS,
             vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS,
             vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS })
      {
        const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
        if (bgInfo.VertexCount == 0)
        {
          continue;
        }
        bundleEncoder.SetBindGroup(3, bgInfo.BindGroup);
        const auto topologyBGInfoName =
          vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
        vtkScopedEncoderDebugGroup(bundleEncoder, topologyBGInfoName);
        bundleEncoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount);
      }
    }
  }

  //------------------------------------------------------------------------------
  void Initialize(vtkPolyData* mesh, int numPoints, std::vector<vtkTypeFloat32>* colors,
    std::vector<vtkTypeFloat32>* transforms, std::vector<vtkTypeFloat32>* normalTransforms,
    vtkTypeUInt32 flatIndex, bool pickable, vtkMTimeType buildMTime)
  {
    this->CurrentInput = mesh;
    this->NumberOfGlyphPoints = numPoints;
    this->InstanceColors = colors;
    this->InstanceTransforms = transforms;
    this->InstanceNormalTransforms = normalTransforms;
    if (flatIndex != this->FlatIndex)
    {
      this->PickingAttributesModified = true;
      this->FlatIndex = flatIndex;
    }
    if (pickable != this->Pickable)
    {
      this->PickingAttributesModified = true;
      this->Pickable = pickable;
    }
    this->GlyphStructuresBuildTime = buildMTime;
  }

  //------------------------------------------------------------------------------
  void RenderPiece(vtkRenderer* renderer, vtkActor* actor) override
  {
    auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
    const auto device = wgpuRenderWindow->GetDevice();
    auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
    auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
    auto* displayProperty = actor->GetProperty();

    switch (wgpuRenderer->GetRenderStage())
    {
      case vtkWebGPURenderer::RenderStageEnum::UpdatingBuffers:
      { // update (i.e, create and write) GPU buffers if the data
        // is outdated.
        this->UpdateMeshGeometryBuffers(wgpuRenderWindow);
        auto* mesh = this->CurrentInput;
        vtkTypeUInt32* vertexCounts[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
        wgpu::Buffer* topologyBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
        wgpu::Buffer*
          edgeArrayBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];

        for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
        {
          auto& bgInfo = this->TopologyBindGroupInfos[i];
          vertexCounts[i] = &(bgInfo.VertexCount);
          topologyBuffers[i] = &(bgInfo.TopologyBuffer);
          edgeArrayBuffers[i] = &(bgInfo.EdgeArrayBuffer);
        }
        this->CellConverter->DispatchMeshToPrimitiveComputePipeline(wgpuConfiguration, mesh,
          displayProperty->GetRepresentation(), vertexCounts, topologyBuffers, edgeArrayBuffers);

        // Handle vertex visibility.
        if (displayProperty->GetVertexVisibility() &&
          // avoids dispatching the cell-to-vertex pipeline again.
          displayProperty->GetRepresentation() != VTK_POINTS)
        {
          // dispatch compute pipeline that extracts cell vertices.
          this->CellConverter->DispatchMeshToPrimitiveComputePipeline(wgpuConfiguration, mesh,
            /*representation=*/VTK_POINTS, vertexCounts, topologyBuffers, edgeArrayBuffers);
        }
        // Rebuild topology bind group if required (when VertexCount > 0)
        for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
        {
          const auto topologySourceType = vtkWebGPUCellToPrimitiveConverter::TopologySourceType(i);
          auto& bgInfo = this->TopologyBindGroupInfos[i];
          // setup bind group
          if (bgInfo.VertexCount > 0)
          {
            const std::string& label =
              vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(topologySourceType);
            bgInfo.BindGroup = this->CreateTopologyBindGroup(device, label, topologySourceType);
            this->RebuildGraphicsPipelines = true;
          }
        }
        // setup graphics pipeline
        if (this->GetNeedToRebuildGraphicsPipelines(actor, renderer))
        {
          // render bundle must reference new bind groups and/or pipelines
          wgpuRenderer->InvalidateBundle();
          this->SetupGraphicsPipelines(device, renderer, actor);
        }
        // invalidate render bundle when any of the cached properties of an actor have changed.
        if (this->CacheActorRendererProperties(actor, renderer))
        {
          wgpuRenderer->InvalidateBundle();
        }
        break;
      }
      case vtkWebGPURenderer::RenderStageEnum::RecordingCommands:
      {
        if (wgpuRenderer->GetUseRenderBundles())
        {
          this->RecordDrawCommands(
            renderer, actor, this->NumberOfGlyphPoints, wgpuRenderer->GetRenderBundleEncoder());
        }
        else
        {
          this->RecordDrawCommands(
            renderer, actor, this->NumberOfGlyphPoints, wgpuRenderer->GetRenderPassEncoder());
        }
        break;
      }
      default:
        break;
    }
    this->CurrentInput = nullptr;
  }
};

#define vtkInternalsDebugMacro(x) vtkDebugWithObjectMacro(this->Self, x)
#define vtkInternalsWarningMacro(x) vtkWarningWithObjectMacro(this->Self, x)
#define vtkInternalsErrorMacro(x) vtkErrorWithObjectMacro(this->Self, x)

class vtkWebGPUGlyph3DMapper::vtkInternals
{
  vtkWebGPUGlyph3DMapper* Self;

  class vtkColorMapper : public vtkMapper
  {
  public:
    vtkTypeMacro(vtkColorMapper, vtkMapper);
    static vtkColorMapper* New() { VTK_STANDARD_NEW_BODY(vtkColorMapper); }
    void Render(vtkRenderer*, vtkActor*) override {}
    vtkUnsignedCharArray* GetColors() { return this->Colors; }
  };

  struct GlyphParameters
  {
    // As many as the no. of points on the input dataset which are glyphed with source.
    std::vector<vtkTypeFloat32> Colors;
    std::vector<vtkTypeFloat32> Transforms;       // transposed
    std::vector<vtkTypeFloat32> NormalTransforms; // transposed
    vtkTimeStamp BuildTime;
    // May be polydata or composite dataset:
    vtkSmartPointer<vtkDataObject> SourceDataObject;
    // maps composite dataset flat index to polydatamapper. Key = -1 for polydata
    // DataObject.
    typedef std::map<int, vtkSmartPointer<vtkWebGPUGlyph3DMapperHelper>> MapperMap;
    MapperMap Mappers;
    int NumberOfPoints;
  };

  struct GlyphParametersCollection
  {
    // No. of entries is equal to number of source data objects.
    std::vector<std::unique_ptr<GlyphParameters>> Entries;
    vtkTimeStamp BuildTime;
  };

  struct RenderBlockState
  {
    std::stack<double> Opacity;
    std::stack<bool> Visibility;
    std::stack<bool> Pickability;
    std::stack<vtkColor3d> Color;
  };

  // No. of items is equal to number of input data sets. (composite datasets are expanded into
  // leaves)
  std::map<const vtkDataSet*, std::shared_ptr<GlyphParametersCollection>> GlyphInputDataSets;
  // Last time BlockAttributes was modified.
  vtkMTimeType BlockMTime;
  vtkNew<vtkColorMapper> ColorMapper;
  RenderBlockState BlockState;

public:
  vtkInternals(vtkWebGPUGlyph3DMapper* self)
    : Self(self)
  {
  }

  //------------------------------------------------------------------------------
  int GetNumberOfChildren(vtkDataObjectTree* tree)
  {
    int result = 0;
    if (tree)
    {
      auto it = vtk::TakeSmartPointer(tree->NewTreeIterator());
      it->SetTraverseSubTree(false);
      it->SetVisitOnlyLeaves(false);
      for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
      {
        ++result;
      }
    }
    return result;
  }

  //------------------------------------------------------------------------------
  vtkDataObject* GetChildDataObject(vtkDataObjectTree* tree, std::size_t child)
  {
    vtkDataObject* result = nullptr;
    if (tree)
    {
      auto it = vtk::TakeSmartPointer(tree->NewTreeIterator());
      it->SetTraverseSubTree(false);
      it->SetVisitOnlyLeaves(false);
      it->InitTraversal();
      for (std::size_t i = 0; i < child; ++i)
      {
        it->GoToNextItem();
      }
      result = it->GetCurrentDataObject();
    }
    return result;
  }

  //------------------------------------------------------------------------------
  void Render(vtkRenderer* renderer, vtkActor* actor, vtkDataObject* inputDataObject)
  {
    // Render the input dataset or every dataset within the input composite dataset.
    this->BlockMTime = this->Self->BlockAttributes ? this->Self->BlockAttributes->GetMTime() : 0;

    if (auto* inputDataSet = vtkDataSet::SafeDownCast(inputDataObject))
    {
      this->RenderDataSet(renderer, actor, inputDataSet, 0, true);
    }
    else if (auto* inputCompositeDataSet = vtkCompositeDataSet::SafeDownCast(inputDataObject))
    {
      vtkNew<vtkActor> blockAct;
      vtkNew<vtkProperty> blockProp;
      blockAct->ShallowCopy(actor);
      blockProp->DeepCopy(blockAct->GetProperty());
      blockAct->SetProperty(blockProp.GetPointer());
      double origColor[4];
      blockProp->GetColor(origColor);

      // Push base-values on the state stack.
      this->BlockState.Visibility.push(true);
      this->BlockState.Pickability.push(true);
      this->BlockState.Opacity.push(blockProp->GetOpacity());
      this->BlockState.Color.emplace(origColor);

      unsigned int flatIndex = 0;
      this->RenderChildren(renderer, blockAct, inputCompositeDataSet, flatIndex);

      // Pop base-values from the state stack.
      this->BlockState.Visibility.pop();
      this->BlockState.Pickability.pop();
      this->BlockState.Opacity.pop();
      this->BlockState.Color.pop();
    }
  }

  //------------------------------------------------------------------------------
  void RenderDataSet(vtkRenderer* renderer, vtkActor* actor, vtkDataSet* inputDataSet,
    unsigned int flatIndex, bool pickable)
  {
    const auto numPoints = inputDataSet->GetNumberOfPoints();
    if (numPoints < 1)
    {
      vtkInternalsDebugMacro(<< "Cannot glyph because there are no points in the input dataset!");
      return;
    }

    // make sure we have glyph parameters for this dataset.
    bool rebuild = false;
    std::shared_ptr<GlyphParametersCollection> glyphParametersCollection;
    auto glyphParametersFound = this->GlyphInputDataSets.find(inputDataSet);
    if (glyphParametersFound == this->GlyphInputDataSets.end())
    {
      glyphParametersCollection = std::make_shared<GlyphParametersCollection>();
      this->GlyphInputDataSets.insert(std::make_pair(inputDataSet, glyphParametersCollection));
      rebuild = true;
    }
    else
    {
      glyphParametersCollection = glyphParametersFound->second;
    }

    // make sure there are entries for each source dataobject.
    auto* sourceTableTree = this->Self->GetSourceTableTree();
    const int sttSize = this->GetNumberOfChildren(sourceTableTree);
    const int numSourceDataSets = this->Self->GetNumberOfInputConnections(1);
    const std::size_t numberOfSources =
      this->Self->UseSourceTableTree ? sttSize : numSourceDataSets;
    bool numberOfSourcesChanged = false;
    if (numberOfSources != glyphParametersCollection->Entries.size())
    {
      glyphParametersCollection->Entries.clear();
      glyphParametersCollection->Entries.reserve(numberOfSources);
      for (std::size_t i = 0; i < numberOfSources; ++i)
      {
        glyphParametersCollection->Entries.emplace_back(new GlyphParameters());
      }
      numberOfSourcesChanged = true;
    }

    // make sure sources are up to date.
    vtkSmartPointer<vtkDataObjectTreeIterator> sttIterator;
    // when a source table tree is present, iterate over all sources and update our cache.
    if (sourceTableTree)
    {
      sttIterator = vtk::TakeSmartPointer(sourceTableTree->NewTreeIterator());
      sttIterator->SetTraverseSubTree(false);
      sttIterator->SetVisitOnlyLeaves(false);
      sttIterator->InitTraversal();
    }
    for (std::size_t i = 0; i < glyphParametersCollection->Entries.size(); ++i)
    {
      // for each source data object
      auto* sourceDataObject = this->Self->UseSourceTableTree ? sttIterator->GetCurrentDataObject()
                                                              : this->Self->GetSource(i);
      auto& glyphParameters = glyphParametersCollection->Entries[i];
      if (glyphParameters->SourceDataObject &&
        !glyphParameters->SourceDataObject->IsA(sourceDataObject->GetClassName()))
      {
        glyphParameters->SourceDataObject = nullptr;
      }
      if (!glyphParameters->SourceDataObject)
      {
        glyphParameters->SourceDataObject = vtk::TakeSmartPointer(sourceDataObject->NewInstance());
      }
      if (numberOfSourcesChanged ||
        sourceDataObject->GetMTime() > glyphParameters->SourceDataObject->GetMTime() ||
        this->Self->GetMTime() > glyphParameters->BuildTime)
      {
        glyphParameters->SourceDataObject->ShallowCopy(sourceDataObject);
      }

      // Create the individual mappers which render the source data object.
      vtkSmartPointer<vtkCompositeDataIterator> sourceCompositeDataIterator;
      if (auto* sourceCompositeDataSet =
            vtkCompositeDataSet::SafeDownCast(glyphParameters->SourceDataObject))
      {
        sourceCompositeDataIterator = sourceCompositeDataSet->NewIterator();
        sourceCompositeDataIterator->InitTraversal();
      }

      while (true)
      {
        vtkSmartPointer<vtkWebGPUGlyph3DMapperHelper> mapper;
        int mapperIdx = sourceCompositeDataIterator
          ? static_cast<int>(sourceCompositeDataIterator->GetCurrentFlatIndex())
          : -1;
        auto mapperFound = glyphParameters->Mappers.find(mapperIdx);
        if (mapperFound == glyphParameters->Mappers.end())
        {
          mapper = vtk::TakeSmartPointer(vtkWebGPUGlyph3DMapperHelper::New());
          glyphParameters->Mappers.insert(std::make_pair(mapperIdx, mapper));
        }
        else
        {
          mapper = mapperFound->second;
        }
        this->CopyInformationToSubMapper(mapper);

        if (sourceCompositeDataIterator)
        {
          sourceCompositeDataIterator->GoToNextItem();
        }
        if (!sourceCompositeDataIterator || sourceCompositeDataIterator->IsDoneWithTraversal())
        {
          break;
        }
      } // end while(true)

      if (sttIterator)
      {
        sttIterator->GoToNextItem();
      }
    } // end for each source data object

    // get the mask array
    vtkBitArray* maskArray = nullptr;
    if (this->Self->Masking)
    {
      maskArray = vtkArrayDownCast<vtkBitArray>(this->Self->GetMaskArray(inputDataSet));
      if (maskArray == nullptr)
      {
        vtkInternalsDebugMacro(<< "masking is enabled but there is no mask array. Ignore masking.");
      }
      else
      {
        if (maskArray->GetNumberOfComponents() != 1)
        {
          vtkInternalsErrorMacro("expecting a mask array with one component, getting "
            << maskArray->GetNumberOfComponents() << " components.");
          return;
        }
      }
    }

    // rebuild all sources for this dataset
    if (rebuild)
    {
      this->RebuildStructures(glyphParametersCollection, numPoints, actor, inputDataSet, maskArray);
    }

    // for each source data object
    for (const auto& glyphParameters : glyphParametersCollection->Entries)
    {
      if (glyphParameters->NumberOfPoints <= 0)
      {
        continue;
      }

      vtkDataObject* sourceDataObject = glyphParameters->SourceDataObject;
      vtkPolyData* mesh = vtkPolyData::SafeDownCast(sourceDataObject);
      vtkCompositeDataSet* sourceCompositeDataSet =
        mesh ? nullptr : vtkCompositeDataSet::SafeDownCast(sourceDataObject);

      vtkSmartPointer<vtkCompositeDataIterator> sourceCompositeDataIterator;
      if (sourceCompositeDataSet)
      {
        sourceCompositeDataIterator = sourceCompositeDataSet->NewIterator();
        sourceCompositeDataIterator->InitTraversal();
      }

      // Either render the polydata, or loop through the composite dataset and
      // render each polydata leaf
      for (;;)
      {
        int mapperIdx = -1;
        if (sourceCompositeDataIterator)
        {
          mesh = vtkPolyData::SafeDownCast(sourceCompositeDataIterator->GetCurrentDataObject());
          mapperIdx = sourceCompositeDataIterator->GetCurrentFlatIndex();
          sourceCompositeDataIterator->GoToNextItem();
        }

        if (mesh && mesh->GetNumberOfPoints() > 0)
        {
          auto mapper = glyphParameters->Mappers[mapperIdx];
          mapper->StaticOn();
          mapper->Initialize(mesh, glyphParameters->NumberOfPoints, &glyphParameters->Colors,
            &glyphParameters->Transforms, &glyphParameters->NormalTransforms, flatIndex, pickable,
            glyphParameters->BuildTime);
          mapper->RenderPiece(renderer, actor);
        }

        if (!sourceCompositeDataIterator || sourceCompositeDataIterator->IsDoneWithTraversal())
        {
          break;
        }
      } // end composite glyph iteration
    }   // end entries
  }

  //------------------------------------------------------------------------------
  void CopyInformationToSubMapper(vtkWebGPUGlyph3DMapperHelper* mapper)
  {
    assert("pre: mapper_exists" && mapper != nullptr);
    mapper->SetStatic(this->Self->Static);
    mapper->ScalarVisibilityOff();
  }

  //------------------------------------------------------------------------------
  void SetupColorMapper() { this->ColorMapper->ShallowCopy(this->Self); }

  //------------------------------------------------------------------------------
  void RenderChildren(
    vtkRenderer* renderer, vtkActor* actor, vtkDataObject* dobj, unsigned int& flatIndex)
  {
    // Push overridden attributes onto the stack.
    // Keep track of attributes that were pushed so that they can be popped after they're applied to
    // the batch element.
    vtkCompositeDataDisplayAttributes* cda = this->Self->BlockAttributes;
    bool overrides_visibility = (cda && cda->HasBlockVisibility(dobj));
    if (overrides_visibility)
    {
      this->BlockState.Visibility.push(cda->GetBlockVisibility(dobj));
    }
    bool overrides_pickability = (cda && cda->HasBlockPickability(dobj));
    if (overrides_pickability)
    {
      this->BlockState.Pickability.push(cda->GetBlockPickability(dobj));
    }
    bool overrides_opacity = (cda && cda->HasBlockOpacity(dobj));
    if (overrides_opacity)
    {
      this->BlockState.Opacity.push(cda->GetBlockOpacity(dobj));
    }
    bool overrides_color = (cda && cda->HasBlockColor(dobj));
    if (overrides_color)
    {
      vtkColor3d color = cda->GetBlockColor(dobj);
      this->BlockState.Color.push(color);
    }
    // Advance flat-index. After this point, flatIndex no longer points to this
    // block.
    const auto originalFlatIndex = flatIndex;
    flatIndex++;

    if (auto dObjTree = vtkDataObjectTree::SafeDownCast(dobj))
    {
      using Opts = vtk::DataObjectTreeOptions;
      for (vtkDataObject* child : vtk::Range(dObjTree, Opts::None))
      {
        if (!child)
        {
          ++flatIndex;
        }
        else
        {
          this->RenderChildren(renderer, actor, child, flatIndex);
        }
      }
    }
    else
    {
      auto ds = vtkDataSet::SafeDownCast(dobj);
      // Skip invisible blocks and unpickable ones when performing selection:
      bool blockVis = this->BlockState.Visibility.top();
      bool blockPick = this->BlockState.Pickability.top();
      if (blockVis)
      {
        if (ds)
        {
          actor->GetProperty()->SetColor(this->BlockState.Color.top().GetData());
          actor->GetProperty()->SetOpacity(this->BlockState.Opacity.top());
          this->RenderDataSet(renderer, actor, ds, originalFlatIndex, blockPick);
        }
        else
        {
          vtkInternalsErrorMacro(<< "Expected a vtkDataObjectTree or vtkDataSet input. Got "
                                 << dobj->GetClassName());
        }
      }
    }
    if (overrides_color)
    {
      this->BlockState.Color.pop();
    }
    if (overrides_opacity)
    {
      this->BlockState.Opacity.pop();
    }
    if (overrides_pickability)
    {
      this->BlockState.Pickability.pop();
    }
    if (overrides_visibility)
    {
      this->BlockState.Visibility.pop();
    }
  }

  //------------------------------------------------------------------------------
  void RebuildStructures(std::shared_ptr<GlyphParametersCollection> glyphParametersCollection,
    vtkIdType numPoints, vtkActor* actor, vtkDataSet* dataset, vtkBitArray* maskArray)
  {
    auto* mapper = this->Self;
    auto* displayProperty = actor->GetProperty();
    double rangeSize = mapper->Range[1] - mapper->Range[0];
    if (rangeSize == 0.0)
    {
      rangeSize = 1.0;
    }
    std::array<vtkTypeFloat32, 4> color;
    if (auto* actorColor = displayProperty->GetColor())
    {
      color[0] = actorColor[0];
      color[1] = actorColor[1];
      color[2] = actorColor[2];
      color[3] = displayProperty->GetOpacity();
    }
    // Verify OrientationArray is consistent with the OrientationMode
    auto* orientationArray = mapper->GetOrientationArray(dataset);
    if (orientationArray != nullptr)
    {
      const int numComponents = orientationArray->GetNumberOfComponents();
      if ((mapper->OrientationMode == ROTATION || mapper->OrientationMode == DIRECTION) &&
        numComponents != 3)
      {
        vtkInternalsErrorMacro("Expected an orientation array with 3 components, got "
          << numComponents << " components");
        return;
      }
      else if (mapper->OrientationMode == QUATERNION && numComponents != 4)
      {
        vtkInternalsErrorMacro("Expected an orientation array with 4 components, got "
          << numComponents << " components");
        return;
      }
    }

    auto* indexArray = mapper->GetSourceIndexArray(dataset);
    auto* scaleArray = mapper->GetScaleArray(dataset);

    this->ColorMapper->SetInputDataObject(dataset);
    this->ColorMapper->MapScalars(displayProperty->GetOpacity());
    auto* colors = this->ColorMapper->GetColors();

    // Traverse all points on input dataset, and transform points on source.
    const auto& numEntries = glyphParametersCollection->Entries.size();
    // how many points from the input dataset are glyphed with Source dataset.
    std::vector<int> numberOfPointsGlyphedPerSource(numEntries, 0);
    if (numEntries > 1 && indexArray)
    {
      // loop over every point
      int index = 0;
      for (vtkIdType pointId = 0; pointId < numPoints; pointId++)
      {
        if (maskArray && maskArray->GetValue(pointId) == 0)
        {
          continue;
        }

        // Compute index into table of glyphs
        double value =
          vtkMath::Norm(indexArray->GetTuple(pointId), indexArray->GetNumberOfComponents());
        index = static_cast<int>(value);
        index = vtkMath::ClampValue(index, 0, static_cast<int>(numEntries) - 1);
        numberOfPointsGlyphedPerSource[index]++;
      }
    }
    else
    {
      numberOfPointsGlyphedPerSource[0] = numPoints;
    }

    // Allocate data structures for each entry.
    for (std::size_t i = 0; i < glyphParametersCollection->Entries.size(); ++i)
    {
      auto& glyphParameters = glyphParametersCollection->Entries[i];
      glyphParameters->Colors.resize(numberOfPointsGlyphedPerSource[i] * 4);
      glyphParameters->Transforms.resize(numberOfPointsGlyphedPerSource[i] * 16);
      glyphParameters->NormalTransforms.resize(numberOfPointsGlyphedPerSource[i] * 9);
      glyphParameters->NumberOfPoints = 0;
      glyphParameters->BuildTime.Modified();
    }

    // loop over every point and fill structures
    int index = 0;
    auto* sourceTableTree = mapper->GetSourceTableTree();

    // cache sources to improve performances
    std::vector<vtkDataObject*> sourceCache(numEntries);
    for (std::size_t i = 0; i < numEntries; i++)
    {
      sourceCache[i] = mapper->UseSourceTableTree ? this->GetChildDataObject(sourceTableTree, i)
                                                  : mapper->GetSource(i);
    }

    double transform[16];
    double normalTransform[9];

    // for each input point
    for (vtkIdType pointId = 0; pointId < numPoints; ++pointId)
    {
      if (!(pointId % 10000))
      {
        mapper->UpdateProgress(static_cast<double>(pointId) / static_cast<double>(numPoints));
        if (mapper->GetAbortExecute())
        {
          break;
        }
      }

      // Skip glyphing masked point.
      if (maskArray && maskArray->GetValue(pointId) == 0)
      {
        continue;
      }

      // Compute index into table of glyphs
      if (indexArray)
      {
        // Compute index into table of glyphs
        double value =
          vtkMath::Norm(indexArray->GetTuple(pointId), indexArray->GetNumberOfComponents());
        index = static_cast<int>(value);
        index = vtkMath::ClampValue(index, 0, static_cast<int>(numEntries) - 1);
      }

      // if source exists at `index`.
      auto* source = (index < static_cast<int>(sourceCache.size()) ? sourceCache[index] : nullptr);
      if (source)
      {
        auto& glyphParameters = glyphParametersCollection->Entries[index];

        std::copy(color.begin(), color.end(),
          &glyphParameters->Colors[glyphParameters->NumberOfPoints * 4]);

        double scaleX = 1.0, scaleY = 1.0, scaleZ = 1.0;
        // Get the scalar and vector data
        if (scaleArray)
        {
          double* tuple = scaleArray->GetTuple(pointId);
          switch (mapper->ScaleMode)
          {
            case SCALE_BY_MAGNITUDE:
              scaleX = scaleY = scaleZ = vtkMath::Norm(tuple, scaleArray->GetNumberOfComponents());
              break;
            case SCALE_BY_COMPONENTS:
              if (scaleArray->GetNumberOfComponents() != 3)
              {
                vtkInternalsErrorMacro("Cannot scale by components since the array \'"
                  << scaleArray->GetName() << "\' does not have 3 components.");
              }
              else
              {
                scaleX = tuple[0];
                scaleY = tuple[1];
                scaleZ = tuple[2];
              }
              break;
            case NO_DATA_SCALING:
            default:
              break;
          }

          // Clamp data scale if enabled
          if (mapper->Clamping && mapper->ScaleMode != NO_DATA_SCALING)
          {
            scaleX =
              (scaleX < mapper->Range[0] ? mapper->Range[0]
                                         : (scaleX > mapper->Range[1] ? mapper->Range[1] : scaleX));
            scaleX = (scaleX - mapper->Range[0]) / rangeSize;
            scaleY =
              (scaleY < mapper->Range[0] ? mapper->Range[0]
                                         : (scaleY > mapper->Range[1] ? mapper->Range[1] : scaleY));
            scaleY = (scaleY - mapper->Range[0]) / rangeSize;
            scaleZ =
              (scaleZ < mapper->Range[0] ? mapper->Range[0]
                                         : (scaleZ > mapper->Range[1] ? mapper->Range[1] : scaleZ));
            scaleZ = (scaleZ - mapper->Range[0]) / rangeSize;
          }
        } // if scaleArray

        scaleX *= mapper->ScaleFactor;
        scaleY *= mapper->ScaleFactor;
        scaleZ *= mapper->ScaleFactor;

        // Now begin copying/transforming glyph
        vtkMatrix4x4::Identity(transform);
        vtkMatrix3x3::Identity(normalTransform);

        // translate Source to Input point
        double x[3];
        dataset->GetPoint(pointId, x);
        transform[3] = x[0];
        transform[7] = x[1];
        transform[11] = x[2];

        if (orientationArray)
        {
          double orientation[4];
          orientationArray->GetTuple(pointId, orientation);

          double rotMatrix[3][3];
          vtkQuaterniond quaternion;

          switch (mapper->OrientationMode)
          {
            case ROTATION:
            {
              double angle = vtkMath::RadiansFromDegrees(orientation[2]);
              vtkQuaterniond qz(cos(0.5 * angle), 0.0, 0.0, sin(0.5 * angle));

              angle = vtkMath::RadiansFromDegrees(orientation[0]);
              vtkQuaterniond qx(cos(0.5 * angle), sin(0.5 * angle), 0.0, 0.0);

              angle = vtkMath::RadiansFromDegrees(orientation[1]);
              vtkQuaterniond qy(cos(0.5 * angle), 0.0, sin(0.5 * angle), 0.0);

              quaternion = qz * qx * qy;
              break;
            }
            case QUATERNION:
              quaternion.Set(orientation);
              break;
            case DIRECTION:
            default:
            {
              if (orientation[1] == 0.0 && orientation[2] == 0.0)
              {
                if (orientation[0] < 0) // just flip x if we need to
                {
                  quaternion.Set(0.0, 0.0, 1.0, 0.0);
                }
              }
              else
              {
                double vMag = vtkMath::Norm(orientation);
                double vNew[3];
                vNew[0] = (orientation[0] + vMag) / 2.0;
                vNew[1] = orientation[1] / 2.0;
                vNew[2] = orientation[2] / 2.0;

                double f = 1.0 / sqrt(vNew[0] * vNew[0] + vNew[1] * vNew[1] + vNew[2] * vNew[2]);
                vNew[0] *= f;
                vNew[1] *= f;
                vNew[2] *= f;

                quaternion.Set(0.0, vNew[0], vNew[1], vNew[2]);
              }
              break;
            }
          }

          quaternion.ToMatrix3x3(rotMatrix);

          for (int i = 0; i < 3; i++)
          {
            for (int j = 0; j < 3; j++)
            {
              transform[4 * i + j] = rotMatrix[i][j];
              normalTransform[3 * i + j] = rotMatrix[j][i]; // transpose
            }
          }
        } // if orientationArray

        if (colors)
        {
          std::array<unsigned char, 4> ubColor;
          colors->GetTypedTuple(pointId, ubColor.data());
          std::transform(ubColor.begin(), ubColor.end(),
            &(glyphParameters->Colors[glyphParameters->NumberOfPoints * 4]),
            [](unsigned char in) { return in / 255.0; });
        }

        // scale data if appropriate
        if (mapper->Scaling)
        {
          if (scaleX == 0.0)
          {
            scaleX = 1.0e-10;
          }
          if (scaleY == 0.0)
          {
            scaleY = 1.0e-10;
          }
          if (scaleZ == 0.0)
          {
            scaleZ = 1.0e-10;
          }

          for (int i = 0; i < 3; i++)
          {
            // inverse of normal matrix is directly computed with inverse scale
            transform[4 * i] *= scaleX;
            normalTransform[i] /= scaleX;
            transform[4 * i + 1] *= scaleY;
            normalTransform[i + 3] /= scaleY;
            transform[4 * i + 2] *= scaleZ;
            normalTransform[i + 6] /= scaleZ;
          }
        }

        // Transpose matrices and copy into vtkTypeFloat32 arrays.
        vtkTypeFloat32* matrices =
          &glyphParameters->Transforms[glyphParameters->NumberOfPoints * 16];
        vtkTypeFloat32* normalTransforms =
          &glyphParameters->NormalTransforms[glyphParameters->NumberOfPoints * 9];
        for (int i = 0; i < 4; i++)
        {
          for (int j = 0; j < 4; j++)
          {
            matrices[i * 4 + j] = transform[j * 4 + i];
          }
        }

        for (int i = 0; i < 3; i++)
        {
          for (int j = 0; j < 3; j++)
          {
            normalTransforms[i * 3 + j] = normalTransform[i * 3 + j];
          }
        }
        glyphParameters->NumberOfPoints++;
      } // if source
    }   // for each input point

    glyphParametersCollection->BuildTime.Modified();
  }

  //------------------------------------------------------------------------------
  void ReleaseGraphicsResources(vtkWindow* window)
  {
    for (auto& glyphParametersCollection : this->GlyphInputDataSets)
    {
      for (auto& glyphParameters : glyphParametersCollection.second->Entries)
      {
        for (auto& mapper : glyphParameters->Mappers)
        {
          mapper.second->ReleaseGraphicsResources(window);
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUGlyph3DMapper);

//------------------------------------------------------------------------------
vtkWebGPUGlyph3DMapper::vtkWebGPUGlyph3DMapper()
{
  this->Internals.reset(new vtkInternals(this));
}

//------------------------------------------------------------------------------
vtkWebGPUGlyph3DMapper::~vtkWebGPUGlyph3DMapper() = default;

//------------------------------------------------------------------------------
void vtkWebGPUGlyph3DMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebGPUGlyph3DMapper::ReleaseGraphicsResources(vtkWindow* window)
{
  this->Internals->ReleaseGraphicsResources(window);
}

//------------------------------------------------------------------------------
void vtkWebGPUGlyph3DMapper::Render(vtkRenderer* render, vtkActor* actor)
{
  auto& internals = (*this->Internals);

  auto* inputDataObject = this->GetInputDataObject(0, 0);
  internals.SetupColorMapper();

  // Create a default source, if no source is specified.
  if (!this->UseSourceTableTree && this->GetSource(0) == nullptr)
  {
    vtkNew<vtkPolyData> defaultSource;
    vtkNew<vtkPoints> defaultPoints;
    defaultPoints->InsertNextPoint(0, 0, 0);
    defaultPoints->InsertNextPoint(1, 0, 0);
    vtkNew<vtkCellArray> lines;
    lines->InsertNextCell({ 0, 1 });
    defaultSource->SetLines(lines);
    this->SetSourceData(defaultSource);
  }

  // Check that configuration of sources on the seconds port are sane.
  auto* sourceTableTree = this->GetSourceTableTree();
  const int numSourceDataSets = this->GetNumberOfInputConnections(1);
  if (this->UseSourceTableTree)
  {
    if (numSourceDataSets > 1)
    {
      vtkErrorMacro("UseSourceTableTree is true, but multiple source datasets are set.");
      return;
    }
    if (!sourceTableTree)
    {
      vtkErrorMacro(
        "UseSourceTableTree is true, but the source dataset is not a vtkDataObjectTree.");
      return;
    }
    auto sttIterator = vtk::TakeSmartPointer(sourceTableTree->NewTreeIterator());
    sttIterator->SetTraverseSubTree(false);
    sttIterator->SetVisitOnlyLeaves(false);
    for (sttIterator->InitTraversal(); !sttIterator->IsDoneWithTraversal();
         sttIterator->GoToNextItem())
    {
      auto* node = sttIterator->GetCurrentDataObject();
      if (!(node->IsA("vtkPolyData") || node->IsA("vtkCompositeDataSet")))
      {
        vtkErrorMacro("The source table tree must only contain vtkPolyData or vtkCompositeDataSet "
                      "children, found a "
          << node->GetClassName() << " instead.");
        return;
      }
    }
  }
  else
  {
    for (int i = 0; i < numSourceDataSets; ++i)
    {
      if (!this->GetSource(i))
      {
        vtkErrorMacro("Source input at index " << i
                                               << " not set, or not "
                                                  "vtkPolyData.");
        return;
      }
    }
  }
  internals.Render(render, actor, inputDataObject);
  this->UpdateProgress(1.0);
}

VTK_ABI_NAMESPACE_END
