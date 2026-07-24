// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// The proxy box stops covering the viewport once the near plane cuts into it, so the mapper
// switches to a full screen pass. Walk the camera into the volume and check it keeps drawing.

#include "vtkWebGPUVolumeTestHelpers.h"

#include "vtkCamera.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"

int TestWebGPUVolumeCameraInside(int argc, char* argv[])
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

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(vtkWebGPUVolumeTest::MakeProperty(range));

  const double background[3] = { 0.05, 0.05, 0.08 };
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(background);
  renderer->AddVolume(volume);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(320, 320);
  window->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->SetViewUp(0.0, 1.0, 0.0);

  // Well outside, just outside with the near plane already inside, and deep inside.
  const double distances[] = { 160.0, 34.0, 8.0 };
  for (double distance : distances)
  {
    camera->SetPosition(0.0, 0.0, distance);
    renderer->ResetCameraClippingRange();
    window->Render();

    const auto frame = vtkWebGPUVolumeTest::Capture(window);
    const std::size_t lit = frame.CountLit(background);
    const std::size_t total = static_cast<std::size_t>(frame.Width) * frame.Height;
    if (lit < total / 20)
    {
      std::cerr << "With the camera " << distance << " away the volume only covered " << lit
                << " of " << total << " pixels.\n";
      return EXIT_FAILURE;
    }
  }

  // From inside, the volume surrounds the camera and should fill the viewport.
  camera->SetPosition(0.0, 0.0, 8.0);
  renderer->ResetCameraClippingRange();
  window->Render();
  const auto inside = vtkWebGPUVolumeTest::Capture(window);
  const std::size_t total = static_cast<std::size_t>(inside.Width) * inside.Height;
  if (inside.CountLit(background) < (9 * total) / 10)
  {
    std::cerr << "The volume did not fill the viewport with the camera inside it.\n";
    return EXIT_FAILURE;
  }

  const int result = vtkRegressionTestImage(window);
  if (result == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !result;
}
