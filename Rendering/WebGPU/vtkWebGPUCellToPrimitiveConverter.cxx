// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Hide VTK_DEPRECATED_IN_9_7_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "Private/vtkWebGPUComputeBufferInternals.h"
#include "VTKCellToGraphicsPrimitive.h"
#include "vtkABINamespace.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellType.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUComputePipeline.h"

VTK_ABI_NAMESPACE_BEGIN

namespace
{
const std::array<std::string, vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES>
  TopologyConversionShaderEntrypoints = { "poly_vertex_to_vertex", "poly_line_to_line",
    "cell_to_points", "polygon_to_triangle", "cell_to_points", "polygon_edges_to_lines" };

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
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUCellToPrimitiveConverter);

//------------------------------------------------------------------------------
vtkWebGPUCellToPrimitiveConverter::vtkWebGPUCellToPrimitiveConverter() = default;

//------------------------------------------------------------------------------
vtkWebGPUCellToPrimitiveConverter::~vtkWebGPUCellToPrimitiveConverter() = default;

//------------------------------------------------------------------------------
void vtkWebGPUCellToPrimitiveConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebGPUCellToPrimitiveConverter::ReleaseGraphicsResources(vtkWindow*)
{
  // Release topology conversion pipelines and reset their build timestamps.
  for (int i = 0; i < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES; ++i)
  {
    this->TopologyBuildTimestamp[i] = vtkTimeStamp();
    this->ComputePipelines[i] = nullptr;
    this->ComputePasses[i] = nullptr;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkWebGPUCellToPrimitiveConverter::GetTessellatedPrimitiveSizeOffsetForCellType(
  int cellType)
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
bool vtkWebGPUCellToPrimitiveConverter::GetNeedToRebuildCellToPrimitiveComputePipeline(
  vtkCellArray* cellArray, TopologySourceType topologySourceType)
{
  return cellArray->GetMTime() > this->TopologyBuildTimestamp[topologySourceType];
}

//------------------------------------------------------------------------------
void vtkWebGPUCellToPrimitiveConverter::UpdateCellToPrimitiveComputePipelineTimestamp(
  TopologySourceType topologySourceType)
{
  this->TopologyBuildTimestamp[topologySourceType].Modified();
}

//------------------------------------------------------------------------------
const char* vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeAsString(
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
const char* vtkWebGPUCellToPrimitiveConverter::GetCellTypeAsString(int cellType)
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
const char* vtkWebGPUCellToPrimitiveConverter::GetTessellatedPrimitiveTypeAsString(
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
std::size_t vtkWebGPUCellToPrimitiveConverter::GetTessellatedPrimitiveSize(
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
vtkWebGPUCellToPrimitiveConverter::TopologySourceType
vtkWebGPUCellToPrimitiveConverter::GetTopologySourceTypeForCellType(
  int cellType, int representation)
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
vtkWebGPUCellToPrimitiveConverter::CreateCellToPrimitiveComputePassForCellType(
  vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration, TopologySourceType topologySourceType)
{
  vtkSmartPointer<vtkWebGPUComputePass> pass;
  vtkSmartPointer<vtkWebGPUComputePipeline> pipeline;

  // Create compute pipeline.
  this->ComputePipelines[topologySourceType] =
    vtk::TakeSmartPointer(vtkWebGPUComputePipeline::New());
  pipeline = this->ComputePipelines[topologySourceType];
  pipeline->SetWGPUConfiguration(wgpuConfiguration);
  // Create compute pass.
  this->ComputePasses[topologySourceType] = pipeline->CreateComputePass();
  pass = this->ComputePasses[topologySourceType];
  pass->SetLabel(TopologyConversionShaderEntrypoints[topologySourceType]);
  pass->SetShaderSource(VTKCellToGraphicsPrimitive);
  pass->SetShaderEntryPoint(::TopologyConversionShaderEntrypoints[topologySourceType]);
  return std::make_pair(pass, pipeline);
}

//------------------------------------------------------------------------------
bool vtkWebGPUCellToPrimitiveConverter::DispatchMeshesToPrimitiveComputePipeline(
  vtkWebGPUConfiguration* wgpuConfiguration, std::vector<vtkPolyData*> meshes, int representation,
  std::vector<std::pair<vtkTypeUInt32, vtkTypeUInt32>>*
    vertexOffsetAndCounts[NUM_TOPOLOGY_SOURCE_TYPES],
  std::array<wgpu::Buffer*, NUM_TOPOLOGY_SOURCE_TYPES>& connectivityBuffers,
  std::array<wgpu::Buffer*, NUM_TOPOLOGY_SOURCE_TYPES>& cellIdBuffers,
  std::array<wgpu::Buffer*, NUM_TOPOLOGY_SOURCE_TYPES>& edgeArrayBuffers)
{
  bool buffersUpdated = false;
  std::array<int, 3> cellTypes{ VTK_POLY_VERTEX, VTK_POLY_LINE, VTK_POLYGON };
  std::vector<vtkCellArray*> cellArrayGroups[3]; // verts, lines, polys
  std::vector<vtkIdType> numberOfPoints;
  for (std::size_t i = 0; i < meshes.size(); ++i)
  {
    auto& mesh = meshes[i];
    cellArrayGroups[0].emplace_back(mesh->GetVerts());
    cellArrayGroups[1].emplace_back(mesh->GetLines());
    cellArrayGroups[2].emplace_back(mesh->GetPolys());
    numberOfPoints.emplace_back(mesh->GetNumberOfPoints());
  }
  for (int i = 0; i < 3; ++i)
  {
    const auto& cellArrays = cellArrayGroups[i];
    const int& vtkcellType = cellTypes[i];
    const auto sourceTopologyType =
      this->GetTopologySourceTypeForCellType(vtkcellType, representation);

    buffersUpdated |= this->DispatchCellArraysToPrimitiveComputePipeline(wgpuConfiguration,
      cellArrays, representation, vtkcellType, numberOfPoints,
      vertexOffsetAndCounts[sourceTopologyType], connectivityBuffers[sourceTopologyType],
      cellIdBuffers[sourceTopologyType], edgeArrayBuffers[sourceTopologyType]);
  }
  return buffersUpdated;
}

//------------------------------------------------------------------------------
bool vtkWebGPUCellToPrimitiveConverter::DispatchMeshToPrimitiveComputePipeline(
  vtkWebGPUConfiguration* wgpuConfiguration, vtkPolyData* mesh, int representation,
  const std::array<vtkTypeUInt32*, NUM_TOPOLOGY_SOURCE_TYPES>& vertexCounts,
  const std::array<wgpu::Buffer*, NUM_TOPOLOGY_SOURCE_TYPES>& connectivityBuffers,
  const std::array<wgpu::Buffer*, NUM_TOPOLOGY_SOURCE_TYPES>& cellIdBuffers,
  const std::array<wgpu::Buffer*, NUM_TOPOLOGY_SOURCE_TYPES>& edgeArrayBuffers,
  const std::array<wgpu::Buffer*, NUM_TOPOLOGY_SOURCE_TYPES>& cellIdOffsetUniformBuffers)
{
  bool buffersUpdated = false;
  vtkTypeUInt32 cellIdOffset = 0;
  {
    const auto idx = this->GetTopologySourceTypeForCellType(VTK_POLY_VERTEX, representation);
    buffersUpdated = this->DispatchCellArrayToPrimitiveComputePipeline(wgpuConfiguration,
      mesh ? mesh->GetVerts() : nullptr, representation, VTK_POLY_VERTEX, cellIdOffset,
      vertexCounts[idx], connectivityBuffers[idx], cellIdBuffers[idx], edgeArrayBuffers[idx],
      cellIdOffsetUniformBuffers[idx]);
    cellIdOffset += static_cast<vtkTypeUInt32>(mesh->GetNumberOfVerts());
  }
  // dispatch compute pipeline that converts polyline to lines.
  {
    const auto idx = this->GetTopologySourceTypeForCellType(VTK_POLY_LINE, representation);
    buffersUpdated |= this->DispatchCellArrayToPrimitiveComputePipeline(wgpuConfiguration,
      mesh ? mesh->GetLines() : nullptr, representation, VTK_POLY_LINE, cellIdOffset,
      vertexCounts[idx], connectivityBuffers[idx], cellIdBuffers[idx], edgeArrayBuffers[idx],
      cellIdOffsetUniformBuffers[idx]);
    cellIdOffset += static_cast<vtkTypeUInt32>(mesh->GetNumberOfLines());
  }
  // dispatch compute pipeline that converts polygon to triangles.
  {
    const auto idx = this->GetTopologySourceTypeForCellType(VTK_POLYGON, representation);
    buffersUpdated |= this->DispatchCellArrayToPrimitiveComputePipeline(wgpuConfiguration,
      mesh ? mesh->GetPolys() : nullptr, representation, VTK_POLYGON, cellIdOffset,
      vertexCounts[idx], connectivityBuffers[idx], cellIdBuffers[idx], edgeArrayBuffers[idx],
      cellIdOffsetUniformBuffers[idx]);
  }
  return buffersUpdated;
}

//------------------------------------------------------------------------------
bool vtkWebGPUCellToPrimitiveConverter::DispatchMeshToPrimitiveComputePipeline(
  vtkWebGPUConfiguration* wgpuConfiguration, vtkPolyData* mesh, int representation,
  vtkTypeUInt32* vertexCounts[NUM_TOPOLOGY_SOURCE_TYPES],
  wgpu::Buffer* connectivityBuffers[NUM_TOPOLOGY_SOURCE_TYPES],
  wgpu::Buffer* cellIdBuffers[NUM_TOPOLOGY_SOURCE_TYPES],
  wgpu::Buffer* edgeArrayBuffers[NUM_TOPOLOGY_SOURCE_TYPES],
  wgpu::Buffer* cellIdOffsetUniformBuffers[NUM_TOPOLOGY_SOURCE_TYPES])
{
  std::array<vtkTypeUInt32*, NUM_TOPOLOGY_SOURCE_TYPES> vertexCountsArr;
  std::copy_n(vertexCounts, NUM_TOPOLOGY_SOURCE_TYPES, vertexCountsArr.begin());
  std::array<wgpu::Buffer*, NUM_TOPOLOGY_SOURCE_TYPES> connectivityBuffersArr;
  std::copy_n(connectivityBuffers, NUM_TOPOLOGY_SOURCE_TYPES, connectivityBuffersArr.begin());
  std::array<wgpu::Buffer*, NUM_TOPOLOGY_SOURCE_TYPES> cellIdBuffersArr;
  std::copy_n(cellIdBuffers, NUM_TOPOLOGY_SOURCE_TYPES, cellIdBuffersArr.begin());
  std::array<wgpu::Buffer*, NUM_TOPOLOGY_SOURCE_TYPES> edgeArrayBuffersArr;
  std::copy_n(edgeArrayBuffers, NUM_TOPOLOGY_SOURCE_TYPES, edgeArrayBuffersArr.begin());
  std::array<wgpu::Buffer*, NUM_TOPOLOGY_SOURCE_TYPES> cellIdOffsetUniformBuffersArr;
  std::copy_n(
    cellIdOffsetUniformBuffers, NUM_TOPOLOGY_SOURCE_TYPES, cellIdOffsetUniformBuffersArr.begin());

  return this->DispatchMeshToPrimitiveComputePipeline(wgpuConfiguration, mesh, representation,
    vertexCountsArr, connectivityBuffersArr, cellIdBuffersArr, edgeArrayBuffersArr,
    cellIdOffsetUniformBuffersArr);
}

//------------------------------------------------------------------------------
bool vtkWebGPUCellToPrimitiveConverter::DispatchCellToPrimitiveComputePipeline(
  vtkWebGPUConfiguration* wgpuConfiguration, vtkCellArray* cells, int representation, int cellType,
  vtkTypeUInt32 cellIdOffset, vtkTypeUInt32* vertexCount, wgpu::Buffer* connectivityBuffer,
  wgpu::Buffer* cellIdBuffer, wgpu::Buffer* edgeArrayBuffer,
  wgpu::Buffer* cellIdOffsetUniformBuffer)
{
  return this->DispatchCellArrayToPrimitiveComputePipeline(wgpuConfiguration, cells, representation,
    cellType, cellIdOffset, vertexCount, connectivityBuffer, cellIdBuffer, edgeArrayBuffer,
    cellIdOffsetUniformBuffer);
}

//------------------------------------------------------------------------------
bool vtkWebGPUCellToPrimitiveConverter::DispatchCellArraysToPrimitiveComputePipeline(
  vtkWebGPUConfiguration* wgpuConfiguration, const std::vector<vtkCellArray*>& cellArrays,
  int representation, int cellType, const std::vector<vtkIdType>& numberOfPoints,
  std::vector<std::pair<vtkTypeUInt32, vtkTypeUInt32>>* vertexOffsetAndCounts,
  wgpu::Buffer* connectivityBuffer, wgpu::Buffer* cellIdBuffer, wgpu::Buffer* edgeArrayBuffer)
{
  vtkLogScopeFunction(TRACE);
  if (cellArrays.empty())
  {
    return false;
  }
  if (wgpuConfiguration == nullptr)
  {
    vtkErrorMacro(<< "WGPUConfiguration is null!");
    return false;
  }
  if (connectivityBuffer == nullptr)
  {
    vtkErrorMacro(<< "connectivityBuffer is null!");
    return false;
  }
  if (cellIdBuffer == nullptr)
  {
    vtkErrorMacro(<< "cellIdBuffer is null!");
    return false;
  }
  if (vertexOffsetAndCounts == nullptr)
  {
    vtkErrorMacro(<< "vertexOffsetAndCounts is null!");
  }
  const auto& batchSize = cellArrays.size();
  if (vertexOffsetAndCounts->size() != batchSize)
  {
    vtkErrorMacro(<< "size of vertexOffsetAndCounts must equal batch size " << batchSize);
    return false;
  }
  const auto idx = this->GetTopologySourceTypeForCellType(cellType, representation);
  vtkIdType totalNumberOfCells = 0;
  bool needToRebuild = false;
  vtkIdType fixedCellSize = 0, lastHomogenousCellSize = 0;
  bool hasHomogeneousCellSize = true;
  vtkIdType totalNumberOfIndices = 0;
  for (std::size_t i = 0; i < cellArrays.size(); ++i)
  {
    const auto& cellArray = cellArrays[i];
    fixedCellSize = cellArray->IsHomogeneous();
    if (fixedCellSize == 0)
    {
      // empty cell array
      continue;
    }
    totalNumberOfCells += cellArray->GetNumberOfCells();
    needToRebuild |= (this->GetNeedToRebuildCellToPrimitiveComputePipeline(cellArray, idx));
    if (i > 0)
    {
      hasHomogeneousCellSize &= (lastHomogenousCellSize == fixedCellSize);
    }
    auto* ids = cellArray->GetConnectivityArray();
    totalNumberOfIndices += ids->GetDataSize();
    lastHomogenousCellSize = fixedCellSize;
  }
  if (!needToRebuild)
  {
    return false;
  }
  if (totalNumberOfCells == 0)
  {
    return false;
  }
  const char* cellTypeAsString = this->GetCellTypeAsString(cellType);
  const char* primitiveTypeAsString = this->GetTessellatedPrimitiveTypeAsString(idx);
  const auto primitiveSize = this->GetTessellatedPrimitiveSize(idx);
  if (hasHomogeneousCellSize && (primitiveSize == static_cast<std::size_t>(fixedCellSize)))
  {
    // Clear existing buffers
    if (cellIdBuffer)
    {
      *cellIdBuffer = nullptr;
    }
    if (edgeArrayBuffer)
    {
      *edgeArrayBuffer = nullptr;
    }
    auto label = std::string("Connectivity-") + primitiveTypeAsString;
    wgpu::BufferDescriptor descriptor{};
    descriptor.label = label.c_str();
    descriptor.mappedAtCreation = false;
    descriptor.nextInChain = nullptr;
    descriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    descriptor.size = totalNumberOfIndices * sizeof(vtkTypeUInt32);
    *connectivityBuffer = wgpuConfiguration->CreateBuffer(descriptor);
    vtkIdType pointOffset = 0;
    std::vector<vtkTypeUInt32> idsToUpload;
    idsToUpload.reserve(totalNumberOfCells * primitiveSize);
    for (std::size_t i = 0; i < batchSize; ++i)
    {
      auto& cellArray = cellArrays[i];
      auto* connectivityArray = cellArray->GetConnectivityArray();
#if defined(VTK_USE_64BIT_IDS)
      if (connectivityArray->GetNumberOfValues() > static_cast<vtkIdType>(VTK_TYPE_UINT32_MAX))
      {
        vtkErrorMacro(
          << "Number of connectivity ids exceeds maximum possible unsigned 32-bit integer.");
        continue;
      }
#endif
      vtkTypeUInt32 arraySize = static_cast<vtkTypeUInt32>(connectivityArray->GetNumberOfValues());
      auto& [vertexOffset, vertexCount] = (*vertexOffsetAndCounts)[i];
      vertexCount = arraySize;
      vertexOffset = static_cast<vtkTypeUInt32>(idsToUpload.size());
#if defined(VTK_USE_64BIT_IDS)
      if (pointOffset > static_cast<vtkIdType>(VTK_TYPE_UINT32_MAX))
      {
        // pointOffset will be cast to a smaller type. warn once about it.
        static bool warnOnce = false;
        if (!warnOnce)
        {
          warnOnce = true;
          vtkWarningMacro(<< "pointOffset exceeds maximum possible unsigned 32-bit integer. Please "
                             "split your dataset into more number of blocks.");
        }
      }
#endif
      const auto pointOffsetU32 = static_cast<vtkTypeUInt32>(pointOffset);
      auto ids = vtk::DataArrayValueRange(connectivityArray);
      for (auto value : ids)
      {
        idsToUpload.emplace_back(value + pointOffsetU32);
      }
      pointOffset += numberOfPoints[i];
    }
    wgpuConfiguration->WriteBuffer(*connectivityBuffer, 0, idsToUpload.data(),
      static_cast<unsigned long>(idsToUpload.size() * sizeof(vtkTypeUInt32)), "Write connectivity");
  }
  else
  {
    // extra workgroups are fine to have.
    int nRequiredWorkGroups = std::ceil(totalNumberOfCells / 64.0);
    std::array<vtkTypeUInt32, 3> nWorkGroupsPerDimension = { 1, 1, 1 };
    if (::Factorize(nRequiredWorkGroups, nWorkGroupsPerDimension) == -1)
    {
      vtkErrorMacro(<< "Number of cellArray is too large to fit in available workgroups");
      return false;
    }
    vtkDebugMacro(<< "Dispatch " << cellTypeAsString
                  << " with workgroups=" << nWorkGroupsPerDimension[0] << 'x'
                  << nWorkGroupsPerDimension[1] << 'x' << nWorkGroupsPerDimension[2]);
    const auto primitiveSizeOffset = this->GetTessellatedPrimitiveSizeOffsetForCellType(cellType);
    std::vector<vtkTypeUInt32> primitiveCounts;
    std::vector<vtkTypeUInt32> idsToUpload;
    std::vector<vtkTypeUInt32> offsetsToUpload{ 0 };
    vtkTypeUInt32 numberOfPrimitives = 0;
    vtkIdType pointOffset = 0;
    for (std::size_t i = 0; i < batchSize; ++i)
    {
      auto& cellArray = cellArrays[i];
      auto& [vertexOffset, vertexCount] = (*vertexOffsetAndCounts)[i];
      vertexOffset = numberOfPrimitives * static_cast<vtkTypeUInt32>(primitiveSize);
#if defined(VTK_USE_64BIT_IDS)
      if (pointOffset > static_cast<vtkIdType>(VTK_TYPE_UINT32_MAX))
      {
        // pointOffset will be cast to a smaller type. warn once about it.
        static bool warnOnce = false;
        if (!warnOnce)
        {
          warnOnce = true;
          vtkWarningMacro(<< "pointOffset exceeds maximum possible unsigned 32-bit integer. Please "
                             "split your dataset into more number of blocks.");
        }
      }
#endif
      const auto applyPointOffset = [offset = static_cast<vtkTypeUInt32>(pointOffset)](
                                      vtkIdType pointId) -> vtkTypeUInt32
      { return pointId + offset; };
      auto cellIterator = vtk::TakeSmartPointer(cellArray->NewIterator());
      for (cellIterator->GoToFirstCell(); !cellIterator->IsDoneWithTraversal();
           cellIterator->GoToNextCell())
      {
        const vtkIdType* cellPts = nullptr;
        vtkIdType cellSize;
        cellIterator->GetCurrentCell(cellSize, cellPts);
        std::transform(
          cellPts, cellPts + cellSize, std::back_inserter(idsToUpload), applyPointOffset);
        offsetsToUpload.emplace_back(offsetsToUpload.back() + cellSize);
        primitiveCounts.emplace_back(numberOfPrimitives);
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
      } // end for cell
      vertexCount = numberOfPrimitives * static_cast<vtkTypeUInt32>(primitiveSize) - vertexOffset;
      pointOffset += numberOfPoints[i];
    } // end for element in batch
    primitiveCounts.emplace_back(numberOfPrimitives);
    // create input buffer for connectivity ids
    vtkNew<vtkWebGPUComputeBuffer> inputConnectivityBuffer;
    inputConnectivityBuffer->SetGroup(0);
    inputConnectivityBuffer->SetBinding(0);
    inputConnectivityBuffer->SetLabel(std::string("InputConnectivity-") + cellTypeAsString);
    inputConnectivityBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
    inputConnectivityBuffer->SetData(idsToUpload);

    // create input buffer for offsets
    vtkNew<vtkWebGPUComputeBuffer> offsetsBuffer;
    offsetsBuffer->SetGroup(0);
    offsetsBuffer->SetBinding(1);
    offsetsBuffer->SetLabel(std::string("Offsets") + cellTypeAsString);
    offsetsBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
    offsetsBuffer->SetData(offsetsToUpload);

    // create input buffer for primitive offsets
    vtkNew<vtkWebGPUComputeBuffer> primitiveCountsBuffer;
    primitiveCountsBuffer->SetGroup(0);
    primitiveCountsBuffer->SetBinding(2);
    primitiveCountsBuffer->SetLabel(std::string("PrimitiveCounts") + primitiveTypeAsString);
    primitiveCountsBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
    primitiveCountsBuffer->SetData(primitiveCounts);
    primitiveCountsBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

    std::vector<vtkTypeUInt32> uniformData = { static_cast<unsigned int>(0) };
    vtkNew<vtkWebGPUComputeBuffer> cellIfOffsetUniformComputeBuffer;
    cellIfOffsetUniformComputeBuffer->SetGroup(0);
    cellIfOffsetUniformComputeBuffer->SetBinding(3);
    cellIfOffsetUniformComputeBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER);
    cellIfOffsetUniformComputeBuffer->SetData(uniformData);
    cellIfOffsetUniformComputeBuffer->SetLabel(std::string("CellIdOffsets-") + cellTypeAsString);
    cellIfOffsetUniformComputeBuffer->SetDataType(
      vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

    vtkNew<vtkWebGPUComputeBuffer> outputConnectivityBuffer;
    outputConnectivityBuffer->SetGroup(0);
    outputConnectivityBuffer->SetBinding(4);
    outputConnectivityBuffer->SetLabel(std::string("OutputConnectivity") + primitiveTypeAsString);
    outputConnectivityBuffer->SetMode(
      vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
    outputConnectivityBuffer->SetByteSize(
      numberOfPrimitives * primitiveSize * sizeof(vtkTypeUInt32));

    vtkNew<vtkWebGPUComputeBuffer> outputCellIdBuffer;
    outputCellIdBuffer->SetGroup(0);
    outputCellIdBuffer->SetBinding(5);
    outputCellIdBuffer->SetLabel(std::string("OutputCellId") + primitiveTypeAsString);
    outputCellIdBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
    outputCellIdBuffer->SetByteSize(numberOfPrimitives * sizeof(vtkTypeUInt32));

    // handle optional edge visibility.
    // this lets fragment shader hide internal edges of a polygon
    // when edge visibility is turned on.
    vtkNew<vtkWebGPUComputeBuffer> edgeArrayComputeBuffer;
    edgeArrayComputeBuffer->SetGroup(0);
    edgeArrayComputeBuffer->SetBinding(6);
    edgeArrayComputeBuffer->SetLabel(std::string("EdgeArray-") + primitiveTypeAsString);
    edgeArrayComputeBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_COMPUTE_STORAGE);
    if (primitiveSize == 3)
    {
      edgeArrayComputeBuffer->SetByteSize(numberOfPrimitives * sizeof(vtkTypeUInt32));
    }
    else
    {
      // placeholder must be aligned to 32-bit boundary
      edgeArrayComputeBuffer->SetByteSize(sizeof(vtkTypeUInt32));
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
      return false;
    }

    // add buffers one by one to the compute pass.
    pass->AddBuffer(inputConnectivityBuffer);
    pass->AddBuffer(offsetsBuffer);
    pass->AddBuffer(primitiveCountsBuffer);
    pass->AddBuffer(cellIfOffsetUniformComputeBuffer); // not used at all.
    pass->AddBuffer(outputConnectivityBuffer);         // not used in cpu
    pass->AddBuffer(outputCellIdBuffer);               // not used in cpu
    pass->AddBuffer(edgeArrayComputeBuffer);           // not used in cpu
    // the topology and edge_array buffers are populated by compute pipeline and their contents
    // read in the graphics pipeline within the vertex and fragment shaders
    // keep reference to the output of compute pipeline, reuse it in the vertex, fragment shaders.
    if (!pipeline->GetRegisteredBuffer(outputConnectivityBuffer, *connectivityBuffer))
    {
      vtkErrorMacro(<< "Failed to get registered buffer for output connectivity.");
    }
    if (!pipeline->GetRegisteredBuffer(outputCellIdBuffer, *cellIdBuffer))
    {
      vtkErrorMacro(<< "Failed to get registered buffer for output cell ids.");
    }
    // do the same for the edge array buffer only when output primitives are triangles.
    if (edgeArrayBuffer)
    {
      if (primitiveSize == 3)
      {
        if (!pipeline->GetRegisteredBuffer(edgeArrayComputeBuffer, *edgeArrayBuffer))
        {
          vtkErrorMacro(<< "edgeArray compute buffer for " << primitiveTypeAsString
                        << " is not registered!");
        }
      }
      else
      {
        *edgeArrayBuffer = nullptr;
      }
    }
    // dispatch
    pass->SetWorkgroups(
      nWorkGroupsPerDimension[0], nWorkGroupsPerDimension[1], nWorkGroupsPerDimension[2]);
    pass->Dispatch();
  }

  // update build timestamp
  this->UpdateCellToPrimitiveComputePipelineTimestamp(idx);

  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUCellToPrimitiveConverter::DispatchCellArrayToPrimitiveComputePipeline(
  vtkWebGPUConfiguration* wgpuConfiguration, vtkCellArray* cellArray, int representation,
  int cellType, vtkTypeUInt32 cellIdOffset, vtkTypeUInt32* vertexCount,
  wgpu::Buffer* connectivityBuffer, wgpu::Buffer* cellIdBuffer, wgpu::Buffer* edgeArrayBuffer,
  wgpu::Buffer* cellIdOffsetUniformBuffer)
{
  if (wgpuConfiguration == nullptr)
  {
    vtkErrorMacro(<< "WGPUConfiguration is null!");
    return false;
  }
  if (vertexCount == nullptr)
  {
    vtkErrorMacro(<< "vertexCount is null!");
    return false;
  }
  if (connectivityBuffer == nullptr)
  {
    vtkErrorMacro(<< "connectivityBuffer is null!");
    return false;
  }
  if (cellIdBuffer == nullptr)
  {
    vtkErrorMacro(<< "cellIdBuffer is null!");
    return false;
  }
  if (cellIdOffsetUniformBuffer == nullptr)
  {
    vtkErrorMacro(<< "cellIdOffsetUniformBuffer is null!");
    return false;
  }

  const auto idx = this->GetTopologySourceTypeForCellType(cellType, representation);

  if (!cellArray)
  {
    *vertexCount = 0;
    return false;
  }
  if (cellArray->GetNumberOfCells() == 0)
  {
    *vertexCount = 0;
    return false;
  }
  if (!this->GetNeedToRebuildCellToPrimitiveComputePipeline(cellArray, idx))
  {
    return false;
  }

  const vtkIdType fixedCellSize = cellArray->IsHomogeneous();
  const bool convertedTo32Bit = fixedCellSize > 0 ? cellArray->ConvertToFixedSize32BitStorage()
                                                  : cellArray->ConvertTo32BitStorage();
  if (!convertedTo32Bit)
  {
    vtkErrorMacro(<< "Failed to convert cell array storage to 32-bit");
    return false;
  }
  const char* cellTypeAsString = this->GetCellTypeAsString(cellType);
  const char* primitiveTypeAsString = this->GetTessellatedPrimitiveTypeAsString(idx);
  const auto primitiveSize = this->GetTessellatedPrimitiveSize(idx);
  if (primitiveSize == static_cast<std::size_t>(fixedCellSize))
  {
    auto* ids = cellArray->GetConnectivityArray();

    *vertexCount = ids->GetNumberOfValues();

    if (cellIdBuffer)
    {
      *cellIdBuffer = nullptr;
    }
    if (edgeArrayBuffer)
    {
      *edgeArrayBuffer = nullptr;
    }

    auto label = std::string("Connectivity-") + primitiveTypeAsString + "@" +
      cellArray->GetObjectDescription();
    *connectivityBuffer =
      wgpuConfiguration->CreateBuffer(ids->GetDataSize() * ids->GetDataTypeSize(),
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, false, label.c_str());

    label = std::string("CellIdOffsetUniform-") + primitiveTypeAsString + "@" +
      cellArray->GetObjectDescription();
    *cellIdOffsetUniformBuffer = wgpuConfiguration->CreateBuffer(sizeof(vtkTypeUInt32),
      wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, false, label.c_str());

    vtkWebGPUComputeBufferInternals::UploadFromDataArray(
      wgpuConfiguration, *connectivityBuffer, ids, "Write connectivity");

    wgpuConfiguration->WriteBuffer(
      *cellIdOffsetUniformBuffer, 0, &cellIdOffset, sizeof(vtkTypeUInt32), "Write cellIdOffset");
  }
  else
  {
    // extra workgroups are fine to have.
    int nRequiredWorkGroups = std::ceil(cellArray->GetNumberOfCells() / 64.0);
    std::array<vtkTypeUInt32, 3> nWorkGroupsPerDimension = { 1, 1, 1 };
    if (::Factorize(nRequiredWorkGroups, nWorkGroupsPerDimension) == -1)
    {
      vtkErrorMacro(<< "Number of cellArray is too large to fit in available workgroups");
      return false;
    }
    vtkDebugMacro(<< "Dispatch " << cellTypeAsString
                  << " with workgroups=" << nWorkGroupsPerDimension[0] << 'x'
                  << nWorkGroupsPerDimension[1] << 'x' << nWorkGroupsPerDimension[2]);

    const vtkIdType primitiveSizeOffset =
      this->GetTessellatedPrimitiveSizeOffsetForCellType(cellType);
    vtkTypeUInt32 numberOfPrimitives = 0;
    std::vector<vtkTypeUInt32> primitiveCounts;
    auto cellIterator = vtk::TakeSmartPointer(cellArray->NewIterator());
    for (cellIterator->GoToFirstCell(); !cellIterator->IsDoneWithTraversal();
         cellIterator->GoToNextCell())
    {
      const vtkIdType* cellPts = nullptr;
      vtkIdType cellSize;
      cellIterator->GetCurrentCell(cellSize, cellPts);
      primitiveCounts.emplace_back(numberOfPrimitives);
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
    primitiveCounts.emplace_back(numberOfPrimitives);
    // create input buffer for connectivity ids
    vtkNew<vtkWebGPUComputeBuffer> inputConnectivityBuffer;
    inputConnectivityBuffer->SetGroup(0);
    inputConnectivityBuffer->SetBinding(0);
    inputConnectivityBuffer->SetLabel(std::string("InputConnectivity-") + cellTypeAsString + "@" +
      cellArray->GetObjectDescription());
    inputConnectivityBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
    inputConnectivityBuffer->SetData(cellArray->GetConnectivityArray());
    inputConnectivityBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);

    // create input buffer for offsets
    vtkNew<vtkWebGPUComputeBuffer> offsetsBuffer;
    offsetsBuffer->SetGroup(0);
    offsetsBuffer->SetBinding(1);
    offsetsBuffer->SetLabel(
      std::string("Offsets-") + cellTypeAsString + "-" + cellArray->GetObjectDescription());
    offsetsBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
    offsetsBuffer->SetData(cellArray->GetOffsetsArray());
    offsetsBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);

    // create input buffer for primitive offsets
    vtkNew<vtkWebGPUComputeBuffer> primitiveCountsBuffer;
    primitiveCountsBuffer->SetGroup(0);
    primitiveCountsBuffer->SetBinding(2);
    primitiveCountsBuffer->SetLabel(std::string("PrimitiveCounts-") + primitiveTypeAsString + "@" +
      cellArray->GetObjectDescription());
    primitiveCountsBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
    primitiveCountsBuffer->SetData(primitiveCounts);
    primitiveCountsBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

    std::vector<vtkTypeUInt32> uniformData = { static_cast<unsigned int>(cellIdOffset) };
    vtkNew<vtkWebGPUComputeBuffer> cellIfOffsetUniformComputeBuffer;
    cellIfOffsetUniformComputeBuffer->SetGroup(0);
    cellIfOffsetUniformComputeBuffer->SetBinding(3);
    cellIfOffsetUniformComputeBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER);
    cellIfOffsetUniformComputeBuffer->SetData(uniformData);
    cellIfOffsetUniformComputeBuffer->SetLabel(
      std::string("CellIdOffsets-") + cellTypeAsString + "-" + cellArray->GetObjectDescription());
    cellIfOffsetUniformComputeBuffer->SetDataType(
      vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

    vtkNew<vtkWebGPUComputeBuffer> outputConnectivityBuffer;
    outputConnectivityBuffer->SetGroup(0);
    outputConnectivityBuffer->SetBinding(4);
    outputConnectivityBuffer->SetLabel(std::string("OutputConnectivity-") + primitiveTypeAsString +
      "@" + cellArray->GetObjectDescription());
    outputConnectivityBuffer->SetMode(
      vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
    outputConnectivityBuffer->SetByteSize(
      numberOfPrimitives * primitiveSize * sizeof(vtkTypeUInt32));

    vtkNew<vtkWebGPUComputeBuffer> outputCellIdBuffer;
    outputCellIdBuffer->SetGroup(0);
    outputCellIdBuffer->SetBinding(5);
    outputCellIdBuffer->SetLabel(std::string("OutputCellId-") + primitiveTypeAsString + "@" +
      cellArray->GetObjectDescription());
    outputCellIdBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
    outputCellIdBuffer->SetByteSize(numberOfPrimitives * sizeof(vtkTypeUInt32));

    // handle optional edge visibility.
    // this lets fragment shader hide internal edges of a polygon
    // when edge visibility is turned on.
    vtkNew<vtkWebGPUComputeBuffer> edgeArrayComputeBuffer;
    edgeArrayComputeBuffer->SetGroup(0);
    edgeArrayComputeBuffer->SetBinding(6);
    edgeArrayComputeBuffer->SetLabel(
      std::string("EdgeArray-") + primitiveTypeAsString + "@" + cellArray->GetObjectDescription());
    edgeArrayComputeBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_COMPUTE_STORAGE);
    if (primitiveSize == 3)
    {
      edgeArrayComputeBuffer->SetByteSize(numberOfPrimitives * sizeof(vtkTypeUInt32));
    }
    else
    {
      // placeholder must be aligned to 32-bit boundary
      edgeArrayComputeBuffer->SetByteSize(sizeof(vtkTypeUInt32));
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
      return false;
    }

    // add buffers one by one to the compute pass.
    pass->AddBuffer(inputConnectivityBuffer);
    pass->AddBuffer(offsetsBuffer);
    pass->AddBuffer(primitiveCountsBuffer);
    pass->AddBuffer(cellIfOffsetUniformComputeBuffer);
    pass->AddBuffer(outputConnectivityBuffer); // not used in cpu
    pass->AddBuffer(outputCellIdBuffer);       // not used in cpu
    pass->AddBuffer(edgeArrayComputeBuffer);   // not used in cpu
    // the topology and edge_array buffers are populated by compute pipeline and their contents
    // read in the graphics pipeline within the vertex and fragment shaders
    // keep reference to the output of compute pipeline, reuse it in the vertex, fragment shaders.
    if (pipeline->GetRegisteredBuffer(outputConnectivityBuffer, *connectivityBuffer))
    {
      *vertexCount = static_cast<vtkTypeUInt32>(numberOfPrimitives * primitiveSize);
    }
    else
    {
      *vertexCount = 0;
    }
    pipeline->GetRegisteredBuffer(outputCellIdBuffer, *cellIdBuffer);
    // do the same for the edge array buffer only when output primitives are triangles.
    if (edgeArrayBuffer)
    {
      if (primitiveSize == 3)
      {
        if (!pipeline->GetRegisteredBuffer(edgeArrayComputeBuffer, *edgeArrayBuffer))
        {
          vtkErrorMacro(<< "edgeArray compute buffer for " << primitiveTypeAsString
                        << " is not registered!");
        }
      }
      else
      {
        *edgeArrayBuffer = nullptr;
      }
    }
    // repeat for cellIdOffset
    if (!pipeline->GetRegisteredBuffer(
          cellIfOffsetUniformComputeBuffer, *cellIdOffsetUniformBuffer))
    {
      vtkErrorMacro(<< "cellIdOffsetUniform compute buffer for " << primitiveTypeAsString
                    << " is not registered!");
    }
    // dispatch
    pass->SetWorkgroups(
      nWorkGroupsPerDimension[0], nWorkGroupsPerDimension[1], nWorkGroupsPerDimension[2]);
    pass->Dispatch();
  }

  // update build timestamp
  this->UpdateCellToPrimitiveComputePipelineTimestamp(idx);

  return true;
}

VTK_ABI_NAMESPACE_END
