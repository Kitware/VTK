// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Adaptive sampling must not change what the volume looks like, and empty space skipping must
// actually reach for the block classification rather than quietly doing nothing.

#include "vtkWebGPUVolumeTestHelpers.h"

#include "vtkCamera.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkWebGPUVolumeRayCastMapper.h"

int TestWebGPUVolumeAdaptiveSampling(int argc, char* argv[])
{
  vtkWebGPUVolumeTest::UseWebGPUBackend();

  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(-31, 31, -31, 31, -31, 31);
  source->Update();

  double range[2];
  source->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  vtkNew<vtkGPUVolumeRayCastMapper> baseMapper;
  auto* mapper = vtkWebGPUVolumeRayCastMapper::SafeDownCast(baseMapper);
  if (mapper == nullptr)
  {
    std::cerr << "The object factory did not hand back a vtkWebGPUVolumeRayCastMapper.\n";
    return EXIT_FAILURE;
  }
  mapper->SetInputConnection(source->GetOutputPort());
  // Jitter would make the two renders differ for reasons that have nothing to do with the step.
  mapper->JitteringOff();

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(vtkWebGPUVolumeTest::MakeProperty(range));

  const double background[3] = { 0.05, 0.05, 0.08 };
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(background);
  renderer->AddVolume(volume);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(400, 400);
  window->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(30);
  renderer->GetActiveCamera()->Elevation(20);
  renderer->ResetCameraClippingRange();

  mapper->AdaptiveSamplingOn();
  window->Render();
  const auto adaptive = vtkWebGPUVolumeTest::Capture(window);

  mapper->AdaptiveSamplingOff();
  window->Render();
  const auto fixed = vtkWebGPUVolumeTest::Capture(window);

  if (fixed.CountLit(background) < 5000)
  {
    std::cerr << "Nothing much was drawn, so there is nothing to compare.\n";
    return EXIT_FAILURE;
  }

  // Blocks are only sampled coarsely where the transfer functions barely change over them, so
  // varying the step must stay close to marching at a fixed one.
  const double mean = vtkWebGPUVolumeTest::MeanDifference(adaptive, fixed);
  if (mean > 3.0)
  {
    std::cerr << "Adaptive sampling moved the image by " << mean
              << " levels on average, which is more than rounding.\n";
    return EXIT_FAILURE;
  }

  // Treat every block as empty and the ray march should have nothing left to composite. This
  // fails if the skip is never taken.
  mapper->AdaptiveSamplingOn();
  mapper->EmptySpaceSkippingOn();
  mapper->SetEmptySpaceOpacityThreshold(1.0);
  window->Render();
  const auto skipped = vtkWebGPUVolumeTest::Capture(window);
  if (skipped.CountLit(background) != 0)
  {
    std::cerr << "Blocks classified as empty were still sampled: " << skipped.CountLit(background)
              << " pixels were drawn.\n";
    return EXIT_FAILURE;
  }

  mapper->SetEmptySpaceOpacityThreshold(0.001);
  window->Render();

  const int result = vtkRegressionTestImage(window);
  if (result == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !result;
}
