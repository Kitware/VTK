// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyLineSource.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include "vtkRenderWindowInteractor.h"

//------------------------------------------------------------------------------
int TestRenderLinesAsTubes(int argc, char* argv[])
{
  bool useParallelProjection = false;
  for (int i = 0; i < argc; ++i)
  {
    if (argv[i] && !strcmp(argv[i], "--ortho"))
    {
      useParallelProjection = true;
    }
  }
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkPolyLineSource> linesLeft;
  linesLeft->SetNumberOfPoints(4);
  linesLeft->SetPoint(0, 0, -2, 0);
  linesLeft->SetPoint(1, 1, -1, 0);
  linesLeft->SetPoint(2, 0, 1, 0);
  linesLeft->SetPoint(3, 1, 2, 0);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(6, 0, 0);
  sphere->SetEndTheta(270.0);
  sphere->SetRadius(3);

  vtkNew<vtkPolyLineSource> linesRight;
  linesRight->SetNumberOfPoints(4);
  linesRight->SetPoint(0, 12, -2, 0);
  linesRight->SetPoint(1, 11, -1, 0);
  linesRight->SetPoint(2, 12, 1, 0);
  linesRight->SetPoint(3, 11, 2, 0);

  {
    vtkNew<vtkAppendPolyData> append;
    append->AddInputConnection(linesLeft->GetOutputPort());
    append->AddInputConnection(sphere->GetOutputPort());
    append->AddInputConnection(linesRight->GetOutputPort());

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(append->GetOutputPort());
    vtkNew<vtkActor> actor;
    renderer->AddActor(actor);
    actor->SetMapper(mapper);
    actor->GetProperty()->SetDiffuseColor(0.4, 1.0, 1.0);
    vtkNew<vtkProperty> backProp;
    backProp->SetDiffuseColor(0.4, 0.65, 0.8);
    actor->SetBackfaceProperty(backProp);

    actor->GetProperty()->EdgeVisibilityOn();
    actor->GetProperty()->SetEdgeColor(1.0, 1.0, 1.0);
    actor->GetProperty()->SetLineWidth(7.0);
    actor->GetProperty()->RenderLinesAsTubesOn();
  }

  renderWindow->SetMultiSamples(0);
  renderer->GetActiveCamera()->SetParallelProjection(useParallelProjection);
  renderer->GetActiveCamera()->Elevation(-45);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->GetActiveCamera()->Zoom(1.5);
  renderer->ResetCamera();
  renderer->ResetCameraClippingRange();
  renderWindow->Render();

  int retVal = vtkRegressionTestImageThreshold(renderWindow, 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
