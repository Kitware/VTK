// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUPolyDataMapper.h"
#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkScalarsToColors.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkTimeStamp.h"
#include "vtkWebGPUActor.h"
#include "vtkWebGPUCamera.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUComputePipeline.h"
#include "vtkWebGPUComputeRenderBuffer.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include "LineShaderOpaque.h"
#include "LineShaderTranslucent.h"
#include "PointShader.h"
#include "ShowVertices.h"
#include "SurfaceMeshShader.h"
#include "VTKCellToGraphicsPrimitive.h"

#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUBufferInternals.h"
#include "Private/vtkWebGPUPipelineLayoutInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"

#include <array>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
const std::array<std::string, vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_NB_TYPES>
  TopologyConversionShaderEntrypoints = { "poly_vertex_to_vertex", "poly_line_to_line",
    "cell_to_points", "polygon_to_triangle", "cell_to_points", "polygon_edges_to_lines" };

const std::array<const char**, vtkWebGPUPolyDataMapper::GFX_PIPELINE_NB_TYPES>
  GraphicsPipelineShaderSources = { &PointShader, &LineShaderOpaque, &LineShaderTranslucent,
    &SurfaceMeshShader, &ShowVertices };

const std::array<wgpu::PrimitiveTopology, vtkWebGPUPolyDataMapper::GFX_PIPELINE_NB_TYPES>
  GraphicsPipelinePrimitiveTypes = { wgpu::PrimitiveTopology::TriangleStrip,
    wgpu::PrimitiveTopology::TriangleList, wgpu::PrimitiveTopology::TriangleStrip,
    wgpu::PrimitiveTopology::TriangleList, wgpu::PrimitiveTopology::TriangleStrip };

std::map<vtkWebGPUPolyDataMapper::GraphicsPipelineType,
  std::vector<vtkWebGPUPolyDataMapper::TopologySourceType>>
  PipelineBindGroupCombos[VTK_SURFACE + 1] = { // VTK_POINTS
    { { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS,
      { vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_VERTS,
        vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_LINE_POINTS,
        vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_POLYGON_POINTS } } },
    // VTK_WIREFRAME
    { { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN,
        { vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_POLYGON_EDGES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_MITER_JOIN,
        { vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_POLYGON_EDGES } } },
    // VTK_SURFACE
    {
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS,
        { vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_VERTS } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN,
        { vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_MITER_JOIN,
        { vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_TRIANGLES,
        { vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_POLYGONS } },
    }
  };

// Function to factor the number into three multiples
int Factorize(vtkTypeUInt32 n, std::array<vtkTypeUInt32, 3>& result)
{
  const vtkTypeUInt32 MAX_WORKGROUPS_PER_DIM = 65535;
  if (n > MAX_WORKGROUPS_PER_DIM)
  {
    result[0] = MAX_WORKGROUPS_PER_DIM;
  }
  else
  {
    result[0] = n;
    result[1] = 1;
    result[2] = 1;
    return 0;
  }
  if (n > (result[0] * MAX_WORKGROUPS_PER_DIM))
  {
    result[1] = MAX_WORKGROUPS_PER_DIM;
  }
  else
  {
    result[1] = std::ceil(n / static_cast<double>(result[0]));
    result[2] = 1;
    return 0;
  }
  if (n > (result[0] * result[1] * MAX_WORKGROUPS_PER_DIM))
  {
    return -1;
  }
  else
  {
    result[2] = std::ceil(n / static_cast<double>(result[0] * result[1]));
    return 0;
  }
}

class MakeEncoderDebugGroup
{
  const wgpu::RenderPassEncoder* PassEncoder = nullptr;
  const wgpu::RenderBundleEncoder* BundleEncoder = nullptr;

public:
  MakeEncoderDebugGroup(const wgpu::RenderPassEncoder& passEncoder, const char* groupLabel)
    : PassEncoder(&passEncoder)
  {
#ifndef NDEBUG
    this->PassEncoder->PushDebugGroup(groupLabel);
#else
    (void)this->PassEncoder;
    (void)groupLabel;
#endif
  }
  MakeEncoderDebugGroup(const wgpu::RenderBundleEncoder& bundleEncoder, const char* groupLabel)
    : BundleEncoder(&bundleEncoder)
  {
#ifndef NDEBUG
    this->BundleEncoder->PushDebugGroup(groupLabel);
#else
    (void)this->BundleEncoder;
    (void)groupLabel;
#endif
  }
#ifndef NDEBUG
  ~MakeEncoderDebugGroup()
  {
    if (this->PassEncoder)
    {
      this->PassEncoder->PopDebugGroup();
    }
    if (this->BundleEncoder)
    {
      this->BundleEncoder->PopDebugGroup();
    }
  }
#else
  ~MakeEncoderDebugGroup() = default;
#endif
};

template <typename DestT>
struct WriteTypedArray
{
  std::size_t ByteOffset = 0;
  const wgpu::Buffer& DstBuffer;
  const wgpu::Device& Device;
  float Denominator = 1.0;

  template <typename SrcArrayT>
  void operator()(SrcArrayT* array)
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
    this->Device.GetQueue().WriteBuffer(
      this->DstBuffer, this->ByteOffset, data->GetPointer(0), nbytes);
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
   * When array 'B' has the same size as previous upload, but it's values have changed and values of
   * array 'A' have not changed, the mapper is designed to partially update only the portion of the
   * buffer which has values corresponding to array 'B'. To facilitate such partial updates, use
   * `Advance` for array 'A' to seek forward all the way to the end of the array 'A' in the webgpu
   * buffer before writing values of 'B'
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
};
}

#define vtkScopedEncoderDebugGroupConcatImpl(s1, s2) s1##s2
#define vtkScopedEncoderDebugGroupConcat(s1, s2) vtkScopedEncoderDebugGroupConcatImpl(s1, s2)
#define vtkScopedEncoderDebugGroupAnonymousVariable(x) vtkScopedEncoderDebugGroupConcat(x, __LINE__)
// Use this macro to annotate a group of commands in an renderpass/bundle encoder.
#define vtkScopedEncoderDebugGroup(encoder, name)                                                  \
  auto vtkScopedEncoderDebugGroupAnonymousVariable(encoderDebugGroup) =                            \
    ::MakeEncoderDebugGroup(encoder, name);

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUPolyDataMapper);

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper::vtkWebGPUPolyDataMapper() = default;

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper::~vtkWebGPUPolyDataMapper() = default;

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HasPointColors: " << (this->HasPointAttributes[POINT_COLORS] ? "On\n" : "Off\n");
  os << indent
     << "HasPointNormals: " << (this->HasPointAttributes[POINT_NORMALS] ? "On\n" : "Off\n");
  os << indent
     << "HasPointTangents: " << (this->HasPointAttributes[POINT_TANGENTS] ? "On\n" : "Off\n");
  os << indent << "HasPointUVs: " << (this->HasPointAttributes[POINT_UVS] ? "On\n" : "Off\n");
  os << indent << "HasCellColors: " << (this->HasCellAttributes[CELL_COLORS] ? "On\n" : "Off\n");
  os << indent << "HasCellNormals: " << (this->HasCellAttributes[CELL_NORMALS] ? "On\n" : "Off\n");
  os << indent << "LastScalarVisibility: " << (this->LastScalarVisibility ? "On\n" : "Off\n");
  os << indent << "LastScalarMode: " << this->LastScalarMode << '\n';
}

