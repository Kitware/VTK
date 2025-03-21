// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "VTKCellToGraphicsPrimitive.h"
#include "vtkABINamespace.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellType.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkTimeStamp.h"
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
  wgpu::Buffer* topologyBuffers[NUM_TOPOLOGY_SOURCE_TYPES],
  wgpu::Buffer* edgeArrayBuffers[NUM_TOPOLOGY_SOURCE_TYPES])
{
  bool buffersUpdated =
    this->DispatchCellToPrimitiveComputePipeline(wgpuConfiguration, mesh->GetVerts(),
      representation, VTK_POLY_VERTEX, 0, vertexCounts, topologyBuffers, edgeArrayBuffers);
  // dispatch compute pipeline that converts polyline to lines.
  const vtkIdType numVerts = mesh->GetNumberOfVerts();
  buffersUpdated |=
    this->DispatchCellToPrimitiveComputePipeline(wgpuConfiguration, mesh->GetLines(),
      representation, VTK_POLY_LINE, numVerts, vertexCounts, topologyBuffers, edgeArrayBuffers);
  // dispatch compute pipeline that converts polygon to triangles.
  const vtkIdType numLines = mesh->GetNumberOfLines();
  buffersUpdated |= this->DispatchCellToPrimitiveComputePipeline(wgpuConfiguration,
    mesh->GetPolys(), representation, VTK_POLYGON, numLines + numVerts, vertexCounts,
    topologyBuffers, edgeArrayBuffers);
  return buffersUpdated;
}

//------------------------------------------------------------------------------
bool vtkWebGPUCellToPrimitiveConverter::DispatchCellToPrimitiveComputePipeline(
  vtkWebGPUConfiguration* wgpuConfiguration, vtkCellArray* cells, int representation, int cellType,
  vtkIdType cellIdOffset, vtkTypeUInt32* vertexCounts[NUM_TOPOLOGY_SOURCE_TYPES],
  wgpu::Buffer* topologyBuffers[NUM_TOPOLOGY_SOURCE_TYPES],
  wgpu::Buffer* edgeArrayBuffers[NUM_TOPOLOGY_SOURCE_TYPES])
{
  const auto idx = this->GetTopologySourceTypeForCellType(cellType, representation);

  if (!cells)
  {
    *(vertexCounts[idx]) = 0;
    return false;
  }
  if (cells->GetNumberOfCells() == 0)
  {
    *(vertexCounts[idx]) = 0;
    return false;
  }
  if (!this->GetNeedToRebuildCellToPrimitiveComputePipeline(cells, idx))
  {
    return false;
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
    return false;
  }
  vtkDebugMacro(<< "Dispatch " << cellTypeAsString
                << " with workgroups=" << nWorkGroupsPerDimension[0] << 'x'
                << nWorkGroupsPerDimension[1] << 'x' << nWorkGroupsPerDimension[2]);

  if (!cells->ConvertTo32BitStorage())
  {
    vtkErrorMacro(<< "Failed to convert cell array storage to 32-bit");
    return false;
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
    std::string("Connectivity-") + cellTypeAsString + "-" + cells->GetObjectDescription());
  connBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  connBuffer->SetData(cells->GetConnectivityArray());
  connBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);

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
  vtkNew<vtkWebGPUComputeBuffer> primIdBuffer;
  primIdBuffer->SetGroup(0);
  primIdBuffer->SetBinding(2);
  primIdBuffer->SetLabel(
    std::string("PrimitiveIds-") + primitiveTypeAsString + "-" + cells->GetObjectDescription());
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
    std::string("CellIdOffsets-") + cellTypeAsString + "-" + cells->GetObjectDescription());
  uniformBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

  std::size_t outputBufferSize = 2 * numberOfPrimitives * primitiveSize * sizeof(vtkTypeUInt32);
  vtkNew<vtkWebGPUComputeBuffer> topologyBuffer;
  topologyBuffer->SetGroup(0);
  topologyBuffer->SetBinding(4);
  topologyBuffer->SetLabel(
    std::string("Topology-") + primitiveTypeAsString + "-" + cells->GetObjectDescription());
  topologyBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
  topologyBuffer->SetByteSize(outputBufferSize);

  // handle optional edge visibility.
  // this lets fragment shader hide internal edges of a polygon
  // when edge visibility is turned on.
  vtkNew<vtkWebGPUComputeBuffer> edgeArrayComputeBuffer;
  edgeArrayComputeBuffer->SetGroup(0);
  edgeArrayComputeBuffer->SetBinding(5);
  edgeArrayComputeBuffer->SetLabel(
    std::string("EdgeArray-") + primitiveTypeAsString + "-" + cells->GetObjectDescription());
  edgeArrayComputeBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_COMPUTE_STORAGE);
  if (primitiveSize == 3)
  {
    edgeArrayComputeBuffer->SetByteSize(numberOfPrimitives * sizeof(vtkTypeUInt32));
  }
  else
  {
    // placeholder must be aligned to 32-bit boundary
    edgeArrayComputeBuffer->SetByteSize(4);
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
  pass->AddBuffer(connBuffer);
  pass->AddBuffer(offsetsBuffer);
  pass->AddBuffer(primIdBuffer);
  pass->AddBuffer(uniformBuffer);
  pass->AddBuffer(topologyBuffer);         // not used in cpu
  pass->AddBuffer(edgeArrayComputeBuffer); // not used in cpu
  // the topology and edge_array buffers are populated by compute pipeline and their contents
  // read in the graphics pipeline within the vertex and fragment shaders
  // keep reference to the output of compute pipeline, reuse it in the vertex, fragment shaders.
  if (pipeline->GetRegisteredBuffer(topologyBuffer, *topologyBuffers[idx]))
  {
    *(vertexCounts[idx]) = numberOfPrimitives * primitiveSize;
  }
  else
  {
    *(vertexCounts[idx]) = 0;
  }
  // do the same for the edge array buffer
  if (edgeArrayBuffers[idx])
  {
    if (!pipeline->GetRegisteredBuffer(edgeArrayComputeBuffer, *(edgeArrayBuffers[idx])))
    {
      vtkErrorMacro(<< "edge array buffer for " << primitiveTypeAsString << " is not registered!");
    }
  }
  // dispatch
  pass->SetWorkgroups(
    nWorkGroupsPerDimension[0], nWorkGroupsPerDimension[1], nWorkGroupsPerDimension[2]);
  pass->Dispatch();

  // update build timestamp
  this->UpdateCellToPrimitiveComputePipelineTimestamp(idx);

  return true;
}

VTK_ABI_NAMESPACE_END
