// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// AMR reaches a GPU ray cast mapper by being resampled onto a regular grid and handed to a
// vtkSmartVolumeMapper, which is what vtkAMRVolumeMapper does inside. Resampling it here rather
// than leaving it to the camera driven region that mapper picks keeps the test about the volume
// rendering. It runs against every backend that is built, so it says that an AMR dataset draws
// the same way whichever ray cast mapper the object factory hands over.

#include "vtkAMRGaussianPulseSource.h"
#include "vtkAMRResampleFilter.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeDataIterator.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

int TestAMRVolumeMapper(int argc, char* argv[])
{
  // A pulse refined twice, so there is more than one level for the resampler to reach into.
  vtkNew<vtkAMRGaussianPulseSource> source;
  source->SetDimension(3);
  source->SetNumberOfLevels(3);
  source->SetRefinementRatio(2);
  source->SetRootSpacing(0.5);
  source->SetPulseWidth(0.5, 0.5, 0.5);
  source->SetPulseAmplitude(10.0);
  source->Update();

  double bounds[6];
  source->GetOutput()->GetBounds(bounds);

  vtkNew<vtkAMRResampleFilter> resample;
  resample->SetInputConnection(source->GetOutputPort());
  resample->SetDemandDrivenMode(0);
  resample->SetNumberOfSamples(64, 64, 64);
  resample->SetMin(bounds[0], bounds[2], bounds[4]);
  resample->SetMax(bounds[1], bounds[3], bounds[5]);
  resample->Update();

  vtkMultiBlockDataSet* resampled = resample->GetOutput();
  vtkUniformGrid* grid = resampled != nullptr && resampled->GetNumberOfBlocks() == 1
    ? vtkUniformGrid::SafeDownCast(resampled->GetBlock(0))
    : nullptr;
  vtkDataArray* pulse =
    grid != nullptr ? grid->GetPointData()->GetArray("Gaussian-Pulse") : nullptr;
  if (pulse == nullptr)
  {
    std::cerr << "The AMR dataset did not resample onto a grid carrying the pulse.\n";
    return EXIT_FAILURE;
  }

  double range[2];
  pulse->GetRange(range);
  std::cout << "pulse resampled over " << range[0] << " to " << range[1] << "\n";

  vtkNew<vtkImageData> image;
  image->SetDimensions(grid->GetDimensions());
  image->SetSpacing(grid->GetSpacing());
  image->SetOrigin(grid->GetOrigin());
  image->GetPointData()->SetScalars(pulse);

  vtkNew<vtkSmartVolumeMapper> mapper;
  mapper->SetInputData(image);
  mapper->SetRequestedRenderModeToGPU();

  // Only the brightest part of the pulse is drawn, so what lands on the screen is a blob about
  // the middle rather than a haze filling the grid.
  vtkNew<vtkColorTransferFunction> color;
  color->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
  color->AddRGBPoint(range[1], 1.0, 0.9, 0.2);

  vtkNew<vtkPiecewiseFunction> opacity;
  opacity->AddPoint(range[0], 0.0);
  opacity->AddPoint(range[0] + 0.5 * (range[1] - range[0]), 0.0);
  opacity->AddPoint(range[1], 0.9);

  vtkNew<vtkVolumeProperty> property;
  property->SetColor(color);
  property->SetScalarOpacity(opacity);
  property->SetScalarOpacityUnitDistance(0.1);
  property->SetInterpolationTypeToLinear();

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  renderer->AddVolume(volume);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(300, 300);
  window->SetMultiSamples(0);
  window->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(30);
  renderer->GetActiveCamera()->Elevation(20);
  renderer->ResetCameraClippingRange();
  window->Render();

  vtkNew<vtkUnsignedCharArray> pixels;
  window->GetPixelData(0, 0, 299, 299, /*front=*/1, pixels);
  const auto at = [&pixels](int x, int y, int channel)
  { return static_cast<int>(pixels->GetValue(3 * (y * 300 + x) + channel)); };

  // The source lays its domain out around the pulse rather than centring it, so where the pulse
  // lands on the screen is worked out rather than assumed.
  renderer->SetWorldPoint(0.0, 0.0, 0.0, 1.0);
  renderer->WorldToDisplay();
  double display[3];
  renderer->GetDisplayPoint(display);
  const int px = static_cast<int>(display[0]);
  const int py = static_cast<int>(display[1]);
  std::cout << "the pulse projects to " << px << ", " << py << "\n";
  if (px < 20 || px > 279 || py < 20 || py > 279)
  {
    std::cerr << "The pulse does not project into the picture, so the camera is wrong.\n";
    return EXIT_FAILURE;
  }

  int atPulse = 0;
  for (int y = py - 15; y <= py + 15; ++y)
  {
    for (int x = px - 15; x <= px + 15; ++x)
    {
      atPulse = std::max({ atPulse, at(x, y, 0), at(x, y, 1), at(x, y, 2) });
    }
  }

  // Only the brightest half of the pulse is given any opacity, so what is drawn is a blob rather
  // than a haze filling the resampled box.
  int lit = 0;
  for (int y = 0; y < 300; ++y)
  {
    for (int x = 0; x < 300; ++x)
    {
      lit += (at(x, y, 0) > 12 || at(x, y, 1) > 12 || at(x, y, 2) > 12) ? 1 : 0;
    }
  }
  const double covered = lit / (300.0 * 300.0);
  std::cout << "brightest at the pulse " << atPulse << ", " << covered << " of the picture lit\n";

  if (atPulse < 40)
  {
    std::cerr << "The resampled AMR pulse did not reach the picture.\n";
    return EXIT_FAILURE;
  }
  if (covered > 0.35)
  {
    std::cerr << "Too much of the picture is lit for what was drawn to be the pulse.\n";
    return EXIT_FAILURE;
  }

  if (vtkRegressionTestImage(window) == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return EXIT_SUCCESS;
}
