#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkLogger.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUComputePipeline.h"
#include "vtkWebGPUConfiguration.h"

#include <array>
#include <cstdlib>
#include <sstream>

namespace
{
struct ParametersInfo
{
  vtkIdType NumberOfCells;
  std::map<std::size_t, double> CellSizeWeights;
};

std::vector<ParametersInfo> ParametersCollection = {
  ParametersInfo{ 10, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } }, // warm up
  ParametersInfo{ 1'000, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } },
  ParametersInfo{ 10'000, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } },
  ParametersInfo{ 100'000, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } },
  ParametersInfo{ 1'000'000, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } },
  ParametersInfo{ 5'000'000, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } },
  ParametersInfo{ 10'000'000, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } }
#if VTK_SIZEOF_VOID_P == 8
  ,
  ParametersInfo{ 15'000'000, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } },
  ParametersInfo{ 20'000'000, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } },
  ParametersInfo{ 25'000'000, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } },
  ParametersInfo{ 35'000'000, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } },
  ParametersInfo{ 40'000'000, { { 3, 0.1 }, { 4, 0.3 }, { 5, 0.1 }, { 6, 0.5 } } },
#endif
};

const std::string Polys2TrisShader = R"(
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

@group(0) @binding(0) var<storage, read> connectivity: array<u32>;
@group(0) @binding(1) var<storage, read> offsets: array<u32>;
@group(0) @binding(2) var<storage, read> triangle_id_offsets: array<u32>;
@group(0) @binding(3) var<storage, read_write> triangle_list: array<u32>;

@compute @workgroup_size(64)
fn polys2tris(
    @builtin(workgroup_id) workgroup_id : vec3<u32>,
    @builtin(local_invocation_index) local_invocation_index: u32,
    @builtin(num_workgroups) num_workgroups: vec3<u32>)
{
  let workgroup_index =
    workgroup_id.x +
    workgroup_id.y * num_workgroups.x +
    workgroup_id.z * num_workgroups.x * num_workgroups.y;
  let cell_id: u32 =
    workgroup_index * 64 +
    local_invocation_index;
  if (cell_id >= arrayLength(&offsets) - 1)
  {
    return;
  }

  let num_triangles_per_cell: u32 = triangle_id_offsets[cell_id + 1u] - triangle_id_offsets[cell_id];

  // where to start writing point indices of a triangle.
  var output_offset: u32 = triangle_id_offsets[cell_id] * 3u;

  // where to obtain the point indices that describe connectivity of a polygon
  let input_offset: u32 = offsets[cell_id];

  for (var i: u32 = 0; i < num_triangles_per_cell; i++) {

    let p0: u32 = connectivity[input_offset];
    let p1: u32 = connectivity[input_offset + i + 1u];
    let p2: u32 = connectivity[input_offset + i + 2u];

    triangle_list[output_offset] = p0;
    output_offset++;

    triangle_list[output_offset] = p1;
    output_offset++;

    triangle_list[output_offset] = p2;
    output_offset++;
  }
}
)";

vtkNew<vtkCellArray> BuildPolygons(
  const std::map<std::size_t, double> cellSizeDistributions, vtkIdType numberOfCells)
{
  vtkNew<vtkMinimalStandardRandomSequence> randomSequence;
  randomSequence->Initialize(1);
  vtkNew<vtkCellArray> polygons;
  for (const auto& iter : cellSizeDistributions)
  {
    const std::size_t cellSize = iter.first;
    const double& weight = iter.second;
    const vtkIdType numberOfPolys = weight * numberOfCells;
    for (vtkIdType i = 0; i < numberOfPolys; ++i)
    {
      polygons->InsertNextCell(static_cast<int>(cellSize));
      for (std::size_t j = 0; j < cellSize; ++j)
      {
        polygons->InsertCellPoint(
          randomSequence->GetNextValue() * 1000); // inserts a random point id from 0-1000.
      }
    }
  }
  return polygons;
}

