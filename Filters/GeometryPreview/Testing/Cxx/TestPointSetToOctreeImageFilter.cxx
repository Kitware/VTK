// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkArrayCalculator.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkOpenGLGPUVolumeRayCastMapper.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointSetToOctreeImageFilter.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkVolumeProperty.h"

int TestPointSetToOctreeImageFilter(int argc, char* argv[])
{
  // Create a sphere
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(0, 0, 0);
  sphere->SetRadius(0.5);
  sphere->SetPhiResolution(2000);
  sphere->SetThetaResolution(2000);

  // create an array which is the sin of the x coordinate
  vtkNew<vtkArrayCalculator> calc;
  calc->SetInputConnection(sphere->GetOutputPort());
  calc->SetAttributeTypeToPointData();
  calc->AddCoordinateScalarVariable("coordsX", 0);
  calc->SetFunction("sin(coordsX)");
  calc->SetResultArrayName("sin_x");

  // generate an image from the sphere and calculate the count
  vtkNew<vtkPointSetToOctreeImageFilter> pointSetToImageFilter;
  pointSetToImageFilter->SetInputConnection(calc->GetOutputPort());
  pointSetToImageFilter->SetNumberOfPointsPerCell(10);
  pointSetToImageFilter->ProcessInputPointArrayOn();
  pointSetToImageFilter->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "sin_x");
  pointSetToImageFilter->ComputeMaxOff();
  pointSetToImageFilter->ComputeCountOn();
  pointSetToImageFilter->Update();
  auto image =
    vtkPartitionedDataSet::SafeDownCast(pointSetToImageFilter->GetOutput())->GetPartition(0);

  // Create transfer mapping scalar value to opacity
  vtkNew<vtkPiecewiseFunction> opacityTransferFunction;
  opacityTransferFunction->AddPoint(20, 0.0);
  opacityTransferFunction->AddPoint(255, 0.2);

  // Create transfer mapping scalar value to color
  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(64.0, 1.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
  colorTransferFunction->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

  // The property describes how the data will look
  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationTypeToLinear();

  vtkNew<vtkOpenGLGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputData(image);
  volumeMapper->SetBlendModeToMaximumIntensity();

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.2, 0.2, 0.5);
  ren->AddViewProp(volume);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(45);
  ren->GetActiveCamera()->Elevation(30);
  ren->ResetCameraClippingRange();
  renWin->Render();
  iren->Initialize();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
