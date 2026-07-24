// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Renders one volume and checks that the mapper drew something with the shape and colours the
// transfer functions asked for.

#include "vtkWebGPUVolumeTestHelpers.h"

#include "vtkCamera.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"

int TestWebGPUVolumeRayCast(int argc, char* argv[])
{
  vtkWebGPUVolumeTest::UseWebGPUBackend();

  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(-31, 31, -31, 31, -31, 31);
  source->Update();

  double range[2];
  source->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  if (!vtkWebGPUVolumeTest::CheckClass(mapper, "vtkWebGPUVolumeRayCastMapper"))
  {
    return EXIT_FAILURE;
  }
  mapper->SetInputConnection(source->GetOutputPort());

  auto property = vtkWebGPUVolumeTest::MakeProperty(range);
  property->ShadeOn();

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);

  const double background[3] = { 0.05, 0.05, 0.08 };
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(background);
  renderer->AddVolume(volume);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(400, 400);
  window->AddRenderer(renderer);
  if (!vtkWebGPUVolumeTest::CheckClass(window, "vtkWebGPURenderWindow"))
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(30);
  renderer->GetActiveCamera()->Elevation(20);
  renderer->ResetCameraClippingRange();
  window->Render();

  const auto frame = vtkWebGPUVolumeTest::Capture(window);
  const std::size_t lit = frame.CountLit(background);
  const std::size_t total = static_cast<std::size_t>(frame.Width) * frame.Height;

  // The volume is framed by ResetCamera, so it should cover a good part of the viewport without
  // filling it, and the corners should be left alone.
  if (lit < total / 8 || lit > (3 * total) / 4)
  {
    std::cerr << "The volume covered " << lit << " of " << total
              << " pixels, which does not look like a framed volume.\n";
    return EXIT_FAILURE;
  }
  for (const auto& corner : { std::pair<int, int>{ 2, 2 },
         std::pair<int, int>{ frame.Width - 3, 2 }, std::pair<int, int>{ 2, frame.Height - 3 },
         std::pair<int, int>{ frame.Width - 3, frame.Height - 3 } })
  {
    const unsigned char* pixel = frame.At(corner.first, corner.second);
    for (int c = 0; c < 3; ++c)
    {
      if (std::abs(static_cast<int>(pixel[c]) - static_cast<int>(background[c] * 255.0 + 0.5)) > 12)
      {
        std::cerr << "The corner at " << corner.first << ", " << corner.second
                  << " is not background.\n";
        return EXIT_FAILURE;
      }
    }
  }

  const int result = vtkRegressionTestImage(window);
  if (result == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !result;
}