//------------------------------------------------------------------------------
vtkPolyDataMapper::MapperHashType vtkWebGPUPolyDataMapper::GenerateHash(vtkPolyData* polydata)
{
  return reinterpret_cast<std::uintptr_t>(polydata);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RenderPiece(vtkRenderer* renderer, vtkActor* actor)
{
  auto wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  // Note for emscripten: the indirection to js getTimeNow is a bit costly. it can quickly add up
  // for really large number of actors. However, vtkRenderWindow caps it to 5 times per second. the
  // cost of this check abort is about 0.2ms per call in emscripten. So, 1 millisecond is the
  // guaranteed cost per number of frames rendered in a second.
  // if (wgpuRenderWindow->CheckAbortStatus())
  // {
  //   return;
  // }

  const auto device = wgpuRenderWindow->GetDevice();
  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
  auto* wgpuActor = reinterpret_cast<vtkWebGPUActor*>(actor);
  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);

  switch (wgpuRenderer->GetRenderStage())
  {
    case vtkWebGPURenderer::RenderStageEnum::UpdatingBuffers:
    { // update (i.e, create and write) GPU buffers if the data is outdated.
      this->UpdateMeshGeometryBuffers(wgpuRenderWindow);
      // dispatch compute pipeline that converts polyvertex to vertices.
      auto* mesh = this->CurrentInput;
      const int representation = actor->GetProperty()->GetRepresentation();
      this->DispatchCellToPrimitiveComputePipeline(
        wgpuConfiguration, mesh->GetVerts(), representation, VTK_POLY_VERTEX, 0);
      // dispatch compute pipeline that converts polyline to lines.
      const vtkIdType numVerts = mesh->GetNumberOfVerts();
      this->DispatchCellToPrimitiveComputePipeline(
        wgpuConfiguration, mesh->GetLines(), representation, VTK_POLY_LINE, numVerts);
      // dispatch compute pipeline that converts polygon to triangles.
      const vtkIdType numLines = mesh->GetNumberOfLines();
      this->DispatchCellToPrimitiveComputePipeline(
        wgpuConfiguration, mesh->GetPolys(), representation, VTK_POLYGON, numLines + numVerts);
      // setup graphics pipeline
      if (this->GetNeedToRebuildGraphicsPipelines(actor))
      {
        // render bundle must reference new bind groups and/or pipelines
        wgpuActor->SetBundleInvalidated(true);
        this->SetupGraphicsPipelines(device, renderer, actor);
      }
      // invalidate render bundle when any of the cached properties of an actor have changed.
      if (this->CacheActorProperties(actor))
      {
        wgpuActor->SetBundleInvalidated(true);
      }
      break;
    }
    case vtkWebGPURenderer::RenderStageEnum::RecordingCommands:
      if (wgpuRenderer->GetUseRenderBundles())
      {
        this->RecordDrawCommands(renderer, actor, wgpuRenderer->GetRenderBundleEncoder());
      }
      else
      {
        this->RecordDrawCommands(renderer, actor, wgpuRenderer->GetRenderPassEncoder());
      }
      break;
    default:
      break;
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUPolyDataMapper::CacheActorProperties(vtkActor* actor)
{
  auto it = this->CachedActorProperties.find(actor);
  auto* displayProperty = actor->GetProperty();
  bool hasTranslucentPolygonalGeometry = false;
  if (actor)
  {
    hasTranslucentPolygonalGeometry = actor->HasTranslucentPolygonalGeometry();
  }
  if (it == this->CachedActorProperties.end())
  {
    ActorState state = {};
    state.LastActorBackfaceCulling = displayProperty->GetBackfaceCulling();
    state.LastActorFrontfaceCulling = displayProperty->GetFrontfaceCulling();
    state.LastRepresentation = displayProperty->GetRepresentation();
    state.LastVertexVisibility = displayProperty->GetVertexVisibility();
    state.LastHasRenderingTranslucentGeometry = hasTranslucentPolygonalGeometry;
    this->CachedActorProperties[actor] = state;
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

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RecordDrawCommands(
  vtkRenderer* renderer, vtkActor* actor, const wgpu::RenderPassEncoder& passEncoder)
{
  passEncoder.SetBindGroup(2, this->MeshAttributeBindGroup);

  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

  auto* displayProperty = actor->GetProperty();
  const int representation = displayProperty->GetRepresentation();
  const bool showVertices = displayProperty->GetVertexVisibility();
  const bool hasTranslucentPolygonalGeometry = actor->HasTranslucentPolygonalGeometry();

  for (const auto& pipelineMapping : PipelineBindGroupCombos[representation])
  {
    const auto& pipelineType = pipelineMapping.first;
    // apply miter join only for translucent pass.
    if (hasTranslucentPolygonalGeometry)
    {
      if (pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN)
      {
        continue;
      }
    }
    else if (pipelineType == GFX_PIPELINE_LINES_MITER_JOIN)
    {
      continue;
    }
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
      const auto topologyBGInfoName = this->GetTopologySourceTypeAsString(bindGroupType);
      vtkScopedEncoderDebugGroup(passEncoder, topologyBGInfoName);
      switch (bindGroupType)
      {
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_VERTS:
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_LINE_POINTS:
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_POLYGON_POINTS:
          passEncoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount);
          break;
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_LINES:
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_POLYGON_EDGES:
          if (pipelineType == GFX_PIPELINE_LINES_MITER_JOIN)
          {
            passEncoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount / 2);
          }
          else if (pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN)
          {
            passEncoder.Draw(/*vertexCount=*/36, /*instanceCount=*/bgInfo.VertexCount / 2);
          }
          break;
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_POLYGONS:
          passEncoder.Draw(/*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/1);
          break;
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_NB_TYPES:
        default:
          break;
      }
    }
  }
  if (showVertices && representation != VTK_POINTS) // Don't draw vertices on top of points.
  {
    const auto& pipelineKey = this->GraphicsPipelineKeys[GFX_PIPELINE_VERTEX_VISIBILITY];
    if (!pipelineKey.empty())
    {
      const auto& pipelineLabel =
        this->GetGraphicsPipelineTypeAsString(GFX_PIPELINE_VERTEX_VISIBILITY);
      passEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      vtkScopedEncoderDebugGroup(passEncoder, pipelineLabel);
      passEncoder.Draw(/*vertexCount=*/4,
        /*instanceCount=*/static_cast<std::uint32_t>(this->CurrentInput->GetNumberOfPoints()));
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RecordDrawCommands(
  vtkRenderer* renderer, vtkActor* actor, const wgpu::RenderBundleEncoder& bundleEncoder)
{
  bundleEncoder.SetBindGroup(2, this->MeshAttributeBindGroup);

  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

  auto* displayProperty = actor->GetProperty();
  const int representation = displayProperty->GetRepresentation();
  const bool showVertices = displayProperty->GetVertexVisibility();
  const bool hasTranslucentPolygonalGeometry = actor->HasTranslucentPolygonalGeometry();

  for (const auto& pipelineMapping : PipelineBindGroupCombos[representation])
  {
    const auto& pipelineType = pipelineMapping.first;
    // apply miter join only for translucent pass.
    if (hasTranslucentPolygonalGeometry)
    {
      if (pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN)
      {
        continue;
      }
    }
    else if (pipelineType == GFX_PIPELINE_LINES_MITER_JOIN)
    {
      continue;
    }
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
      const auto topologyBGInfoName = this->GetTopologySourceTypeAsString(bindGroupType);
      vtkScopedEncoderDebugGroup(bundleEncoder, topologyBGInfoName);
      switch (bindGroupType)
      {
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_VERTS:
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_LINE_POINTS:
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_POLYGON_POINTS:
          bundleEncoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount);
          break;
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_LINES:
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_POLYGON_EDGES:
          if (pipelineType == GFX_PIPELINE_LINES_MITER_JOIN)
          {
            bundleEncoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount / 2);
          }
          else if (pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN)
          {
            bundleEncoder.Draw(/*vertexCount=*/36, /*instanceCount=*/bgInfo.VertexCount / 2);
          }
          break;
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_POLYGONS:
          bundleEncoder.Draw(/*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/1);
          break;
        case vtkWebGPUPolyDataMapper::TOPOLOGY_SOURCE_NB_TYPES:
        default:
          break;
      }
    }
  }
  if (showVertices && representation != VTK_POINTS) // Don't draw vertices on top of points.
  {
    const auto& pipelineKey = this->GraphicsPipelineKeys[GFX_PIPELINE_VERTEX_VISIBILITY];
    if (!pipelineKey.empty())
    {
      const auto& pipelineLabel =
        this->GetGraphicsPipelineTypeAsString(GFX_PIPELINE_VERTEX_VISIBILITY);
      bundleEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      vtkScopedEncoderDebugGroup(bundleEncoder, pipelineLabel);
      bundleEncoder.Draw(/*vertexCount=*/4,
        /*instanceCount=*/static_cast<std::uint32_t>(this->CurrentInput->GetNumberOfPoints()));
    }
  }
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUPolyDataMapper::CreateMeshAttributeBindGroupLayout(
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
      // clang-format on
    },
    label);
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUPolyDataMapper::CreateTopologyBindGroupLayout(
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

//------------------------------------------------------------------------------
wgpu::RenderPipeline vtkWebGPUPolyDataMapper::CreateRenderPipeline(const wgpu::Device& device,
  wgpu::RenderPipelineDescriptor* pipelineDescriptor, const wgpu::ShaderModule& shaderModule,
  wgpu::PrimitiveTopology primitiveTopology)
{
  auto* pipelineDescriptorVtk =
    static_cast<vtkWebGPURenderPipelineDescriptorInternals*>(pipelineDescriptor);
  // set shader module
  pipelineDescriptorVtk->vertex.module = shaderModule;
  pipelineDescriptorVtk->cFragment.module = shaderModule;
  // set primitive type
  pipelineDescriptorVtk->primitive.topology = primitiveTopology;
  // create pipeline
  return device.CreateRenderPipeline(pipelineDescriptorVtk);
}

//------------------------------------------------------------------------------
wgpu::BindGroup vtkWebGPUPolyDataMapper::CreateMeshAttributeBindGroup(
  const wgpu::Device& device, const std::string& label)
{
  auto layout = this->CreateMeshAttributeBindGroupLayout(device, label + "_LAYOUT");
  return vtkWebGPUBindGroupInternals::MakeBindGroup(device, layout,
    {
      // clang-format off
      { 0, this->AttributeDescriptorBuffer, 0 },
      { 1, this->MeshSSBO.Point.Buffer, 0 },
      { 2, this->MeshSSBO.Cell.Buffer, 0 }
      // clang-format on
    },
    label);
}

//------------------------------------------------------------------------------
wgpu::BindGroup vtkWebGPUPolyDataMapper::CreateTopologyBindGroup(
  const wgpu::Device& device, const std::string& label, TopologySourceType topologySourceType)
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

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetPointAttributeByteSize(
  vtkWebGPUPolyDataMapper::PointDataAttributes attribute)
{
  switch (attribute)
  {
    case PointDataAttributes::POINT_POSITIONS:
      return this->CurrentInput->GetNumberOfPoints() * 3 * sizeof(vtkTypeFloat32);

    case PointDataAttributes::POINT_COLORS:
      return this->HasPointAttributes[POINT_COLORS]
        ? this->Colors->GetDataSize() * sizeof(vtkTypeFloat32)
        : 0;

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

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetCellAttributeByteSize(
  vtkWebGPUPolyDataMapper::CellDataAttributes attribute)
{
  switch (attribute)
  {
    case CellDataAttributes::CELL_COLORS:
      if (this->HasCellAttributes[attribute])
      {
        // are we using a single color value replicated over all cells?
        if (this->FieldDataTupleId > -1 && this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA)
        {
          return this->CurrentInput->GetNumberOfCells() * this->Colors->GetNumberOfComponents() *
            sizeof(vtkTypeFloat32);
        }
        else
        {
          return this->Colors->GetDataSize() * sizeof(vtkTypeFloat32);
        }
      }

      break;

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

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetPointAttributeElementSize(
  vtkWebGPUPolyDataMapper::PointDataAttributes attribute)
{
  switch (attribute)
  {
    case PointDataAttributes::POINT_POSITIONS:
      return 3 * sizeof(vtkTypeFloat32);

    case PointDataAttributes::POINT_COLORS:
      if (this->HasPointAttributes[POINT_COLORS])
      {
        return vtkDataArray::SafeDownCast(this->Colors)->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case PointDataAttributes::POINT_NORMALS:
      if (this->HasPointAttributes[attribute])
      {
        return this->CurrentInput->GetPointData()->GetNormals()->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case PointDataAttributes::POINT_TANGENTS:
      if (this->HasPointAttributes[attribute])
      {
        return this->CurrentInput->GetPointData()->GetTangents()->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case PointDataAttributes::POINT_UVS:
      if (this->HasPointAttributes[attribute])
      {
        return this->CurrentInput->GetPointData()->GetTCoords()->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;

    default:
      break;
  }

  return 0;
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetCellAttributeElementSize(
  vtkWebGPUPolyDataMapper::CellDataAttributes attribute)
{
  switch (attribute)
  {
    case CellDataAttributes::CELL_COLORS:
      if (this->HasCellAttributes[attribute])
      {
        return vtkDataArray::SafeDownCast(this->Colors)->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;

    case CellDataAttributes::CELL_NORMALS:
      if (this->HasCellAttributes[attribute])
      {
        return this->CurrentInput->GetCellData()->GetNormals()->GetNumberOfComponents() *
          sizeof(vtkTypeFloat32);
      }

      break;
    default:
      break;
  }

  return 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkWebGPUPolyDataMapper::GetPointAttributeByteOffset(
  vtkWebGPUPolyDataMapper::PointDataAttributes attribute)
{
  vtkIdType accumulatedOffset = 0;

  for (int attributeIndex = 0; attributeIndex <= PointDataAttributes::POINT_NB_ATTRIBUTES;
       attributeIndex++)
  {
    PointDataAttributes attributeInOrder = this->PointDataAttributesOrder[attributeIndex];
    if (attributeInOrder != attribute)
    {
      accumulatedOffset +=
        this->GetPointAttributeByteSize(static_cast<PointDataAttributes>(attributeInOrder));
    }
    else
    {
      break;
    }
  }

  return accumulatedOffset;
}

//------------------------------------------------------------------------------
vtkIdType vtkWebGPUPolyDataMapper::GetCellAttributeByteOffset(
  vtkWebGPUPolyDataMapper::CellDataAttributes attribute)
{
  vtkIdType accumulatedOffset = 0;

  for (int attributeIndex = 0; attributeIndex <= CellDataAttributes::CELL_NB_ATTRIBUTES;
       attributeIndex++)
  {
    CellDataAttributes attributeInOrder = this->CellDataAttributesOrder[attributeIndex];
    if (attributeInOrder != attribute)
    {
      accumulatedOffset +=
        this->GetCellAttributeByteSize(static_cast<CellDataAttributes>(attributeInOrder));
    }
    else
    {
      break;
    }
  }

  return accumulatedOffset;
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetExactPointBufferSize()
{
  unsigned long result = 0;

  result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_POSITIONS);
  result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_COLORS);
  result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_NORMALS);
  result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_TANGENTS);
  result += this->GetPointAttributeByteSize(PointDataAttributes::POINT_UVS);

  result = vtkWebGPUConfiguration::Align(result, 32);
  return result;
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetExactCellBufferSize()
{
  unsigned long result = 0;

  result += this->GetCellAttributeByteSize(CellDataAttributes::CELL_COLORS);
  result += this->GetCellAttributeByteSize(CellDataAttributes::CELL_NORMALS);

  result = vtkWebGPUConfiguration::Align(result, 32);
  return result;
}

//------------------------------------------------------------------------------
bool vtkWebGPUPolyDataMapper::GetNeedToRemapScalars(vtkPolyData* mesh)
{
  if (mesh == nullptr)
  {
    // so that the previous colors are invalidated.
    this->LastScalarVisibility = this->ScalarVisibility;
    this->LastScalarMode = this->ScalarMode;
    return true;
  }
  int cellFlag = 0;
  vtkAbstractArray* scalars = vtkAbstractMapper::GetAbstractScalars(
    mesh, this->ScalarMode, this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);

  if (!scalars)
  {
    // so that the previous colors are invalidated.
    this->LastScalarVisibility = this->ScalarVisibility;
    this->LastScalarMode = this->ScalarMode;
    return true;
  }

  bool remapScalars = false;
  if (this->LastScalarVisibility != this->ScalarVisibility)
  {
    this->LastScalarVisibility = this->ScalarVisibility;
    remapScalars |= true;
  }
  if (this->LastScalarMode != this->ScalarMode)
  {
    this->LastScalarMode = this->ScalarMode;
    remapScalars |= true;
  }
  if (this->ScalarVisibility)
  {
    if (cellFlag)
    {
      if (mesh->GetCellData()->GetMTime() > this->CellAttributesBuildTimestamp[CELL_COLORS])
      {
        remapScalars |= true;
      }
      if (this->GetLookupTable()->GetMTime() > this->CellAttributesBuildTimestamp[CELL_COLORS])
      {
        remapScalars |= true;
      }
    }
    else
    {
      if (mesh->GetPointData()->GetMTime() > this->PointAttributesBuildTimestamp[POINT_COLORS])
      {
        remapScalars |= true;
      }
      if (this->GetLookupTable()->GetMTime() > this->PointAttributesBuildTimestamp[POINT_COLORS])
      {
        remapScalars |= true;
      }
    }
  }
  return remapScalars;
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::DeducePointCellAttributeAvailability(vtkPolyData* mesh)
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
  if (this->Colors != nullptr && this->Colors->GetNumberOfValues() > 0)
  {
    // we've point scalars mapped to colors.
    this->HasPointAttributes[POINT_COLORS] = true;
  }
  // check for cell normals
  this->HasCellAttributes[CELL_NORMALS] = cellData->GetNormals() != nullptr;
  // check for cell scalars
  if (this->ScalarVisibility)
  {
    // We must figure out how the scalars should be mapped to the polydata.
    if ((this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
          !this->CurrentInput->GetPointData()->GetScalars()) &&
      this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA && this->Colors &&
      this->Colors->GetNumberOfTuples() > 0)
    {
      this->HasCellAttributes[CELL_COLORS] = true;
      // reset point color state when cell scalars must be visible
      this->HasPointAttributes[POINT_COLORS] = false;
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ResetPointCellAttributeState()
{
  for (int i = 0; i < POINT_NB_ATTRIBUTES; ++i)
  {
    this->HasPointAttributes[i] = false;
  }
  for (int i = 0; i < CELL_NB_ATTRIBUTES; ++i)
  {
    this->HasCellAttributes[i] = false;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::UpdateMeshGeometryBuffers(vtkWebGPURenderWindow* wgpuRenderWindow)
{
  if ((this->CachedInput != nullptr) && (this->CurrentInput != nullptr) &&
    (this->CurrentInput != this->CachedInput))
  {
    // invalidate any existing pipeline/bindgroups because input mesh changed.
    this->ReleaseGraphicsResources(wgpuRenderWindow);
  }
  if (this->CachedInput == nullptr)
  {
    vtkDebugMacro(<< "No cached input.");
    this->InvokeEvent(vtkCommand::StartEvent, nullptr);
    if (!this->Static)
    {
      this->GetInputAlgorithm()->Update();
    }
    this->CachedInput = this->CurrentInput = this->GetInput();
    this->InvokeEvent(vtkCommand::EndEvent, nullptr);
  }
  else
  {
    this->CurrentInput = this->CachedInput;
  }
  if (this->CurrentInput == nullptr)
  {
    vtkErrorMacro(<< "No input!");
    // TODO: Destroy existing mesh buffers.
    return;
  }

  // if there are no points then we are done
  if (!this->CurrentInput->GetPoints())
  {
    // TODO: Destroy existing mesh buffers.
    return;
  }

  const bool remapScalars = this->GetNeedToRemapScalars(this->CurrentInput);
  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  if (remapScalars)
  {
    // Get rid of old texture color coordinates if any
    if (this->ColorCoordinates)
    {
      this->ColorCoordinates->UnRegister(this);
      this->ColorCoordinates = nullptr;
    }
    // Get rid of old texture color coordinates if any
    if (this->Colors)
    {
      this->Colors->UnRegister(this);
      this->Colors = nullptr;
    }
    int cellFlag = 0;
    this->MapScalars(this->CurrentInput, 1.0, cellFlag);
  }
  this->DeducePointCellAttributeAvailability(this->CurrentInput);
  ///@{ TODO:
  // // If we are coloring by texture, then load the texture map.
  // if (this->ColorTextureMap)
  // {
  //   if (this->InternalColorTexture == nullptr)
  //   {
  //     this->InternalColorTexture = vtkOpenGLTexture::New();
  //     this->InternalColorTexture->RepeatOff();
  //   }
  //   this->InternalColorTexture->SetInputData(this->ColorTextureMap);
  // }
  ///@}

  MeshAttributeDescriptor meshAttrDescriptor;

  vtkPointData* pointData = this->CurrentInput->GetPointData();
  vtkDataArray* pointPositions = this->CurrentInput->GetPoints()->GetData();
  vtkDataArray* pointColors =
    this->HasPointAttributes[POINT_COLORS] ? vtkDataArray::SafeDownCast(this->Colors) : nullptr;
  vtkDataArray* pointNormals = pointData->GetNormals();
  vtkDataArray* pointTangents = pointData->GetTangents();
  vtkDataArray* pointUvs = pointData->GetTCoords();

  using DispatchT = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;

  // Realloc WGPUBuffer to fit all point attributes.
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
    const auto label = "pointdata@" + this->CurrentInput->GetObjectDescription();
    pointBufDescriptor.label = label.c_str();
    pointBufDescriptor.mappedAtCreation = false;
    pointBufDescriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    this->MeshSSBO.Point.Buffer = wgpuRenderWindow->CreateDeviceBuffer(pointBufDescriptor);
    this->MeshSSBO.Point.Size = requiredPointBufferSize;
    for (int attributeIndex = 0; attributeIndex < PointDataAttributes::POINT_NB_ATTRIBUTES;
         attributeIndex++)
    {
      this->PointAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
    }
    updatePointDescriptor = true;
  }

  const auto& device = wgpuRenderWindow->GetDevice();
  ::WriteTypedArray<vtkTypeFloat32> pointDataWriter{ 0, this->MeshSSBO.Point.Buffer, device, 1. };

  pointDataWriter.Denominator = 1.0;
  pointDataWriter.ByteOffset = 0;
  std::size_t lastByteOffset = 0;
  for (int attributeIndex = 0; attributeIndex < PointDataAttributes::POINT_NB_ATTRIBUTES;
       attributeIndex++)
  {
    switch (PointDataAttributesOrder[attributeIndex])
    {
      case PointDataAttributes::POINT_POSITIONS:
        meshAttrDescriptor.Positions.Start = pointDataWriter.ByteOffset / sizeof(vtkTypeFloat32);

        if (pointPositions->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(pointPositions, pointDataWriter))
          {
            pointDataWriter(pointPositions);
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
          this->DebugLogBufferUpload("Positions", pointDataWriter.ByteOffset - lastByteOffset);
        }
        else
        {
          pointDataWriter.Advance(pointPositions);
        }
        meshAttrDescriptor.Positions.NumComponents = pointPositions->GetNumberOfComponents();
        meshAttrDescriptor.Positions.NumTuples = pointPositions->GetNumberOfTuples();

        break;

      case PointDataAttributes::POINT_COLORS:
        pointDataWriter.Denominator = 255.0f;
        meshAttrDescriptor.Colors.Start = pointDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (pointColors &&
          pointColors->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(pointColors, pointDataWriter))
          {
            pointDataWriter(pointColors);
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
          this->DebugLogBufferUpload("Colors", pointDataWriter.ByteOffset - lastByteOffset);
        }
        else
        {
          pointDataWriter.Advance(pointColors);
        }
        // rollback to default denominator
        pointDataWriter.Denominator = 1.0f;
        meshAttrDescriptor.Colors.NumComponents =
          pointColors ? pointColors->GetNumberOfComponents() : 0;
        meshAttrDescriptor.Colors.NumTuples = pointColors ? pointColors->GetNumberOfTuples() : 0;

        break;

      case PointDataAttributes::POINT_NORMALS:
        meshAttrDescriptor.Normals.Start = pointDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (pointNormals &&
          pointNormals->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(pointNormals, pointDataWriter))
          {
            pointDataWriter(pointNormals);
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
          this->DebugLogBufferUpload("Normals", pointDataWriter.ByteOffset - lastByteOffset);
        }
        else
        {
          pointDataWriter.Advance(pointNormals);
        }
        meshAttrDescriptor.Normals.NumComponents =
          pointNormals ? pointNormals->GetNumberOfComponents() : 0;
        meshAttrDescriptor.Normals.NumTuples = pointNormals ? pointNormals->GetNumberOfTuples() : 0;
        break;

      case PointDataAttributes::POINT_TANGENTS:
        meshAttrDescriptor.Tangents.Start = pointDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (pointTangents &&
          pointTangents->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(pointTangents, pointDataWriter))
          {
            pointDataWriter(pointTangents);
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
          this->DebugLogBufferUpload("Tangents", pointDataWriter.ByteOffset - lastByteOffset);
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
        if (pointUvs && pointUvs->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(pointUvs, pointDataWriter))
          {
            pointDataWriter(pointUvs);
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
          this->DebugLogBufferUpload("UVs", pointDataWriter.ByteOffset - lastByteOffset);
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
    lastByteOffset = pointDataWriter.ByteOffset;
  }

  ::WriteTypedArray<vtkTypeFloat32> cellDataWriter{ 0, this->MeshSSBO.Cell.Buffer, device, 1. };

  vtkCellData* cellData = this->CurrentInput->GetCellData();
  vtkDataArray* cellColors = nullptr;
  vtkNew<vtkUnsignedCharArray> cellColorsFromFieldData;
  if (this->HasCellAttributes[CELL_COLORS])
  {
    // are we using a single color value replicated over all cells?
    if (this->FieldDataTupleId > -1 && this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA)
    {
      const vtkIdType numCells = this->CurrentInput->GetNumberOfCells();
      const int numComponents = this->Colors->GetNumberOfComponents();
      cellColorsFromFieldData->SetNumberOfComponents(numComponents);
      cellColorsFromFieldData->SetNumberOfTuples(numCells);
      for (int i = 0; i < numComponents; ++i)
      {
        cellColorsFromFieldData->FillComponent(
          i, this->Colors->GetComponent(this->FieldDataTupleId, i));
      }
      cellColors = cellColorsFromFieldData.Get();
    }
    else
    {
      cellColors = this->Colors;
    }
  }
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
    const auto label = "celldata@" + this->CurrentInput->GetObjectDescription();
    cellBufDescriptor.label = label.c_str();
    cellBufDescriptor.mappedAtCreation = false;
    cellBufDescriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    this->MeshSSBO.Cell.Buffer = wgpuRenderWindow->CreateDeviceBuffer(cellBufDescriptor);
    this->MeshSSBO.Cell.Size = requiredCellBufferSize;
    for (int attributeIndex = 0; attributeIndex < CellDataAttributes::CELL_NB_ATTRIBUTES;
         attributeIndex++)
    {
      this->CellAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
    }
    updateCellArrayDescriptor = true;
  }

  for (int attributeIndex = 0; attributeIndex < CellDataAttributes::CELL_NB_ATTRIBUTES;
       attributeIndex++)
  {
    switch (CellDataAttributesOrder[attributeIndex])
    {
      case CellDataAttributes::CELL_COLORS:
      {
        meshAttrDescriptor.CellColors.Start = cellDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        cellDataWriter.Denominator = 255.0f;
        if (cellColors &&
          cellColors->GetMTime() > this->CellAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(cellColors, cellDataWriter))
          {
            cellDataWriter(cellColors);
          }
          this->CellAttributesBuildTimestamp[attributeIndex].Modified();
          this->DebugLogBufferUpload("Cell colors", pointDataWriter.ByteOffset);
        }
        else
        {
          cellDataWriter.Advance(cellColors);
        }
        // rollback to default denominator
        cellDataWriter.Denominator = 1.0f;
        meshAttrDescriptor.CellColors.NumComponents =
          cellColors ? cellColors->GetNumberOfComponents() : 0;
        meshAttrDescriptor.CellColors.NumTuples = cellColors ? cellColors->GetNumberOfTuples() : 0;

        break;
      }

      case CellDataAttributes::CELL_NORMALS:
      {
        meshAttrDescriptor.CellNormals.Start = cellDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (cellNormals &&
          cellNormals->GetMTime() > this->CellAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(cellNormals, cellDataWriter))
          {
            cellDataWriter(cellNormals);
          }
          this->CellAttributesBuildTimestamp[attributeIndex].Modified();
          this->DebugLogBufferUpload("Cell normals", pointDataWriter.ByteOffset);
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

  if (this->AttributeDescriptorBuffer == nullptr)
  {
    this->AttributeDescriptorBuffer =
      vtkWebGPUBufferInternals::Upload(device, 0, &meshAttrDescriptor, sizeof(meshAttrDescriptor),
        wgpu::BufferUsage::Storage, "Mesh attribute descriptor");
    this->DebugLogBufferUpload("Mesh attribute descriptor", sizeof(meshAttrDescriptor));
  }
  else
  {
    // handle partial updates
    if (updatePointDescriptor || updateCellArrayDescriptor)
    {
      device.GetQueue().WriteBuffer(
        this->AttributeDescriptorBuffer, 0, &meshAttrDescriptor, sizeof(meshAttrDescriptor));
      this->DebugLogBufferUpload("Mesh attribute descriptor", sizeof(meshAttrDescriptor));
    }
  }

  vtkDebugMacro(<< ((updatePointDescriptor || updateCellArrayDescriptor) ? "rebuilt" : "")
                << (updatePointDescriptor ? " point" : "")
                << ((updatePointDescriptor && updateCellArrayDescriptor) ? " and" : "")
                << (updateCellArrayDescriptor ? " cell" : "")
                << ((updatePointDescriptor || updateCellArrayDescriptor)
                       ? " buffers"
                       : "reuse point and cell buffers"));
  if (updatePointDescriptor || updateCellArrayDescriptor)
  {
    // Create bind group for the point/cell attribute buffers.
    this->MeshAttributeBindGroup =
      this->CreateMeshAttributeBindGroup(device, "MeshAttributeBindGroup");
    this->RebuildGraphicsPipelines = true;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkWebGPUPolyDataMapper::GetTessellatedPrimitiveSizeOffsetForCellType(int cellType)
{
  switch (cellType)
  {
    case VTK_POLYGON:
    case VTK_QUAD:
      return 2;
    case VTK_TRIANGLE_STRIP:
      return 1;
    case VTK_TRIANGLE:
      return 0;
    case VTK_POLY_LINE:
      return 1;
    case VTK_LINE:
      return 0;
    case VTK_POLY_VERTEX:
      return 0;
    case VTK_VERTEX:
      return 0;
    default:
      return 0;
  }
}

//------------------------------------------------------------------------------
const char* vtkWebGPUPolyDataMapper::GetTopologySourceTypeAsString(
  TopologySourceType topologySourceType)
{
  switch (topologySourceType)
  {
    case TOPOLOGY_SOURCE_VERTS:
      return "TOPOLOGY_SOURCE_VERTS";
    case TOPOLOGY_SOURCE_LINES:
      return "TOPOLOGY_SOURCE_LINES";
    case TOPOLOGY_SOURCE_LINE_POINTS:
      return "TOPOLOGY_SOURCE_LINE_POINTS";
    case TOPOLOGY_SOURCE_POLYGONS:
      return "TOPOLOGY_SOURCE_POLYGONS";
    case TOPOLOGY_SOURCE_POLYGON_POINTS:
      return "TOPOLOGY_SOURCE_POLYGON_POINTS";
    case TOPOLOGY_SOURCE_POLYGON_EDGES:
      return "TOPOLOGY_SOURCE_POLYGON_EDGES";
    default:
      return "";
  }
}

//------------------------------------------------------------------------------
const char* vtkWebGPUPolyDataMapper::GetGraphicsPipelineTypeAsString(
  GraphicsPipelineType graphicsPipelineType)
{
  switch (graphicsPipelineType)
  {
    case GFX_PIPELINE_POINTS:
      return "GFX_PIPELINE_POINTS";
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
      return "GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN";
    case GFX_PIPELINE_LINES_MITER_JOIN:
      return "GFX_PIPELINE_LINES_MITER_JOIN";
    case GFX_PIPELINE_TRIANGLES:
      return "GFX_PIPELINE_TRIANGLES";
    case GFX_PIPELINE_VERTEX_VISIBILITY:
      return "GFX_PIPELINE_VERTEX_VISIBILITY";
    default:
      return "";
  }
}

//------------------------------------------------------------------------------
const char* vtkWebGPUPolyDataMapper::GetCellTypeAsString(int cellType)
{
  switch (cellType)
  {
    case VTK_POLYGON:
      return "polygon";
    case VTK_QUAD:
      return "quad";
    case VTK_TRIANGLE_STRIP:
      return "triangle-strip";
    case VTK_TRIANGLE:
      return "triangle";
    case VTK_POLY_LINE:
      return "polyline";
    case VTK_LINE:
      return "line";
    case VTK_POLY_VERTEX:
      return "polyvertex";
    case VTK_VERTEX:
      return "vertex";
    default:
      return "";
  }
}

//------------------------------------------------------------------------------
const char* vtkWebGPUPolyDataMapper::GetTessellatedPrimitiveTypeAsString(
  TopologySourceType topologySourceType)
{
  switch (topologySourceType)
  {
    case TOPOLOGY_SOURCE_VERTS:
      return "point-list";
    case TOPOLOGY_SOURCE_LINES:
      return "line-list";
    case TOPOLOGY_SOURCE_LINE_POINTS:
      return "point-list";
    case TOPOLOGY_SOURCE_POLYGONS:
      return "triangle-list";
    case TOPOLOGY_SOURCE_POLYGON_POINTS:
      return "point-list";
    case TOPOLOGY_SOURCE_POLYGON_EDGES:
    default:
      return "line-list";
  }
}

//------------------------------------------------------------------------------
std::size_t vtkWebGPUPolyDataMapper::GetTessellatedPrimitiveSize(
  TopologySourceType topologySourceType)
{
  switch (topologySourceType)
  {
    case TOPOLOGY_SOURCE_VERTS:
      return 1;
    case TOPOLOGY_SOURCE_LINES:
      return 2;
    case TOPOLOGY_SOURCE_LINE_POINTS:
      return 1;
    case TOPOLOGY_SOURCE_POLYGONS:
      return 3;
    case TOPOLOGY_SOURCE_POLYGON_POINTS:
      return 1;
    case TOPOLOGY_SOURCE_POLYGON_EDGES:
    default:
      return 2;
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUPolyDataMapper::GetNeedToRebuildCellToPrimitiveComputePipeline(
  vtkCellArray* cells, TopologySourceType topologySourceType)
{
  return cells->GetMTime() > this->TopologyBuildTimestamp[topologySourceType];
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::UpdateCellToPrimitiveComputePipelineTimestamp(
  TopologySourceType topologySourceType)
{
  this->TopologyBuildTimestamp[topologySourceType].Modified();
}

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper::TopologySourceType
vtkWebGPUPolyDataMapper::GetTopologySourceTypeForCellType(int cellType, int representation)
{
  switch (cellType)
  {
    case VTK_POLYGON:
    case VTK_QUAD:
    case VTK_TRIANGLE_STRIP:
    case VTK_TRIANGLE:
      switch (representation)
      {
        case VTK_SURFACE:
          return TOPOLOGY_SOURCE_POLYGONS;
        case VTK_WIREFRAME:
          return TOPOLOGY_SOURCE_POLYGON_EDGES;
        case VTK_POINTS:
        default:
          return TOPOLOGY_SOURCE_POLYGON_POINTS;
      }
    case VTK_POLY_LINE:
    case VTK_LINE:
      switch (representation)
      {
        case VTK_SURFACE:
        case VTK_WIREFRAME:
          return TOPOLOGY_SOURCE_LINES;
        case VTK_POINTS:
        default:
          return TOPOLOGY_SOURCE_LINE_POINTS;
      }
    case VTK_POLY_VERTEX:
    case VTK_VERTEX:
    default:
      return TOPOLOGY_SOURCE_VERTS;
  }
}

//------------------------------------------------------------------------------
std::pair<vtkSmartPointer<vtkWebGPUComputePass>, vtkSmartPointer<vtkWebGPUComputePipeline>>
vtkWebGPUPolyDataMapper::CreateCellToPrimitiveComputePassForCellType(
  vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration, TopologySourceType topologySourceType)
{
  vtkSmartPointer<vtkWebGPUComputePass> pass;
  vtkSmartPointer<vtkWebGPUComputePipeline> pipeline;

  auto& bgInfo = this->TopologyBindGroupInfos[topologySourceType];

  // Create compute pipeline.
  bgInfo.ComputePipeline = vtk::TakeSmartPointer(vtkWebGPUComputePipeline::New());
  pipeline = bgInfo.ComputePipeline;
  pipeline->SetWGPUConfiguration(wgpuConfiguration);
  // Create compute pass.
  bgInfo.ComputePass = pipeline->CreateComputePass();
  pass = bgInfo.ComputePass;
  pass->SetLabel("Split polygonal faces into individual triangles");
  pass->SetShaderSource(VTKCellToGraphicsPrimitive);
  pass->SetShaderEntryPoint(::TopologyConversionShaderEntrypoints[topologySourceType]);
  return std::make_pair(pass, pipeline);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::DispatchCellToPrimitiveComputePipeline(
  vtkWebGPUConfiguration* wgpuConfiguration, vtkCellArray* cells, int representation, int cellType,
  vtkIdType cellIdOffset)
{
  const auto idx = this->GetTopologySourceTypeForCellType(cellType, representation);
  auto& bgInfo = this->TopologyBindGroupInfos[idx];

  if (!cells)
  {
    bgInfo.VertexCount = 0;
    // TODO: Destroy existing buffers.
    return;
  }
  if (cells->GetNumberOfCells() == 0)
  {
    bgInfo.VertexCount = 0;
    // TODO: Destroy existing buffers.
    return;
  }
  if (!this->GetNeedToRebuildCellToPrimitiveComputePipeline(cells, idx))
  {
    return;
  }

  const char* cellTypeAsString = this->GetCellTypeAsString(cellType);
  const char* primitiveTypeAsString = this->GetTessellatedPrimitiveTypeAsString(idx);
  const auto primitiveSize = this->GetTessellatedPrimitiveSize(idx);
  // extra workgroups are fine to have.
  int nRequiredWorkGroups = std::ceil(cells->GetNumberOfCells() / 64.0);
  std::array<vtkTypeUInt32, 3> nWorkGroupsPerDimension = { 1, 1, 1 };
  if (::Factorize(nRequiredWorkGroups, nWorkGroupsPerDimension) == -1)
  {
    vtkErrorMacro(<< "Number of cells is too large to fit in available workgroups");
    return;
  }
  vtkDebugMacro(<< "Dispatch " << cellTypeAsString
                << " with workgroups=" << nWorkGroupsPerDimension[0] << 'x'
                << nWorkGroupsPerDimension[1] << 'x' << nWorkGroupsPerDimension[2]);

  if (!cells->ConvertTo32BitStorage())
  {
    vtkErrorMacro(<< "Failed to convert cell array storage to 32-bit");
    return;
  }
  const vtkIdType primitiveSizeOffset =
    this->GetTessellatedPrimitiveSizeOffsetForCellType(cellType);
  std::size_t numberOfPrimitives = 0;
  std::vector<vtkTypeUInt32> primitiveIdOffsets;
  auto cellIterator = vtk::TakeSmartPointer(cells->NewIterator());
  for (cellIterator->GoToFirstCell(); !cellIterator->IsDoneWithTraversal();
       cellIterator->GoToNextCell())
  {
    const vtkIdType* cellPts = nullptr;
    vtkIdType cellSize;
    cellIterator->GetCurrentCell(cellSize, cellPts);
    primitiveIdOffsets.emplace_back(numberOfPrimitives);
    if (representation == VTK_WIREFRAME && cellType == VTK_POLYGON)
    {
      // number of sides in polygon.
      numberOfPrimitives += cellSize;
    }
    else if (representation == VTK_POINTS)
    {
      // number of points in cell.
      numberOfPrimitives += cellSize;
    }
    else
    {
      numberOfPrimitives += (cellSize - primitiveSizeOffset);
    }
  }
  primitiveIdOffsets.emplace_back(numberOfPrimitives);
  // create input buffer for connectivity ids
  vtkNew<vtkWebGPUComputeBuffer> connBuffer;
  connBuffer->SetGroup(0);
  connBuffer->SetBinding(0);
  connBuffer->SetLabel(
    std::string("connectivity/") + cellTypeAsString + "@" + cells->GetObjectDescription());
  connBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  connBuffer->SetData(cells->GetConnectivityArray());
  connBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);

  // create input buffer for offsets
  vtkNew<vtkWebGPUComputeBuffer> offsetsBuffer;
  offsetsBuffer->SetGroup(0);
  offsetsBuffer->SetBinding(1);
  offsetsBuffer->SetLabel(
    std::string("offsets/") + cellTypeAsString + "@" + cells->GetObjectDescription());
  offsetsBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  offsetsBuffer->SetData(cells->GetOffsetsArray());
  offsetsBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);

  // create input buffer for primitive offsets
  vtkNew<vtkWebGPUComputeBuffer> primIdBuffer;
  primIdBuffer->SetGroup(0);
  primIdBuffer->SetBinding(2);
  primIdBuffer->SetLabel(
    std::string("primIds/") + primitiveTypeAsString + "@" + cells->GetObjectDescription());
  primIdBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  primIdBuffer->SetData(primitiveIdOffsets);
  primIdBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

  std::vector<vtkTypeUInt32> uniformData = { static_cast<unsigned int>(cellIdOffset) };
  vtkNew<vtkWebGPUComputeBuffer> uniformBuffer;
  uniformBuffer->SetGroup(0);
  uniformBuffer->SetBinding(3);
  uniformBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER);
  uniformBuffer->SetData(uniformData);
  uniformBuffer->SetLabel(
    std::string("cell offsets/") + cellTypeAsString + "@" + cells->GetObjectDescription());
  uniformBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

  std::size_t outputBufferSize = 2 * numberOfPrimitives * primitiveSize * sizeof(vtkTypeUInt32);
  vtkNew<vtkWebGPUComputeBuffer> topologyBuffer;
  topologyBuffer->SetGroup(0);
  topologyBuffer->SetBinding(4);
  topologyBuffer->SetLabel(
    std::string("topology/") + primitiveTypeAsString + "@" + cells->GetObjectDescription());
  topologyBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_COMPUTE_STORAGE);
  topologyBuffer->SetByteSize(outputBufferSize);

  // handle optional edge visibility.
  // this lets fragment shader hide internal edges of a polygon
  // when edge visibility is turned on.
  vtkNew<vtkWebGPUComputeBuffer> edgeArrayBuffer;
  edgeArrayBuffer->SetGroup(0);
  edgeArrayBuffer->SetBinding(5);
  edgeArrayBuffer->SetLabel(
    std::string("edge array/") + primitiveTypeAsString + "@" + cells->GetObjectDescription());
  edgeArrayBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_COMPUTE_STORAGE);
  if (primitiveSize == 3)
  {
    edgeArrayBuffer->SetByteSize(numberOfPrimitives * sizeof(vtkTypeUInt32));
  }
  else
  {
    // placeholder must be aligned to 32-bit boundary
    edgeArrayBuffer->SetByteSize(4);
  }

  // obtain a compute pass for cell type.
  vtkSmartPointer<vtkWebGPUComputePass> pass;
  vtkSmartPointer<vtkWebGPUComputePipeline> pipeline;
  std::tie(pass, pipeline) =
    this->CreateCellToPrimitiveComputePassForCellType(wgpuConfiguration, idx);
  if (!(pass || pipeline))
  {
    vtkErrorMacro(<< "Failed to create a compute pass/pipeline to convert " << cellTypeAsString
                  << "->" << primitiveTypeAsString);
    return;
  }

  // add buffers one by one to the compute pass.
  pass->AddBuffer(connBuffer);
  pass->AddBuffer(offsetsBuffer);
  pass->AddBuffer(primIdBuffer);
  pass->AddBuffer(uniformBuffer);
  pass->AddBuffer(topologyBuffer);  // not used in cpu
  pass->AddBuffer(edgeArrayBuffer); // not used in cpu
  // the topology and edge_array buffers are populated by compute pipeline and their contents
  // read in the graphics pipeline within the vertex and fragment shaders
  // keep reference to the output of compute pipeline, reuse it in the vertex, fragment shaders.
  if (pipeline->GetRegisteredBuffer(topologyBuffer, bgInfo.TopologyBuffer))
  {
    bgInfo.VertexCount = numberOfPrimitives * primitiveSize;
  }
  else
  {
    bgInfo.VertexCount = 0;
  }
  // do the same for the edge array buffer
  if (!pipeline->GetRegisteredBuffer(edgeArrayBuffer, bgInfo.EdgeArrayBuffer))
  {
    vtkErrorMacro(<< "edge array buffer for " << primitiveTypeAsString << " is not registered!");
  }
  // dispatch
  pass->SetWorkgroups(
    nWorkGroupsPerDimension[0], nWorkGroupsPerDimension[1], nWorkGroupsPerDimension[2]);
  pass->Dispatch();

  // setup bind group
  if (bgInfo.VertexCount > 0)
  {
    const std::string& label = this->GetTopologySourceTypeAsString(idx);
    bgInfo.BindGroup = this->CreateTopologyBindGroup(wgpuConfiguration->GetDevice(), label, idx);
    this->RebuildGraphicsPipelines = true;
  }

  // update build timestamp
  this->UpdateCellToPrimitiveComputePipelineTimestamp(idx);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::SetupGraphicsPipelines(
  const wgpu::Device& device, vtkRenderer* renderer, vtkActor* actor)
{
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

  vtkWebGPURenderPipelineDescriptorInternals descriptor;
  descriptor.vertex.entryPoint = "vertexMain";
  descriptor.vertex.bufferCount = 0;
  descriptor.cFragment.entryPoint = "fragmentMain";
  descriptor.cTargets[0].format = wgpuRenderWindow->GetPreferredSurfaceTextureFormat();
  ///@{ TODO: Only for valid depth stencil formats
  auto depthState = descriptor.EnableDepthStencil(wgpuRenderWindow->GetDepthStencilFormat());
  depthState->depthWriteEnabled = true;
  depthState->depthCompare = wgpu::CompareFunction::Less;
  ///@}

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
  bgls.emplace_back(
    this->CreateMeshAttributeBindGroupLayout(device, "MeshAttributeBindGroupLayout"));
  auto layoutWithoutTopology =
    vtkWebGPUPipelineLayoutInternals::MakePipelineLayout(device, bgls, "pipelineLayout");
  bgls.emplace_back(this->CreateTopologyBindGroupLayout(device, "TopologyBindGroupLayout"));
  auto layoutWithTopology =
    vtkWebGPUPipelineLayoutInternals::MakePipelineLayout(device, bgls, "pipelineLayout");

  for (int i = 0; i < GFX_PIPELINE_NB_TYPES; ++i)
  {
    const auto seed = std::to_string(i) + this->GetClassName();
    descriptor.label = this->GetGraphicsPipelineTypeAsString(static_cast<GraphicsPipelineType>(i));
    descriptor.primitive.topology = GraphicsPipelinePrimitiveTypes[i];
    if (i == GFX_PIPELINE_VERTEX_VISIBILITY)
    {
      descriptor.layout = layoutWithoutTopology;
    }
    else
    {
      descriptor.layout = layoutWithTopology;
    }
    // generate a unique key for the pipeline descriptor and shader source pointer
    this->GraphicsPipelineKeys[i] =
      wgpuPipelineCache->GetPipelineKey(&descriptor, *GraphicsPipelineShaderSources[i]);
    // create a pipeline if it does not already exist
    if (wgpuPipelineCache->GetRenderPipeline(this->GraphicsPipelineKeys[i]) == nullptr)
    {
      wgpuPipelineCache->CreateRenderPipeline(
        &descriptor, wgpuRenderer, *GraphicsPipelineShaderSources[i]);
    }
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUPolyDataMapper::GetNeedToRebuildGraphicsPipelines(vtkActor* actor)
{
  if (this->RebuildGraphicsPipelines)
  {
    return true;
  }
  auto it = this->CachedActorProperties.find(actor);
  if (it == this->CachedActorProperties.end())
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
void vtkWebGPUPolyDataMapper::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  // Release mesh buffers, bind groups and reset the attribute build timestamps.
  this->MeshSSBO.Cell.Buffer = nullptr;
  this->MeshSSBO.Cell.Size = 0;
  for (int attributeIndex = 0; attributeIndex < CELL_NB_ATTRIBUTES; ++attributeIndex)
  {
    this->CellAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
  }
  this->MeshSSBO.Point.Buffer = nullptr;
  this->MeshSSBO.Point.Size = 0;
  for (int attributeIndex = 0; attributeIndex < POINT_NB_ATTRIBUTES; ++attributeIndex)
  {
    this->PointAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
  }
  this->LastScalarMode = -1;
  this->LastScalarVisibility = false;
  this->AttributeDescriptorBuffer = nullptr;
  this->MeshAttributeBindGroup = nullptr;

  // Release topology conversion pipelines and reset their build timestamps.
  for (int i = 0; i < TOPOLOGY_SOURCE_NB_TYPES; ++i)
  {
    this->TopologyBindGroupInfos[i] = TopologyBindGroupInfo{};
    this->TopologyBuildTimestamp[i] = vtkTimeStamp();
    this->IndirectDrawBufferUploadTimeStamp[i] = vtkTimeStamp();
  }
  this->RebuildGraphicsPipelines = true;
  this->CachedActorProperties.clear();
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::MapDataArrayToVertexAttribute(const char*, const char*, int, int) {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::MapDataArrayToMultiTextureAttribute(
  const char*, const char*, int, int)
{
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RemoveVertexAttributeMapping(const char*) {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RemoveAllVertexAttributeMappings() {}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ProcessSelectorPixelBuffers(
  vtkHardwareSelector*, std::vector<unsigned int>&, vtkProp*)
{
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputeRenderBuffer>
vtkWebGPUPolyDataMapper::AcquirePointAttributeComputeRenderBuffer(PointDataAttributes attribute,
  int bufferGroup, int bufferBinding, int uniformsGroup, int uniformsBinding)
{
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer =
    vtkSmartPointer<vtkWebGPUComputeRenderBuffer>::New();

  std::stringstream label;
  label << "Compute render buffer with point attribute " << static_cast<int>(attribute)
        << " and group/binding/uniformGroup/uniformBinding: " << bufferGroup << "/" << bufferBinding
        << "/" << uniformsGroup << "/" << uniformsBinding;

  renderBuffer->SetPointBufferAttribute(attribute);
  renderBuffer->SetCellBufferAttribute(CellDataAttributes::CELL_UNDEFINED);
  renderBuffer->SetGroup(bufferGroup);
  renderBuffer->SetBinding(bufferBinding);
  renderBuffer->SetRenderUniformsGroup(uniformsGroup);
  renderBuffer->SetRenderUniformsBinding(uniformsBinding);
  renderBuffer->SetLabel(label.str());

  this->NotSetupComputeRenderBuffers.insert(renderBuffer);

  return renderBuffer;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkWebGPUComputeRenderBuffer>
vtkWebGPUPolyDataMapper::AcquireCellAttributeComputeRenderBuffer(CellDataAttributes attribute,
  int bufferGroup, int bufferBinding, int uniformsGroup, int uniformsBinding)
{
  vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer =
    vtkSmartPointer<vtkWebGPUComputeRenderBuffer>::New();

  std::stringstream label;
  label << "Compute render buffer with cell attribute " << static_cast<int>(attribute)
        << " and group/binding/uniformGroup/uniformBinding: " << bufferGroup << "/" << bufferBinding
        << "/" << uniformsGroup << "/" << uniformsBinding;

  renderBuffer->SetPointBufferAttribute(PointDataAttributes::POINT_UNDEFINED);
  renderBuffer->SetCellBufferAttribute(attribute);
  renderBuffer->SetGroup(bufferGroup);
  renderBuffer->SetBinding(bufferBinding);
  renderBuffer->SetRenderUniformsGroup(uniformsGroup);
  renderBuffer->SetRenderUniformsBinding(uniformsBinding);
  renderBuffer->SetLabel(label.str());

  this->NotSetupComputeRenderBuffers.insert(renderBuffer);

  return renderBuffer;
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ComputeBounds()
{
  this->CachedInput = this->GetInput();
  this->InvokeEvent(vtkCommand::StartEvent, nullptr);
  if (!this->Static)
  {
    this->GetInputAlgorithm()->Update();
  }
  this->InvokeEvent(vtkCommand::EndEvent, nullptr);
  if (!this->CachedInput)
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
  }
  this->CachedInput->GetCellsBounds(this->Bounds);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::DebugLogBufferUpload(
  const std::string& attributeName, std::size_t numberOfBytes)
{
  vtkDebugMacro(<< "[" << attributeName << "]"
                << " Uploaded " << numberOfBytes << " bytes");
#ifdef NDEBUG
  (void)attributeName;
  (void)numberOfBytes;
#endif
}

VTK_ABI_NAMESPACE_END
