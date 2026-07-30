// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// This example tests the vtkCompassWidget

#include "vtkAnnotatedCubeActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCompassRepresentation.h"
#include "vtkCompassWidget.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"

constexpr char eventLog[] = "# StreamVersion 1.2\n"
                            "MouseMoveEvent 590 467 0 0 0 0 0\n"
                            "LeftButtonPressEvent 590 467 0 0 0 0 0\n"
                            "MouseMoveEvent 566 398 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 566 398 0 0 0 0 0\n"

                            "MouseMoveEvent 533 441 0 0 0 0 0\n"
                            "LeftButtonPressEvent 533 441 0 0 0 0 0\n"
                            "MouseMoveEvent 534 418 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 534 418 0 0 0 0 0\n"

                            "MouseMoveEvent 514 431 0 0 0 0 0\n"
                            "LeftButtonPressEvent 514 431 0 0 0 0 0\n"
                            "MouseMoveEvent 514 416 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 514 416 0 0 0 0 0\n";

class vtkICWValueChangedCallback : public vtkCommand
{
public:
  static vtkICWValueChangedCallback* New() { return new vtkICWValueChangedCallback(); }
  void Execute(
    vtkObject* caller, unsigned long vtkNotUsed(eventId), void* vtkNotUsed(callData)) override
  {
    vtkCompassWidget* widget = vtkCompassWidget::SafeDownCast(caller);
    vtkCamera* camera = widget->GetCurrentRenderer()->GetActiveCamera();

    // calculate new camera position from compass widget parameters
    double distance = widget->GetDistance();
    double tilt = widget->GetTilt();
    double heading = widget->GetHeading();

    double pos[3];
    pos[0] =
      distance * cos(vtkMath::RadiansFromDegrees(heading)) * cos(vtkMath::RadiansFromDegrees(tilt));
    pos[1] =
      distance * sin(vtkMath::RadiansFromDegrees(heading)) * cos(vtkMath::RadiansFromDegrees(tilt));
    pos[2] = distance * sin(vtkMath::RadiansFromDegrees(tilt));

    camera->SetPosition(pos);
    camera->SetFocalPoint(0, 0, 0);
    camera->SetViewUp(0, 0, 1);
    camera->SetClippingRange(0.1, distance + 1);

    widget->GetCurrentRenderer()->Render();
  }
  vtkICWValueChangedCallback() = default;
};

int TestCompassWidget(int argc, char* argv[])
{
  // a cube with text on its faces
  vtkNew<vtkAnnotatedCubeActor> actor;
  actor->GetCubeProperty()->SetColor(0, 0, 1);

  // a renderer and render window
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetMultiSamples(0);

  // an interactor
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // create the widget and its representation
  vtkNew<vtkCompassRepresentation> compassRepresentation;
  compassRepresentation->SetMinimumDistance(2);
  compassRepresentation->SetMaximumDistance(10);

  vtkNew<vtkCompassWidget> compassWidget;
  compassWidget->SetInteractor(renderWindowInteractor);
  compassWidget->SetRepresentation(compassRepresentation);
  compassWidget->SetDistance(5.0);
  compassWidget->SetTiltSpeed(45);
  compassWidget->SetDistanceSpeed(2);

  // create the callback
  vtkNew<vtkICWValueChangedCallback> valueChangedCallback;
  compassWidget->AddObserver(vtkCommand::WidgetValueChangedEvent, valueChangedCallback);

  // add the actors to the scene
  renderer->AddActor(actor);

  renderWindow->SetSize(640, 480);
  renderWindow->SetWindowName("CompassWidget");

  renderWindow->Render();
  compassWidget->EnabledOn();

  // no interactor style - camera is moved by widget callback
  renderWindowInteractor->SetInteractorStyle(nullptr);

  // set camera to initial position
  compassWidget->InvokeEvent(vtkCommand::WidgetValueChangedEvent);

  return vtkTesting::InteractorEventLoop(argc, argv, renderWindowInteractor, eventLog);
}
