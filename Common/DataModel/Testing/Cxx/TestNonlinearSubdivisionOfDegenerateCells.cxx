// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPoints.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridGeometryFilter.h"
#include <vtkCamera.h>

int TestNonlinearSubdivisionOfDegenerateCells(int argc, char* argv[])
{
  vtkNew<vtkUnstructuredGrid> grid;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(5);
  points->SetPoint(0, 0, 0, 0);
  points->SetPoint(1, 1, 0, 0);
  points->SetPoint(2, 0, 1, 0);
  points->SetPoint(3, 1, 1, 0);
  points->SetPoint(4, 0.5, 0.5, 0.5);
  grid->SetPoints(points);
  vtkIdType connectivity1[8] = { 4, 1, 3, 4, 0, 0, 0, 0 };
  vtkIdType connectivity2[8] = { 0, 0, 0, 0, 3, 4, 4, 2 };
  grid->InsertNextCell(VTK_LAGRANGE_HEXAHEDRON, 8, connectivity1);
  grid->InsertNextCell(VTK_LAGRANGE_HEXAHEDRON, 8, connectivity2);

  // extract surface
  vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
  surfaceFilter->SetInputData(grid);
  surfaceFilter->SetNonlinearSubdivisionLevel(3);
  surfaceFilter->PassThroughCellIdsOff();
  surfaceFilter->PassThroughPointIdsOff();
  surfaceFilter->FastModeOn();
  surfaceFilter->Update();

  // Create a mapper and actor
  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(surfaceFilter->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkProperty> prop = vtkSmartPointer<vtkProperty>::New();
  prop->LightingOff();
  prop->SetRepresentationToSurface();
  prop->EdgeVisibilityOff();
  prop->SetOpacity(0.5);

  // set property
  actor->SetProperty(prop);

  // Visualize
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Azimuth(30);
  renderer->GetActiveCamera()->Elevation(10);
  renderWindow->SetSize(600, 600);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (retVal == vtkRegressionTester::PASSED) ? EXIT_SUCCESS : EXIT_FAILURE;
}
