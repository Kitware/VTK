// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Every blend mode has to draw the volume and has to draw it differently from the others.

#include "vtkWebGPUVolumeTestHelpers.h"

#include "vtkCamera.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkVolumeMapper.h"

int TestWebGPUVolumeBlendModes(int argc, char* argv[])
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
  mapper->SetAverageIPScalarRange(range[0], range[1]);

  // The opacity never reaches zero, otherwise the minimum intensity mode has nothing to show.
  auto property = vtkWebGPUVolumeTest::MakeProperty(range);
  property->GetScalarOpacity(0)->RemoveAllPoints();
  property->GetScalarOpacity(0)->AddPoint(range[0], 0.3);
  property->GetScalarOpacity(0)->AddPoint(range[1], 0.8);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);

  const double background[3] = { 0.05, 0.05, 0.08 };
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(background);
  renderer->AddVolume(volume);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(320, 320);
  window->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(30);
  renderer->GetActiveCamera()->Elevation(20);
  renderer->ResetCameraClippingRange();

  const std::pair<int, const char*> modes[] = {
    { vtkVolumeMapper::COMPOSITE_BLEND, "composite" },
    { vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND, "maximum intensity" },
    { vtkVolumeMapper::MINIMUM_INTENSITY_BLEND, "minimum intensity" },
    { vtkVolumeMapper::AVERAGE_INTENSITY_BLEND, "average intensity" },
    { vtkVolumeMapper::ADDITIVE_BLEND, "additive" },
  };

  std::vector<vtkWebGPUVolumeTest::Frame> frames;
  for (const auto& mode : modes)
  {
    mapper->SetBlendMode(mode.first);
    window->Render();
    frames.push_back(vtkWebGPUVolumeTest::Capture(window));
    if (frames.back().CountLit(background) < 2000)
    {
      std::cerr << "The " << mode.second << " mode barely drew anything.\n";
      return EXIT_FAILURE;
    }
  }

  for (std::size_t i = 0; i < frames.size(); ++i)
  {
    for (std::size_t j = i + 1; j < frames.size(); ++j)
    {
      if (vtkWebGPUVolumeTest::MeanDifference(frames[i], frames[j]) < 1.0)
      {
        std::cerr << "The " << modes[i].second << " and " << modes[j].second
                  << " modes produced the same image.\n";
        return EXIT_FAILURE;
      }
    }
  }

  mapper->SetBlendModeToComposite();
  window->Render();

  const int result = vtkRegressionTestImage(window);
  if (result == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !result;
}
