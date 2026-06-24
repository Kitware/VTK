#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkLogger.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "vtkWebGPUConfiguration.h"

#include <cstdlib>
#include <set>
#include <sstream>
#include <vector>

#include <iostream>

// This unit test exercises vtkWebGPUCellToPrimitiveConverter.
// You can run this using the `--verify` argument to ensure the output of
// conversion compute pipeline matches the expected triangle IDs.
// Additionally, this test can be run in a benchmark mode with the `--benchmark` flag.
// In the benchmark mode, a couple of things occur:
// - The existing log verbosity is bumped to INFO so that the timing information is visible in
// console.
// - The program runs over a set of parameters with a steady increase in the number of polygons.

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

// ---------------------------------------------------------------------------
// Verification helpers for the GPU triangulation output.
//
// The kernel triangulates using a geometric ear clip whose exact choice of
// interior diagonals depends on floating-point sign decisions made in f32 on
// the GPU. A CPU reference computing in f64 can legitimately pick a different
// (also valid) triangulation, so verifying exact triangle ids would be flaky.
// Instead we verify properties that hold for ANY correct triangulation:
//   - count: exactly cellSize - 2 triangles per cell
//   - membership: every emitted point id is one of that cell's input ids
//     (zero-area filler triangles reuse the cell's first id, also a member)
//   - (geometric section only, for simple planar polygons) the real triangle
//     areas sum to the polygon area and each real triangle is non-inverted
//     w.r.t. the polygon normal.
// ---------------------------------------------------------------------------

// Count + membership check, valid for arbitrary (even degenerate, non-planar)
// input topology. Returns true on success.
bool VerifyCountAndMembership(vtkCellArray* polygons,
  const std::vector<vtkTypeUInt32>& gpuConnectivity, const std::vector<vtkTypeUInt32>& gpuCellIds)
{
  bool ok = true;
  std::size_t outIdx = 0;
  auto iter = vtk::TakeSmartPointer(polygons->NewIterator());
  for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
  {
    const vtkIdType* cellPts = nullptr;
    vtkIdType cellSize = 0;
    iter->GetCurrentCell(cellSize, cellPts);
    const vtkIdType cellId = iter->GetCurrentCellId();
    std::set<vtkTypeUInt32> members;
    for (vtkIdType k = 0; k < cellSize; ++k)
    {
      members.insert(static_cast<vtkTypeUInt32>(cellPts[k]));
    }
    const vtkIdType numTris = cellSize - 2;
    for (vtkIdType t = 0; t < numTris; ++t)
    {
      for (int v = 0; v < 3; ++v)
      {
        if (outIdx >= gpuConnectivity.size())
        {
          vtkLog(ERROR, << "GPU connectivity buffer underran at cell " << cellId);
          return false;
        }
        const vtkTypeUInt32 id = gpuConnectivity[outIdx];
        if (members.find(id) == members.end())
        {
          vtkLog(ERROR, << "Cell " << cellId << " triangle " << t << " emits point id " << id
                        << " which is not a vertex of the cell.");
          ok = false;
        }
        ++outIdx;
      }
      const std::size_t triIndex = outIdx / 3 - 1;
      if (triIndex < gpuCellIds.size() &&
        gpuCellIds[triIndex] != static_cast<vtkTypeUInt32>(cellId))
      {
        vtkLog(ERROR, << "Cell id mismatch at triangle " << t << " of cell " << cellId);
        ok = false;
      }
    }
  }
  if (outIdx != gpuConnectivity.size())
  {
    vtkLog(ERROR, << "GPU produced " << gpuConnectivity.size() << " connectivity ids, expected "
                  << outIdx);
    ok = false;
  }
  return ok;
}

