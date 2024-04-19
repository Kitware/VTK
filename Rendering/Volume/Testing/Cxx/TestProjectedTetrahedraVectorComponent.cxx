// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkProjectedTetrahedraMapper.h"

#include "vtkActor.h"
#include "vtkCellTypeSource.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

int TestProjectedTetrahedraVectorComponent(int argc, char* argv[])
{
  // Create the standard renderer, render window, and interactor.
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren1;
  renWin->AddRenderer(ren1);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->SetDesiredUpdateRate(3);

  // check for driver support
  renWin->Render();
  vtkNew<vtkProjectedTetrahedraMapper> volumeMapper;
  if (!volumeMapper->IsSupported(renWin))
  {
    volumeMapper->Delete();
    iren->Delete();
    vtkGenericWarningMacro("Projected tetrahedra is not supported. Skipping tests.");
    return 0;
  }

  // Create the data source
  vtkNew<vtkCellTypeSource> cellSource;
  cellSource->SetCellType(VTK_TETRA);
  cellSource->SetBlocksDimensions(10, 10, 10);
  cellSource->Update();
  auto dataset = cellSource->GetOutput();

  // Copy the cell points to the point array to generate a vector array
  vtkSmartPointer<vtkDataArray> pointsCopy;
  auto points = dataset->GetPoints()->GetData();
  pointsCopy.TakeReference(points->NewInstance());
  pointsCopy->DeepCopy(points);
  pointsCopy->SetName("coords");

  dataset->GetPointData()->AddArray(pointsCopy);

  // Create transfer mapping scalar value to opacity.
  vtkNew<vtkPiecewiseFunction> opacityTransferFunction;
  opacityTransferFunction->AddPoint(0.0, 0.0);
  opacityTransferFunction->AddPoint(31.0, 1.0);

  // Create transfer mapping scalar value to color.
  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(31.0, 0.0, 1.0, 0.0);

  // Color by the Y coordinate
  colorTransferFunction->SetVectorMode(vtkScalarsToColors::COMPONENT);
  colorTransferFunction->SetVectorComponent(1); // y

  // The property describes how the data will look.
  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationTypeToLinear();

  // The mapper that renders the volume data.
  volumeMapper->SetInputData(dataset);
  volumeMapper->SetScalarModeToUsePointFieldData();
  volumeMapper->SetArrayAccessMode(VTK_GET_ARRAY_BY_NAME);
  volumeMapper->SelectScalarArray("coords");

  // The volume holds the mapper and the property and can be used to
  // position/orient the volume.
  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren1->AddVolume(volume);

  renWin->SetSize(300, 300);

  ren1->ResetCamera();

  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin, 75);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  if ((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR))
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
