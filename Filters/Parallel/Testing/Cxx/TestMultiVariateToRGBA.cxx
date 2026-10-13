// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Volume render a four component dataset using the RGBA output of
 * vtkMultiVariateToRGBA. Also checks that the computed colors match a
 * color wheel with the orientation reported by the filter.
 */

#include <vtkAlgorithmOutput.h>
#include <vtkArrayCalculator.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataSet.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMath.h>
#include <vtkMultiVariateToRGBA.h>
#include <vtkNew.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>
#include <vtkTestingObjectFactory.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

#include <algorithm>
#include <cmath>
#include <iostream>

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

namespace
{
/**
 * Returns true if the color of every point is the color found at the
 * position of the point on a color wheel with the orientation reported by
 * the filter (GetColorWheelStartAngle, GetColorWheelClockwiseOrdering).
 */
bool ColorsMatchColorWheel(vtkAlgorithmOutput* input, bool flipY)
{
  vtkNew<vtkMultiVariateToRGBA> filter;
  filter->SetInputConnection(input);
  filter->SetStoreGBC(true);
  filter->SetFlipY(flipY);
  filter->Update();
  vtkPointData* pointData =
    vtkDataSet::SafeDownCast(filter->GetOutputDataObject(0))->GetPointData();
  vtkDataArray* gbc = pointData->GetArray("gbc");
  vtkDataArray* rgba = pointData->GetArray("rgba");
  if (!gbc || !rgba)
  {
    std::cerr << "Missing gbc or rgba array for FlipY " << flipY << std::endl;
    return false;
  }
  const double startAngle = filter->GetColorWheelStartAngle();
  const double direction = filter->GetColorWheelClockwiseOrdering() ? -1.0 : 1.0;
  double maxDifference = 0;
  vtkIdType numberOfColors = 0;
  for (vtkIdType i = 0; i < gbc->GetNumberOfTuples(); ++i)
  {
    double xy[2] = { gbc->GetComponent(i, 0), gbc->GetComponent(i, 1) };
    double radius = std::hypot(xy[0], xy[1]);
    if (radius < 1e-6)
    {
      // the angle is not defined at the center
      continue;
    }
    double angle = vtkMath::DegreesFromRadians(std::atan2(xy[1], xy[0]));
    double hue = std::fmod(direction * (angle - startAngle), 360.0);
    hue = (hue < 0 ? hue + 360.0 : hue) / 360.0;
    double rgb[3];
    vtkMultiVariateToRGBA::HSLToRGB(
      hue, std::min(radius, 1.0), vtkMultiVariateToRGBA::ColorLightness, rgb);
    for (int c = 0; c < 3; ++c)
    {
      maxDifference = std::max(maxDifference, std::abs(rgb[c] - rgba->GetComponent(i, c)));
    }
    ++numberOfColors;
  }
  if (numberOfColors == 0 || maxDifference > 1e-6)
  {
    std::cerr << "Colors do not match the color wheel for FlipY " << flipY << ": compared "
              << numberOfColors << " colors, maximum difference " << maxDifference << std::endl;
    return false;
  }
  return true;
}
}

int TestMultiVariateToRGBA(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif
  controller->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(controller);

  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();

  // Downsampled MIEC (mixed ionic-electronic conductor) dataset from:
  // Ge, M., Huang, X., Yan, H. et al. Three-dimensional imaging of grain
  // boundaries via quantitative fluorescence X-ray tomography analysis.
  // Commun Mater 3, 37 (2022). https://doi.org/10.1038/s43246-022-00259-x
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(
    (dataRoot + "/Data/Ce_Co_Fe_Gd_small_10.1038_s43246-022-00259-x.vti").c_str());

  if (!ColorsMatchColorWheel(reader->GetOutputPort(), true) ||
    !ColorsMatchColorWheel(reader->GetOutputPort(), false))
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkMultiVariateToRGBA> colorFilter;
  colorFilter->SetInputConnection(reader->GetOutputPort());
  colorFilter->Update();

  // find opacity range
  vtkImageData* data = vtkImageData::SafeDownCast(colorFilter->GetOutputDataObject(0));
  vtkDataArray* colors = data->GetPointData()->GetArray("rgba");
  double* opacityRange = colors->GetRange(3);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(colorFilter->GetOutputPort());
  mapper->AutoAdjustSampleDistancesOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectScalarArray("rgba");

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(opacityRange[0], 0.0);
  scalarOpacity->AddPoint(opacityRange[1], 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetIndependentComponents(false);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderer> renderer;
  renderer->AddVolume(volume);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 600);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  iren->SetInteractorStyle(style);

  renderWindow->Render();
  renderer->ResetCamera();
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->Azimuth(180);
  camera->Zoom(1.5);
  iren->Initialize();

  // Changing IndependentComponents on an already rendered volume used to
  // crash the GPU mapper.
  volumeProperty->SetIndependentComponents(true);
  renderWindow->Render();
  volumeProperty->SetIndependentComponents(false);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  controller->Finalize();
  vtkMultiProcessController::SetGlobalController(nullptr);
  return retVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
