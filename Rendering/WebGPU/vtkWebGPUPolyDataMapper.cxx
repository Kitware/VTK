// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUPolyDataMapper.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchDataSetArrayList.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
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
#include "vtkWebGPURenderPipelineCache.h"
#include "vtkWebGPURenderTextureDeviceResource.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"
#include "vtkWebGPUTexture.h"

#include "vtkPolyDataFSWGSL.h"
#include "vtkPolyDataVSWGSL.h"

#include "Private/vtkWebGPUActorInternals.h"
#include "Private/vtkWebGPUBindGroupInternals.h"
#include "Private/vtkWebGPUBindGroupLayoutInternals.h"
#include "Private/vtkWebGPUPipelineLayoutInternals.h"
#include "Private/vtkWebGPURenderPipelineDescriptorInternals.h"

#include <array>
#include <iostream>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
std::map<vtkWebGPUPolyDataMapper::GraphicsPipelineType,
  std::vector<vtkWebGPUCellToPrimitiveConverter::TopologySourceType>>
  PipelineBindGroupCombos[VTK_SURFACE + 1] = { // VTK_POINTS
    { { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS_SHAPED,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS } } },
    // VTK_WIREFRAME
    { { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_THICK,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_MITER_JOIN,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES,
          vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES } } },
    // VTK_SURFACE
    {
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS_SHAPED,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_THICK,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_MITER_JOIN,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_TRIANGLES,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES } },
      { vtkWebGPUPolyDataMapper::GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE,
        { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS } },
    }
  };

