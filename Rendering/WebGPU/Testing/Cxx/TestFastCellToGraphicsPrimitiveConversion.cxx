#include "vtkCellArray.h"
#include "vtkLogger.h"
#include "vtkStringFormatter.h"
#include "vtkWebGPUCellToPrimitiveConverter.h"
#include "vtkWebGPUConfiguration.h"

#include "vtk_wgpu.h" // for WebGPU C API

#include <cstdlib>

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
      WGPUBuffer buffer;
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
      WGPUBuffer ConnectivityBuffer;
      WGPUBuffer CellIdBuffer;
      WGPUBuffer EdgeArrayBuffer;
      WGPUBuffer CellIdOffsetUniformBuffer;
    } converterData;
    vtkLogStartScope(INFO, "Compute triangle lists in GPU");
    converter->DispatchCellArrayToPrimitiveComputePipeline(wgpuConfig, triangles, VTK_SURFACE,
      VTK_POLYGON, 0, &converterData.VertexCount, &converterData.ConnectivityBuffer,
      &converterData.CellIdBuffer, &converterData.EdgeArrayBuffer,
      &converterData.CellIdOffsetUniformBuffer);
    vtkLogEndScope("Compute triangle lists in GPU");

    if (verifyPointIds)
    {
      {
        // create new buffer to hold mapped data.
        WGPUBuffer connectivityBuffer = converterData.ConnectivityBuffer;
        const auto byteSize = wgpuBufferGetSize(connectivityBuffer);
        WGPUBuffer dstBuffer = wgpuConfig->CreateBuffer(byteSize,
          static_cast<WGPUBufferUsage>(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead), false,
          "ConnectivityDest");
        WGPUDevice device = wgpuConfig->GetDevice();
        // copy connectivity data into the dstBuffer
        WGPUCommandEncoder commandEncoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(
          commandEncoder, connectivityBuffer, 0, dstBuffer, 0, byteSize);
        WGPUCommandBuffer copyCommand = wgpuCommandEncoderFinish(commandEncoder, nullptr);
        WGPUQueue queue = wgpuDeviceGetQueue(device);
        wgpuQueueSubmit(queue, 1, &copyCommand);
        // map the destination buffer and verify it's contents.
        auto onConnectivityBufferMapped =
          [](WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*)
        {
          auto* userMapData = static_cast<MapData*>(userdata1);
          if (status == WGPUMapAsyncStatus_Success)
          {
            vtkLogScopeF(INFO, "Triangle lists buffer is now mapped");
            const void* mappedRange =
              wgpuBufferGetConstMappedRange(userMapData->buffer, 0, userMapData->byteSize);
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
            wgpuBufferUnmap(userMapData->buffer);
          }
          else
          {
            vtkLogF(
              WARNING, "Could not map buffer with error status: %u", static_cast<uint32_t>(status));
          }
        };
        mapData->buffer = dstBuffer;
        mapData->byteSize = byteSize;
        WGPUBufferMapCallbackInfo mapCallbackInfo = {};
        mapCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        mapCallbackInfo.callback = onConnectivityBufferMapped;
        mapCallbackInfo.userdata1 = mapData;
        wgpuBufferMapAsync(dstBuffer, WGPUMapMode_Read, 0, byteSize, mapCallbackInfo);
        // wait for mapping to finish.
        bool workDone = false;
        WGPUQueueWorkDoneCallbackInfo workDoneInfo = {};
        workDoneInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        workDoneInfo.callback = [](WGPUQueueWorkDoneStatus, WGPUStringView, void* userdata1, void*)
        { *static_cast<bool*>(userdata1) = true; };
        workDoneInfo.userdata1 = &workDone;
        wgpuQueueOnSubmittedWorkDone(queue, workDoneInfo);
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