// Function to factor the number into three multiples
std::array<vtkTypeUInt32, 3> Factorize(
  vtkTypeUInt32 n, vtkTypeUInt32 maxWorkGroupsPerDimension, bool& success)
{
  std::array<vtkTypeUInt32, 3> factors = { 1, 1, 1 };
  if (n > maxWorkGroupsPerDimension)
  {
    factors[0] = maxWorkGroupsPerDimension;
  }
  else
  {
    factors[0] = n;
    factors[1] = 1;
    factors[2] = 1;
    success = true;
  }
  if (n > (factors[0] * maxWorkGroupsPerDimension))
  {
    factors[1] = maxWorkGroupsPerDimension;
  }
  else
  {
    factors[1] = std::ceil(n / static_cast<double>(factors[0]));
    factors[2] = 1;
    success = true;
  }
  if (n > (factors[0] * factors[1] * maxWorkGroupsPerDimension))
  {
    success = false;
  }
  else
  {
    factors[2] = std::ceil(n / static_cast<double>(factors[0] * factors[1]));
    success = true;
  }
  return factors;
}

std::string CellSizeWeightsToString(const ParametersInfo& parameters)
{
  std::stringstream result;
  for (const auto& iter : parameters.CellSizeWeights)
  {
    result << vtkIdType(std::ceil(iter.second * parameters.NumberOfCells));
    switch (iter.first)
    {
      case 3:
        result << " triangles, ";
        break;
      case 4:
        result << " quads, ";
        break;
      case 5:
        result << " pentagons, ";
        break;
      case 6:
        result << " hexagons, ";
        break;
      case 7:
        result << " heptagons, ";
        break;
      case 8:
        result << " octagons, ";
        break;
      default:
        result << " " << iter.first << "-gon, ";
        break;
    }
  }
  return result.str();
}
}

