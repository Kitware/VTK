// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestLineRendering(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->InsertPoint(0, -1, -1, 0.0);
  points->InsertPoint(1, 0.0, 1.5, 0.0);
  points->InsertPoint(2, 1, -1, 0.0);
  points->InsertPoint(3, -2, -2, 0.0);
  points->InsertPoint(4, 0.0, 2.5, 0.0);
  points->InsertPoint(5, 2, -2, 0.0);
  points->InsertPoint(6, -3, -3, 0.0);
  points->InsertPoint(7, 0.0, 3.5, 0.0);
  points->InsertPoint(8, 3, -3, 0.0);
  points->InsertPoint(9, -4, -4, 0.0);
  points->InsertPoint(10, 0.0, 4.5, 0.0);
  points->InsertPoint(11, 4, -4, 0.0);
  polydata->SetPoints(points);
  vtkNew<vtkCellArray> lines;
  lines->InsertNextCell({ 0, 1 });
  lines->InsertNextCell({ 1, 2 });
  lines->InsertNextCell({ 3, 4, 5 });
  lines->InsertNextCell({ 6, 7, 8 });
  lines->InsertNextCell({ 11, 10, 9 });
  polydata->SetLines(lines);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata);

  vtkNew<vtkActor> actor;
  actor->GetProperty()->SetLineWidth(4);
  bool translucent = false;
  for (int i = 0; i < argc; ++i)
  {
    if (strcmp(argv[i], "--translucent") == 0)
    {
      translucent = true;
      break;
    }
  }
  actor->GetProperty()->SetOpacity(translucent ? 0.4 : 1.0);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderer->ResetCamera();
  renderer->SetBackground(0.2, 0.3, 0.4);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  renWin->Render();

  const int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