template <typename DestT>
struct WriteTypedArray
{
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
      this->DstBuffer, 0, data->GetPointer(0), nbytes, description);
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
  vtkLogScopeFunction(TRACE);
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
  vtkLog(TRACE,
    "RenderPiece for actor: " << actor << " in renderer: " << renderer
                              << " in stage: " << static_cast<int>(wgpuRenderer->GetRenderStage()));
  switch (wgpuRenderer->GetRenderStage())
  {
    case vtkWebGPURenderer::RenderStageEnum::SyncDeviceResources:
    {
      // update (i.e, create and write) GPU buffers if the data is outdated.
      this->UpdateMeshGeometryBuffers(wgpuRenderWindow);
      auto* mesh = this->CurrentInput;
      if (mesh == nullptr || mesh->GetNumberOfPoints() == 0)
      {
        wgpuRenderer->InvalidateBundle();
        return;
      }
      if (this->ColorTextureHostResource)
      {
        this->ColorTextureHostResource->Render(renderer);
      }
      this->UpdateClippingPlanesBuffer(wgpuConfiguration, actor);
      vtkTypeUInt32* vertexCounts[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
      wgpu::Buffer*
        connectivityBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
      wgpu::Buffer* cellIdBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
      wgpu::Buffer* edgeArrayBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
      wgpu::Buffer*
        cellIdOffsetUniformBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];

      for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
      {
        auto& bgInfo = this->TopologyBindGroupInfos[i];
        vertexCounts[i] = &(bgInfo.VertexCount);
        connectivityBuffers[i] = &(bgInfo.ConnectivityBuffer);
        cellIdBuffers[i] = &(bgInfo.CellIdBuffer);
        edgeArrayBuffers[i] = &(bgInfo.EdgeArrayBuffer);
        cellIdOffsetUniformBuffers[i] = &(bgInfo.CellIdOffsetUniformBuffer);
      }
      bool updateTopologyBindGroup = false;
      updateTopologyBindGroup |= this->CellConverter->DispatchMeshToPrimitiveComputePipeline(
        wgpuConfiguration, mesh, displayProperty->GetRepresentation(), vertexCounts,
        connectivityBuffers, cellIdBuffers, edgeArrayBuffers, cellIdOffsetUniformBuffers);
      // Handle vertex visibility.
      if (displayProperty->GetVertexVisibility() &&
        // avoids dispatching the cell-to-vertex pipeline again.
        displayProperty->GetRepresentation() != VTK_POINTS)
      {
        // dispatch compute pipeline that extracts cell vertices.
        updateTopologyBindGroup |= this->CellConverter->DispatchMeshToPrimitiveComputePipeline(
          wgpuConfiguration, mesh, VTK_POINTS, vertexCounts, connectivityBuffers, cellIdBuffers,
          edgeArrayBuffers, cellIdOffsetUniformBuffers);
      }
      // Rebuild topology bind group if required (when VertexCount > 0)
      for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
      {
        const auto topologySourceType = vtkWebGPUCellToPrimitiveConverter::TopologySourceType(i);
        auto& bgInfo = this->TopologyBindGroupInfos[i];
        // setup bind group
        if (updateTopologyBindGroup && bgInfo.VertexCount > 0)
        {
          const std::string& label = this->GetObjectDescription() + "-" +
            vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(topologySourceType);
          bgInfo.BindGroup = this->CreateTopologyBindGroup(
            wgpuConfiguration->GetDevice(), label, topologySourceType);
          this->RebuildGraphicsPipelines = true;
        }
        if (bgInfo.VertexCount == 0)
        {
          if (bgInfo.ConnectivityBuffer)
          {
            bgInfo.ConnectivityBuffer.Destroy();
            bgInfo.ConnectivityBuffer = nullptr;
          }
          if (bgInfo.CellIdBuffer)
          {
            bgInfo.CellIdBuffer.Destroy();
            bgInfo.CellIdBuffer = nullptr;
          }
          if (bgInfo.EdgeArrayBuffer)
          {
            bgInfo.EdgeArrayBuffer.Destroy();
            bgInfo.EdgeArrayBuffer = nullptr;
          }
          if (bgInfo.CellIdOffsetUniformBuffer)
          {
            bgInfo.CellIdOffsetUniformBuffer.Destroy();
            bgInfo.CellIdOffsetUniformBuffer = nullptr;
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
        vtkLog(TRACE, "rebuild graphics pipelines");
        // Create bind group for the point/cell attribute buffers.
        this->MeshAttributeBindGroup = this->CreateMeshAttributeBindGroup(
          wgpuConfiguration->GetDevice(), this->GetObjectDescription() + "-MeshAttributeBindGroup");
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
      auto* mesh = this->CurrentInput;
      if (mesh == nullptr || mesh->GetNumberOfPoints() == 0)
      {
        wgpuRenderer->InvalidateBundle();
        return;
      }
      if (wgpuRenderer->GetUseRenderBundles())
      {
        this->RecordDrawCommands(renderer, actor, wgpuRenderer->GetRenderBundleEncoder());
      }
      else
      {
        this->RecordDrawCommands(renderer, actor, wgpuRenderer->GetRenderPassEncoder());
      }
      break;
    }
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
    state.LastPointSize = displayProperty->GetPointSize();
    state.LastLineWidth = displayProperty->GetLineWidth();
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
    if (auto* webgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer))
    {
      if (webgpuRenderer->GetUseRenderBundles())
      {
        if (state.LastPointSize != displayProperty->GetPointSize())
        {
          cacheChanged = true;
        }
        state.LastPointSize = displayProperty->GetPointSize();
        if (state.LastLineWidth != displayProperty->GetLineWidth())
        {
          cacheChanged = true;
        }
        state.LastLineWidth = displayProperty->GetLineWidth();
      }
    }
    return cacheChanged;
  }
}

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper::DrawCallArgs vtkWebGPUPolyDataMapper::GetDrawCallArgs(
  GraphicsPipelineType pipelineType,
  vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType)
{
  const auto& bgInfo = this->TopologyBindGroupInfos[topologySourceType];
  switch (topologySourceType)
  {
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS:
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS:
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS:
      if (pipelineType == GFX_PIPELINE_POINTS ||
        pipelineType == GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/1 };
      }
      if (pipelineType == GFX_PIPELINE_POINTS_SHAPED ||
        pipelineType == GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount };
      }
      break;
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINES:
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_EDGES:
      if (pipelineType == GFX_PIPELINE_LINES ||
        pipelineType == GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/1 };
      }
      if (pipelineType == GFX_PIPELINE_LINES_THICK ||
        pipelineType == GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount / 2 };
      }
      if (pipelineType == GFX_PIPELINE_LINES_MITER_JOIN ||
        pipelineType == GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*vertexCount=*/4, /*instanceCount=*/bgInfo.VertexCount / 2 };
      }
      if (pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN ||
        pipelineType == GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE)
      {
        return { /*vertexCount=*/36, /*instanceCount=*/bgInfo.VertexCount / 2 };
      }
      break;
    case vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS:
      return { /*vertexCount=*/bgInfo.VertexCount, /*instanceCount=*/1 };
    case vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES:
    default:
      break;
  }
  return {};
}

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper::DrawCallArgs vtkWebGPUPolyDataMapper::GetDrawCallArgsForDrawingVertices(
  vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType)
{
  const auto& bgInfo = this->TopologyBindGroupInfos[topologySourceType];
  return { /*VertexCount=*/4, /*InstanceCount=*/bgInfo.VertexCount };
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RecordDrawCommands(
  vtkRenderer* renderer, vtkActor* actor, const wgpu::RenderPassEncoder& passEncoder)
{
  vtkLogScopeFunction(TRACE);
  passEncoder.SetBindGroup(2, this->MeshAttributeBindGroup);

  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

  auto* displayProperty = actor->GetProperty();
  const float pointSize = displayProperty->GetPointSize();
  const float lineWidth = displayProperty->GetLineWidth();
  const auto lineJoinType = displayProperty->GetLineJoin();
  const int representation = displayProperty->GetRepresentation();
  const bool showVertices = displayProperty->GetVertexVisibility();

  for (const auto& pipelineMapping : PipelineBindGroupCombos[representation])
  {
    const auto& pipelineType = pipelineMapping.first;
    // Skip pipelines that are not supported.
    if (!this->IsPipelineSupported(pipelineType))
    {
      continue;
    }
    bool skip = false;
    switch (pipelineType)
    {
      case GFX_PIPELINE_POINTS:
        // do not draw points wider than 1 pixel using GFX_PIPELINE_POINTS pipeline.
        // instead, let the GFX_PIPELINE_POINTS_SHAPED pipeline render the points
        // if that is supported.
        skip = (pointSize > 1) && this->IsPipelineSupported(GFX_PIPELINE_POINTS_SHAPED);
        break;
      case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
        // do not draw points wider than 1 pixel using GFX_PIPELINE_POINTS pipeline.
        // instead, let the GFX_PIPELINE_POINTS_SHAPED pipeline render the points
        // if that is supported.
        skip = (pointSize > 1) &&
          this->IsPipelineSupported(GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE);
        break;
      case GFX_PIPELINE_POINTS_SHAPED:
      case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
        // Skip GFX_PIPELINE_POINTS_SHAPED for pointSize <= 1
        skip = pointSize <= 1;
        break;
      case GFX_PIPELINE_LINES:
        // do not draw lines wider than 1 pixel using GFX_PIPELINE_LINES pipeline.
        // instead, let the GFX_PIPELINE_LINES_THICK pipeline render the points.
        // GFX_PIPELINE_LINES_MITER_JOIN is used if "vtkProperty::UseMiterJoin" is turned on.
        // GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN is used if "vtkProperty::UseRoundCapRoundJoin" is
        // turned on.
        if (lineWidth > 1)
        {
          skip = this->IsPipelineSupported(GFX_PIPELINE_LINES_THICK) ||
            this->IsPipelineSupported(GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN) ||
            this->IsPipelineSupported(GFX_PIPELINE_LINES_MITER_JOIN);
        }
        break;
      case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
        // do not draw lines wider than 1 pixel using GFX_PIPELINE_LINES pipeline.
        // instead, let the GFX_PIPELINE_LINES_THICK pipeline render the points.
        // GFX_PIPELINE_LINES_MITER_JOIN is used if "vtkProperty::UseMiterJoin" is turned on.
        // GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN is used if "vtkProperty::UseRoundCapRoundJoin" is
        // turned on.
        if (lineWidth > 1)
        {
          skip = this->IsPipelineSupported(GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE) ||
            this->IsPipelineSupported(
              GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE) ||
            this->IsPipelineSupported(GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE);
        }
        break;
      case GFX_PIPELINE_LINES_THICK:
      case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
        skip = (lineWidth <= 1) || (lineJoinType != vtkProperty::LineJoinType::None);
        break;
      case GFX_PIPELINE_LINES_MITER_JOIN:
      case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
        skip = (lineWidth <= 1) || (lineJoinType != vtkProperty::LineJoinType::MiterJoin);
        break;
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
        skip = (lineWidth <= 1) || (lineJoinType != vtkProperty::LineJoinType::RoundCapRoundJoin);
        break;
      case GFX_PIPELINE_TRIANGLES:
      case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_NB_TYPES:
        break;
    }
    if (skip)
    {
      continue;
    }

    std::vector<vtkWebGPUCellToPrimitiveConverter::TopologySourceType> homogeneousBindGroupTypes,
      nonHomogeneousBindGroupTypes;
    for (const auto& bindGroupType : pipelineMapping.second)
    {
      const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
      if (bgInfo.VertexCount == 0)
      {
        continue;
      }
      if (bgInfo.CellIdBuffer == nullptr)
      {
        homogeneousBindGroupTypes.emplace_back(bindGroupType);
      }
      else
      {
        nonHomogeneousBindGroupTypes.emplace_back(bindGroupType);
      }
    }

    const auto& pipelineKey = this->GraphicsPipelineKeys[pipelineMapping.first];
    const auto& pipelineLabel =
      this->GetObjectDescription() + this->GetGraphicsPipelineTypeAsString(pipelineMapping.first);
    if (IsPipelineForHomogeneousCellSize(pipelineType))
    {
      if (!homogeneousBindGroupTypes.empty())
      {
        // Draw using homogeneous pipeline for bindgroups with homogeneous cells.
        passEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
        vtkScopedEncoderDebugGroup(passEncoder, pipelineLabel.c_str());
        for (const auto& bindGroupType : homogeneousBindGroupTypes)
        {
          const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
          passEncoder.SetBindGroup(3, bgInfo.BindGroup);
          const auto topologyBGInfoName =
            vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
          vtkScopedEncoderDebugGroup(passEncoder, topologyBGInfoName);
          const auto args = this->GetDrawCallArgs(pipelineType, bindGroupType);
          passEncoder.Draw(args.VertexCount, args.InstanceCount);
        }
      }
    }
    else if (!nonHomogeneousBindGroupTypes.empty())
    {
      // Draw using non-homogeneous pipeline for bindgroups with non-homogeneous cells.
      passEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      vtkScopedEncoderDebugGroup(passEncoder, pipelineLabel.c_str());
      for (const auto& bindGroupType : nonHomogeneousBindGroupTypes)
      {
        const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
        passEncoder.SetBindGroup(3, bgInfo.BindGroup);
        const auto topologyBGInfoName =
          vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
        vtkScopedEncoderDebugGroup(passEncoder, topologyBGInfoName);
        const auto args = this->GetDrawCallArgs(pipelineType, bindGroupType);
        passEncoder.Draw(args.VertexCount, args.InstanceCount);
      }
    }
  }
  if (showVertices && (representation != VTK_POINTS)) // Don't draw vertices on top of points.
  {
    std::vector<vtkWebGPUCellToPrimitiveConverter::TopologySourceType> homogeneousBindGroupTypes,
      nonHomogeneousBindGroupTypes;
    for (const auto& bindGroupType : { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS,
           vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS,
           vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS })
    {
      const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
      if (bgInfo.VertexCount == 0)
      {
        continue;
      }
      if (bgInfo.CellIdBuffer == nullptr)
      {
        homogeneousBindGroupTypes.emplace_back(bindGroupType);
      }
      else
      {
        nonHomogeneousBindGroupTypes.emplace_back(bindGroupType);
      }
    }
    if (!homogeneousBindGroupTypes.empty())
    {
      GraphicsPipelineType pipelineType;
      if (pointSize > 1 &&
        this->IsPipelineSupported(GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE))
      {
        pipelineType = GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE;
      }
      else
      {
        pipelineType = GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE;
      }
      const auto& pipelineKey = this->GraphicsPipelineKeys[pipelineType];
      const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(pipelineType);
      passEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      vtkScopedEncoderDebugGroup(passEncoder, pipelineLabel);
      for (const auto& bindGroupType : homogeneousBindGroupTypes)
      {
        const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
        passEncoder.SetBindGroup(3, bgInfo.BindGroup);
        const auto topologyBGInfoName =
          vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
        vtkScopedEncoderDebugGroup(passEncoder, topologyBGInfoName);
        const auto args = this->GetDrawCallArgsForDrawingVertices(bindGroupType);
        passEncoder.Draw(args.VertexCount, args.InstanceCount);
      }
    }
    if (!nonHomogeneousBindGroupTypes.empty())
    {
      GraphicsPipelineType pipelineType;
      if (pointSize > 1 && this->IsPipelineSupported(GFX_PIPELINE_POINTS_SHAPED))
      {
        pipelineType = GFX_PIPELINE_POINTS_SHAPED;
      }
      else
      {
        pipelineType = GFX_PIPELINE_POINTS;
      }
      const auto& pipelineKey = this->GraphicsPipelineKeys[pipelineType];
      const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(pipelineType);
      passEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      vtkScopedEncoderDebugGroup(passEncoder, pipelineLabel);
      for (const auto& bindGroupType : nonHomogeneousBindGroupTypes)
      {
        const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
        passEncoder.SetBindGroup(3, bgInfo.BindGroup);
        const auto topologyBGInfoName =
          vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
        vtkScopedEncoderDebugGroup(passEncoder, topologyBGInfoName);
        const auto args = this->GetDrawCallArgsForDrawingVertices(bindGroupType);
        passEncoder.Draw(args.VertexCount, args.InstanceCount);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::RecordDrawCommands(
  vtkRenderer* renderer, vtkActor* actor, const wgpu::RenderBundleEncoder& bundleEncoder)
{
  vtkLog(TRACE, "record draw commands to bundle");
  bundleEncoder.SetBindGroup(2, this->MeshAttributeBindGroup);

  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  auto* wgpuPipelineCache = wgpuRenderWindow->GetWGPUPipelineCache();

  auto* displayProperty = actor->GetProperty();
  const float pointSize = displayProperty->GetPointSize();
  const float lineWidth = displayProperty->GetLineWidth();
  const auto lineJoinType = displayProperty->GetLineJoin();
  const int representation = displayProperty->GetRepresentation();
  const bool showVertices = displayProperty->GetVertexVisibility();

  for (const auto& pipelineMapping : PipelineBindGroupCombos[representation])
  {
    const auto& pipelineType = pipelineMapping.first;
    // Skip pipelines that are not supported.
    if (!this->IsPipelineSupported(pipelineType))
    {
      continue;
    }
    bool skip = false;
    switch (pipelineType)
    {
      case GFX_PIPELINE_POINTS:
        // do not draw points wider than 1 pixel using GFX_PIPELINE_POINTS pipeline.
        // instead, let the GFX_PIPELINE_POINTS_SHAPED pipeline render the points
        // if that is supported.
        skip = (pointSize > 1) && this->IsPipelineSupported(GFX_PIPELINE_POINTS_SHAPED);
        break;
      case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
        // do not draw points wider than 1 pixel using GFX_PIPELINE_POINTS pipeline.
        // instead, let the GFX_PIPELINE_POINTS_SHAPED pipeline render the points
        // if that is supported.
        skip = (pointSize > 1) &&
          this->IsPipelineSupported(GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE);
        break;
      case GFX_PIPELINE_POINTS_SHAPED:
      case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
        // Skip GFX_PIPELINE_POINTS_SHAPED for pointSize <= 1
        skip = pointSize <= 1;
        break;
      case GFX_PIPELINE_LINES:
        // do not draw lines wider than 1 pixel using GFX_PIPELINE_LINES pipeline.
        // instead, let the GFX_PIPELINE_LINES_THICK pipeline render the points.
        // GFX_PIPELINE_LINES_MITER_JOIN is used if "vtkProperty::UseMiterJoin" is turned on.
        // GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN is used if "vtkProperty::UseRoundCapRoundJoin" is
        // turned on.
        if (lineWidth > 1)
        {
          skip = this->IsPipelineSupported(GFX_PIPELINE_LINES_THICK) ||
            this->IsPipelineSupported(GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN) ||
            this->IsPipelineSupported(GFX_PIPELINE_LINES_MITER_JOIN);
        }
        break;
      case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
        // do not draw lines wider than 1 pixel using GFX_PIPELINE_LINES pipeline.
        // instead, let the GFX_PIPELINE_LINES_THICK pipeline render the points.
        // GFX_PIPELINE_LINES_MITER_JOIN is used if "vtkProperty::UseMiterJoin" is turned on.
        // GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN is used if "vtkProperty::UseRoundCapRoundJoin" is
        // turned on.
        if (lineWidth > 1)
        {
          skip = this->IsPipelineSupported(GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE) ||
            this->IsPipelineSupported(
              GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE) ||
            this->IsPipelineSupported(GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE);
        }
        break;
      case GFX_PIPELINE_LINES_THICK:
      case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
        skip = (lineWidth <= 1) || (lineJoinType != vtkProperty::LineJoinType::None);
        break;
      case GFX_PIPELINE_LINES_MITER_JOIN:
      case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
        skip = (lineWidth <= 1) || (lineJoinType != vtkProperty::LineJoinType::MiterJoin);
        break;
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
        skip = (lineWidth <= 1) || (lineJoinType != vtkProperty::LineJoinType::RoundCapRoundJoin);
        break;
      case GFX_PIPELINE_TRIANGLES:
      case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_NB_TYPES:
        break;
    }
    if (skip)
    {
      continue;
    }

    std::vector<vtkWebGPUCellToPrimitiveConverter::TopologySourceType> homogeneousBindGroupTypes,
      nonHomogeneousBindGroupTypes;
    for (const auto& bindGroupType : pipelineMapping.second)
    {
      const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
      if (bgInfo.VertexCount == 0)
      {
        continue;
      }
      if (bgInfo.CellIdBuffer == nullptr)
      {
        homogeneousBindGroupTypes.emplace_back(bindGroupType);
      }
      else
      {
        nonHomogeneousBindGroupTypes.emplace_back(bindGroupType);
      }
    }

    const auto& pipelineKey = this->GraphicsPipelineKeys[pipelineMapping.first];
    const auto& pipelineLabel =
      this->GetObjectDescription() + this->GetGraphicsPipelineTypeAsString(pipelineMapping.first);
    if (IsPipelineForHomogeneousCellSize(pipelineType))
    {
      if (!homogeneousBindGroupTypes.empty())
      {
        // Draw using homogeneous pipeline for bindgroups with homogeneous cells.
        bundleEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
        vtkScopedEncoderDebugGroup(bundleEncoder, pipelineLabel.c_str());
        for (const auto& bindGroupType : homogeneousBindGroupTypes)
        {
          const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
          bundleEncoder.SetBindGroup(3, bgInfo.BindGroup);
          const auto topologyBGInfoName =
            vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
          vtkScopedEncoderDebugGroup(bundleEncoder, topologyBGInfoName);
          const auto args = this->GetDrawCallArgs(pipelineType, bindGroupType);
          bundleEncoder.Draw(args.VertexCount, args.InstanceCount);
        }
      }
    }
    else if (!nonHomogeneousBindGroupTypes.empty())
    {
      // Draw using non-homogeneous pipeline for bindgroups with non-homogeneous cells.
      bundleEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      vtkScopedEncoderDebugGroup(bundleEncoder, pipelineLabel.c_str());
      for (const auto& bindGroupType : nonHomogeneousBindGroupTypes)
      {
        const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
        bundleEncoder.SetBindGroup(3, bgInfo.BindGroup);
        const auto topologyBGInfoName =
          vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
        vtkScopedEncoderDebugGroup(bundleEncoder, topologyBGInfoName);
        const auto args = this->GetDrawCallArgs(pipelineType, bindGroupType);
        bundleEncoder.Draw(args.VertexCount, args.InstanceCount);
      }
    }
  }
  if (showVertices && (representation != VTK_POINTS)) // Don't draw vertices on top of points.
  {
    std::vector<vtkWebGPUCellToPrimitiveConverter::TopologySourceType> homogeneousBindGroupTypes,
      nonHomogeneousBindGroupTypes;
    for (const auto& bindGroupType : { vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_VERTS,
           vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_LINE_POINTS,
           vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGON_POINTS })
    {
      const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
      if (bgInfo.VertexCount == 0)
      {
        continue;
      }
      if (bgInfo.CellIdBuffer == nullptr)
      {
        homogeneousBindGroupTypes.emplace_back(bindGroupType);
      }
      else
      {
        nonHomogeneousBindGroupTypes.emplace_back(bindGroupType);
      }
    }
    if (!homogeneousBindGroupTypes.empty())
    {
      GraphicsPipelineType pipelineType;
      if (pointSize > 1 &&
        this->IsPipelineSupported(GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE))
      {
        pipelineType = GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE;
      }
      else
      {
        pipelineType = GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE;
      }
      const auto& pipelineKey = this->GraphicsPipelineKeys[pipelineType];
      const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(pipelineType);
      bundleEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      vtkScopedEncoderDebugGroup(bundleEncoder, pipelineLabel);
      for (const auto& bindGroupType : homogeneousBindGroupTypes)
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
        const auto args = this->GetDrawCallArgsForDrawingVertices(bindGroupType);
        bundleEncoder.Draw(args.VertexCount, args.InstanceCount);
      }
    }
    if (!nonHomogeneousBindGroupTypes.empty())
    {
      GraphicsPipelineType pipelineType;
      if (pointSize > 1 && this->IsPipelineSupported(GFX_PIPELINE_POINTS_SHAPED))
      {
        pipelineType = GFX_PIPELINE_POINTS_SHAPED;
      }
      else
      {
        pipelineType = GFX_PIPELINE_POINTS;
      }
      const auto& pipelineKey = this->GraphicsPipelineKeys[pipelineType];
      const auto& pipelineLabel = this->GetGraphicsPipelineTypeAsString(pipelineType);
      bundleEncoder.SetPipeline(wgpuPipelineCache->GetRenderPipeline(pipelineKey));
      vtkScopedEncoderDebugGroup(bundleEncoder, pipelineLabel);
      for (const auto& bindGroupType : nonHomogeneousBindGroupTypes)
      {
        const auto& bgInfo = this->TopologyBindGroupInfos[bindGroupType];
        bundleEncoder.SetBindGroup(3, bgInfo.BindGroup);
        const auto topologyBGInfoName =
          vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(bindGroupType);
        vtkScopedEncoderDebugGroup(bundleEncoder, topologyBGInfoName);
        const auto args = this->GetDrawCallArgsForDrawingVertices(bindGroupType);
        bundleEncoder.Draw(args.VertexCount, args.InstanceCount);
      }
    }
  }
}

//------------------------------------------------------------------------------
std::vector<wgpu::BindGroupLayoutEntry> vtkWebGPUPolyDataMapper::GetMeshBindGroupLayoutEntries()
{
  std::vector<wgpu::BindGroupLayoutEntry> entries;
  std::uint32_t bindingId = 0;
  for (int attributeIndex = 0; attributeIndex < POINT_NB_ATTRIBUTES; ++attributeIndex)
  {
    if (this->HasPointAttributes[attributeIndex])
    {
      entries.emplace_back(vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
        bindingId++, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage });
    }
  }
  if (this->HasPointAttributes[POINT_COLOR_UVS])
  {
    if (this->ColorTextureHostResource != nullptr)
    {
      if (auto devRc = this->ColorTextureHostResource->GetDeviceResource())
      {
        entries.emplace_back(
          devRc->MakeSamplerBindGroupLayoutEntry(bindingId++, wgpu::ShaderStage::Fragment));
        entries.emplace_back(
          devRc->MakeTextureViewBindGroupLayoutEntry(bindingId++, wgpu::ShaderStage::Fragment));
      }
    }
  }
  for (int attributeIndex = 0; attributeIndex < CELL_NB_ATTRIBUTES; ++attributeIndex)
  {
    if (this->HasCellAttributes[attributeIndex])
    {
      entries.emplace_back(vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
        bindingId++, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage });
    }
  }
  if (this->GetNumberOfClippingPlanes() > 0)
  {
    entries.emplace_back(vtkWebGPUBindGroupLayoutInternals::LayoutEntryInitializationHelper{
      bindingId++, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
      wgpu::BufferBindingType::ReadOnlyStorage });
  }
  return entries;
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUPolyDataMapper::CreateMeshAttributeBindGroupLayout(
  const wgpu::Device& device, const std::string& label)
{
  return vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(
    device, this->GetMeshBindGroupLayoutEntries(), label);
}

//------------------------------------------------------------------------------
wgpu::BindGroupLayout vtkWebGPUPolyDataMapper::CreateTopologyBindGroupLayout(
  const wgpu::Device& device, const std::string& label, bool homogeneousCellSize, bool useEdgeArray)
{
  if (homogeneousCellSize)
  {
    return vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      {
        // clang-format off
        // connectivity
        { 0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // cell_id_offset
        { 1, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::Uniform },
        // clang-format on
      },
      label);
  }
  if (useEdgeArray)
  {
    return vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      {
        // clang-format off
        // connectivity
        { 0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // cell_ids
        { 1, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // edge_array
        { 2, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // clang-format on
      },
      label);
  }
  else
  {
    return vtkWebGPUBindGroupLayoutInternals::MakeBindGroupLayout(device,
      {
        // clang-format off
        // connectivity
        { 0, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // cell_ids
        { 1, wgpu::ShaderStage::Vertex, wgpu::BufferBindingType::ReadOnlyStorage },
        // clang-format on
      },
      label);
  }
}

//------------------------------------------------------------------------------
std::vector<wgpu::BindGroupEntry> vtkWebGPUPolyDataMapper::GetMeshBindGroupEntries()
{
  std::vector<wgpu::BindGroupEntry> entries;
  std::uint32_t bindingId = 0;
  for (int attributeIndex = 0; attributeIndex < POINT_NB_ATTRIBUTES; ++attributeIndex)
  {
    if (this->HasPointAttributes[attributeIndex])
    {
      const auto initializer =
        vtkWebGPUBindGroupInternals::BindingInitializationHelper{ bindingId++,
          this->PointBuffers[attributeIndex].Buffer, 0 };
      entries.emplace_back(initializer.GetAsBinding());
    }
  }
  if (this->HasPointAttributes[POINT_COLOR_UVS])
  {
    if (this->ColorTextureHostResource != nullptr)
    {
      if (auto devRc = this->ColorTextureHostResource->GetDeviceResource())
      {
        entries.emplace_back(devRc->MakeSamplerBindGroupEntry(bindingId++));
        entries.emplace_back(devRc->MakeTextureViewBindGroupEntry(bindingId++));
      }
    }
  }
  for (int attributeIndex = 0; attributeIndex < CELL_NB_ATTRIBUTES; ++attributeIndex)
  {
    if (this->HasCellAttributes[attributeIndex])
    {
      const auto initializer =
        vtkWebGPUBindGroupInternals::BindingInitializationHelper{ bindingId++,
          this->CellBuffers[attributeIndex].Buffer, 0 };
      entries.emplace_back(initializer.GetAsBinding());
    }
  }
  if (this->GetNumberOfClippingPlanes() > 0)
  {
    const auto initializer = vtkWebGPUBindGroupInternals::BindingInitializationHelper{ bindingId++,
      this->ClippingPlanesBuffer, 0 };
    entries.emplace_back(initializer.GetAsBinding());
  }
  return entries;
}

//------------------------------------------------------------------------------
wgpu::BindGroup vtkWebGPUPolyDataMapper::CreateMeshAttributeBindGroup(
  const wgpu::Device& device, const std::string& label)
{
  auto layout = this->CreateMeshAttributeBindGroupLayout(device, label + "_LAYOUT");

  return vtkWebGPUBindGroupInternals::MakeBindGroup(
    device, layout, this->GetMeshBindGroupEntries(), label);
}

//------------------------------------------------------------------------------
wgpu::BindGroup vtkWebGPUPolyDataMapper::CreateTopologyBindGroup(const wgpu::Device& device,
  const std::string& label,
  vtkWebGPUCellToPrimitiveConverter::TopologySourceType topologySourceType)
{
  const auto& info = this->TopologyBindGroupInfos[topologySourceType];
  bool homogeneousCellSize = info.CellIdBuffer == nullptr;
  bool useEdgeArray = info.EdgeArrayBuffer != nullptr;
  auto layout = this->CreateTopologyBindGroupLayout(
    device, label + "_LAYOUT", homogeneousCellSize, useEdgeArray);
  if (homogeneousCellSize)
  {
    return vtkWebGPUBindGroupInternals::MakeBindGroup(device, layout,
      {
        { 0, info.ConnectivityBuffer, 0 },
        { 1, info.CellIdOffsetUniformBuffer, 0 },
      },
      label);
  }
  if (useEdgeArray)
  {
    return vtkWebGPUBindGroupInternals::MakeBindGroup(device, layout,
      {
        { 0, info.ConnectivityBuffer, 0 },
        { 1, info.CellIdBuffer, 0 },
        { 2, info.EdgeArrayBuffer, 0 },
      },
      label);
  }
  else
  {
    return vtkWebGPUBindGroupInternals::MakeBindGroup(device, layout,
      {
        { 0, info.ConnectivityBuffer, 0 },
        { 1, info.CellIdBuffer, 0 },
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

    case PointDataAttributes::POINT_COLOR_UVS:
      if (this->HasPointAttributes[attribute])
      {
        return this->ColorCoordinates->GetNumberOfValues() * sizeof(vtkTypeFloat32);
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

    case PointDataAttributes::POINT_COLOR_UVS:
      if (this->HasPointAttributes[attribute])
      {
        return this->ColorCoordinates->GetNumberOfComponents() * sizeof(vtkTypeFloat32);
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
unsigned long vtkWebGPUPolyDataMapper::GetExactPointBufferSize(
  vtkWebGPUPolyDataMapper::PointDataAttributes attribute)
{
  unsigned long result = 0;

  switch (attribute)
  {
    case POINT_POSITIONS:
      result = this->GetPointAttributeByteSize(POINT_POSITIONS);
      break;
    case POINT_COLORS:
      result = this->GetPointAttributeByteSize(POINT_COLORS);
      break;
    case POINT_NORMALS:
      result = this->GetPointAttributeByteSize(POINT_NORMALS);
      break;
    case POINT_TANGENTS:
      result = this->GetPointAttributeByteSize(POINT_TANGENTS);
      break;
    case POINT_UVS:
      result = this->GetPointAttributeByteSize(POINT_UVS);
      break;
    case POINT_COLOR_UVS:
      result = this->GetPointAttributeByteSize(POINT_COLOR_UVS);
      break;
    case POINT_NB_ATTRIBUTES:
    case POINT_UNDEFINED:
      break;
  }
  result = vtkWebGPUConfiguration::Align(result, 32);
  return result;
}

//------------------------------------------------------------------------------
unsigned long vtkWebGPUPolyDataMapper::GetExactCellBufferSize(
  vtkWebGPUPolyDataMapper::CellDataAttributes attribute)
{
  unsigned long result = 0;

  switch (attribute)
  {
    case CELL_COLORS:
      result = this->GetCellAttributeByteSize(CELL_COLORS);
      break;
    case CELL_NORMALS:
      result = this->GetCellAttributeByteSize(CELL_NORMALS);
      break;
    case CELL_NB_ATTRIBUTES:
    case CELL_UNDEFINED:
      break;
  }

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
  if (this->ColorCoordinates != nullptr && this->ColorCoordinates->GetNumberOfValues() > 0)
  {
    this->HasPointAttributes[POINT_COLOR_UVS] = true;
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
  vtkLogScopeFunction(TRACE);
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
  if (this->ColorTextureMap)
  {
    if (this->ColorTextureHostResource == nullptr)
    {
      this->ColorTextureHostResource = vtk::TakeSmartPointer(vtkWebGPUTexture::New());
      this->ColorTextureHostResource->RepeatOff();
    }
    this->ColorTextureHostResource->SetInputData(this->ColorTextureMap);
  }
  else
  {
    this->ColorTextureHostResource = nullptr;
  }
  this->DeducePointCellAttributeAvailability(this->CurrentInput);

  vtkPointData* pointData = this->CurrentInput->GetPointData();
  vtkDataArray* pointPositions = this->CurrentInput->GetPoints()->GetData();
  vtkDataArray* pointColors =
    this->HasPointAttributes[POINT_COLORS] ? vtkDataArray::SafeDownCast(this->Colors) : nullptr;
  vtkDataArray* pointNormals = pointData->GetNormals();
  vtkDataArray* pointTangents = pointData->GetTangents();
  vtkDataArray* pointUvs = pointData->GetTCoords();
  vtkDataArray* colorUvs =
    this->HasPointAttributes[POINT_COLOR_UVS] ? this->ColorCoordinates : nullptr;

  using DispatchT = vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::PointArrays>;

  auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();

  const char* pointAttribLabels[PointDataAttributes::POINT_NB_ATTRIBUTES] = { "point_coordinates",
    "point_colors", "point_normals", "point_tangents", "point_uvs", "point_color_uvs" };
  for (int attributeIndex = 0; attributeIndex < PointDataAttributes::POINT_NB_ATTRIBUTES;
       attributeIndex++)
  {
    uint64_t currentBufferSize = 0;
    const uint64_t requiredBufferSize =
      this->GetExactPointBufferSize(static_cast<PointDataAttributes>(attributeIndex));
    if (this->PointBuffers[attributeIndex].Buffer)
    {
      currentBufferSize = this->PointBuffers[attributeIndex].Size;
    }
    if (currentBufferSize != requiredBufferSize)
    {
      if (this->PointBuffers[attributeIndex].Buffer)
      {
        this->PointBuffers[attributeIndex].Buffer.Destroy();
        this->PointBuffers[attributeIndex].Size = 0;
      }
      wgpu::BufferDescriptor descriptor{};
      descriptor.size = requiredBufferSize;
      const auto label = pointAttribLabels[attributeIndex] + std::string("@") +
        this->CurrentInput->GetObjectDescription();
      descriptor.label = label.c_str();
      descriptor.mappedAtCreation = false;
      descriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
      this->PointBuffers[attributeIndex].Buffer = wgpuConfiguration->CreateBuffer(descriptor);
      this->PointBuffers[attributeIndex].Size = requiredBufferSize;
      // invalidate timestamp
      this->PointAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
      this->RebuildGraphicsPipelines = true;
    }
    ::WriteTypedArray<vtkTypeFloat32> attributeWriter{ this->PointBuffers[attributeIndex].Buffer,
      wgpuConfiguration, 1. };
    switch (PointDataAttributesOrder[attributeIndex])
    {
      case PointDataAttributes::POINT_POSITIONS:
        if (pointPositions->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(
                pointPositions, attributeWriter, pointAttribLabels[attributeIndex]))
          {
            attributeWriter(pointPositions, pointAttribLabels[attributeIndex]);
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
        }
        break;

      case PointDataAttributes::POINT_COLORS:
        attributeWriter.Denominator = 255.0f;
        if (pointColors &&
          pointColors->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(pointColors, attributeWriter, pointAttribLabels[attributeIndex]))
          {
            attributeWriter(pointColors, pointAttribLabels[attributeIndex]);
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
        }
        break;

      case PointDataAttributes::POINT_NORMALS:
        if (pointNormals &&
          pointNormals->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(pointNormals, attributeWriter, pointAttribLabels[attributeIndex]))
          {
            attributeWriter(pointNormals, pointAttribLabels[attributeIndex]);
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
        }
        break;

      case PointDataAttributes::POINT_TANGENTS:
        if (pointTangents &&
          pointTangents->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(
                pointTangents, attributeWriter, pointAttribLabels[attributeIndex]))
          {
            attributeWriter(pointTangents, pointAttribLabels[attributeIndex]);
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
        }
        break;

      case PointDataAttributes::POINT_UVS:
        if (pointUvs && pointUvs->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(pointUvs, attributeWriter, pointAttribLabels[attributeIndex]))
          {
            attributeWriter(pointUvs, pointAttribLabels[attributeIndex]);
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
        }
        break;
      case PointDataAttributes::POINT_COLOR_UVS:
        if (colorUvs && colorUvs->GetMTime() > this->PointAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(colorUvs, attributeWriter, pointAttribLabels[attributeIndex]))
          {
            attributeWriter(colorUvs, pointAttribLabels[attributeIndex]);
          }
          this->PointAttributesBuildTimestamp[attributeIndex].Modified();
        }
        break;

      default:
        break;
    }
  }

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

  const char* cellAttribLabels[CellDataAttributes::CELL_NB_ATTRIBUTES] = { "cell_colors",
    "cell_normals" };
  for (int attributeIndex = 0; attributeIndex < CellDataAttributes::CELL_NB_ATTRIBUTES;
       attributeIndex++)
  {
    uint64_t currentBufferSize = 0;
    const uint64_t requiredBufferSize =
      this->GetExactCellBufferSize(static_cast<CellDataAttributes>(attributeIndex));
    if (this->CellBuffers[attributeIndex].Buffer)
    {
      currentBufferSize = this->CellBuffers[attributeIndex].Size;
    }
    if (currentBufferSize != requiredBufferSize)
    {
      if (this->CellBuffers[attributeIndex].Buffer)
      {
        this->CellBuffers[attributeIndex].Buffer.Destroy();
        this->CellBuffers[attributeIndex].Size = 0;
      }
      wgpu::BufferDescriptor descriptor{};
      descriptor.size = requiredBufferSize;
      const auto label = cellAttribLabels[attributeIndex] + std::string("@") +
        this->CurrentInput->GetObjectDescription();
      descriptor.label = label.c_str();
      descriptor.mappedAtCreation = false;
      descriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
      this->CellBuffers[attributeIndex].Buffer = wgpuConfiguration->CreateBuffer(descriptor);
      this->CellBuffers[attributeIndex].Size = requiredBufferSize;
      // invalidate timestamp
      this->CellAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
      this->RebuildGraphicsPipelines = true;
    }
    ::WriteTypedArray<vtkTypeFloat32> attributeWriter{ this->CellBuffers[attributeIndex].Buffer,
      wgpuConfiguration, 1. };
    switch (CellDataAttributesOrder[attributeIndex])
    {
      case CellDataAttributes::CELL_COLORS:
      {
        attributeWriter.Denominator = 255.0f;
        if (cellColors &&
          cellColors->GetMTime() > this->CellAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(cellColors, attributeWriter, cellAttribLabels[attributeIndex]))
          {
            attributeWriter(cellColors, cellAttribLabels[attributeIndex]);
          }
          this->CellAttributesBuildTimestamp[attributeIndex].Modified();
        }
        break;
      }

      case CellDataAttributes::CELL_NORMALS:
      {
        if (cellNormals &&
          cellNormals->GetMTime() > this->CellAttributesBuildTimestamp[attributeIndex])
        {
          if (!DispatchT::Execute(cellNormals, attributeWriter, cellAttribLabels[attributeIndex]))
          {
            attributeWriter(cellNormals, cellAttribLabels[attributeIndex]);
          }
          this->CellAttributesBuildTimestamp[attributeIndex].Modified();
        }
        break;
      }

      default:
        break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::UpdateClippingPlanesBuffer(
  vtkWebGPUConfiguration* wgpuConfiguration, vtkActor* actor)
{
  if (this->GetNumberOfClippingPlanes() == 0)
  {
    // Release any previously allocated buffer
    if (this->ClippingPlanesBuffer)
    {
      this->ClippingPlanesBuffer.Destroy();
      this->ClippingPlanesBuffer = nullptr;
    }
    return;
  }
  if (this->GetNumberOfClippingPlanes() > 6)
  {
    // we only support up to 6 clipping planes.
    // this is a limitation of the shader code and the way we handle clipping planes.
    // if more than 6 clipping planes are needed, then we need to use a different approach.
    // for now, we just log a warning.
    vtkWarningMacro(<< "Too many clipping planes: " << this->GetNumberOfClippingPlanes()
                    << ", maximum is " << 6);
  }
  this->ClippingPlanesData.PlaneCount = std::min(this->GetNumberOfClippingPlanes(), 6);
  vtkMTimeType planesMTime = 0;
  for (vtkTypeUInt32 i = 0; i < this->ClippingPlanesData.PlaneCount; i++)
  {
    planesMTime = std::max(planesMTime, this->ClippingPlanes->GetItem(i)->GetMTime());
  }
  if (planesMTime < this->ClippingPlanesBuildTimestamp)
  {
    // no need to update the clipping planes buffer, it is already up to date.
    return;
  }
  // create a wgpu buffer for clipping planes if not already created.
  if (this->ClippingPlanesBuffer == nullptr)
  {
    const auto label = this->GetObjectDescription() + "-ClippingPlanesBuffer";
    wgpu::BufferDescriptor desc = {};
    desc.label = label.c_str();
    desc.mappedAtCreation = false;
    desc.size = vtkWebGPUConfiguration::Align(sizeof(this->ClippingPlanesData), 16);
    desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    this->ClippingPlanesBuffer = wgpuConfiguration->CreateBuffer(desc);
  }
  vtkNew<vtkMatrix4x4> modelToWorldMatrix;
  std::array<double, 3> scale = { 1, 1, 1 };
  std::array<double, 3> shift = { 0, 0, 0 };
  for (vtkTypeUInt32 i = 0; i < this->ClippingPlanesData.PlaneCount; i++)
  {
    double planeEquation[4];
    actor->GetModelToWorldMatrix(modelToWorldMatrix);
    this->GetClippingPlaneInDataCoords(modelToWorldMatrix, i, planeEquation);

    // multiply by shift scale if set
    this->ClippingPlanesData.PlaneEquations[i][0] = planeEquation[0] / scale[0];
    this->ClippingPlanesData.PlaneEquations[i][1] = planeEquation[1] / scale[1];
    this->ClippingPlanesData.PlaneEquations[i][2] = planeEquation[2] / scale[2];
    this->ClippingPlanesData.PlaneEquations[i][3] = planeEquation[3] + planeEquation[0] * shift[0] +
      planeEquation[1] * shift[1] + planeEquation[2] * shift[2];
  }
  this->ClippingPlanesBuildTimestamp.Modified();
  wgpuConfiguration->WriteBuffer(this->ClippingPlanesBuffer, 0, &this->ClippingPlanesData,
    sizeof(this->ClippingPlanesData), "ClippingPlanesBufferUpdate");
}

//------------------------------------------------------------------------------
const char* vtkWebGPUPolyDataMapper::GetGraphicsPipelineTypeAsString(
  GraphicsPipelineType graphicsPipelineType)
{
  switch (graphicsPipelineType)
  {
    case GFX_PIPELINE_POINTS:
      return "GFX_PIPELINE_POINTS";
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
      return "GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE";
    case GFX_PIPELINE_POINTS_SHAPED:
      return "GFX_PIPELINE_POINTS_SHAPED";
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      return "GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE";
    case GFX_PIPELINE_LINES:
      return "GFX_PIPELINE_LINES";
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
      return "GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE";
    case GFX_PIPELINE_LINES_THICK:
      return "GFX_PIPELINE_LINES_THICK";
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
      return "GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE";
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
      return "GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN";
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
      return "GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE";
    case GFX_PIPELINE_LINES_MITER_JOIN:
      return "GFX_PIPELINE_LINES_MITER_JOIN";
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
      return "GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE";
    case GFX_PIPELINE_TRIANGLES:
      return "GFX_PIPELINE_TRIANGLES";
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      return "GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE";
    case GFX_PIPELINE_NB_TYPES:
      break;
  }
  return "";
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::SetupGraphicsPipelines(
  const wgpu::Device& device, vtkRenderer* renderer, vtkActor* actor)
{
  vtkLogScopeFunction(TRACE);
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

  std::vector<wgpu::BindGroupLayout> basicBGLayoutEntries;
  wgpuRenderer->PopulateBindgroupLayouts(basicBGLayoutEntries);
  wgpuActor->Internals->PopulateBindgroupLayouts(basicBGLayoutEntries);
  basicBGLayoutEntries.emplace_back(this->CreateMeshAttributeBindGroupLayout(
    device, this->GetObjectDescription() + "MeshAttributeBindGroupLayout"));

  for (int i = 0; i < GFX_PIPELINE_NB_TYPES; ++i)
  {
    const auto pipelineType = static_cast<GraphicsPipelineType>(i);
    if (!this->IsPipelineSupported(pipelineType))
    {
      continue;
    }
    auto bgls = basicBGLayoutEntries;
    // add topology bind group layout.
    const bool homogeneousCellSize = IsPipelineForHomogeneousCellSize(pipelineType);
    const bool useEdgeArrray = pipelineType == GFX_PIPELINE_TRIANGLES ||
      pipelineType == GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE;
    bgls.emplace_back(this->CreateTopologyBindGroupLayout(device,
      this->GetObjectDescription() + "TopologyBindGroupLayout", homogeneousCellSize,
      useEdgeArrray));

    descriptor.layout = vtkWebGPUPipelineLayoutInternals::MakePipelineLayout(
      device, bgls, this->GetObjectDescription() + "-PipelineLayout");

    const auto label =
      this->GetObjectDescription() + this->GetGraphicsPipelineTypeAsString(pipelineType);
    descriptor.label = label.c_str();
    descriptor.primitive.topology = this->GetPrimitiveTopologyForPipeline(pipelineType);
    std::string vertexShaderSource = vtkPolyDataVSWGSL;
    std::string fragmentShaderSource = vtkPolyDataFSWGSL;
    this->ApplyShaderReplacements(
      pipelineType, wgpuRenderer, wgpuActor, vertexShaderSource, fragmentShaderSource);
    // generate a unique key for the pipeline descriptor and shader source pointer
    this->GraphicsPipelineKeys[i] = wgpuPipelineCache->GetPipelineKey(
      &descriptor, vertexShaderSource.c_str(), fragmentShaderSource.c_str());
    // create a pipeline if it does not already exist
    if (wgpuPipelineCache->GetRenderPipeline(this->GraphicsPipelineKeys[i]) == nullptr)
    {
      wgpuPipelineCache->CreateRenderPipeline(
        &descriptor, wgpuRenderWindow, vertexShaderSource.c_str(), fragmentShaderSource.c_str());
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ApplyShaderReplacements(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* wgpuRenderer, vtkWebGPUActor* wgpuActor, std::string& vss, std::string& fss)
{
  std::fill(this->NumberOfBindings.begin(), this->NumberOfBindings.end(), 0);

  this->ReplaceShaderConstantsDef(pipelineType, wgpuRenderer, wgpuActor, vss, fss);
  this->ReplaceShaderActorDef(pipelineType, wgpuRenderer, wgpuActor, vss, fss);
  this->ReplaceShaderCustomDef(pipelineType, wgpuRenderer, wgpuActor, vss, fss);
  this->ReplaceShaderClippingPlanesDef(pipelineType, wgpuRenderer, wgpuActor, vss, fss);

  this->ReplaceShaderRendererBindings(pipelineType, wgpuRenderer, wgpuActor, vss, fss);
  this->ReplaceShaderActorBindings(pipelineType, wgpuRenderer, wgpuActor, vss, fss);
  this->ReplaceShaderMeshAttributeBindings(pipelineType, wgpuRenderer, wgpuActor, vss, fss);
  this->ReplaceShaderClippingPlanesBindings(pipelineType, wgpuRenderer, wgpuActor, vss, fss);
  this->ReplaceShaderTopologyBindings(pipelineType, wgpuRenderer, wgpuActor, vss, fss);
  this->ReplaceShaderCustomBindings(pipelineType, wgpuRenderer, wgpuActor, vss, fss);

  this->ReplaceShaderVertexOutputDef(pipelineType, wgpuRenderer, wgpuActor, vss, fss);

  this->ReplaceVertexShaderInputDef(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderMainStart(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderCamera(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderNormalTransform(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderVertexId(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderPrimitiveId(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderCellId(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderPosition(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderClippingPlanes(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderPositionVC(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderPicking(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderColors(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderUVs(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderEdges(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderNormals(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderTangents(pipelineType, wgpuRenderer, wgpuActor, vss);
  this->ReplaceVertexShaderMainEnd(pipelineType, wgpuRenderer, wgpuActor, vss);

  this->ReplaceFragmentShaderOutputDef(pipelineType, wgpuRenderer, wgpuActor, fss);
  this->ReplaceFragmentShaderMainStart(pipelineType, wgpuRenderer, wgpuActor, fss);
  this->ReplaceFragmentShaderClippingPlanes(pipelineType, wgpuRenderer, wgpuActor, fss);
  this->ReplaceFragmentShaderColors(pipelineType, wgpuRenderer, wgpuActor, fss);
  this->ReplaceFragmentShaderNormals(pipelineType, wgpuRenderer, wgpuActor, fss);
  this->ReplaceFragmentShaderEdges(pipelineType, wgpuRenderer, wgpuActor, fss);
  this->ReplaceFragmentShaderLights(pipelineType, wgpuRenderer, wgpuActor, fss);
  this->ReplaceFragmentShaderPicking(pipelineType, wgpuRenderer, wgpuActor, fss);
  this->ReplaceFragmentShaderMainEnd(pipelineType, wgpuRenderer, wgpuActor, fss);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceShaderConstantsDef(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss, std::string& vtkNotUsed(fss))
{
  std::string code;
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
      break;
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
    {
      code = R"(
/**
* (-1, 1) |-------------------------------|(1, 1)
*         |-                              |
*         |    -                          |
*         |        -                      |
* (-1, 0) |              -                |
*         |                   -           |
*         |                        -      |
*         |                              -|
* (-1,-1) |-------------------------------|(1, -1)
*/
// this triangle strip describes a quad spanning a bi-unit domain.
const TRIANGLE_VERTS = array(
  vec2f(-1, -1),
  vec2f(1, -1),
  vec2f(-1, 1),
  vec2f(1, 1),
);)";
      break;
    }
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
      break;
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    {
      code = R"(
/**
  * (0, 0.5) |-------------------------------|(1, 0.5)
  *         /|-                              |\
  *       /  |    -                          |  \
  *     /  \ |        -                      | /  \
  * (0, 0)---|              -                |-----|
  *     \  / |                   -           |    /
  *       \  |                        -      | \ /
  *         \|                              -|/
  * (0,-0.5) |-------------------------------|(1, -0.5)
  * The semicircle points are generated using this python snippet.
    import math
    def generate_instance_round_round(tris, resolution):
      for step in range(resolution):
        theta0 = math.pi / 2 + (step * math.pi) / resolution
        theta1 = math.pi / 2 + ((step + 1) * math.pi) / resolution
        tris.append([0, 0, 0])
        tris.append([0.5 * math.cos(theta0), 0.5 * math.sin(theta0), 0])
        tris.append([0.5 * math.cos(theta1), 0.5 * math.sin(theta1), 0])
      for step in range(resolution):
        theta0 = (3 * math.pi) / 2 + (step * math.pi) / resolution
        theta1 = (3 * math.pi) / 2 + ((step + 1) * math.pi) / resolution
        tris.append([0, 0, 1])
        tris.append([0.5 * math.cos(theta0), 0.5 * math.sin(theta0), 1])
        tris.append([0.5 * math.cos(theta1), 0.5 * math.sin(theta1), 1])
    tris = []
    resolution = 10  # example resolution
    generate_instance_round_round(tris, resolution)
    print(tris)
  */
const TRIANGLE_VERTS = array(
  vec3(0, -0.5, 0),
  vec3(0, -0.5, 1),
  vec3(0, 0.5, 1),
  vec3(0, -0.5, 0),
  vec3(0, 0.5, 1),
  vec3(0, 0.5, 0),
  // left semicircle
  vec3(0, 0, 0),
  vec3(3.061616997868383e-17, 0.5, 0),
  vec3(-0.2938926261462365, 0.4045084971874737, 0),
  vec3(0, 0, 0),
  vec3(-0.2938926261462365, 0.4045084971874737, 0),
  vec3(-0.47552825814757677, 0.15450849718747375, 0),
  vec3(0, 0, 0),
  vec3(-0.47552825814757677, 0.15450849718747375, 0),
  vec3(-0.4755282581475768, -0.15450849718747364, 0),
  vec3(0, 0, 0),
  vec3(-0.4755282581475768, -0.15450849718747364, 0),
  vec3(-0.2938926261462366, -0.40450849718747367, 0),
  vec3(0, 0, 0),
  vec3(-0.2938926261462366, -0.40450849718747367, 0),
  vec3(-9.184850993605148e-17, -0.5, 0),
  // right semicircle
  vec3(0, 0, 1),
  vec3(-9.184850993605148e-17, -0.5, 1),
  vec3(0.29389262614623646, -0.4045084971874738, 1),
  vec3(0, 0, 1),
  vec3(0.29389262614623646, -0.4045084971874738, 1),
  vec3(0.47552825814757677, -0.1545084971874738, 1),
  vec3(0, 0, 1),
  vec3(0.47552825814757677, -0.1545084971874738, 1),
  vec3(0.4755282581475768, 0.1545084971874736, 1),
  vec3(0, 0, 1),
  vec3(0.4755282581475768, 0.1545084971874736, 1),
  vec3(0.2938926261462367, 0.4045084971874736, 1),
  vec3(0, 0, 1),
  vec3(0.2938926261462367, 0.4045084971874736, 1),
  vec3(1.5308084989341916e-16, 0.5, 1)
);)";
      break;
    }
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
    {
      code = R"(
/**
  * (0, 0.5) |-------------------------------|(1, 0.5)
  *          |-                              |
  *          |    -                          |
  *          |        -                      |
  * (0, 0)   |              -                |
  *          |                   -           |
  *          |                        -      |
  *          |                              -|
  * (0,-0.5) |-------------------------------|(1, -0.5)
  */
const TRIANGLE_VERTS = array(
  vec2(0, -0.5),
  vec2(1, -0.5),
  vec2(0, 0.5),
  vec2(1, 0.5),
);)";
      break;
    }
    case GFX_PIPELINE_TRIANGLES:
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_NB_TYPES:
      break;
  }
  if (!code.empty())
  {
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Constants::Def", code, /*all=*/true);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceShaderActorDef(GraphicsPipelineType vtkNotUsed(pipelineType),
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss, std::string& fss)
{
  const std::string code = R"(
struct Actor
{
  transform: ActorTransform,
  render_options: ActorRenderOptions,
  color_options: ActorColorOptions,
};
)";
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Actor::Def", code, /*all=*/true);
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Actor::Def", code, /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceShaderClippingPlanesDef(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss, std::string& fss)
{
  if (this->GetNumberOfClippingPlanes() == 0)
  {
    return;
  }
  const std::string code = R"(
struct ClippingPlanes
{
  plane_equations: array<vec4<f32>, 6>,
  count: u32,
};
)";
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::ClippingPlanes::Def", code, /*all=*/true);
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::ClippingPlanes::Def", code, /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceShaderCustomDef(GraphicsPipelineType vtkNotUsed(pipelineType),
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vtkNotUsed(vss), std::string& vtkNotUsed(fss))
{
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceShaderRendererBindings(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss, std::string& fss)
{
  auto& bindingId = this->NumberOfBindings[GROUP_RENDERER];

  std::stringstream codeStream;
  codeStream << "@group(" << GROUP_RENDERER << ") @binding(" << bindingId++
             << ") var<uniform> scene_transform: SceneTransform;\n";
  codeStream << "@group(" << GROUP_RENDERER << ") @binding(" << bindingId++
             << ") var<storage, read> scene_lights: SceneLights;\n";

  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::Renderer::Bindings", codeStream.str(), /*all=*/true);
  vtkWebGPURenderPipelineCache::Substitute(
    fss, "//VTK::Renderer::Bindings", codeStream.str(), /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceShaderActorBindings(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* wgpuActor, std::string& vss, std::string& fss)
{
  auto& bindingId = this->NumberOfBindings[GROUP_ACTOR];

  std::stringstream codeStream;
  codeStream << "@group(" << GROUP_ACTOR << ") @binding(" << bindingId++
             << ") var<storage, read> actor: Actor;";
  if (auto* wgpuTexture = vtkWebGPUTexture::SafeDownCast(wgpuActor->GetTexture()))
  {
    if (auto devRc = wgpuTexture->GetDeviceResource())
    {
      const char* textureSampleTypeStr =
        vtkWebGPURenderTextureDeviceResource::GetTextureSampleTypeString(devRc->GetSampleType());
      codeStream << "\n@group(" << GROUP_ACTOR << ") @binding(" << bindingId++
                 << ") var actor_texture_sampler: sampler;\n";
      codeStream << "@group(" << GROUP_ACTOR << ") @binding(" << bindingId++
                 << ") var actor_texture: texture_2d<" << textureSampleTypeStr << ">;\n";
    }
  }

  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::Actor::Bindings", codeStream.str(), /*all=*/true);
  vtkWebGPURenderPipelineCache::Substitute(
    fss, "//VTK::Actor::Bindings", codeStream.str(), /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceShaderClippingPlanesBindings(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss, std::string& fss)
{
  if (this->GetNumberOfClippingPlanes() == 0)
  {
    return;
  }
  auto& bindingId = this->NumberOfBindings[GROUP_CLIPPING_PLANES];

  std::stringstream codeStream;
  codeStream << "@group(" << GROUP_CLIPPING_PLANES << ") @binding(" << bindingId++
             << ") var<storage, read> clipping_planes: ClippingPlanes;";

  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::ClippingPlanes::Bindings", codeStream.str(), /*all=*/true);
  vtkWebGPURenderPipelineCache::Substitute(
    fss, "//VTK::ClippingPlanes::Bindings", codeStream.str(), /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceShaderMeshAttributeBindings(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss, std::string& fss)
{
  auto& bindingId = this->NumberOfBindings[GROUP_MESH];

  const char* pointAttributeLabels[POINT_NB_ATTRIBUTES] = { "point_coordinates", "point_colors",
    "point_normals", "point_tangents", "point_uvs", "point_color_uvs" };

  std::stringstream codeStream;
  for (int i = 0; i < POINT_NB_ATTRIBUTES; ++i)
  {
    if (this->HasPointAttributes[i])
    {
      codeStream << "@group(" << GROUP_MESH << ") @binding(" << bindingId++
                 << ") var<storage, read> " << pointAttributeLabels[i] << " : array<f32>;\n ";
    }
  }
  if (this->HasPointAttributes[POINT_COLOR_UVS])
  {
    if (auto deviceResource = this->ColorTextureHostResource->GetDeviceResource())
    {
      codeStream << "@group(" << GROUP_MESH << ") @binding(" << bindingId++
                 << ") var point_color_sampler: sampler;\n ";
      const char* textureSampleTypeStr =
        vtkWebGPURenderTextureDeviceResource::GetTextureSampleTypeString(
          deviceResource->GetSampleType());
      codeStream << "@group(" << GROUP_MESH << ") @binding(" << bindingId++
                 << ") var point_color_texture: texture_2d<" << textureSampleTypeStr << ">;\n";
    }
  }
  const char* cellAttributeLabels[CELL_NB_ATTRIBUTES] = { "cell_colors", "cell_normals" };
  for (int i = 0; i < CELL_NB_ATTRIBUTES; ++i)
  {
    if (this->HasCellAttributes[i])
    {
      codeStream << "@group(" << GROUP_MESH << ") @binding(" << bindingId++
                 << ") var<storage, read> " << cellAttributeLabels[i] << " : array<f32>;\n ";
    }
  }
  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::Mesh::Bindings", codeStream.str(), /*all=*/true);
  vtkWebGPURenderPipelineCache::Substitute(
    fss, "//VTK::Mesh::Bindings", codeStream.str(), /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceShaderCustomBindings(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vtkNotUsed(vss), std::string& vtkNotUsed(fss))
{
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceShaderTopologyBindings(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss, std::string& vtkNotUsed(fss))
{
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Topology::Bindings", R"(
      @group(3) @binding(0) var<storage, read> connectivity: array<u32>;
      @group(3) @binding(1) var<storage, read> cell_ids: array<u32>;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_TRIANGLES:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Topology::Bindings", R"(
    @group(3) @binding(0) var<storage, read> connectivity: array<u32>;
    @group(3) @binding(1) var<storage, read> cell_ids: array<u32>;
    @group(3) @binding(2) var<storage, read> edge_array: array<f32>;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Topology::Bindings", R"(
    @group(3) @binding(0) var<storage, read> connectivity: array<u32>;
    @group(3) @binding(1) var<uniform> cell_id_offset: u32;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_NB_TYPES:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderInputDef(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss)
{
  std::stringstream codeStream;
  codeStream << "struct VertexInput\n{\n";
  // Triangle pipeline does not need instance ID.
  if (pipelineType != GFX_PIPELINE_TRIANGLES)
  {
    codeStream << "  @builtin(instance_index) instance_id: u32,\n";
  }
  codeStream << "  @builtin(vertex_index) vertex_id: u32,\n};";
  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::VertexInput::Def", codeStream.str(), /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceShaderVertexOutputDef(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss, std::string& fss)
{
  std::stringstream codeStream;
  std::size_t id = 0;
  codeStream << "struct VertexOutput\n{\n";
  codeStream << "  @builtin(position) position: vec4<f32>,\n";
  codeStream << "  @location(" << id++ << ") color: vec4<f32>,\n";
  codeStream << "  @location(" << id++ << ") uv: vec2<f32>,\n";
  codeStream << "  @location(" << id++ << ") lut_uv: vec2<f32>,\n";
  codeStream << "  @location(" << id++ << ") position_VC: vec4<f32>,\n";
  codeStream << "  @location(" << id++ << ") normal_VC: vec3<f32>,\n";
  if (this->HasPointAttributes[POINT_TANGENTS])
  {
    codeStream << "  @location(" << id++ << ") tangent_VC: vec3<f32>,\n";
  }
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
      break;
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      codeStream << "  @location(" << id++ << ") p_coord: vec2<f32>,\n";
      break;
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
      break;
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
      codeStream << "  @location(" << id++ << ") dist_to_centerline: f32,\n";
      break;
    case GFX_PIPELINE_TRIANGLES:
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      codeStream << "  @location(" << id++ << ") edge_dists: vec3<f32>,\n";
      codeStream << "  @location(" << id++ << ") @interpolate(flat) hide_edge: f32,\n";
      break;
    case GFX_PIPELINE_NB_TYPES:
      break;
  }
  if (this->GetNumberOfClippingPlanes() > 0)
  {
    codeStream << "  @location(" << id++ << ") clip_dists_0: vec3<f32>,\n";
    codeStream << "  @location(" << id++ << ") clip_dists_1: vec3<f32>,\n";
  }
  codeStream << "  @location(" << id++ << ") @interpolate(flat) cell_id: u32,\n";
  codeStream << "  @location(" << id++ << ") @interpolate(flat) prop_id: u32,\n";
  codeStream << "  @location(" << id++ << ") @interpolate(flat) composite_id: u32,\n";
  codeStream << "  @location(" << id++ << ") @interpolate(flat) process_id: u32,\n};";
  vtkWebGPURenderPipelineCache::Substitute(
    vss, "//VTK::VertexOutput::Def", codeStream.str(), /*all=*/true);
  vtkWebGPURenderPipelineCache::Substitute(
    fss, "//VTK::VertexOutput::Def", codeStream.str(), /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderMainStart(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexMain::Start", R"(@vertex
fn vertexMain(vertex: VertexInput) -> VertexOutput
{
  var output: VertexOutput;
)",
    /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderCamera(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Camera::Impl",
    "let model_view_projection = scene_transform.projection * scene_transform.view * "
    "actor.transform.world;",
    /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderNormalTransform(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::NormalTransform::Impl",
    "let normal_model_view = scene_transform.normal * actor.transform.normal;",
    /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderVertexId(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss)
{
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
        "let pull_vertex_id: u32 = vertex.vertex_id;",
        /*all=*/true);
      break;
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
        R"(let pull_vertex_id: u32 = vertex.instance_id;
  let p_coord_id = vertex.vertex_id;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
        R"(let pull_vertex_id = vertex.vertex_id;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
        R"(let p_coord_id = vertex.vertex_id;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_TRIANGLES:
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexId::Impl",
        "let pull_vertex_id: u32 = vertex.vertex_id;",
        /*all=*/true);
      break;
    case GFX_PIPELINE_NB_TYPES:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderPrimitiveId(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss)
{
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::PrimitiveId::Impl",
        R"(let primitive_size: u32 = 1u;
  let primitive_id: u32 = pull_vertex_id;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::PrimitiveId::Impl",
        R"(let primitive_size: u32 = 2u;
  let primitive_id: u32 = pull_vertex_id / primitive_size;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::PrimitiveId::Impl",
        R"(let primitive_id: u32 = vertex.instance_id;
  let primitive_size: u32 = 2u;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_TRIANGLES:
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::PrimitiveId::Impl",
        R"(let primitive_size: u32 = 3u;
  let primitive_id: u32 = pull_vertex_id / primitive_size;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_NB_TYPES:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderCellId(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss)
{
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN:
    case GFX_PIPELINE_TRIANGLES:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::CellId::Impl",
        R"(let cell_id = cell_ids[primitive_id];)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::CellId::Impl",
        R"(let cell_id = primitive_id + cell_id_offset;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_NB_TYPES:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderPosition(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss)
{
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Position::Impl",
        R"(// pull the point id
  let point_id = connectivity[pull_vertex_id];
  // pull the position for this vertex.
  let vertex_MC = vec4<f32>(point_coordinates[3u * point_id], point_coordinates[3u * point_id + 1u], point_coordinates[3u * point_id + 2u], 1);
  // transform to view and then to clip space.
  let vertex_DC = model_view_projection * vertex_MC;
  // transform to 2-D screen plane.
  let resolution = scene_transform.viewport.zw;
  let vertex_screen = resolution * (0.5 * vertex_DC.xy / vertex_DC.w + 0.5);

  var point_size = actor.render_options.point_size;
  // The point rendering algorithm is unstable for point_size < 1.0
  if point_size < 1.0
  {
    point_size = 1.0;
  }
  output.p_coord = TRIANGLE_VERTS[p_coord_id];
  let adjusted_vertex_screen = vertex_screen + 0.5 * point_size * output.p_coord;
  output.position = vec4(vertex_DC.w * ((2.0 * adjusted_vertex_screen) / resolution - 1.0), vertex_DC.z, vertex_DC.w);)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Position::Impl",
        R"(var width: f32 = actor.render_options.line_width;
    if (width < 1.0)
    {
      // lines thinner than 1 pixel don't look good.
      width = 1.0;
    }
    let p_coord = TRIANGLE_VERTS[p_coord_id];
  
    let pull_vertex_id = select(2 * primitive_id, 2 * primitive_id + 1, p_coord.x == 1);
    let vertex_MC = vec4<f32>(point_coordinates[3u * connectivity[pull_vertex_id]],
      point_coordinates[3u * connectivity[pull_vertex_id] + 1u],
      point_coordinates[3u * connectivity[pull_vertex_id] + 2u], 1);
  
    // pull the point id
    let point_id = connectivity[pull_vertex_id];
  
    let p0_id: u32 = 2 * primitive_id;
    let p1_id = p0_id + 1;

    let p0_point_id: u32 = connectivity[p0_id];
    let p1_point_id: u32 = connectivity[p1_id];

    let p0_MC = vec4(point_coordinates[3u * p0_point_id], point_coordinates[3u * p0_point_id + 1u], point_coordinates[3u * p0_point_id + 2u], 1);
    let p1_MC = vec4(point_coordinates[3u * p1_point_id], point_coordinates[3u * p1_point_id + 1u], point_coordinates[3u * p1_point_id + 2u], 1);

    // transform to view and then to clip space.
    let p0_DC = model_view_projection * p0_MC;
    let p1_DC = model_view_projection * p1_MC;

    // transform to 2-D screen plane.
    let resolution = scene_transform.viewport.zw;
    let p0_screen = resolution * (0.5 * p0_DC.xy / p0_DC.w + 0.5);
    let p1_screen = resolution * (0.5 * p1_DC.xy / p1_DC.w + 0.5);

    // Expand the line segment into a quad by moving the vertices along X, and Y dimension
    // of the parametric space. 
    let x_basis = normalize(p1_screen - p0_screen);
    let y_basis = vec2(-x_basis.y, x_basis.x);

    let adjusted_p1 = p0_screen + p_coord.x * x_basis + p_coord.y * y_basis * width;
    let adjusted_p2 = p1_screen + p_coord.x * x_basis + p_coord.y * y_basis * width;
    let p = mix(adjusted_p1, adjusted_p2, p_coord.x);

    // used to select the z, w coordinate.
    let p_DC = mix(p0_DC, p1_DC, p_coord.x);

    output.position = vec4(p_DC.w * ((2.0 * p) / resolution - 1.0), p_DC.z, p_DC.w);
    output.dist_to_centerline = p_coord.y;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Position::Impl",
        R"(var width: f32 = actor.render_options.line_width;
  if (width < 1.0)
  {
    // lines thinner than 1 pixel don't look good.
    width = 1.0;
  }
  let p_coord = TRIANGLE_VERTS[p_coord_id];

  let p0_id: u32 = 2 * primitive_id;
  let p1_id = p0_id + 1;

  let p0_point_id: u32 = connectivity[p0_id];
  let p1_point_id: u32 = connectivity[p1_id];

  let p0_MC = vec4(point_coordinates[3u * p0_point_id], point_coordinates[3u * p0_point_id + 1u], point_coordinates[3u * p0_point_id + 2u], 1);
  let p1_MC = vec4(point_coordinates[3u * p1_point_id], point_coordinates[3u * p1_point_id + 1u], point_coordinates[3u * p1_point_id + 2u], 1);

  // transform to view and then to clip space.
  let p0_DC = model_view_projection * p0_MC;
  let p1_DC = model_view_projection * p1_MC;

  // transform to 2-D screen plane.
  let resolution = scene_transform.viewport.zw;
  let p0_screen = resolution * (0.5 * p0_DC.xy / p0_DC.w + 0.5);
  let p1_screen = resolution * (0.5 * p1_DC.xy / p1_DC.w + 0.5);

  let x_basis = normalize(p1_screen - p0_screen);
  let y_basis = vec2(-x_basis.y, x_basis.x);

  let adjusted_p1 = p0_screen + (p_coord.x * x_basis + p_coord.y * y_basis) * width;
  let adjusted_p2 = p1_screen + (p_coord.x * x_basis + p_coord.y * y_basis) * width;
  let p = mix(adjusted_p1, adjusted_p2, p_coord.z);

  // used to select the z, w coordinate.
  let p_DC = mix(p0_DC, p1_DC, p_coord.z);

  output.position = vec4(p_DC.w * ((2.0 * p) / resolution - 1.0), p_DC.z, p_DC.w);
  output.dist_to_centerline = p_coord.y;

  let pull_vertex_id = select(p0_id, p1_id, p_coord.z == 1);
  // pull the point id
  let point_id = connectivity[pull_vertex_id];
  let vertex_MC = vec4<f32>(point_coordinates[3u * point_id],
    point_coordinates[3u * point_id + 1u],
    point_coordinates[3u * point_id + 2u], 1);)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES_MITER_JOIN:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Position::Impl",
        R"(var width: f32 = actor.render_options.line_width;
  if (width < 1.0)
  {
    // lines thinner than 1 pixel don't look good.
    width = 1.0;
  }
  let p_coord = TRIANGLE_VERTS[p_coord_id];

  let pull_vertex_id = select(2 * primitive_id, 2 * primitive_id + 1, p_coord.x == 1);
  let vertex_MC = vec4<f32>(point_coordinates[3u * connectivity[pull_vertex_id]],
    point_coordinates[3u * connectivity[pull_vertex_id] + 1u],
    point_coordinates[3u * connectivity[pull_vertex_id] + 2u], 1);

  // pull the point id
  let point_id = connectivity[pull_vertex_id];

  var is_polyline_rl: bool = false; // whether polyline is going from right -> left in the connectivity buffer.
  var is_polyline_lr: bool = false; // whether polyline is going from left -> right in the connectivity buffer.
  var is_non_intersecting_vertex: bool = true;
  if (primitive_id > 0 && p_coord.x == 0)
  {
    is_polyline_rl = cell_ids[primitive_id - 1] == cell_ids[primitive_id];
  }
  else if (primitive_id < arrayLength(&cell_ids) - 1 && p_coord.x == 1)
  {
    is_polyline_lr = cell_ids[primitive_id + 1] == cell_ids[primitive_id];
  }
  if (is_polyline_lr || is_polyline_rl)
  {
    let p0_id = select(pull_vertex_id + 1, pull_vertex_id - 1, is_polyline_lr);
    let p1_id = pull_vertex_id;
    let p2_id = select(pull_vertex_id - 2, pull_vertex_id + 2, is_polyline_lr);

    var pos = p_coord;

    if (p_coord.x == 1)
    {
      pos = vec2(1.0 - p_coord.x, -p_coord.y);
    }

    let p0_point_id = connectivity[p0_id];
    let p1_point_id = connectivity[p1_id];
    let p2_point_id = connectivity[p2_id];

    let p0_MC = vec4<f32>(point_coordinates[3u * p0_point_id], point_coordinates[3u * p0_point_id + 1u], point_coordinates[3u * p0_point_id + 2u], 1);
    let p1_MC = vec4<f32>(point_coordinates[3u * p1_point_id], point_coordinates[3u * p1_point_id + 1u], point_coordinates[3u * p1_point_id + 2u], 1);
    let p2_MC = vec4<f32>(point_coordinates[3u * p2_point_id], point_coordinates[3u * p2_point_id + 1u], point_coordinates[3u * p2_point_id + 2u], 1);

    // transform to view and then to clip space.
    let p0_DC = model_view_projection * p0_MC;
    let p1_DC = model_view_projection * p1_MC;
    let p2_DC = model_view_projection * p2_MC;

    // transform to 2-D screen plane.
    let resolution = scene_transform.viewport.zw;
    let p0_screen = resolution * (0.5 * p0_DC.xy / p0_DC.w + 0.5);
    let p1_screen = resolution * (0.5 * p1_DC.xy / p1_DC.w + 0.5);
    let p2_screen = resolution * (0.5 * p2_DC.xy / p2_DC.w + 0.5);

    // Find the normal vector.
    let tangent = normalize(normalize(p2_screen - p1_screen) + normalize(p1_screen - p0_screen));
    let normal = vec2f(-tangent.y, tangent.x);

    // Find the vector perpendicular to p0_screen -> p1_screen.
    let p01 = p1_screen - p0_screen;
    let p21 = p1_screen - p2_screen;
    let p01_normal = normalize(vec2f(-p01.y, p01.x));

    // Determine the bend direction.
    let sigma = sign(dot(p01 + p21, normal));
    if (sign(pos.y) == sigma)
    {
      // This is an intersecting vertex. Adjust the position so that there's no overlap.
      let offset: vec2<f32> =  0.5 * width * -sigma * normal / dot(normal, p01_normal);
      if (length(offset) < min(length(p01), length(p21))) // clamp excessive offsets
      {
        let adjusted_pos: vec2<f32> = p1_screen + offset;
        output.position = vec4<f32>(p1_DC.w * ((2.0 * adjusted_pos) / resolution - 1.0), p1_DC.z, p1_DC.w);
        is_non_intersecting_vertex = false;
      }
    }
  }
  if (is_non_intersecting_vertex)
  {
    let p0_id: u32 = 2 * primitive_id;
    let p1_id = p0_id + 1;

    let p0_point_id: u32 = connectivity[p0_id];
    let p1_point_id: u32 = connectivity[p1_id];

    let p0_MC = vec4<f32>(point_coordinates[3u * p0_point_id], point_coordinates[3u * p0_point_id + 1u], point_coordinates[3u * p0_point_id + 2u], 1);
    let p1_MC = vec4<f32>(point_coordinates[3u * p1_point_id], point_coordinates[3u * p1_point_id + 1u], point_coordinates[3u * p1_point_id + 2u], 1);
    
    // transform to view and then to clip space.
    let p0_DC = model_view_projection * p0_MC;
    let p1_DC = model_view_projection * p1_MC;

    // transform to 2-D screen plane.
    let resolution = scene_transform.viewport.zw;
    let p0_screen = resolution * (0.5 * p0_DC.xy / p0_DC.w + 0.5);
    let p1_screen = resolution * (0.5 * p1_DC.xy / p1_DC.w + 0.5);

    // Expand the line segment into a quad by moving the vertices along X, and Y dimension
    // of the parametric space. 
    let x_basis = normalize(p1_screen - p0_screen);
    let y_basis = vec2(-x_basis.y, x_basis.x);
    let adjusted_p1 = p0_screen + p_coord.x * x_basis + p_coord.y * y_basis * width;
    let adjusted_p2 = p1_screen + p_coord.x * x_basis + p_coord.y * y_basis * width;
    let p = mix(adjusted_p1, adjusted_p2, p_coord.x);

    // used to select the z, w coordinate.
    let p_DC = mix(p0_DC, p1_DC, p_coord.x);

    output.position = vec4(p_DC.w * ((2.0 * p) / resolution - 1.0), p_DC.z, p_DC.w);
  }
  output.dist_to_centerline = p_coord.y;)",
        /*all=*/true);
      break;

    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Position::Impl",
        R"(var width: f32 = actor.render_options.line_width;
  if (width < 1.0)
  {
    // lines thinner than 1 pixel don't look good.
    width = 1.0;
  }
  let p_coord = TRIANGLE_VERTS[p_coord_id];

  let pull_vertex_id = select(2 * primitive_id, 2 * primitive_id + 1, p_coord.x == 1);
  let vertex_MC = vec4<f32>(point_coordinates[3u * connectivity[pull_vertex_id]],
    point_coordinates[3u * connectivity[pull_vertex_id] + 1u],
    point_coordinates[3u * connectivity[pull_vertex_id] + 2u], 1);

  // pull the point id
  let point_id = connectivity[pull_vertex_id];

  let p0_id: u32 = 2 * primitive_id;
  let p1_id = p0_id + 1;

  let p0_point_id: u32 = connectivity[p0_id];
  let p1_point_id: u32 = connectivity[p1_id];

  let p0_MC = vec4<f32>(point_coordinates[3u * p0_point_id], point_coordinates[3u * p0_point_id + 1u], point_coordinates[3u * p0_point_id + 2u], 1);
  let p1_MC = vec4<f32>(point_coordinates[3u * p1_point_id], point_coordinates[3u * p1_point_id + 1u], point_coordinates[3u * p1_point_id + 2u], 1);

  // transform to view and then to clip space.
  let p0_DC = model_view_projection * p0_MC;
  let p1_DC = model_view_projection * p1_MC;

  // transform to 2-D screen plane.
  let resolution = scene_transform.viewport.zw;
  let p0_screen = resolution * (0.5 * p0_DC.xy / p0_DC.w + 0.5);
  let p1_screen = resolution * (0.5 * p1_DC.xy / p1_DC.w + 0.5);

  // Expand the line segment into a quad by moving the vertices along X, and Y dimension
  // of the parametric space. 
  let x_basis = normalize(p1_screen - p0_screen);
  let y_basis = vec2(-x_basis.y, x_basis.x);
  let adjusted_p1 = p0_screen + p_coord.x * x_basis + p_coord.y * y_basis * width;
  let adjusted_p2 = p1_screen + p_coord.x * x_basis + p_coord.y * y_basis * width;
  let p = mix(adjusted_p1, adjusted_p2, p_coord.x);

  // used to select the z, w coordinate.
  let p_DC = mix(p0_DC, p1_DC, p_coord.x);

  output.position = vec4(p_DC.w * ((2.0 * p) / resolution - 1.0), p_DC.z, p_DC.w);
  output.dist_to_centerline = p_coord.y;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_TRIANGLES:
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Position::Impl",
        R"(// pull the point id
  let point_id = connectivity[pull_vertex_id];
  // pull the position for this vertex.
  let vertex_MC = vec4<f32>(point_coordinates[3u * point_id], point_coordinates[3u * point_id + 1u], point_coordinates[3u * point_id + 2u], 1);

  // NDC transforms
  output.position = model_view_projection * vertex_MC;)",
        /*all=*/true);
      break;
    case GFX_PIPELINE_NB_TYPES:
      break;
  }
}

void vtkWebGPUPolyDataMapper::ReplaceVertexShaderClippingPlanes(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss)
{
  if (this->GetNumberOfClippingPlanes() == 0)
  {
    return;
  }
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::ClippingPlanes::Impl",
    R"(for (var i: u32 = 0u; i < clipping_planes.count && i < 3u; i++)
    {
      let plane_eq = clipping_planes.plane_equations[i];
      output.clip_dists_0[i % 3u] = dot(plane_eq, vertex_MC);
    }
    for (var i: u32 = 3u; i < clipping_planes.count; i++)
    {
      let plane_eq = clipping_planes.plane_equations[i];
      output.clip_dists_1[i % 3u] = dot(plane_eq, vertex_MC);
    })",
    /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderPositionVC(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::PositionVC::Impl",
    "output.position_VC = scene_transform.inverted_projection * output.position;",
    /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderPicking(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Picking::Impl", R"(
    // Write indices
    output.cell_id = cell_id;
    output.prop_id = actor.color_options.id;
    output.composite_id = 0;
    output.process_id = 0;
    )",
    /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderColors(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss)
{
  if (this->HasPointAttributes[POINT_COLORS])
  {
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Colors::Impl", R"(
      // Smooth shading
      output.color = vec4<f32>(
        point_colors[4u * point_id],
        point_colors[4u * point_id + 1u],
        point_colors[4u * point_id + 2u],
        point_colors[4u * point_id + 3u],
      );
      )",
      /*all=*/true);
  }
  else if (this->HasCellAttributes[CELL_COLORS])
  {
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Colors::Impl", R"(
        // Flat shading
        output.color = vec4<f32>(
          cell_colors[4u * cell_id],
          cell_colors[4u * cell_id + 1u],
          cell_colors[4u * cell_id + 2u],
          cell_colors[4u * cell_id + 3u],
        );
      )",
      /*all=*/true);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderUVs(GraphicsPipelineType vtkNotUsed(pipelineType),
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss)
{
  std::stringstream codeStream;
  if (this->HasPointAttributes[POINT_UVS])
  {
    codeStream << R"(
      output.uv[0] = point_uvs[2u * point_id];
      output.uv[1] = point_uvs[2u * point_id + 1u];
    )";
  }
  if (this->HasPointAttributes[POINT_COLOR_UVS])
  {
    codeStream << R"(
      output.lut_uv[0] = point_color_uvs[2u * point_id];
      output.lut_uv[1] = point_color_uvs[2u * point_id + 1u];
    )";
  }
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::UVs::Impl", codeStream.str(), /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderEdges(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss)
{
  if (pipelineType == GFX_PIPELINE_TRIANGLES ||
    pipelineType == GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE)
  {
    std::stringstream codeStream;
    codeStream << R"(// Representation: VTK_SURFACE + Edge visibility turned on
  let representation = getRepresentation(actor.render_options.flags);
  let show_edges = getEdgeVisibility(actor.render_options.flags);
  if (representation == VTK_SURFACE && show_edges)
  {
    let i0 = primitive_id * 3u;
    let p0_point_id = connectivity[i0];
    let p1_point_id = connectivity[i0 + 1u];
    let p2_point_id = connectivity[i0 + 2u];

    let p0_MC = vec4<f32>(point_coordinates[3u * p0_point_id], point_coordinates[3u * p0_point_id + 1u], point_coordinates[3u * p0_point_id + 2u], 1);
    let p1_MC = vec4<f32>(point_coordinates[3u * p1_point_id], point_coordinates[3u * p1_point_id + 1u], point_coordinates[3u * p1_point_id + 2u], 1);
    let p2_MC = vec4<f32>(point_coordinates[3u * p2_point_id], point_coordinates[3u * p2_point_id + 1u], point_coordinates[3u * p2_point_id + 2u], 1);

    let p0_3D_DC: vec4<f32> = model_view_projection * p0_MC;
    let p1_3D_DC: vec4<f32> = model_view_projection * p1_MC;
    let p2_3D_DC: vec4<f32> = model_view_projection * p2_MC;
    let p0_DC: vec2<f32> = p0_3D_DC.xy / p0_3D_DC.w;
    let p1_DC: vec2<f32> = p1_3D_DC.xy / p1_3D_DC.w;
    let p2_DC: vec2<f32> = p2_3D_DC.xy / p2_3D_DC.w;
    let use_vertex_id: u32 = pull_vertex_id % 3u;
    let scale = scene_transform.viewport.zw * 0.5;)";
    if (pipelineType == GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE)
    {
      codeStream << R"(
    let edge_value: f32 = -1.0;)";
    }
    else
    {
      codeStream << R"(
    let edge_value: f32 = edge_array[primitive_id];)";
    }
    codeStream << R"(
    if use_vertex_id == 0u {
      let v10 = scale * (p1_DC - p0_DC);
      let v20 = scale * (p2_DC - p0_DC) ;
      let area0: f32 = abs(v10.x * v20.y - v10.y * v20.x);
      let h0: f32 = area0 / length(v10 - v20);
      output.edge_dists = vec3<f32>(h0 * p0_3D_DC.w, 0.0, 0.0);
    } else if use_vertex_id == 1u {
      let v01 = scale * (p0_DC - p1_DC);
      let v21 = scale * (p2_DC - p1_DC) ;
      let area1: f32 = abs(v01.x * v21.y - v01.y * v21.x);
      let h1: f32 = area1 / length(v01 - v21);
      output.edge_dists = vec3<f32>(0.0, h1 * p1_3D_DC.w, 0.0);
    } else if use_vertex_id == 2u {
      let v02 = scale * (p0_DC - p2_DC);
      let v12 = scale * (p1_DC - p2_DC) ;
      let area2: f32 = abs(v02.x * v12.y - v02.y * v12.x);
      let h2: f32 = area2 / length(v02 - v12);
      output.edge_dists = vec3<f32>(0.0, 0.0, h2 * p2_3D_DC.w);
    }
    output.hide_edge = edge_value;
  })";
    vtkWebGPURenderPipelineCache::Substitute(
      vss, "//VTK::Edges::Impl", codeStream.str(), /*all=*/true);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderNormals(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& vss)
{
  if (this->HasPointAttributes[POINT_NORMALS])
  {
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Normals::Impl",
      R"(// pull normal of this vertex from cell normals
  let normal_MC = vec3f(point_normals[3u * point_id], point_normals[3u * point_id + 1u], point_normals[3u * point_id + 2u]);
  output.normal_VC = normal_model_view * normal_MC;)",
      /*all=*/true);
  }
  else if (this->HasCellAttributes[CELL_NORMALS])
  {
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Normals::Impl",
      R"(// this if is after cell normals, so that when both are available, point normals are used.
  // pull normal of this vertex from point normals
  let normal_MC = vec3f(cell_normals[3u * cell_id], cell_normals[3u * cell_id + 1u], cell_normals[3u * cell_id + 2u]);
  output.normal_VC = normal_model_view * normal_MC;)",
      /*all=*/true);
  }
  else
  {
    switch (pipelineType)
    {
      case GFX_PIPELINE_POINTS:
      case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_POINTS_SHAPED:
      case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES:
      case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_THICK:
      case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
      case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
      case GFX_PIPELINE_LINES_MITER_JOIN:
      case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
        vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Normals::Impl",
          "output.normal_VC = vec3<f32>(0.0, 0.0, 1.0);",
          /*all=*/true);
        break;
      case GFX_PIPELINE_TRIANGLES:
      case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
        vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Normals::Impl",
          R"(let next_id: u32 = (pull_vertex_id + 1u) % 3u;
  let prev_id: u32 = (pull_vertex_id + 2u) % 3u;
  let next_point_id = connectivity[primitive_id * 3u + next_id];
  let prev_point_id = connectivity[primitive_id * 3u + prev_id];
  let next_MC = vec3f(point_coordinates[3u * next_point_id], point_coordinates[3u * next_point_id + 1u], point_coordinates[3u * next_point_id + 2u]);
  let prev_MC = vec3f(point_coordinates[3u * prev_point_id], point_coordinates[3u * prev_point_id + 1u], point_coordinates[3u * prev_point_id + 2u]);
  let normal_MC = computeFaceNormal(vertex_MC.xyz, next_MC, prev_MC);
  output.normal_VC = normal_model_view * normal_MC;)",
          /*all=*/true);
        break;
      case GFX_PIPELINE_NB_TYPES:
        break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderTangents(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss)
{
  if (this->HasPointAttributes[POINT_TANGENTS])
  {
    vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::Tangents::Impl",
      R"(let tangent_MC = vec3f(point_tangents[3u * point_id], point_tangents[3u * point_id + 1u], point_tangents[3u * point_id + 2u]);
  output.tangent_VC = normal_model_view * tangent_MC;)",
      /*all=*/true);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceVertexShaderMainEnd(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& vss)
{
  vtkWebGPURenderPipelineCache::Substitute(vss, "//VTK::VertexMain::End", R"( return output;
})",
    /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceFragmentShaderOutputDef(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& fss)
{
  const bool usesFragDepth = pipelineType == GFX_PIPELINE_POINTS_SHAPED ||
    pipelineType == GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE;
  if (usesFragDepth)
  {
    vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::FragmentOutput::Def",
      R"(struct FragmentOutput
{
  @builtin(frag_depth) frag_depth: f32,
  @location(0) color: vec4<f32>,
  @location(1) ids: vec4<u32>, // {cell, prop, composite, process}Id
};)",
      /*all=*/true);
  }
  else
  {
    vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::FragmentOutput::Def",
      R"(struct FragmentOutput
{
  @location(0) color: vec4<f32>,
  @location(1) ids: vec4<u32>, // {cell, prop, composite, process}Id
};)",
      /*all=*/true);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceFragmentShaderMainStart(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& fss)
{
  const std::string basicCode = R"(@fragment
fn fragmentMain(
  vertex: VertexOutput) -> FragmentOutput {
  var output: FragmentOutput;)";
  const std::string frontFacingCode = R"(@fragment
fn fragmentMain(
  @builtin(front_facing) is_front_facing: bool,
  vertex: VertexOutput) -> FragmentOutput {
  var output: FragmentOutput;)";

  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(
        fss, "//VTK::FragmentMain::Start", basicCode, /*all=*/true);
      break;
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(
        fss, "//VTK::FragmentMain::Start", frontFacingCode, /*all=*/true);
      break;
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(
        fss, "//VTK::FragmentMain::Start", basicCode, /*all=*/true);
      break;
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_TRIANGLES:
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(
        fss, "//VTK::FragmentMain::Start", frontFacingCode, /*all=*/true);
      break;
    case GFX_PIPELINE_NB_TYPES:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceFragmentShaderClippingPlanes(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& fss)
{
  if (this->GetNumberOfClippingPlanes() == 0)
  {
    return;
  }
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::ClippingPlanes::Impl",
    R"(for (var i: u32 = 0u; i < clipping_planes.count && i < 3u; i++)
    {
      if (vertex.clip_dists_0[i % 3u] < 0)
      {
        discard;
      }
    }
    for (var i: u32 = 3u; i < clipping_planes.count; i++)
    {
      if (vertex.clip_dists_1[i % 3u] < 0)
      {
        discard;
      }
    })",
    /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceFragmentShaderColors(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* wgpuActor, std::string& fss)
{
  std::string basicColorFSImpl = R"(var ambient_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var diffuse_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var specular_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var opacity: f32;
  ambient_color = actor.color_options.ambient_color;
  diffuse_color = actor.color_options.diffuse_color;
  opacity = actor.color_options.opacity;
  )";
  if (this->HasPointAttributes[POINT_COLORS] || this->HasCellAttributes[CELL_COLORS])
  {
    basicColorFSImpl += R"(
  ambient_color = vertex.color.rgb;
  diffuse_color = vertex.color.rgb;
  opacity = vertex.color.a;
)";
  }
  else if (this->HasPointAttributes[POINT_COLOR_UVS])
  {
    if (this->ColorTextureHostResource != nullptr)
    {
      if (this->ColorTextureHostResource->GetDeviceResource() != nullptr)
      {
        basicColorFSImpl += R"(
  let lut_tex_color: vec4<f32> = textureSample(point_color_texture, point_color_sampler, vertex.lut_uv);
  ambient_color = ambient_color * lut_tex_color.rgb;
  diffuse_color = diffuse_color * lut_tex_color.rgb;
  opacity = opacity * lut_tex_color.a;
)";
      }
    }
  }

  if (this->HasPointAttributes[POINT_UVS])
  {
    if (auto* wgpuTexture = vtkWebGPUTexture::SafeDownCast(wgpuActor->GetTexture()))
    {
      if (wgpuTexture->GetDeviceResource() != nullptr)
      {
        basicColorFSImpl += R"(
  let actor_tex_color: vec4<f32> = textureSample(actor_texture, actor_texture_sampler, vertex.uv);
  ambient_color = ambient_color * actor_tex_color.rgb;
  diffuse_color = diffuse_color * actor_tex_color.rgb;
  opacity = opacity * actor_tex_color.a;
)";
      }
    }
  }
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Colors::Impl",
        basicColorFSImpl +
          R"(// Colors are acquired either from a global per-actor color, or from per-vertex colors, or from cell colors.
  let show_vertices = getVertexVisibility(actor.render_options.flags);
  if (show_vertices)
  {
    // use vertex color instead of point scalar colors when drawing vertices.
    ambient_color = actor.color_options.vertex_color;
    diffuse_color = actor.color_options.vertex_color;
    opacity = actor.color_options.opacity;
  })",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_TRIANGLES:
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Colors::Impl", basicColorFSImpl,
        /*all=*/true);
      break;
    case GFX_PIPELINE_NB_TYPES:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceFragmentShaderNormals(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& fss)
{
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Normals::Impl",
        "var normal_VC: vec3<f32> = normalize(vertex.normal_VC);",
        /*all=*/true);
      break;
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Normals::Impl",
        R"(let d = length(vertex.p_coord); // distance of fragment from the input vertex.
  let shape = getPoint2DShape(actor.render_options.flags);
  let draw_spheres = getRenderPointsAsSpheres(actor.render_options.flags);
  if (((shape == POINT_2D_ROUND) || draw_spheres) && (d > 1))
  {
    discard;
  }

  let point_size = clamp(actor.render_options.point_size, 1.0f, 100000.0f);
  var normal_VC = normalize(vertex.normal_VC);
  if (draw_spheres)
  {
    if (d > 1)
    {
      discard;
    }
    normal_VC = normalize(vec3f(vertex.p_coord, 1));
    normal_VC.z = sqrt(1.0f - d * d);
    // Pushes the fragment in order to fake a sphere.
    // See Rendering/OpenGL2/PixelsToZBufferConversion.txt for the math behind this. Note that,
    // that document assumes the conventions for depth buffer in OpenGL,
    // where, the z-buffer spans [-1, 1]. In WebGPU, the depth buffer spans [0, 1].
    let r = point_size / (scene_transform.viewport.z * scene_transform.projection[0][0]);
    if (getUseParallelProjection(scene_transform.flags))
    {
      let s = scene_transform.projection[2][2];
      output.frag_depth = vertex.position.z + normal_VC.z * r * s;
    }
    else
    {
      let s = -scene_transform.projection[2][2];
      output.frag_depth = (s - vertex.position.z) / (normal_VC.z * r - 1.0) + s;
    }
  }
  else
  {
    output.frag_depth = vertex.position.z;
  })",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Normals::Impl",
        "var normal_VC: vec3<f32> = normalize(vertex.normal_VC);",
        /*all=*/true);
      break;
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Normals::Impl",
        R"(let dist_to_centerline = abs(vertex.dist_to_centerline);
  // adjust z component of normal in order to emulate a tube if necessary.
  var normal_VC: vec3<f32> = normalize(vertex.normal_VC);
  let draw_tubes = getRenderLinesAsTubes(actor.render_options.flags);
  if (draw_tubes)
  {
    normal_VC.z = 1.0 - 2.0 * dist_to_centerline;
  })",
        /*all=*/true);
      break;

    case GFX_PIPELINE_TRIANGLES:
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Normals::Impl",
        R"(var normal_VC: vec3<f32> = normalize(vertex.normal_VC);
  if !is_front_facing
  {
    if (normal_VC.z < 0.0)
    {
      normal_VC = -vertex.normal_VC;
      normal_VC = normalize(normal_VC);
    }
  }
  else if normal_VC.z < 0.0
  {
    normal_VC.z = -normal_VC.z;
  })",
        /*all=*/true);
      break;
    case GFX_PIPELINE_NB_TYPES:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceFragmentShaderEdges(GraphicsPipelineType pipelineType,
  vtkWebGPURenderer* vtkNotUsed(wgpuRenderer), vtkWebGPUActor* vtkNotUsed(wgpuActor),
  std::string& fss)
{
  if (pipelineType == GFX_PIPELINE_TRIANGLES ||
    pipelineType == GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE)
  {
    vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Edges::Impl",
      R"(// Representation: VTK_SURFACE with edge visibility turned on.
  let representation = getRepresentation(actor.render_options.flags);
  let show_edges = getEdgeVisibility(actor.render_options.flags);
  if (representation == VTK_SURFACE && show_edges)
  {
    let use_line_width_for_edge_thickness = getUseLineWidthForEdgeThickness(actor.render_options.flags);
    let line_width: f32 = select(actor.render_options.edge_width, actor.render_options.line_width, use_line_width_for_edge_thickness);
    // Undo perspective correction.
    let dists = vertex.edge_dists.xyz * vertex.position.w;
    var d: f32 = 0.0;
    // Compute the shortest distance to the edge
    if vertex.hide_edge == 2.0
    {
      d = min(dists[0], dists[2]);
    }
    else if vertex.hide_edge == 1.0
    {
      d = dists[0];
    }
    else if vertex.hide_edge == 0.0
    {
      d = min(dists[0], dists[1]);
    }
    else
    {
      // no edge is hidden
      d = min(dists[0], min(dists[1], dists[2]));
    }
    let half_line_width: f32 = 0.5 * line_width;
    let I: f32 = select(exp2(-2.0 * (d - half_line_width) * (d - half_line_width)), 1.0, d < half_line_width);
    diffuse_color = mix(diffuse_color, actor.color_options.edge_color, I);
    ambient_color = mix(ambient_color, actor.color_options.edge_color, I);

    let draw_tubes = getRenderLinesAsTubes(actor.render_options.flags);
    if (draw_tubes)
    {
      if (d < 1.1 * half_line_width)
      {
        // extend 10% to hide jagged artifacts on the edge-surface interface.
        normal_VC.z = 1.0 - (d / half_line_width);
      }
    }
  })",
      /*all=*/true);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceFragmentShaderLights(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Lights::Impl",
    R"(if scene_lights.count == 0u
  {
    // allow post-processing this pixel.
    output.color = vec4<f32>(
      actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color,
      actor.color_options.opacity * opacity
    );
  }
  else if scene_lights.count == 1u
  {
    let light: SceneLight = scene_lights.values[0];
    if light.positional == 1u
    {
      // TODO: positional
      output.color = vec4<f32>(
          actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color,
          actor.color_options.opacity * opacity
      );
    }
    else
    {
      // headlight
      let df: f32 = max(0.000001f, normal_VC.z);
      let sf: f32 = pow(df, actor.color_options.specular_power);
      diffuse_color = df * diffuse_color * light.color;
      specular_color = sf * actor.color_options.specular_intensity * actor.color_options.specular_color * light.color;
      output.color = vec4<f32>(
          actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color + specular_color,
          actor.color_options.opacity * opacity
      );
    }
  }
  else
  {
    // TODO: light kit
    output.color = vec4<f32>(
      actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color,
      opacity
    );
  }
  // pre-multiply colors
  output.color = vec4(output.color.rgb * opacity, opacity);)",
    /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceFragmentShaderPicking(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::Picking::Impl",
    R"(
    output.ids.x = vertex.cell_id + 1;
    output.ids.y = vertex.prop_id + 1;
    output.ids.z = vertex.composite_id + 1;
    output.ids.w = vertex.process_id + 1;)",
    /*all=*/true);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper::ReplaceFragmentShaderMainEnd(
  GraphicsPipelineType vtkNotUsed(pipelineType), vtkWebGPURenderer* vtkNotUsed(wgpuRenderer),
  vtkWebGPUActor* vtkNotUsed(wgpuActor), std::string& fss)
{
  vtkWebGPURenderPipelineCache::Substitute(fss, "//VTK::FragmentMain::End", R"(return output;
})",
    /*all=*/true);
}

//------------------------------------------------------------------------------
wgpu::PrimitiveTopology vtkWebGPUPolyDataMapper::GetPrimitiveTopologyForPipeline(
  GraphicsPipelineType pipelineType)
{
  wgpu::PrimitiveTopology topology = wgpu::PrimitiveTopology::Undefined;
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
      topology = wgpu::PrimitiveTopology::PointList;
      break;
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
      topology = wgpu::PrimitiveTopology::TriangleStrip;
      break;
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
      topology = wgpu::PrimitiveTopology::LineList;
      break;
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
      topology = wgpu::PrimitiveTopology::TriangleStrip;
      break;
    case GFX_PIPELINE_TRIANGLES:
      topology = wgpu::PrimitiveTopology::TriangleList;
      break;
    case GFX_PIPELINE_NB_TYPES:
      break;
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
      break;
  }
  return topology;
}

//------------------------------------------------------------------------------
bool vtkWebGPUPolyDataMapper::IsPipelineForHomogeneousCellSize(GraphicsPipelineType pipelineType)
{
  bool result = false;
  switch (pipelineType)
  {
    case GFX_PIPELINE_POINTS:
    case GFX_PIPELINE_POINTS_SHAPED:
    case GFX_PIPELINE_LINES:
    case GFX_PIPELINE_LINES_THICK:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN:
    case GFX_PIPELINE_LINES_MITER_JOIN:
    case GFX_PIPELINE_TRIANGLES:
      result = false;
      break;
    case GFX_PIPELINE_POINTS_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_POINTS_SHAPED_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_THICK_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_ROUND_CAP_ROUND_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_LINES_MITER_JOIN_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_TRIANGLES_HOMOGENEOUS_CELL_SIZE:
    case GFX_PIPELINE_NB_TYPES:
      result = true;
      break;
  }
  return result;
}

//------------------------------------------------------------------------------
bool vtkWebGPUPolyDataMapper::GetNeedToRebuildGraphicsPipelines(
  vtkActor* actor, vtkRenderer* renderer)
{
  if (this->RebuildGraphicsPipelines)
  {
    return true;
  }
  // have the clipping planes changed?
  if (this->LastNumClipPlanes != this->ClippingPlanesData.PlaneCount)
  {
    this->LastNumClipPlanes = this->ClippingPlanesData.PlaneCount;
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
  for (int attributeIndex = 0; attributeIndex < CELL_NB_ATTRIBUTES; ++attributeIndex)
  {
    this->CellBuffers[attributeIndex] = {};
    this->CellAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
  }
  for (int attributeIndex = 0; attributeIndex < POINT_NB_ATTRIBUTES; ++attributeIndex)
  {
    this->PointBuffers[attributeIndex] = {};
    this->PointAttributesBuildTimestamp[attributeIndex] = vtkTimeStamp();
  }
  if (this->ClippingPlanesBuffer)
  {
    this->ClippingPlanesBuffer.Destroy();
    this->ClippingPlanesBuffer = nullptr;
  }
  if (this->ColorTextureHostResource != nullptr)
  {
    this->ColorTextureHostResource->ReleaseGraphicsResources(w);
    this->ColorTextureHostResource = nullptr;
  }
  this->ClippingPlanesBuildTimestamp = vtkTimeStamp();
  this->LastScalarMode = -1;
  this->LastScalarVisibility = false;
  this->MeshAttributeBindGroup = nullptr;

  // Release topology conversion pipelines and reset their build timestamps.
  for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
  {
    this->TopologyBindGroupInfos[i] = TopologyBindGroupInfo{};
    this->IndirectDrawBufferUploadTimeStamp[i] = vtkTimeStamp();
  }
  this->CellConverter->ReleaseGraphicsResources(w);
  this->RebuildGraphicsPipelines = true;
  this->LastNumClipPlanes = VTK_TYPE_UINT32_MAX;
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