int TestComputeTriangulation(int argc, char* argv[])
{
  bool verifyPointIds = false;
  bool runBenchmarks = false;
  for (int i = 0; i < argc; ++i)
  {
    if (std::string(argv[i]) == "--verify")
    {
      verifyPointIds = true;
    }
    if (std::string(argv[i]) == "--benchmark")
    {
      runBenchmarks = true;
    }
  }
  vtkNew<vtkWebGPUConfiguration> wgpuConfig;
  std::array<int, 4> bufferIndices = { -1, -1, -1, -1 };

  std::size_t numParameterGroups = runBenchmarks ? ::ParametersCollection.size() : 3;
  for (std::size_t i = 0; i < numParameterGroups; ++i)
  {
    const auto& parameters = ::ParametersCollection[i];
    std::string scopeId = std::to_string(parameters.NumberOfCells) + " cells";
    vtkLogScopeF(INFO, "%s", scopeId.c_str());
    vtkLog(INFO, << CellSizeWeightsToString(parameters));

    vtkLogStartScope(INFO, "Build polygons");
    auto polygons = BuildPolygons(parameters.CellSizeWeights, parameters.NumberOfCells);
    vtkLogEndScope("Build polygons");

    vtkLogStartScope(INFO, "Compute triangle lists in CPU");
    auto iter = polygons->NewIterator();
    std::vector<vtkTypeUInt32> expectedTris;
    for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
    {
      const vtkIdType* cellPts = nullptr;
      vtkIdType cellSize;
      iter->GetCurrentCell(cellSize, cellPts);
      const int numSubTriangles = cellSize - 2;
      // save memory by not storing the point ids when verification is disabled
      for (int j = 0; j < numSubTriangles; ++j)
      {
        expectedTris.emplace_back(cellPts[0]);
        expectedTris.emplace_back(cellPts[j + 1]);
        expectedTris.emplace_back(cellPts[j + 2]);
      }
    }
    vtkLogEndScope("Compute triangle lists in CPU");
    if (!verifyPointIds)
    {
      expectedTris.clear();
    }

    vtkLogStartScope(INFO, "Compute triangle ID offsets.");
    vtkTypeUInt32 numTriangles = 0;
    std::vector<vtkTypeUInt32> triangleIdOffsets;
    for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
    {
      const vtkIdType* cellPts = nullptr;
      vtkIdType cellSize;
      iter->GetCurrentCell(cellSize, cellPts);
      triangleIdOffsets.emplace_back(numTriangles);
      numTriangles += (cellSize - 2u);
    }
    triangleIdOffsets.emplace_back(numTriangles);
    iter->Delete();
    vtkLogEndScope("Compute triangle ID offsets.");

    if (polygons->IsStorage64Bit())
    {
      vtkLogScopeF(INFO, "Convert to 32-bit storage");
      polygons->ConvertTo32BitStorage();
    }

    // create compute pipeline
    vtkNew<vtkWebGPUComputePipeline> pipeline;
    pipeline->SetWGPUConfiguration(wgpuConfig);
    pipeline->SetLabel("triangulate polygons pipeline");

    auto computePass = pipeline->CreateComputePass();
    computePass->SetLabel("triangulate polygons pass");
    computePass->SetShaderEntryPoint("polys2tris");
    // set shader capable of triangulating polygons with 32-bit indices.
    computePass->SetShaderSource(Polys2TrisShader);
    // create input buffer for connectivity ids
    vtkNew<vtkWebGPUComputeBuffer> connBuffer;
    connBuffer->SetGroup(0);
    connBuffer->SetBinding(0);
    connBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
    connBuffer->SetData(polygons->GetConnectivityArray());
    connBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);

    // create input buffer for offsets
    vtkNew<vtkWebGPUComputeBuffer> offsetsBuffer;
    offsetsBuffer->SetGroup(0);
    offsetsBuffer->SetBinding(1);
    offsetsBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
    offsetsBuffer->SetData(polygons->GetOffsetsArray());
    offsetsBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::VTK_DATA_ARRAY);

    // create input buffer for triangle ID offsets
    vtkNew<vtkWebGPUComputeBuffer> triangleIdOffsetsBuffer;
    triangleIdOffsetsBuffer->SetGroup(0);
    triangleIdOffsetsBuffer->SetBinding(2);
    triangleIdOffsetsBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
    triangleIdOffsetsBuffer->SetData(triangleIdOffsets);
    triangleIdOffsetsBuffer->SetDataType(vtkWebGPUComputeBuffer::BufferDataType::STD_VECTOR);

    std::size_t outputBufferSize = numTriangles * 3 * sizeof(vtkTypeUInt32);
    vtkNew<vtkWebGPUComputeBuffer> trisBuffer;
    trisBuffer->SetGroup(0);
    trisBuffer->SetBinding(3);
    trisBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
    trisBuffer->SetByteSize(outputBufferSize);

    // add buffers to the compute pass
    bufferIndices[0] = computePass->AddBuffer(connBuffer);
    bufferIndices[1] = computePass->AddBuffer(offsetsBuffer);
    bufferIndices[2] = computePass->AddBuffer(triangleIdOffsetsBuffer);
    bufferIndices[3] = computePass->AddBuffer(trisBuffer);

    // dispatch the problem size over sufficient number of workgroups
    int numRequiredWorkGroups = std::ceil(polygons->GetNumberOfCells() / 64.0);
    bool success = true;
    const auto gridSize =
      Factorize(numRequiredWorkGroups, /*maxWorkGroupsPerDimension=*/65535, success);
    if (!success)
    {
      vtkLog(ERROR, << "Number of cells is too large to fit in available workgroups");
      return EXIT_FAILURE;
    }
    vtkLog(INFO, << "Dispatch grid sz " << gridSize[0] << 'x' << gridSize[1] << 'x' << gridSize[2]);
    computePass->SetWorkgroups(gridSize[0], gridSize[1], gridSize[2]);
    computePass->Dispatch();

    auto onBufferMapped = [](const void* mappedData, void* userdata)
    {
      vtkLogScopeF(INFO, "Triangle lists buffer is now mapped");
      auto expected = *(reinterpret_cast<std::vector<vtkTypeUInt32>*>(userdata));
      const vtkTypeUInt32* mappedDataAsU32 = static_cast<const vtkTypeUInt32*>(mappedData);
      for (std::size_t j = 0; j < expected.size(); j++)
      {
        if (mappedDataAsU32[j] != expected[j])
        {
          vtkLog(ERROR, << "Point ID at location " << j << " does not match. Found "
                        << mappedDataAsU32[j] << ", expected value " << expected[j]);
          break;
        }
      }
    };
    if (verifyPointIds)
    {
      computePass->ReadBufferFromGPU(bufferIndices[3], onBufferMapped, &expectedTris);
    }

    vtkLogStartScope(INFO, "Compute triangle lists in GPU");
    pipeline->Update();
    vtkLogEndScope("Compute triangle lists in GPU");
  }
  return EXIT_SUCCESS;
}
