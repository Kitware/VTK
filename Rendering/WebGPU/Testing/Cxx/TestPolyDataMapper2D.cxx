// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCaptionActor2D.h"
#include "vtkCellArray.h"
#include "vtkCoordinate.h"
#include "vtkElevationFilter.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"

int TestPolyDataMapper2D(int argc, char* argv[])
{
  vtkNew<vtkPolyDataMapper2D> mapper;
  vtkNew<vtkElevationFilter> elevation;

  mapper->SetInputConnection(elevation->GetOutputPort());
  mapper->ScalarVisibilityOff();

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--point-colors"))
    {
      mapper->SetScalarModeToUsePointData();
      mapper->ScalarVisibilityOn();
      break;
    }
    else if (!strcmp(argv[i], "--cell-colors"))
    {
      mapper->SetScalarModeToUseCellData();

      vtkNew<vtkPointDataToCellData> p2c;
      p2c->SetInputConnection(elevation->GetOutputPort());
      mapper->SetInputConnection(p2c->GetOutputPort());
      mapper->ScalarVisibilityOn();
      break;
    }
  }
  vtkNew<vtkActor2D> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetPointSize(10);
  actor->GetProperty()->SetLineWidth(10);
  actor->GetProperty()->SetColor(1.0, 0.4, 0.4);

  vtkNew<vtkPolyData> mesh;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(10);

  vtkNew<vtkCellArray> vertices, lines, polygons;
  mesh->SetPoints(points);
  mesh->SetPolys(polygons);
  mesh->SetLines(lines);
  mesh->SetVerts(vertices);

  points->SetPoint(0, 0.4, 0.4, 0.0);
  points->SetPoint(1, 0.4, 0.6, 0.0);
  points->SetPoint(2, 0.6, 0.4, 0.0);
  points->SetPoint(3, 0.6, 0.6, 0.0);
  points->SetPoint(4, 0.5, 0.9, 0.0);
  points->SetPoint(5, 0.1, 0.1, 0.0);
  points->SetPoint(6, 0.3, 0.2, 0.0);
  points->SetPoint(7, 0.2, 0.1, 0.0);
  points->SetPoint(8, 0.8, 0.8, 0.0);
  points->SetPoint(9, 0.8, 0.2, 0.0);

  polygons->InsertNextCell({ 0, 1, 3, 2 });
  polygons->InsertNextCell({ 1, 3, 4 });
  lines->InsertNextCell({ 5, 7, 6 });
  vertices->InsertNextCell({ 9, 8 });

  elevation->SetLowPoint(0, 0, 0);
  elevation->SetHighPoint(0, 1, 0);
  elevation->SetInputData(mesh);

  vtkNew<vtkCoordinate> NDCToViewport;
  NDCToViewport->SetCoordinateSystemToNormalizedDisplay();
  mapper->SetTransformCoordinate(NDCToViewport);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.4, 0.4, 0.4);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);

  renderWindow->SetMultiSamples(0);
  renderWindow->Render();

  const int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !retVal;
}
