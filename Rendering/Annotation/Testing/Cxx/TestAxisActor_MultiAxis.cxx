// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "TestAxisActorInternal.h"
#include "vtkAxisActor.h"
#include "vtkRenderWindowInteractor.h"

//------------------------------------------------------------------------------
int TestAxisActor_MultiAxis(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkAxisActor> xAxis;
  ::InitializeXAxis(xAxis);
  vtkNew<vtkAxisActor> yAxis;
  ::InitializeYAxis(yAxis);
  vtkNew<vtkAxisActor> zAxis;
  ::InitializeZAxis(zAxis);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(xAxis);
  renderer->AddActor(yAxis);
  renderer->AddActor(zAxis);
  renderer->SetBackground(.5, .5, .5);
  vtkCamera* camera = renderer->GetActiveCamera();
  xAxis->SetCamera(camera);
  yAxis->SetCamera(camera);
  zAxis->SetCamera(camera);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  renderWindow->SetSize(500, 500);
  renderWindow->SetMultiSamples(0);
  renderWindow->Render();
  camera->Azimuth(45);
  camera->Elevation(45);
  renderer->ResetCameraScreenSpace();
  renderWindow->Render();

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);
  interactor->Start();

  return EXIT_SUCCESS;
}
