// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME Test of vtkLabelPlacementMapper
// .SECTION Description
// this program tests vtkLabelPlacementMapper which uses a sophisticated algorithm to
// prune labels/icons preventing them from overlapping.

#include <vtkActor2D.h>
#include <vtkCellArray.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>

int TestTransformCoordinateUseDouble(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);

  // Create a box around the renderers
  vtkNew<vtkPolyData> poly;
  vtkNew<vtkPoints> points;

  // Shift the points so they don't fall right between 2 pixels but on the on
  // located on the top right.
  double shift = 0.0002;
  points->InsertNextPoint(0. + shift, 0. + shift, 0); // bottom-left
  points->InsertNextPoint(1. + shift, 0. + shift, 0); // bottom-right
  points->InsertNextPoint(1. + shift, 1. + shift, 0); // top-right
  points->InsertNextPoint(0. + shift, 1. + shift, 0); // top-left

  vtkNew<vtkCellArray> cells;
  cells->InsertNextCell(5);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(2);
  cells->InsertCellPoint(3);
  cells->InsertCellPoint(0);

  poly->SetPoints(points);
  poly->SetLines(cells);

  int i = 6;
  double x = 0.;
  double y = 1. / 8.;
  double width = 1. / 4.;
  double height = 1. / 8.;

  vtkNew<vtkRenderer> emptyRenderer;
  emptyRenderer->SetViewport(0, 0, width, height);
  renderWindow->AddRenderer(emptyRenderer);

  while (--i)
  {
    vtkNew<vtkRenderer> renderer;
    renderer->SetViewport(x, y, x + width, y + height);

    vtkNew<vtkCoordinate> boxCoordinate;
    boxCoordinate->SetCoordinateSystemToNormalizedViewport();
    boxCoordinate->SetViewport(renderer);

    vtkNew<vtkPolyDataMapper2D> polyDataMapper;
    polyDataMapper->SetInputData(poly);
    polyDataMapper->SetTransformCoordinate(boxCoordinate);
    polyDataMapper->SetTransformCoordinateUseDouble(true);

    vtkNew<vtkActor2D> boxActor;
    boxActor->SetMapper(polyDataMapper);

    renderer->AddViewProp(boxActor);

    renderWindow->AddRenderer(renderer);

    if (i % 2)
    {
      x += width;
      y -= height;
      height *= 2.;
    }
    else
    {
      x -= width;
      y += height;
      width *= 2.;
    }
  }

  // Render and interact
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  renderWindow->SetMultiSamples(0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return !retVal;
}
