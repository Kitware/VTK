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

int TestVertexRendering(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  // the one and only true triangle
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->InsertPoint(0, -1, -1, 0);
  points->InsertPoint(1, 0, 1.5, 0);
  points->InsertPoint(2, 1, -1, 0);
  points->InsertPoint(3, -2, -2, 0);
  points->InsertPoint(4, 0, 2.5, 0);
  points->InsertPoint(5, 2, -2, 0);
  points->InsertPoint(6, -3, -3, 0);
  points->InsertPoint(7, 0, 3.5, 0);
  points->InsertPoint(8, 3, -3, 0);
  points->InsertPoint(9, -4, -4, 0);
  points->InsertPoint(10, 0, 4.5, 0);
  points->InsertPoint(11, 4, -4, 0);
  polydata->SetPoints(points);
  vtkNew<vtkCellArray> verts;
  verts->InsertNextCell({ 0 });
  verts->InsertNextCell({ 1 });
  verts->InsertNextCell({ 2 });
  verts->InsertNextCell({ 3, 4, 5 });
  verts->InsertNextCell({ 6, 7, 8, 11, 10, 9 });
  polydata->SetVerts(verts);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->DebugOn();
  mapper->SetInputData(polydata);

  vtkNew<vtkActor> actor;
  actor->GetProperty()->SetPointSize(6);
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
