// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkArrayCalculator.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkOctreeImageToPointSetFilter.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointSetToOctreeImageFilter.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

int TestOctreeImageToPointSetFilter(int argc, char* argv[])
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
  pointSetToImageFilter->SetNumberOfPointsPerCell(300);
  pointSetToImageFilter->ProcessInputPointArrayOn();
  pointSetToImageFilter->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "sin_x");
  pointSetToImageFilter->ComputeMaxOn();
  pointSetToImageFilter->ComputeCountOn();

  // convert the octree image to a point set
  vtkNew<vtkOctreeImageToPointSetFilter> imageToPointSetFilter;
  imageToPointSetFilter->SetInputConnection(pointSetToImageFilter->GetOutputPort());
  imageToPointSetFilter->ProcessInputCellArrayOn();
  imageToPointSetFilter->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "sin_x");
  imageToPointSetFilter->SetCellArrayComponent(1);

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(imageToPointSetFilter->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.2, 0.2, 0.5);
  ren->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera();
  renWin->Render();
  iren->Initialize();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
