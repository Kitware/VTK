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
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSphereSource.h"

int TestTheQuadWireframeRepresentation(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->InsertPoint(0, -1, -1, 1);
  points->InsertPoint(1, 1, -1, 1);
  points->InsertPoint(2, -1, 1, 1);
  points->InsertPoint(3, 1, 1, 1);
  polydata->SetPoints(points);
  vtkNew<vtkCellArray> quad;
  quad->InsertNextCell({ 0, 1, 3, 2 });
  polydata->SetPolys(quad);

  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(4);
  colors->SetNumberOfTuples(3);
  colors->InsertComponent(0, 0, 255);
  colors->InsertComponent(0, 1, 0);
  colors->InsertComponent(0, 2, 0);
  colors->InsertComponent(0, 3, 255);
  colors->InsertComponent(1, 0, 0);
  colors->InsertComponent(1, 1, 255);
  colors->InsertComponent(1, 2, 0);
  colors->InsertComponent(1, 3, 255);
  colors->InsertComponent(2, 0, 0);
  colors->InsertComponent(2, 1, 0);
  colors->InsertComponent(2, 2, 255);
  colors->InsertComponent(2, 3, 255);
  colors->InsertComponent(3, 0, 255);
  colors->InsertComponent(3, 1, 255);
  colors->InsertComponent(3, 2, 0);
  colors->InsertComponent(3, 3, 255);
  polydata->GetPointData()->SetScalars(colors);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->DebugOn();
  mapper->SetInputData(polydata);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetLineWidth(2);
  actor->GetProperty()->SetRepresentationToWireframe();
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
