// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkFiniteDifferenceGradientEstimator.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "vtkDebugLeaks.h"
#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

// Create an 8x7 grid of render windows in a renderer and render a volume
// using various techniques for testing purposes
int TestFinalColorWindowLevel(int argc, char* argv[])
{

  // Create the renderers, render window, and interactor
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);

  // Read the data from a vtk file
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.vtk");
  vtkNew<vtkStructuredPointsReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  // Create a transfer function mapping scalar value to opacity
  vtkNew<vtkPiecewiseFunction> oTFun;
  oTFun->AddSegment(10, 0.0, 255, 0.3);

  // Create a transfer function mapping scalar value to color (color)
  vtkNew<vtkDoubleArray> ctPoints;
  ctPoints->InsertNextValue(0);
  ctPoints->InsertNextValue(64);
  ctPoints->InsertNextValue(128);
  ctPoints->InsertNextValue(192);
  ctPoints->InsertNextValue(255);
  vtkNew<vtkDoubleArray> ctColors;
  ctColors->SetNumberOfComponents(3);
  ctColors->InsertNextTuple3(1.0, 0.0, 0.0);
  ctColors->InsertNextTuple3(1.0, 1.0, 0.0);
  ctColors->InsertNextTuple3(0.0, 1.0, 0.0);
  ctColors->InsertNextTuple3(0.0, 1.0, 1.0);
  ctColors->InsertNextTuple3(0.0, 0.0, 1.0);
  vtkNew<vtkColorTransferFunction> cTFun;
  cTFun->AllowDuplicateScalarsOn();
  cTFun->AddRGBPoints(ctPoints, ctColors);

  vtkNew<vtkVolumeProperty> property;
  property->SetShade(0);
  property->SetAmbient(0.3);
  property->SetDiffuse(1.0);
  property->SetSpecular(0.2);
  property->SetSpecularPower(50.0);
  property->SetScalarOpacity(oTFun);
  property->SetColor(cTFun);
  property->SetInterpolationTypeToLinear();

  vtkNew<vtkFixedPointVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkVolume> volume;
  volume->SetProperty(property);
  volume->SetMapper(mapper);
  ren->AddViewProp(volume);

  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);

  mapper->SetFinalColorWindow(.5);
  mapper->SetFinalColorLevel(.75);

  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);

  // Interact with the data at 3 frames per second
  iren->SetDesiredUpdateRate(3.0);
  iren->SetStillUpdateRate(0.001);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
