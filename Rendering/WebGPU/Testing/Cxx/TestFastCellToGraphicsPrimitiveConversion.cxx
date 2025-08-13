#include "vtkCellArray.h"
#include "vtkLogger.h"
#include "vtkStringFormatter.h"
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "vtkWebGPUConfiguration.h"

#include <cstdlib>

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
std::vector<vtkIdType> NumPrimitives = {
  10,
  100,
  1'000,
  10'000,
  100'000,
  1'000'000,
  5'000'000,
  10'000'000,
#if VTK_SIZEOF_VOID_P == 8
  15'000'000,
  20'000'000,
  25'000'000,
  35'000'000,
  40'000'000,
#endif
};
}

int TestFastCellToGraphicsPrimitiveConversion(int argc, char* argv[])
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
  std::size_t numParameterGroups = runBenchmarks ? ::NumPrimitives.size() : 2;
  for (std::size_t i = 0; i < numParameterGroups; ++i)
  {
    vtkNew<vtkWebGPUConfiguration> wgpuConfig;
    wgpuConfig->Initialize();

    const auto& numPrimitives = ::NumPrimitives[i];
    std::string scopeId = vtk::to_string(numPrimitives) + " cells";
    vtkLogScopeF(INFO, "%s", scopeId.c_str());

    vtkLogStartScope(INFO, "Build triangles");
    vtkNew<vtkCellArray> triangles;
    for (vtkIdType j = 0, k = 0; j < numPrimitives; ++j)
    {
      triangles->InsertNextCell({ k++, k++, k++ });
    }
    vtkLogEndScope("Build triangles");

    struct MapData
    {
      wgpu::Buffer buffer;
      std::size_t byteSize;
      vtkIdType numVertices;
    };
    MapData* mapData = new MapData();
    mapData->numVertices = triangles->GetNumberOfConnectivityIds();

    // As the `vtkWebGPUCellToPrimitiveConverter` class is designed to convert 64-bit connectivity
    // and offsets to 32-bit prior to dispatching the compute pipeline, the reported time taken for
    // the dispatch call includes the time for conversion on the CPU. To avoid that, here, we
    // prebuild 32-bit arrays so that the GPU timing excludes time taken to convert 64-bit arrays.
    vtkLogStartScope(INFO, "Convert to 32-bit storage");
    triangles->ConvertTo32BitStorage();
    vtkLogEndScope("Convert to 32-bit storage");
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
    converter->DispatchCellToPrimitiveComputePipeline(wgpuConfig, triangles, VTK_SURFACE,
      VTK_POLYGON, 0, &converterData.VertexCount, &converterData.ConnectivityBuffer,
      &converterData.CellIdBuffer, &converterData.EdgeArrayBuffer,
      &converterData.CellIdOffsetUniformBuffer);
    vtkLogEndScope("Compute triangle lists in GPU");

    if (verifyPointIds)
    {
      {
        // create new buffer to hold mapped data.
        const auto byteSize = converterData.ConnectivityBuffer.GetSize();
        auto dstBuffer = wgpuConfig->CreateBuffer(byteSize,
          wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, false, "ConnectivityDest");
        // copy connectivity data into the dstBuffer
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
            for (vtkIdType j = 0; j < userMapData->numVertices; j++)
            {
              if (mappedDataAsU32[j] != static_cast<vtkTypeUInt32>(j))
              {
                vtkLog(ERROR, << "Value at location " << j << " does not match. Found "
                              << mappedDataAsU32[j] << ", expected value " << j);
                break;
              }
              else
              {
                vtkLog(TRACE, << "value: " << mappedDataAsU32[j] << "|"
                              << "expected: " << j);
              }
            }
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
#if defined(WGPU_BREAKING_CHANGE_QUEUE_WORK_DONE_CALLBACK_MESSAGE)
          [](wgpu::QueueWorkDoneStatus, wgpu::StringView, bool* userdata) { *userdata = true; },
#else
          [](wgpu::QueueWorkDoneStatus, bool* userdata) { *userdata = true; },
#endif
          &workDone);
        while (!workDone)
        {
          wgpuConfig->ProcessEvents();
        }
      }
    }
    delete mapData;
  }
  return EXIT_SUCCESS;
}
