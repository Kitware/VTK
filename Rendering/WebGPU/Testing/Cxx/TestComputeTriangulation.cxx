#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkLogger.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "vtkWebGPUConfiguration.h"

#include <cstdlib>
#include <sstream>

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
  std::size_t numParameterGroups = runBenchmarks ? ::ParametersCollection.size() : 3;
  for (std::size_t i = 0; i < numParameterGroups; ++i)
  {
    vtkNew<vtkWebGPUConfiguration> wgpuConfig;

    const auto& parameters = ::ParametersCollection[i];
    std::string scopeId = std::to_string(parameters.NumberOfCells) + " cells";
    vtkLogScopeF(INFO, "%s", scopeId.c_str());
    vtkLog(INFO, << CellSizeWeightsToString(parameters));

    vtkLogStartScope(INFO, "Build polygons");
    auto polygons = BuildPolygons(parameters.CellSizeWeights, parameters.NumberOfCells);
    vtkLogEndScope("Build polygons");

    struct MapData
    {
      wgpu::Buffer buffer;
      std::size_t byteSize;
      std::vector<vtkTypeUInt32> expectedValues;
    };
    MapData* mapData = new MapData();

    // As the `vtkWebGPUCellToPrimitiveConverter` class is designed to convert 64-bit connectivity
    // and offsets to 32-bit prior to dispatching the compute pipeline, the reported time taken for
    // the dispatch call includes the time for conversion on the CPU. To avoid that, here, we
    // prebuild 32-bit arrays so that the GPU timing excludes time taken to convert 64-bit arrays.
    vtkLogStartScope(INFO, "Convert to 32-bit storage");
    polygons->ConvertTo32BitStorage();
    vtkLogEndScope("Convert to 32-bit storage");

    vtkLogStartScope(INFO, "Compute triangle lists in CPU");
    auto iter = vtk::TakeSmartPointer(polygons->NewIterator());

    for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
    {
      const vtkIdType* cellPts = nullptr;
      const vtkIdType cellId = iter->GetCurrentCellId();
      vtkIdType cellSize;
      iter->GetCurrentCell(cellSize, cellPts);
      const int numSubTriangles = cellSize - 2;
      // save memory by not storing the point ids when verification is disabled
      for (int j = 0; j < numSubTriangles; ++j)
      {
        mapData->expectedValues.emplace_back(cellId);
        mapData->expectedValues.emplace_back(cellPts[0]);
        mapData->expectedValues.emplace_back(cellId);
        mapData->expectedValues.emplace_back(cellPts[j + 1]);
        mapData->expectedValues.emplace_back(cellId);
        mapData->expectedValues.emplace_back(cellPts[j + 2]);
      }
    }
    vtkLogEndScope("Compute triangle lists in CPU");
    if (!verifyPointIds)
    {
      mapData->expectedValues.clear();
    }
    vtkNew<vtkWebGPUCellToPrimitiveConverter> converter;
    // prepare converter data.
    struct ConverterData
    {
      vtkTypeUInt32 VertexCount;
      wgpu::Buffer TopologyBuffer;
      wgpu::Buffer EdgeArrayBuffer;
    } converterData[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES];
    vtkTypeUInt32* vertexCounts[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES] = {};
    wgpu::Buffer*
      topologyBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES] = {};
    wgpu::Buffer*
      edgeArrayBuffers[vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES] = {};
    for (int topologySourceTypeIdx = 0;
         topologySourceTypeIdx < vtkWebGPUCellToPrimitiveConverter::NUM_TOPOLOGY_SOURCE_TYPES;
         ++topologySourceTypeIdx)
    {
      vertexCounts[topologySourceTypeIdx] = &converterData[topologySourceTypeIdx].VertexCount;
      topologyBuffers[topologySourceTypeIdx] = &converterData[topologySourceTypeIdx].TopologyBuffer;
      edgeArrayBuffers[topologySourceTypeIdx] =
        &converterData[topologySourceTypeIdx].EdgeArrayBuffer;
    }
    vtkLogStartScope(INFO, "Compute triangle lists in GPU");
    converter->DispatchCellToPrimitiveComputePipeline(wgpuConfig, polygons, VTK_SURFACE,
      VTK_POLYGON, 0, vertexCounts, topologyBuffers, edgeArrayBuffers);
    vtkLogEndScope("Compute triangle lists in GPU");

    if (verifyPointIds)
    {
      auto& polygonData =
        converterData[vtkWebGPUCellToPrimitiveConverter::TOPOLOGY_SOURCE_POLYGONS];
      // create new buffer to hold mapped data.
      const auto byteSize = polygonData.TopologyBuffer.GetSize();
      auto dstBuffer = wgpuConfig->CreateBuffer(
        byteSize, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, false, nullptr);
      // copy topology data from the output of compute pipeline into the dstBuffer
      wgpu::CommandEncoder commandEncoder = wgpuConfig->GetDevice().CreateCommandEncoder();
      commandEncoder.CopyBufferToBuffer(polygonData.TopologyBuffer, 0, dstBuffer, 0, byteSize);
      auto copyCommand = commandEncoder.Finish();
      wgpuConfig->GetDevice().GetQueue().Submit(1, &copyCommand);
      // map the destination buffer and verify it's contents.
      auto onBufferMapped = [](wgpu::MapAsyncStatus status, wgpu::StringView, MapData* userMapData)
      {
        if (status == wgpu::MapAsyncStatus::Success)
        {
          vtkLogScopeF(INFO, "Triangle lists buffer is now mapped");
          const void* mappedRange =
            userMapData->buffer.GetConstMappedRange(0, userMapData->byteSize);
          const vtkTypeUInt32* mappedDataAsU32 = static_cast<const vtkTypeUInt32*>(mappedRange);
          for (std::size_t j = 0; j < userMapData->expectedValues.size(); j++)
          {
            if (mappedDataAsU32[j] != userMapData->expectedValues[j])
            {
              vtkLog(ERROR, << "Value at location " << j << " does not match. Found "
                            << mappedDataAsU32[j] << ", expected value "
                            << userMapData->expectedValues[j]);
              break;
            }
            else
            {
              vtkLog(TRACE, << "value: " << mappedDataAsU32[j] << "|"
                            << "expected: " << userMapData->expectedValues[j]);
            }
          }
          userMapData->buffer.Unmap();
          delete userMapData;
        }
        else
        {
          vtkLogF(
            WARNING, "Could not map buffer with error status: %u", static_cast<uint32_t>(status));
          delete userMapData;
        }
      };
      mapData->buffer = dstBuffer;
      mapData->byteSize = byteSize;
      dstBuffer.MapAsync(wgpu::MapMode::Read, 0, byteSize, wgpu::CallbackMode::AllowProcessEvents,
        onBufferMapped, mapData);
      // wait for mapping to finish.
      bool workDone = false;
      wgpuConfig->GetDevice().GetQueue().OnSubmittedWorkDone(
        wgpu::CallbackMode::AllowProcessEvents,
        [](wgpu::QueueWorkDoneStatus, bool* userdata) { *static_cast<bool*>(userdata) = true; },
        &workDone);
      while (!workDone)
      {
        wgpuConfig->ProcessEvents();
      }
    }
    else
    {
      delete mapData;
    }
  }
  return EXIT_SUCCESS;
}
