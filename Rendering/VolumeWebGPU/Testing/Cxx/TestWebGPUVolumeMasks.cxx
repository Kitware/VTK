// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// The two mask types vtkGPUVolumeRayCastMapper takes. The shared test suite draws label maps at a
// threshold loose enough to pass without drawing any label at all, so this asks instead that each
// label be recognisable by the colour only it was given.

#include "vtkWebGPUVolumeTestHelpers.h"

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <iostream>

namespace
{

/// A mask laid out like \a volume, holding \a left where x is below the middle and \a right above.
vtkSmartPointer<vtkImageData> MakeMask(
  vtkImageData* volume, unsigned char left, unsigned char right)
{
  int extent[6];
  volume->GetExtent(extent);
  const int middle = (extent[0] + extent[1]) / 2;

  vtkNew<vtkUnsignedCharArray> values;
  values->SetNumberOfComponents(1);
  values->SetNumberOfTuples(volume->GetNumberOfPoints());

  vtkIdType at = 0;
  for (int z = extent[4]; z <= extent[5]; ++z)
  {
    for (int y = extent[2]; y <= extent[3]; ++y)
    {
      for (int x = extent[0]; x <= extent[1]; ++x)
      {
        values->SetValue(at++, x < middle ? left : right);
      }
    }
  }

  vtkSmartPointer<vtkImageData> mask = vtkSmartPointer<vtkImageData>::New();
  mask->SetExtent(extent);
  mask->SetSpacing(volume->GetSpacing());
  mask->SetOrigin(volume->GetOrigin());
  mask->GetPointData()->SetScalars(values);
  return mask;
}

/// Average colour of the lit pixels of one half of a frame.
void AverageOfHalf(
  const vtkWebGPUVolumeTest::Frame& frame, bool leftHalf, const double background[3], double out[3])
{
  const int from = leftHalf ? 0 : frame.Width / 2;
  const int to = leftHalf ? frame.Width / 2 : frame.Width;
  double total[3] = { 0.0, 0.0, 0.0 };
  std::size_t count = 0;
  for (int y = 0; y < frame.Height; ++y)
  {
    for (int x = from; x < to; ++x)
    {
      const unsigned char* pixel = frame.At(x, y);
      bool lit = false;
      for (int c = 0; c < 3; ++c)
      {
        lit = lit || std::abs(pixel[c] - static_cast<int>(background[c] * 255.0 + 0.5)) > 12;
      }
      if (!lit)
      {
        continue;
      }
      for (int c = 0; c < 3; ++c)
      {
        total[c] += pixel[c];
      }
      ++count;
    }
  }
  for (int c = 0; c < 3; ++c)
  {
    out[c] = count > 0 ? total[c] / count : 0.0;
  }
}

} // namespace

