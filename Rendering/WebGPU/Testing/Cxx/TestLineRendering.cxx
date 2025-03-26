// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkConeSource.h"
#include "vtkElevationFilter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkObject.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSphereSource.h"

int TestLineRendering(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  // the one and only true triangle
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->InsertPoint(0, -100, -100, 0.0);
  points->InsertPoint(1, 0.0, 150, 0.0);
  points->InsertPoint(2, 100, -100, 0.0);
  points->InsertPoint(3, -200, -200, 0.0);
  points->InsertPoint(4, 0.0, 250, 0.0);
  points->InsertPoint(5, 200, -200, 0.0);
  points->InsertPoint(6, -300, -300, 0.0);
  points->InsertPoint(7, 0.0, 350, 0.0);
  points->InsertPoint(8, 300, -300, 0.0);
  points->InsertPoint(9, -400, -400, 0.0);
  points->InsertPoint(10, 0.0, 450, 0.0);
  points->InsertPoint(11, 400, -400, 0.0);
  polydata->SetPoints(points);
  vtkNew<vtkCellArray> lines;
  lines->InsertNextCell({ 0, 1 });
  lines->InsertNextCell({ 1, 2 });
  lines->InsertNextCell({ 3, 4, 5 });
  lines->InsertNextCell({ 6, 7, 8 });
  lines->InsertNextCell({ 11, 10, 9 });
  polydata->SetLines(lines);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->DebugOn();
  mapper->SetInputData(polydata);

  vtkNew<vtkActor> actor;
  actor->GetProperty()->SetLineWidth(4);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderer->ResetCamera();
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->Render();

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  renWin->Render();

  iren->Start();
  return 0;
}
