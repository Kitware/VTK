// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "Private/vtkWebGPUComputeBufferInternals.h"
#include "VTKCellToGraphicsPrimitive.h"
#include "vtkABINamespace.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellType.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSetGet.h"
#include "vtkTimeStamp.h"
#include "vtkType.h"
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUComputePipeline.h"

#include <array>
#include <webgpu/webgpu_cpp.h>

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
  vtkCellArray* cells, TopologySourceType topologySourceType)
{
  return cells->GetMTime() > this->TopologyBuildTimestamp[topologySourceType];
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
bool vtkWebGPUCellToPrimitiveConverter::DispatchMeshToPrimitiveComputePipeline(
  vtkWebGPUConfiguration* wgpuConfiguration, vtkPolyData* mesh, int representation,
  vtkTypeUInt32* vertexCounts[NUM_TOPOLOGY_SOURCE_TYPES],
  wgpu::Buffer* connectivityBuffers[NUM_TOPOLOGY_SOURCE_TYPES],
  wgpu::Buffer* cellIdBuffers[NUM_TOPOLOGY_SOURCE_TYPES],
  wgpu::Buffer* edgeArrayBuffers[NUM_TOPOLOGY_SOURCE_TYPES],
  wgpu::Buffer* cellIdOffsetUniformBuffers[NUM_TOPOLOGY_SOURCE_TYPES])
{
  bool buffersUpdated = false;
  vtkTypeUInt32 cellIdOffset = 0;
  {
    const auto idx = this->GetTopologySourceTypeForCellType(VTK_POLY_VERTEX, representation);
    buffersUpdated = this->DispatchCellToPrimitiveComputePipeline(wgpuConfiguration,
      mesh ? mesh->GetVerts() : nullptr, representation, VTK_POLY_VERTEX, cellIdOffset,
      vertexCounts[idx], connectivityBuffers[idx], cellIdBuffers[idx], edgeArrayBuffers[idx],
      cellIdOffsetUniformBuffers[idx]);
    cellIdOffset += static_cast<vtkTypeUInt32>(mesh->GetNumberOfVerts());
  }
  // dispatch compute pipeline that converts polyline to lines.
  {
    const auto idx = this->GetTopologySourceTypeForCellType(VTK_POLY_LINE, representation);
    buffersUpdated |= this->DispatchCellToPrimitiveComputePipeline(wgpuConfiguration,
      mesh ? mesh->GetLines() : nullptr, representation, VTK_POLY_LINE, cellIdOffset,
      vertexCounts[idx], connectivityBuffers[idx], cellIdBuffers[idx], edgeArrayBuffers[idx],
      cellIdOffsetUniformBuffers[idx]);
    cellIdOffset += static_cast<vtkTypeUInt32>(mesh->GetNumberOfLines());
  }
  // dispatch compute pipeline that converts polygon to triangles.
  {
    const auto idx = this->GetTopologySourceTypeForCellType(VTK_POLYGON, representation);
    buffersUpdated |= this->DispatchCellToPrimitiveComputePipeline(wgpuConfiguration,
      mesh ? mesh->GetPolys() : nullptr, representation, VTK_POLYGON, cellIdOffset,
      vertexCounts[idx], connectivityBuffers[idx], cellIdBuffers[idx], edgeArrayBuffers[idx],
      cellIdOffsetUniformBuffers[idx]);
  }
  return buffersUpdated;
}

//------------------------------------------------------------------------------
bool vtkWebGPUCellToPrimitiveConverter::DispatchCellToPrimitiveComputePipeline(
  vtkWebGPUConfiguration* wgpuConfiguration, vtkCellArray* cells, int representation, int cellType,
  vtkTypeUInt32 cellIdOffset, vtkTypeUInt32* vertexCount, wgpu::Buffer* connectivityBuffer,
  wgpu::Buffer* cellIdBuffer, wgpu::Buffer* edgeArrayBuffer,
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

  if (!cells)
  {
    *vertexCount = 0;
    return false;
  }
  if (cells->GetNumberOfCells() == 0)
  {
    *vertexCount = 0;
    return false;
  }
  if (!this->GetNeedToRebuildCellToPrimitiveComputePipeline(cells, idx))
  {
    return false;
  }

  const vtkIdType fixedCellSize = cells->IsHomogeneous();
  const bool convertedTo32Bit =
    fixedCellSize > 0 ? cells->ConvertToFixedSize32BitStorage() : cells->ConvertTo32BitStorage();
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
    auto* ids = cells->GetConnectivityArray();

    *vertexCount = ids->GetNumberOfValues();

    if (cellIdBuffer)
    {
      *cellIdBuffer = nullptr;
    }
    if (edgeArrayBuffer)
    {
      *edgeArrayBuffer = nullptr;
    }

    auto label =
      std::string("Connectivity-") + primitiveTypeAsString + "@" + cells->GetObjectDescription();
    *connectivityBuffer =
      wgpuConfiguration->CreateBuffer(ids->GetDataSize() * ids->GetDataTypeSize(),
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, false, label.c_str());

    label = std::string("CellIdOffsetUniform-") + primitiveTypeAsString + "@" +
      cells->GetObjectDescription();
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
    int nRequiredWorkGroups = std::ceil(cells->GetNumberOfCells() / 64.0);
    std::array<vtkTypeUInt32, 3> nWorkGroupsPerDimension = { 1, 1, 1 };
    if (::Factorize(nRequiredWorkGroups, nWorkGroupsPerDimension) == -1)
    {
      vtkErrorMacro(<< "Number of cells is too large to fit in available workgroups");
      return false;
    }
    vtkDebugMacro(<< "Dispatch " << cellTypeAsString
                  << " with workgroups=" << nWorkGroupsPerDimension[0] << 'x'
                  << nWorkGroupsPerDimension[1] << 'x' << nWorkGroupsPerDimension[2]);

    const vtkIdType primitiveSizeOffset =
      this->GetTessellatedPrimitiveSizeOffsetForCellType(cellType);
    vtkTypeUInt32 numberOfPrimitives = 0;
    std::vector<vtkTypeUInt32> primitiveCounts;
    auto cellIterator = vtk::TakeSmartPointer(cells->NewIterator());
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
    inputConnectivityBuffer->SetLabel(
      std::string("InputConnectivity-") + cellTypeAsString + "@" + cells->GetObjectDescription());
    inputConnectivityBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
    inputConnectivityBuffer->SetData(cells->GetConnectivityArray());
    inputConnectivityBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);

    // create input buffer for offsets
    vtkNew<vtkWebGPUComputeBuffer> offsetsBuffer;
    offsetsBuffer->SetGroup(0);
    offsetsBuffer->SetBinding(1);
    offsetsBuffer->SetLabel(
      std::string("Offsets-") + cellTypeAsString + "-" + cells->GetObjectDescription());
    offsetsBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
    offsetsBuffer->SetData(cells->GetOffsetsArray());
    offsetsBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);

    // create input buffer for primitive offsets
    vtkNew<vtkWebGPUComputeBuffer> primitiveCountsBuffer;
    primitiveCountsBuffer->SetGroup(0);
    primitiveCountsBuffer->SetBinding(2);
    primitiveCountsBuffer->SetLabel(std::string("PrimitiveCounts-") + primitiveTypeAsString + "@" +
      cells->GetObjectDescription());
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
      std::string("CellIdOffsets-") + cellTypeAsString + "-" + cells->GetObjectDescription());
    cellIfOffsetUniformComputeBuffer->SetDataType(
      vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

    vtkNew<vtkWebGPUComputeBuffer> outputConnectivityBuffer;
    outputConnectivityBuffer->SetGroup(0);
    outputConnectivityBuffer->SetBinding(4);
    outputConnectivityBuffer->SetLabel(std::string("OutputConnectivity-") + primitiveTypeAsString +
      "@" + cells->GetObjectDescription());
    outputConnectivityBuffer->SetMode(
      vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
    outputConnectivityBuffer->SetByteSize(
      numberOfPrimitives * primitiveSize * sizeof(vtkTypeUInt32));

    vtkNew<vtkWebGPUComputeBuffer> outputCellIdBuffer;
    outputCellIdBuffer->SetGroup(0);
    outputCellIdBuffer->SetBinding(5);
    outputCellIdBuffer->SetLabel(
      std::string("OutputCellId-") + primitiveTypeAsString + "@" + cells->GetObjectDescription());
    outputCellIdBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
    outputCellIdBuffer->SetByteSize(numberOfPrimitives * sizeof(vtkTypeUInt32));

    // handle optional edge visibility.
    // this lets fragment shader hide internal edges of a polygon
    // when edge visibility is turned on.
    vtkNew<vtkWebGPUComputeBuffer> edgeArrayComputeBuffer;
    edgeArrayComputeBuffer->SetGroup(0);
    edgeArrayComputeBuffer->SetBinding(6);
    edgeArrayComputeBuffer->SetLabel(
      std::string("EdgeArray-") + primitiveTypeAsString + "@" + cells->GetObjectDescription());
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
