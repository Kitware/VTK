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
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "vtkWebGPUCommandEncoderDebugGroup.h"
#include "vtkWebGPUComputePass.h"
#include "vtkWebGPUComputePipeline.h"
#include "vtkWebGPUComputeRenderBuffer.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include "LineShaderOpaque.h"
#include "LineShaderTranslucent.h"
#include "PointShader.h"
#include "SurfaceMeshShader.h"

#include "Private/vtkWebGPUActorInternals.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUPipelineLayoutInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"

#include <array>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
const std::array<const char**, vtkWebGPUPolyDataMapper::GFX_PIPELINE_NB_TYPES>
  GraphicsPipelineShaderSources = { &PointShader, &LineShaderOpaque, &LineShaderTranslucent,
    &SurfaceMeshShader };

const std::array<wgpu::PrimitiveTopology, vtkWebGPUPolyDataMapper::GFX_PIPELINE_NB_TYPES>
  GraphicsPipelinePrimitiveTypes = { wgpu::PrimitiveTopology::TriangleStrip,
    wgpu::PrimitiveTopology::TriangleList, wgpu::PrimitiveTopology::TriangleStrip,
    wgpu::PrimitiveTopology::TriangleList };

std::map<vtkWebGPUPolyDataMapper::GraphicsPipelineType,
  std::vector<vtkWebGPUCellToPrimitiveConverter::TopologySourceType>>
  PipelineBindGroupCombos[VTK_SURFACE + 1] = { // VTK_POINTS
    { { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS,
      { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS,
        vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS,
        vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS } } },
    // VTK_WIREFRAME
    { { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_MITER_JOIN,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES } } },
    // VTK_SURFACE
    {
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_MITER_JOIN,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_TRIANGLES,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS } },
    }
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
  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  auto* displayProperty = actor->GetProperty();

  switch (wgpuRenderer->GetRenderStage())
  {
    case vtkWebGPURenderer::RenderStageEnum::UpdatingBuffers:
    {
      // update (i.e, create and write) GPU buffers if the data is outdated.
      this->UpdateMeshGeometryBuffers(wgpuRenderWindow);
      auto* mesh = this->CurrentInput;
      vtkTypeUInt32* vertexCounts[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
      wgpu::Buffer* topologyBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
      wgpu::Buffer* edgeArrayBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];

      for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
      {
        auto& bgInfo = this->TopologyBindGroupInfos[i];
        vertexCounts[i] = &(bgInfo.VertexCount);
        topologyBuffers[i] = &(bgInfo.TopologyBuffer);
        edgeArrayBuffers[i] = &(bgInfo.EdgeArrayBuffer);
      }
      bool updateTopologyBindGroup =
        this->CellConverter->DispatchMeshToPrimitiveComputePipeline(wgpuConfiguration, mesh,
          displayProperty->GetRepresentation(), vertexCounts, topologyBuffers, edgeArrayBuffers);
      // Handle vertex visibility.
      if (displayProperty->GetVertexVisibility() &&
        // avoids dispatching the cell-to-vertex pipeline again.
        displayProperty->GetRepresentation() != VTK_POINTS)
      {
        // dispatch compute pipeline that extracts cell vertices.
        updateTopologyBindGroup |=
          this->CellConverter->DispatchMeshToPrimitiveComputePipeline(wgpuConfiguration, mesh,
            /*representation=*/VTK_POINTS, vertexCounts, topologyBuffers, edgeArrayBuffers);
      }
      // Rebuild topology bind group if required (when VertexCount > 0)
      for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
      {
        const auto topologySourceType = vtkWebGPUCellToPrimitiveConverter::TopologySourceType(i);
        auto& bgInfo = this->TopologyBindGroupInfos[i];
        // setup bind group
        if (updateTopologyBindGroup && bgInfo.VertexCount > 0)
        {
          const std::string& label =
            vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(topologySourceType);
          bgInfo.BindGroup = this->CreateTopologyBindGroup(
            wgpuConfiguration->GetDevice(), label, topologySourceType);
          this->RebuildGraphicsPipelines = true;
        }
        else if (bgInfo.VertexCount == 0)
        {
          if (bgInfo.TopologyBuffer)
          {
            bgInfo.TopologyBuffer.Destroy();
            bgInfo.TopologyBuffer = nullptr;
          }
          if (bgInfo.EdgeArrayBuffer)
          {
            bgInfo.EdgeArrayBuffer.Destroy();
            bgInfo.EdgeArrayBuffer = nullptr;
          }
          if (bgInfo.BindGroup != nullptr)
          {
            bgInfo.BindGroup = nullptr;
            this->RebuildGraphicsPipelines = true;
          }
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
bool vtkWebGPUPolyDataMapper::CacheActorRendererProperties(vtkActor* actor, vtkRenderer* renderer)
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
      const auto topologyBGInfoName =
        vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
      vtkScopedEncoderDebugGroup(passEncoder, topologyBGInfoName);
      switch (bindGroupType)
      {
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS:
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS:
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS:
          passEncoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount);
          break;
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES:
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES:
          if (pipelineType == GFX_PIPELINE_LINES_MITER_JOIN)
          {
            passEncoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount / 2);
          }
          else if (pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN)
          {
            passEncoder.Draw(/*vertexCount=*/36, /*instanceCount=*/bgInfo.VertexCount / 2);
          }
          break;
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS:
          passEncoder.Draw(/*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/1);
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
    passEncoder.Draw(/*vertexCount=*/4,
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
      passEncoder.SetBindGroup(3, bgInfo.BindGroup);
      const auto topologyBGInfoName =
        vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
      vtkScopedEncoderDebugGroup(passEncoder, topologyBGInfoName);
      passEncoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount);
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
      const auto topologyBGInfoName =
        vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
      vtkScopedEncoderDebugGroup(bundleEncoder, topologyBGInfoName);
      switch (bindGroupType)
      {
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS:
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS:
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS:
          bundleEncoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount);
          break;
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES:
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES:
          if (pipelineType == GFX_PIPELINE_LINES_MITER_JOIN)
          {
            bundleEncoder.Draw(/*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount / 2);
          }
          else if (pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN)
          {
            bundleEncoder.Draw(/*vertexCount=*/36, /*instanceCount=*/bgInfo.VertexCount / 2);
          }
          break;
        case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS:
          bundleEncoder.Draw(/*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/1);
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
wgpu::BindGroup vtkWebGPUPolyDataMapper::CreateTopologyBindGroup(const wgpu::Device& device,
  const std::string& label,
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
    // invalidate any existing pipeline/bindgroups because input mesh changed.
    this->ReleaseGraphicsResources(wgpuRenderWindow);
    return;
  }

  // if there are no points then we are done
  if (!this->CurrentInput->GetPoints())
  {
    // invalidate any existing pipeline/bindgroups because input mesh changed.
    this->ReleaseGraphicsResources(wgpuRenderWindow);
    return;
  }

  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  int cellFlag = 0;
  this->MapScalars(this->CurrentInput, 1.0, cellFlag);
  this->DeducePointCellAttributeAvailability(this->CurrentInput);

  MeshAttributeDescriptor meshAttrDescriptor = {};
  meshAttrDescriptor.Pickable = true;

  vtkPointData* pointData = this->CurrentInput->GetPointData();
  vtkDataArray* pointPositions = this->CurrentInput->GetPoints()->GetData();
  vtkDataArray* pointColors =
    this->HasPointAttributes[POINT_COLORS] ? vtkDataArray::SafeDownCast(this->Colors) : nullptr;
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
    const auto label = "PointAttributes-" + this->CurrentInput->GetObjectDescription();
    pointBufDescriptor.label = label.c_str();
    pointBufDescriptor.mappedAtCreation = false;
    pointBufDescriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    this->MeshSSBO.Point.Buffer = wgpuConfiguration->CreateBuffer(pointBufDescriptor);
    this->MeshSSBO.Point.Size = requiredPointBufferSize;
    for (int attributeIndex = 0; attributeIndex < PointDataAttributes::POINT_NB_ATTRIBUTES;
         attributeIndex++)
    {
      this->PointAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
    }
    updatePointDescriptor = true;
  }

  ::WriteTypedArray<vtkTypeFloat32> pointDataWriter{ 0, this->MeshSSBO.Point.Buffer,
    wgpuConfiguration, 1. };

  pointDataWriter.Denominator = 1.0;
  pointDataWriter.ByteOffset = 0;
  for (int attributeIndex = 0; attributeIndex < PointDataAttributes::POINT_NB_ATTRIBUTES;
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

      case PointDataAttributes::POINT_COLORS:
        pointDataWriter.Denominator = 255.0f;
        meshAttrDescriptor.Colors.Start = pointDataWriter.ByteOffset / sizeof(vtkTypeFloat32);
        if (pointColors &&
          pointColors->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(pointColors, pointDataWriter, "Colors"))
          {
            pointDataWriter(pointColors, "Colors");
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
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
        meshAttrDescriptor.Normals.NumTuples = pointNormals ? pointNormals->GetNumberOfTuples() : 0;
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
        if (pointUvs && pointUvs->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
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

  ::WriteTypedArray<vtkTypeFloat32> cellDataWriter{ 0, this->MeshSSBO.Cell.Buffer,
    wgpuConfiguration, 1. };

  vtkCellData* cellData = this->CurrentInput->GetCellData();
  vtkDataArray* cellColors = nullptr;
  vtkSmartPointer<vtkUnsignedCharArray> cellColorsFromFieldData;
  if (this->HasCellAttributes[CELL_COLORS])
  {
    // are we using a single color value replicated over all cells?
    if (this->FieldDataTupleId > -1 && this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA)
    {
      const vtkIdType numCells = this->CurrentInput->GetNumberOfCells();
      const int numComponents = this->Colors->GetNumberOfComponents();
      cellColorsFromFieldData = vtk::TakeSmartPointer(vtkUnsignedCharArray::New());
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
    const auto label = "CellAttributes-" + this->CurrentInput->GetObjectDescription();
    cellBufDescriptor.label = label.c_str();
    cellBufDescriptor.mappedAtCreation = false;
    cellBufDescriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    this->MeshSSBO.Cell.Buffer = wgpuConfiguration->CreateBuffer(cellBufDescriptor);
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
          if (!DispatchT::Execute(cellColors, cellDataWriter, "Cell colors"))
          {
            cellDataWriter(cellColors, "Cell colors");
          }
          this->CellAttributesBuildTimestamp[attributeIndex].Modified();
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

  // handle partial updates
  if (updatePointDescriptor || updateCellArrayDescriptor)
  {
    const std::string meshAttrDescriptorLabel =
      "MeshAttributeDescriptor-" + this->CurrentInput->GetObjectDescription();
    if (this->AttributeDescriptorBuffer == nullptr)
    {
      this->AttributeDescriptorBuffer = wgpuConfiguration->CreateBuffer(sizeof(meshAttrDescriptor),
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
        /*mappedAtCreation=*/false, meshAttrDescriptorLabel.c_str());
    }
    wgpuConfiguration->WriteBuffer(this->AttributeDescriptorBuffer, 0, &meshAttrDescriptor,
      sizeof(meshAttrDescriptor), meshAttrDescriptorLabel.c_str());
    // Create bind group for the point/cell attribute buffers.
    this->MeshAttributeBindGroup =
      this->CreateMeshAttributeBindGroup(wgpuConfiguration->GetDevice(), "MeshAttributeBindGroup");
    this->RebuildGraphicsPipelines = true;
  }

  vtkDebugMacro(<< ((updatePointDescriptor || updateCellArrayDescriptor) ? "rebuilt" : "")
                << (updatePointDescriptor ? " point" : "")
                << ((updatePointDescriptor && updateCellArrayDescriptor) ? " and" : "")
                << (updateCellArrayDescriptor ? " cell" : "")
                << ((updatePointDescriptor || updateCellArrayDescriptor)
                       ? " buffers"
                       : "reuse point and cell buffers"));
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
    default:
      return "";
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::SetupGraphicsPipelines(
  const wgpu::Device& device, vtkRenderer* renderer, vtkActor* actor)
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

  for (int i = 0; i < GFX_PIPELINE_NB_TYPES; ++i)
  {
    descriptor.label = this->GetGraphicsPipelineTypeAsString(static_cast<GraphicsPipelineType>(i));
    descriptor.primitive.topology = GraphicsPipelinePrimitiveTypes[i];
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
bool vtkWebGPUPolyDataMapper::GetNeedToRebuildGraphicsPipelines(
  vtkActor* actor, vtkRenderer* renderer)
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
  for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
  {
    this->TopologyBindGroupInfos[i] = TopologyBindGroupInfo{};
    this->IndirectDrawBufferUploadTimeStamp[i] = vtkTimeStamp();
  }
  this->CellConverter->ReleaseGraphicsResources(w);
  this->RebuildGraphicsPipelines = true;
  for (auto& it : this->CachedActorRendererProperties)
  {
    if (auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(it.first.second))
    {
      wgpuRenderer->InvalidateBundle();
    }
  }
  this->CachedActorRendererProperties.clear();
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

VTK_ABI_NAMESPACE_END
