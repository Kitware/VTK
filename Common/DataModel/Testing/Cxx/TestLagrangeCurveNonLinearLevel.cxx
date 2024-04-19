// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPoints.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredGrid.h"

int TestLagrangeCurveNonLinearLevel(int argc, char* argv[])
{
  vtkNew<vtkUnstructuredGrid> dataset;
  // add points
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(4);
  points->SetPoint(0, 0, 1, 0);
  points->SetPoint(1, 0.33, 0.8, 0);
  points->SetPoint(2, 0.66, 0.5, 0);
  points->SetPoint(3, 1, 0, 0);
  dataset->SetPoints(points);
  // add cell
  vtkIdType cell[4] = { 0, 3, 1, 2 };
  dataset->InsertNextCell(VTK_LAGRANGE_CURVE, 3, cell);

  // extract surface
  vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
  surfaceFilter->SetInputData(dataset);
  surfaceFilter->SetNonlinearSubdivisionLevel(2);

  // Create a mapper and actor
  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(surfaceFilter->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Visualize
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->ResetCamera();
  renderWindow->SetSize(300, 300);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (retVal == vtkRegressionTester::PASSED) ? EXIT_SUCCESS : EXIT_FAILURE;
}