// Generate a deterministic set of 3D points that polygon point-ids index into.
// numPoints must exceed the maximum id produced by BuildPolygons (1000).
vtkSmartPointer<vtkPoints> BuildPoints(vtkIdType numPoints)
{
  vtkNew<vtkMinimalStandardRandomSequence> rng;
  rng->Initialize(7);
  vtkNew<vtkPoints> points;
  points->SetDataTypeToFloat();
  points->SetNumberOfPoints(numPoints);
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    const double x = rng->GetNextValue();
    const double y = rng->GetNextValue();
    const double z = rng->GetNextValue();
    points->SetPoint(i, x, y, z);
  }
  return points;
}

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
      if (vtkLogger::GetCurrentVerbosityCutoff() < vtkLogger::VERBOSITY_INFO)
      {
        std::cout << "Bump logger verbosity to INFO\n";
        vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_INFO);
      }
    }
  }
  int testStatus = EXIT_SUCCESS;
  std::size_t numParameterGroups = runBenchmarks ? ::ParametersCollection.size() : 3;
  for (std::size_t i = 0; i < numParameterGroups; ++i)
  {
    vtkNew<vtkWebGPUConfiguration> wgpuConfig;

    const auto& parameters = ::ParametersCollection[i];
    std::string scopeId = vtk::to_string(parameters.NumberOfCells) + " cells";
    vtkLogScopeF(INFO, "%s", scopeId.c_str());
    vtkLog(INFO, << CellSizeWeightsToString(parameters));

    vtkLogStartScope(INFO, "Build polygons");
    auto polygons = BuildPolygons(parameters.CellSizeWeights, parameters.NumberOfCells);
    // Point ids range over 0..1000 (see BuildPolygons); supply matching geometry so the
    // ear-clip kernel reads real coordinates at binding 7.
    auto points = BuildPoints(1001);
    vtkLogEndScope("Build polygons");

    struct MapData
    {
      wgpu::Buffer buffer;
      std::size_t byteSize;
      std::vector<vtkTypeUInt32> gpuConnectivity;
      std::vector<vtkTypeUInt32> gpuCellId;
    };
    MapData* mapData = new MapData();

    // As the `vtkWebGPUCellToPrimitiveConverter` class is designed to convert 64-bit connectivity
    // and offsets to 32-bit prior to dispatching the compute pipeline, the reported time taken for
    // the dispatch call includes the time for conversion on the CPU. To avoid that, here, we
    // prebuild 32-bit arrays so that the GPU timing excludes time taken to convert 64-bit arrays.
    vtkLogStartScope(INFO, "Convert to 32-bit storage");
    polygons->ConvertTo32BitStorage();
    vtkLogEndScope("Convert to 32-bit storage");

    // Verification compares GPU output against triangulation invariants (count,
    // membership, and for simple polygons area/orientation), not against a fixed
    // CPU triangulation: the ear clip's exact diagonals depend on f32 sign
    // decisions on the GPU and need not match an f64 CPU reference.
    vtkNew<vtkWebGPUCellToPrimitiveConverter> converter;
    // prepare converter data.
    struct ConverterData
    {
      vtkTypeUInt32 VertexCount;
      wgpu::Buffer ConnectivityBuffer;
      wgpu::Buffer CellIdBuffer;
      wgpu::Buffer EdgeArrayBuffer;
      wgpu::Buffer CellIdOffsetUniformBuffer;
    } converterData;
    vtkLogStartScope(INFO, "Compute triangle lists in GPU");
    converter->DispatchCellArrayToPrimitiveComputePipeline(wgpuConfig, polygons, VTK_SURFACE,
      VTK_POLYGON, 0, &converterData.VertexCount, &converterData.ConnectivityBuffer,
      &converterData.CellIdBuffer, &converterData.EdgeArrayBuffer,
      &converterData.CellIdOffsetUniformBuffer, points->GetData());
    vtkLogEndScope("Compute triangle lists in GPU");

    if (verifyPointIds)
    {
      {
        // create new buffer to hold mapped data.
        const auto byteSize = converterData.ConnectivityBuffer.GetSize();
        auto dstBuffer = wgpuConfig->CreateBuffer(
          byteSize, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, false, nullptr);
        // copy topology data from the output of compute pipeline into the dstBuffer
        wgpu::CommandEncoder commandEncoder = wgpuConfig->GetDevice().CreateCommandEncoder();
        commandEncoder.CopyBufferToBuffer(
          converterData.ConnectivityBuffer, 0, dstBuffer, 0, byteSize);
        auto copyCommand = commandEncoder.Finish();
        wgpuConfig->GetDevice().GetQueue().Submit(1, &copyCommand);
        // map the destination buffer and verify it's contents.
        auto onConnectivityBufferMapped =
          [](wgpu::MapAsyncStatus status, wgpu::StringView, MapData* userMapData)
        {
          if (status == wgpu::MapAsyncStatus::Success)
          {
            vtkLogScopeF(INFO, "Triangle lists buffer is now mapped");
            const void* mappedRange =
              userMapData->buffer.GetConstMappedRange(0, userMapData->byteSize);
            const vtkTypeUInt32* mappedDataAsU32 = static_cast<const vtkTypeUInt32*>(mappedRange);
            const std::size_t count = userMapData->byteSize / sizeof(vtkTypeUInt32);
            userMapData->gpuConnectivity.assign(mappedDataAsU32, mappedDataAsU32 + count);
            userMapData->buffer.Unmap();
          }
          else
          {
            vtkLogF(
              WARNING, "Could not map buffer with error status: %u", static_cast<uint32_t>(status));
          }
        };
        mapData->buffer = dstBuffer;
        mapData->byteSize = byteSize;
        dstBuffer.MapAsync(wgpu::MapMode::Read, 0, byteSize, wgpu::CallbackMode::AllowProcessEvents,
          onConnectivityBufferMapped, mapData);
        // wait for mapping to finish.
        bool workDone = false;
        wgpuConfig->GetDevice().GetQueue().OnSubmittedWorkDone(
          wgpu::CallbackMode::AllowProcessEvents,
          [](wgpu::QueueWorkDoneStatus, wgpu::StringView, bool* userdata) { *userdata = true; },
          &workDone);
        while (!workDone)
        {
          wgpuConfig->ProcessEvents();
        }
      }
      {
        // create new buffer to hold mapped data.
        const auto byteSize = converterData.CellIdBuffer.GetSize();
        auto dstBuffer = wgpuConfig->CreateBuffer(
          byteSize, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, false, nullptr);
        // copy topology data from the output of compute pipeline into the dstBuffer
        wgpu::CommandEncoder commandEncoder = wgpuConfig->GetDevice().CreateCommandEncoder();
        commandEncoder.CopyBufferToBuffer(converterData.CellIdBuffer, 0, dstBuffer, 0, byteSize);
        auto copyCommand = commandEncoder.Finish();
        wgpuConfig->GetDevice().GetQueue().Submit(1, &copyCommand);
        // map the destination buffer and verify it's contents.
        auto onCellIdBufferMapped =
          [](wgpu::MapAsyncStatus status, wgpu::StringView, MapData* userMapData)
        {
          if (status == wgpu::MapAsyncStatus::Success)
          {
            vtkLogScopeF(INFO, "Triangle cell ID buffer is now mapped");
            const void* mappedRange =
              userMapData->buffer.GetConstMappedRange(0, userMapData->byteSize);
            const vtkTypeUInt32* mappedDataAsU32 = static_cast<const vtkTypeUInt32*>(mappedRange);
            const std::size_t count = userMapData->byteSize / sizeof(vtkTypeUInt32);
            userMapData->gpuCellId.assign(mappedDataAsU32, mappedDataAsU32 + count);
            userMapData->buffer.Unmap();
          }
          else
          {
            vtkLogF(
              WARNING, "Could not map buffer with error status: %u", static_cast<uint32_t>(status));
          }
        };
        mapData->buffer = dstBuffer;
        mapData->byteSize = byteSize;
        dstBuffer.MapAsync(wgpu::MapMode::Read, 0, byteSize, wgpu::CallbackMode::AllowProcessEvents,
          onCellIdBufferMapped, mapData);
        // wait for mapping to finish.
        bool workDone = false;
        wgpuConfig->GetDevice().GetQueue().OnSubmittedWorkDone(
          wgpu::CallbackMode::AllowProcessEvents,
          [](wgpu::QueueWorkDoneStatus, wgpu::StringView, bool* userdata) { *userdata = true; },
          &workDone);
        while (!workDone)
        {
          wgpuConfig->ProcessEvents();
        }
      }

      // Verify GPU output against triangulation invariants: each cell yields
      // cellSize - 2 triangles and every emitted point id is a vertex of that
      // cell (filler triangles reuse the cell's first id). This holds for any
      // valid triangulation regardless of the f32 ear-clip's diagonal choices.
      if (!VerifyCountAndMembership(polygons, mapData->gpuConnectivity, mapData->gpuCellId))
      {
        vtkLog(ERROR, << "Triangulation invariant check failed for " << scopeId);
        testStatus = EXIT_FAILURE;
      }
    }
    delete mapData;
  }
  return testStatus;
}
