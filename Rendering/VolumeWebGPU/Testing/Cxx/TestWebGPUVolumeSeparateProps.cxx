// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Three volumes that overlap in space, added to the renderer as three ordinary vtkVolume props
// with a mapper and a property each. Nothing is assembled by hand: no vtkMultiVolume, no second
// input port. They must still be ray cast together, blend order independently, and land on the
// same picture as the equivalent one mapper arrangement.

#include "vtkWebGPUVolumeTestHelpers.h"

#include "vtkCamera.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageGaussianSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkWebGPUVolumeRayCastCoordinator.h"
#include "vtkWebGPUVolumeRayCastMapper.h"

namespace
{
struct Blob
{
  double Position[3];
  double Color[3];
};

const Blob Blobs[3] = {
  { { 0.0, 0.0, 0.0 }, { 1.0, 0.15, 0.15 } },
  { { 15.0, 0.0, 0.0 }, { 0.15, 1.0, 0.15 } },
  { { 7.5, 13.0, 0.0 }, { 0.3, 0.4, 1.0 } },
};

vtkSmartPointer<vtkImageGaussianSource> MakeBlob()
{
  vtkNew<vtkImageGaussianSource> source;
  source->SetWholeExtent(0, 47, 0, 47, 0, 47);
  source->SetCenter(24.0, 24.0, 24.0);
  source->SetMaximum(1.0);
  source->SetStandardDeviation(10.0);
  source->Update();
  source->GetOutput()->SetOrigin(-24.0, -24.0, -24.0);
  return source;
}

vtkSmartPointer<vtkVolumeProperty> MakeBlobProperty(const double rgb[3])
{
  const double range[2] = { 0.0, 1.0 };
  auto property = vtkWebGPUVolumeTest::MakeProperty(range, rgb);
  property->SetScalarOpacityUnitDistance(6.0);
  property->GetScalarOpacity(0)->AddPoint(0.15, 0.0);
  return property;
}

void PointCamera(vtkRenderer* renderer)
{
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(20);
  renderer->GetActiveCamera()->Elevation(15);
  renderer->ResetCameraClippingRange();
}

/// One prop per blob, added in the given order.
vtkWebGPUVolumeTest::Frame RenderAsProps(const int order[3], vtkRenderWindow* window)
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.08, 0.08, 0.1);

  std::vector<vtkSmartPointer<vtkImageGaussianSource>> sources;
  for (int slot = 0; slot < 3; ++slot)
  {
    const Blob& blob = Blobs[order[slot]];
    auto source = MakeBlob();
    sources.push_back(source);

    vtkNew<vtkGPUVolumeRayCastMapper> mapper;
    mapper->SetInputConnection(source->GetOutputPort());
    if (auto* webgpu = vtkWebGPUVolumeRayCastMapper::SafeDownCast(mapper))
    {
      webgpu->JitteringOff();
    }

    vtkNew<vtkVolume> volume;
    volume->SetMapper(mapper);
    volume->SetProperty(MakeBlobProperty(blob.Color));
    volume->SetPosition(blob.Position[0], blob.Position[1], blob.Position[2]);
    renderer->AddVolume(volume);
  }

  window->AddRenderer(renderer);
  PointCamera(renderer);
  window->Render();

  auto frame = vtkWebGPUVolumeTest::Capture(window);
  window->RemoveRenderer(renderer);
  return frame;
}

/// The same three blobs, this time on three ports of a single mapper.
vtkWebGPUVolumeTest::Frame RenderAsPorts(vtkRenderWindow* window)
{
  vtkNew<vtkWebGPUVolumeRayCastMapper> mapper;
  mapper->JitteringOff();

  std::vector<vtkSmartPointer<vtkImageGaussianSource>> sources;
  for (int port = 0; port < 3; ++port)
  {
    auto source = MakeBlob();
    // A port carries no transform of its own, so the blob is moved in the data instead.
    source->GetOutput()->SetOrigin(-24.0 + Blobs[port].Position[0], -24.0 + Blobs[port].Position[1],
      -24.0 + Blobs[port].Position[2]);
    sources.push_back(source);
    mapper->SetInputConnection(port, source->GetOutputPort());
    mapper->SetVolumeProperty(port, MakeBlobProperty(Blobs[port].Color));
  }

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(MakeBlobProperty(Blobs[0].Color));

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.08, 0.08, 0.1);
  renderer->AddVolume(volume);

  window->AddRenderer(renderer);
  PointCamera(renderer);
  window->Render();

  auto frame = vtkWebGPUVolumeTest::Capture(window);
  window->RemoveRenderer(renderer);
  return frame;
}
}

int TestWebGPUVolumeSeparateProps(int argc, char* argv[])
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
  const auto first = RenderAsProps(forward, window);
  const auto second = RenderAsProps(reversed, window);

  const double background[3] = { 0.08, 0.08, 0.1 };
  if (first.CountLit(background) < 1000)
  {
    std::cerr << "The three volume props barely drew anything.\n";
    return EXIT_FAILURE;
  }

  // Three props, three mappers, one ray march: adding them the other way round cannot change
  // what comes out.
  const int largest = vtkWebGPUVolumeTest::LargestDifference(first, second);
  if (largest > 2)
  {
    std::cerr << "Adding the volume props in the other order changed the image by " << largest
              << " levels, so they are not being ray cast together.\n";
    return EXIT_FAILURE;
  }

  // Where the red and the green blob meet, neither colour should have won outright.
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

  // Separate props and several inputs on one mapper are two ways of saying the same thing.
  const auto ports = RenderAsPorts(window);
  const double drift = vtkWebGPUVolumeTest::MeanDifference(first, ports);
  if (drift > 1.0)
  {
    std::cerr << "Three props and three ports of one mapper differ by " << drift
              << " levels on average; they should describe the same scene.\n";
    return EXIT_FAILURE;
  }

  const int result = vtkRegressionTestImage(window);
  if (result == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !result;
}
