// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// A volume whose source can produce sub-extents is read a brick at a time rather than whole. This
// asks that the source is never asked for the whole volume, that only the bricks the view reaches
// are read, and that the picture is the one the whole volume would have given.

#include "vtkWebGPUVolumeTestHelpers.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkInformation.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkWebGPUVolumeRayCastCoordinator.h"
#include "vtkWebGPUVolumeRayCastMapper.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"

#include <array>
#include <iostream>
#include <vector>

namespace
{
using Extent = std::array<int, 6>;
constexpr Extent Whole = { -63, 64, -63, 64, -63, 64 };

void RecordRequest(vtkObject* caller, unsigned long, void* clientData, void*)
{
  auto* requests = static_cast<std::vector<Extent>*>(clientData);
  vtkInformation* info = vtkAlgorithm::SafeDownCast(caller)->GetOutputInformation(0);
  Extent extent;
  info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent.data());
  requests->push_back(extent);
}

struct Scene
{
  vtkNew<vtkXMLImageDataReader> Reader;
  vtkNew<vtkGPUVolumeRayCastMapper> Mapper;
  vtkWebGPUVolumeRayCastMapper* WebGPU = nullptr;
  vtkNew<vtkVolume> Volume;
  vtkNew<vtkRenderer> Renderer;
  vtkNew<vtkRenderWindow> Window;
  vtkNew<vtkCallbackCommand> Observer;
  std::vector<Extent> Requests;

  Scene(const std::string& path, int thresholdMB, vtkVolumeProperty* property)
  {
    this->Reader->SetFileName(path.c_str());
    this->Observer->SetCallback(RecordRequest);
    this->Observer->SetClientData(&this->Requests);
    this->Reader->AddObserver(vtkCommand::StartEvent, this->Observer);

    this->Mapper->SetInputConnection(this->Reader->GetOutputPort());
    this->WebGPU = vtkWebGPUVolumeRayCastMapper::SafeDownCast(this->Mapper);
    if (this->WebGPU != nullptr)
    {
      this->WebGPU->SetStreamingThresholdMB(thresholdMB);
    }
    this->Volume->SetMapper(this->Mapper);
    this->Volume->SetProperty(property);
    this->Renderer->AddVolume(this->Volume);
    this->Window->SetSize(300, 300);
    this->Window->SetMultiSamples(0);
    this->Window->AddRenderer(this->Renderer);

    this->Renderer->ResetCamera();
    this->Renderer->GetActiveCamera()->Azimuth(30);
    this->Renderer->GetActiveCamera()->Elevation(20);
    this->Renderer->ResetCameraClippingRange();
  }

  /// Render until the coordinator says the picture has stopped changing. Returns how many frames
  /// that took, or 0 when it never did.
  int RenderUntilComplete(int limit = 20)
  {
    auto* coordinator = vtkWebGPUVolumeRayCastCoordinator::GetForRenderer(this->Renderer);
    for (int frame = 1; frame <= limit; ++frame)
    {
      this->Window->Render();
      if (coordinator != nullptr && coordinator->GetComplete())
      {
        return frame;
      }
    }
    return 0;
  }
};
}

int TestWebGPUVolumeStreaming(int argc, char* argv[])
{
  // Uncompressed appended data, so the reader can pull a sub-extent out of the file.
  char* temporary =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  const std::string path = std::string(temporary) + "/TestWebGPUVolumeStreaming.vti";
  delete[] temporary;
  {
    vtkNew<vtkRTAnalyticSource> source;
    source->SetWholeExtent(Whole[0], Whole[1], Whole[2], Whole[3], Whole[4], Whole[5]);
    vtkNew<vtkXMLImageDataWriter> writer;
    writer->SetInputConnection(source->GetOutputPort());
    writer->SetFileName(path.c_str());
    writer->SetDataModeToAppended();
    writer->EncodeAppendedDataOff();
    writer->SetCompressorTypeToNone();
    writer->Write();
  }

  const double range[2] = { 37.0, 277.0 };
  auto property = vtkWebGPUVolumeTest::MakeProperty(range);

  Scene whole(path, 1 << 20, property);
  if (!vtkWebGPUVolumeTest::CheckClass(whole.Mapper, "vtkWebGPUVolumeRayCastMapper"))
  {
    return EXIT_FAILURE;
  }
  const int wholeFrames = whole.RenderUntilComplete();
  const auto expected = vtkWebGPUVolumeTest::Capture(whole.Window);

  Scene streamed(path, 0, property);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(streamed.Window);
  const int streamedFrames = streamed.RenderUntilComplete();
  const auto picture = vtkWebGPUVolumeTest::Capture(streamed.Window);
  std::cout << "complete after " << wholeFrames << " frame(s) whole, " << streamedFrames
            << " streamed\n";
  // A streamed brick is also the first look at its cells, which are classified the frame after.
  if (wholeFrames != 1 || streamedFrames < 2)
  {
    std::cerr << "Expected the whole volume to settle in one frame and the streamed one to take "
                 "at least a second.\n";
    return EXIT_FAILURE;
  }

  if (streamed.WebGPU == nullptr || !streamed.WebGPU->IsStreaming(0) ||
    whole.WebGPU->IsStreaming(0))
  {
    std::cerr << "Only the scene with a zero threshold should be streamed.\n";
    return EXIT_FAILURE;
  }
  for (const Extent& extent : streamed.Requests)
  {
    if (extent == Whole)
    {
      std::cerr << "The streamed volume asked its source for the whole extent.\n";
      return EXIT_FAILURE;
    }
  }
  if (streamed.Requests.size() < 2)
  {
    std::cerr << "The streamed volume was read in " << streamed.Requests.size()
              << " requests, so it was not read a brick at a time.\n";
    return EXIT_FAILURE;
  }

  const int difference = vtkWebGPUVolumeTest::LargestDifference(expected, picture);
  std::cout << "streamed in " << streamed.Requests.size()
            << " requests; largest difference from the whole volume " << difference << "\n";
  if (difference > 2)
  {
    std::cerr << "Streaming changed the picture by " << difference << ".\n";
    return EXIT_FAILURE;
  }

  // Close in on one corner: the bricks the rays no longer reach must not be read.
  Scene corner(path, 0, property);
  corner.Renderer->GetActiveCamera()->SetFocalPoint(-50, -50, -50);
  corner.Renderer->GetActiveCamera()->Zoom(6.0);
  corner.Renderer->ResetCameraClippingRange();
  corner.RenderUntilComplete();
  std::cout << "a view of one corner read " << corner.Requests.size() << " of "
            << streamed.Requests.size() << "\n";
  if (corner.Requests.size() >= streamed.Requests.size())
  {
    std::cerr << "A view of one corner read as much as a view of the whole volume.\n";
    return EXIT_FAILURE;
  }

  if (vtkRegressionTestImage(streamed.Window) == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return EXIT_SUCCESS;
}