int TestWebGPUVolumeMasks(int argc, char* argv[])
{
  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(-24, 24, -24, 24, -24, 24);
  source->Update();

  double range[2];
  source->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  if (!vtkWebGPUVolumeTest::CheckClass(mapper, "vtkWebGPUVolumeRayCastMapper"))
  {
    return EXIT_FAILURE;
  }
  mapper->SetInputConnection(source->GetOutputPort());

  // The volume's own colours are grey, so anything coloured came from a label.
  auto property = vtkWebGPUVolumeTest::MakeProperty(range);
  vtkNew<vtkColorTransferFunction> grey;
  grey->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
  grey->AddRGBPoint(range[1], 0.7, 0.7, 0.7);
  property->SetColor(grey);

  // One label draws red and the other green, both over the volume's own opacity.
  vtkNew<vtkColorTransferFunction> red;
  red->AddRGBPoint(range[0], 1.0, 0.0, 0.0);
  red->AddRGBPoint(range[1], 1.0, 0.0, 0.0);
  vtkNew<vtkColorTransferFunction> green;
  green->AddRGBPoint(range[0], 0.0, 1.0, 0.0);
  green->AddRGBPoint(range[1], 0.0, 1.0, 0.0);
  property->SetLabelColor(1, red);
  property->SetLabelColor(2, green);

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

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  // Straight down the x axis, so the two halves of the mask land on the two halves of the picture.
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(90);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->ResetCameraClippingRange();
  window->Render();

  const auto plain = vtkWebGPUVolumeTest::Capture(window);
  const std::size_t plainLit = plain.CountLit(background);
  if (plainLit == 0)
  {
    std::cerr << "The volume drew nothing before any mask was set.\n";
    return EXIT_FAILURE;
  }

  // A label map colours what it names and hides nothing, so the volume stays the size it was.
  auto labels = MakeMask(source->GetOutput(), 1, 2);
  mapper->SetMaskInput(labels);
  mapper->SetMaskTypeToLabelMap();
  mapper->SetMaskBlendFactor(1.0);
  window->Render();

  const auto labelled = vtkWebGPUVolumeTest::Capture(window);
  const std::size_t labelledLit = labelled.CountLit(background);
  const double covered = static_cast<double>(labelledLit) / plainLit;
  if (covered < 0.8 || covered > 1.25)
  {
    std::cerr << "A label map drew " << labelledLit << " lit pixels against " << plainLit
              << " unmasked, so it cut the volume rather than only recolouring it.\n";
    return EXIT_FAILURE;
  }

  double left[3];
  double right[3];
  AverageOfHalf(labelled, true, background, left);
  AverageOfHalf(labelled, false, background, right);
  std::cout << "label halves: (" << left[0] << ", " << left[1] << ", " << left[2] << ") and ("
            << right[0] << ", " << right[1] << ", " << right[2] << ")\n";

  // One half has to be the red label and the other the green one, whichever way round the camera
  // put them.
  const bool redThenGreen = left[0] > left[1] && right[1] > right[0];
  const bool greenThenRed = left[1] > left[0] && right[0] > right[1];
  if (!redThenGreen && !greenThenRed)
  {
    std::cerr << "Neither half of the picture took the colour of the label covering it.\n";
    return EXIT_FAILURE;
  }
  const double* reddest = redThenGreen ? left : right;
  const double* greenest = redThenGreen ? right : left;
  if (reddest[0] < 2.0 * reddest[2] || greenest[1] < 2.0 * greenest[2])
  {
    std::cerr << "The labelled halves are not far enough from the volume's own grey.\n";
    return EXIT_FAILURE;
  }

  // Blending all the way back to the volume's own functions has to undo the labels entirely.
  mapper->SetMaskBlendFactor(0.0);
  window->Render();
  const auto unblended = vtkWebGPUVolumeTest::Capture(window);
  const int blendChange = vtkWebGPUVolumeTest::LargestDifference(plain, unblended);
  if (blendChange > 2)
  {
    std::cerr << "A mask blend factor of zero still changed the picture by " << blendChange
              << ".\n";
    return EXIT_FAILURE;
  }

  // Halfway is halfway: the labels show, but not as strongly as they did.
  mapper->SetMaskBlendFactor(0.5);
  window->Render();
  const auto halfway = vtkWebGPUVolumeTest::Capture(window);
  const double towardsLabels = vtkWebGPUVolumeTest::MeanDifference(plain, halfway);
  const double towardsPlain = vtkWebGPUVolumeTest::MeanDifference(labelled, halfway);
  std::cout << "half blended: " << towardsLabels << " from plain, " << towardsPlain
            << " from fully labelled\n";
  if (towardsLabels <= 0.0 || towardsPlain <= 0.0)
  {
    std::cerr << "Half a mask blend factor landed on one end or the other.\n";
    return EXIT_FAILURE;
  }

  // A binary mask instead keeps only what it names, so half the volume goes.
  auto binary = MakeMask(source->GetOutput(), 0, 1);
  mapper->SetMaskInput(binary);
  mapper->SetMaskTypeToBinary();
  window->Render();

  const auto masked = vtkWebGPUVolumeTest::Capture(window);
  const std::size_t maskedLit = masked.CountLit(background);
  const double kept = static_cast<double>(maskedLit) / plainLit;
  std::cout << "binary mask kept " << kept << " of the lit pixels\n";
  if (kept > 0.75)
  {
    std::cerr << "A binary mask covering half the volume kept " << kept
              << " of the picture, so it cut nothing.\n";
    return EXIT_FAILURE;
  }
  if (kept < 0.2)
  {
    std::cerr << "A binary mask covering half the volume kept only " << kept
              << " of the picture, so it cut too much.\n";
    return EXIT_FAILURE;
  }

  mapper->SetMaskInput(labels);
  mapper->SetMaskTypeToLabelMap();
  mapper->SetMaskBlendFactor(1.0);
  window->Render();
  if (vtkRegressionTestImage(window) == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return EXIT_SUCCESS;
}
