// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// This example tests the vtkCamera3DWidget

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkCamera.h>
#include <vtkCamera3DRepresentation.h>
#include <vtkCamera3DWidget.h>
#include <vtkCameraHandleSource.h>
#include <vtkCubeSource.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>

int TestCamera3DWidget(int, char*[])
{
  vtkNew<vtkSphereSource> sphereSource;
  vtkNew<vtkCubeSource> cubeSource;
  cubeSource->SetCenter(0.0, 0.0, 2.0);
  vtkNew<vtkAppendPolyData> source;
  source->AddInputConnection(sphereSource->GetOutputPort());
  source->AddInputConnection(cubeSource->GetOutputPort());
  source->Update();

  // Create mapper and actor
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Renderers and one render window
  double mainViewport[4] = { 0.0, 0.0, 0.5, 1.0 };
  double cameraViewport[4] = { 0.5, 0.0, 1.0, 1.0 };
  vtkNew<vtkRenderer> mainRenderer;

  mainRenderer->SetViewport(mainViewport);
  mainRenderer->AddActor(actor);
  mainRenderer->SetBackground(0.7, 0.7, 1.0);

  vtkNew<vtkRenderer> cameraRenderer;
  cameraRenderer->SetViewport(cameraViewport);
  cameraRenderer->InteractiveOff();
  cameraRenderer->AddActor(actor);
  cameraRenderer->SetBackground(0.8, 0.8, 1.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 300);
  renderWindow->AddRenderer(mainRenderer);
  renderWindow->AddRenderer(cameraRenderer);
  renderWindow->SetWindowName("cameraWidget");

  // An interactor
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Camera widget and its representation
  vtkNew<vtkCamera3DRepresentation> cameraRepresentation;
  vtkNew<vtkCamera3DWidget> cameraWidget;
  cameraWidget->SetInteractor(renderWindowInteractor);
  cameraWidget->SetRepresentation(cameraRepresentation);

  // If you want to set the camera, do it before placing the widget
  cameraRepresentation->SetCamera(cameraRenderer->GetActiveCamera());
  // Placing widget is optional, if you do, camera will be moved toward bounds
  cameraRepresentation->PlaceWidget(actor->GetBounds());

  // Render
  renderWindowInteractor->Initialize();
  renderWindow->Render();
  cameraWidget->On();

  // Begin mouse interaction
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
