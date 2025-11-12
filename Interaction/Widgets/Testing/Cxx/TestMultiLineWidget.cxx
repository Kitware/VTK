// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkActor.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkMultiLineRepresentation.h>
#include <vtkMultiLineWidget.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

static const char* eventLog = "# StreamVersion 1.2\n"
                              "ExposeEvent 0 299 0 0 0 0 0\n"
                              "MouseMoveEvent 246 163 0 0 0 0 0\n"
                              "MouseMoveEvent 101 179 0 0 0 0 0\n"
                              "LeftButtonPressEvent 101 179 0 0 0 0 0\n"
                              "MouseMoveEvent 101 177 0 0 0 0 0\n"
                              "MouseMoveEvent 98 34 0 0 0 0 0\n"
                              "LeftButtonReleaseEvent 98 34 0 0 0 0 0\n"
                              "MouseMoveEvent 98 36 0 0 0 0 0\n"
                              "MouseMoveEvent 73 129 0 0 0 0 0\n"
                              "LeftButtonPressEvent 73 129 0 0 0 0 0\n"
                              "MouseMoveEvent 72 129 0 0 0 0 0\n"
                              "MouseMoveEvent 38 160 0 0 0 0 0\n"
                              "LeftButtonReleaseEvent 38 160 0 0 0 0 0\n"
                              "MouseMoveEvent 39 160 0 0 0 0 0\n"
                              "MouseMoveEvent 249 247 0 0 0 0 0\n"
                              "LeftButtonPressEvent 249 247 0 0 0 0 0\n"
                              "MouseMoveEvent 250 245 0 0 0 0 0\n"
                              "MouseMoveEvent 258 190 0 0 0 0 0\n"
                              "LeftButtonReleaseEvent 258 190 0 0 0 0 0\n";

int TestMultiLineWidget(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkSphereSource> sphereSource;

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphereSource->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetWindowName("TestMultiLineWidget");

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  vtkNew<vtkMultiLineWidget> multiLineWidget;

  vtkNew<vtkMultiLineRepresentation> multiLineRepresentation;
  multiLineWidget->CreateDefaultRepresentation();

  multiLineRepresentation->GetEndPointProperty()->SetColor(1, 0, 1);
  multiLineRepresentation->GetEndPoint2Property()->SetColor(1, 0, 1);

  multiLineWidget->SetInteractor(renderWindowInteractor);
  multiLineWidget->SetRepresentation(multiLineRepresentation);

  multiLineRepresentation->GetLineProperty()->SetColor(1, 1, 0);

  multiLineRepresentation->SetDirectionalLine(true);

  // record events
  vtkNew<vtkInteractorEventRecorder> recorder;

  recorder->SetInteractor(renderWindowInteractor);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);

  renderWindowInteractor->Initialize();
  multiLineWidget->On();
  renderWindow->Render();
  recorder->Play();

  recorder->Off();

  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
