// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Three volumes that overlap in space go straight onto consecutive ports of one mapper, each with
// its own transfer functions and without a vtkMultiVolume to hold them together. The result must
// not depend on the order the ports were connected in.

#include "vtkWebGPUVolumeTestHelpers.h"

#include "vtkCamera.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageGaussianSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkWebGPUVolumeRayCastMapper.h"

namespace
{
struct Blob
{
  double Origin[3];
  double Color[3];
};

const Blob Blobs[3] = {
  { { -24.0, -24.0, -24.0 }, { 1.0, 0.15, 0.15 } },
  { { -9.0, -24.0, -24.0 }, { 0.15, 1.0, 0.15 } },
  { { -16.5, -11.0, -24.0 }, { 0.3, 0.4, 1.0 } },
};

/// Builds the scene with the blobs handed to the mapper in the given order.
vtkWebGPUVolumeTest::Frame RenderOrder(const int order[3], vtkRenderWindow* window)
{
  vtkNew<vtkWebGPUVolumeRayCastMapper> mapper;
  mapper->JitteringOff();

  std::vector<vtkSmartPointer<vtkImageGaussianSource>> sources;
  for (int port = 0; port < 3; ++port)
  {
    const Blob& blob = Blobs[order[port]];

    vtkNew<vtkImageGaussianSource> source;
    source->SetWholeExtent(0, 47, 0, 47, 0, 47);
    source->SetCenter(24.0, 24.0, 24.0);
    source->SetMaximum(1.0);
    source->SetStandardDeviation(10.0);
    source->Update();
    source->GetOutput()->SetOrigin(blob.Origin[0], blob.Origin[1], blob.Origin[2]);
    sources.emplace_back(source);

    mapper->SetInputConnection(port, source->GetOutputPort());

    const double range[2] = { 0.0, 1.0 };
    auto property = vtkWebGPUVolumeTest::MakeProperty(range, blob.Color);
    property->SetScalarOpacityUnitDistance(6.0);
    mapper->SetVolumeProperty(port, property);
  }

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  // Only reached for ports without an override, which there are none of here.
  volume->SetProperty(vtkWebGPUVolumeTest::MakeProperty(Blobs[0].Color));

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.08, 0.08, 0.1);
  renderer->AddVolume(volume);

  window->AddRenderer(renderer);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(20);
  renderer->GetActiveCamera()->Elevation(15);
  renderer->ResetCameraClippingRange();
  window->Render();

  auto frame = vtkWebGPUVolumeTest::Capture(window);
  window->RemoveRenderer(renderer);
  return frame;
}
}

int TestWebGPUVolumeOverlap(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> window;
  window->SetSize(400, 320);
  if (!vtkWebGPUVolumeTest::CheckClass(window, "vtkWebGPURenderWindow"))
  {
    return EXIT_FAILURE;
  }
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  const int forward[3] = { 0, 1, 2 };
  const int reversed[3] = { 2, 1, 0 };
  const auto first = RenderOrder(forward, window);
  const auto second = RenderOrder(reversed, window);

  const double background[3] = { 0.08, 0.08, 0.1 };
  if (first.CountLit(background) < 1000)
  {
    std::cerr << "The three volumes barely drew anything.\n";
    return EXIT_FAILURE;
  }

  // Co-located media are blended by how much each one contributes, so connecting the inputs the
  // other way round must give the same picture up to rounding.
  const int largest = vtkWebGPUVolumeTest::LargestDifference(first, second);
  if (largest > 2)
  {
    std::cerr << "Reversing the port order changed the image by " << largest
              << " levels, so overlapping volumes are not being blended order independently.\n";
    return EXIT_FAILURE;
  }

  // Where the red and green blobs meet, neither colour should have won outright.
  bool foundMix = false;
  for (int y = 0; y < first.Height && !foundMix; ++y)
  {
    for (int x = 0; x < first.Width; ++x)
    {
      const unsigned char* pixel = first.At(x, y);
      if (pixel[0] > 40 && pixel[1] > 40 && pixel[0] + pixel[1] > 120)
      {
        foundMix = true;
        break;
      }
    }
  }
  if (!foundMix)
  {
    std::cerr << "No pixel mixes the red and the green volume, so they are not overlapping.\n";
    return EXIT_FAILURE;
  }

  const int result = vtkRegressionTestImage(window);
  if (result == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !result;
}
