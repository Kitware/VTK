// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Two settings that the OpenGL mapper honours and that nothing in the shared test suite exercises
// against more than one backend: the interpolation type of the property, and the window and level
// vtkGPUVolumeRayCastMapper puts over the finished colour.

#include "vtkWebGPUVolumeTestHelpers.h"

#include "vtkCamera.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <iostream>

int TestWebGPUVolumeInterpolationAndLevel(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  // Few enough voxels that nearest and linear cannot help but differ.
  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(-9, 9, -9, 9, -9, 9);
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
  property->SetInterpolationTypeToLinear();

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);

  const double background[3] = { 0.0, 0.0, 0.0 };
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(background);
  renderer->AddVolume(volume);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(300, 300);
  window->SetMultiSamples(0);
  window->AddRenderer(renderer);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(25);
  renderer->ResetCameraClippingRange();
  window->Render();

  const auto linear = vtkWebGPUVolumeTest::Capture(window);

  // Nearest neighbour has to change the picture, and has to keep drawing the same volume.
  property->SetInterpolationTypeToNearest();
  window->Render();
  const auto nearest = vtkWebGPUVolumeTest::Capture(window);

  const int interpolationChange = vtkWebGPUVolumeTest::LargestDifference(linear, nearest);
  if (interpolationChange == 0)
  {
    std::cerr << "Asking for nearest neighbour interpolation changed nothing.\n";
    return EXIT_FAILURE;
  }
  const std::size_t linearLit = linear.CountLit(background);
  const std::size_t nearestLit = nearest.CountLit(background);
  const double covered = static_cast<double>(nearestLit) / std::max<std::size_t>(linearLit, 1);
  if (covered < 0.8 || covered > 1.25)
  {
    std::cerr
      << "Nearest neighbour drew " << nearestLit << " lit pixels against " << linearLit
      << " for linear, which is a different volume rather than a differently sampled one.\n";
    return EXIT_FAILURE;
  }
  std::cout << "nearest against linear: largest change " << interpolationChange << ", coverage "
            << covered << "\n";

  // A window of one and a level of one half is what the mapper starts with, so asking for it
  // again has to leave the picture alone.
  property->SetInterpolationTypeToLinear();
  mapper->SetFinalColorWindow(1.0);
  mapper->SetFinalColorLevel(0.5);
  window->Render();
  const auto neutral = vtkWebGPUVolumeTest::Capture(window);
  const int neutralChange = vtkWebGPUVolumeTest::LargestDifference(linear, neutral);
  if (neutralChange != 0)
  {
    std::cerr << "The default window and level changed the picture by " << neutralChange << ".\n";
    return EXIT_FAILURE;
  }

  // Halving the window doubles the contrast, so what was lit gets brighter.
  mapper->SetFinalColorWindow(0.5);
  window->Render();
  const auto windowed = vtkWebGPUVolumeTest::Capture(window);
  if (vtkWebGPUVolumeTest::LargestDifference(linear, windowed) == 0)
  {
    std::cerr << "Halving the final colour window changed nothing.\n";
    return EXIT_FAILURE;
  }
  if (windowed.CountLit(background) < linearLit)
  {
    std::cerr << "Halving the window should not have dimmed the picture.\n";
    return EXIT_FAILURE;
  }

  // Raising the level darkens it again, and by enough to undo that.
  mapper->SetFinalColorWindow(1.0);
  mapper->SetFinalColorLevel(0.9);
  window->Render();
  const auto levelled = vtkWebGPUVolumeTest::Capture(window);
  if (levelled.CountLit(background) >= windowed.CountLit(background))
  {
    std::cerr << "Raising the final colour level should have darkened the picture.\n";
    return EXIT_FAILURE;
  }
  std::cout << "lit pixels: linear " << linearLit << ", window 0.5 "
            << windowed.CountLit(background) << ", level 0.9 " << levelled.CountLit(background)
            << "\n";

  return EXIT_SUCCESS;
}
