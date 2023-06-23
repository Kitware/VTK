/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOrientationWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkOrientationWidget

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkCubeSource.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOrientationRepresentation.h>
#include <vtkOrientationWidget.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

// Callback for the interaction
class vtkOrientationCallback : public vtkCommand
{
public:
  static vtkOrientationCallback* New() { return new vtkOrientationCallback; }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkOrientationWidget* widget = reinterpret_cast<vtkOrientationWidget*>(caller);
    vtkOrientationRepresentation* repr =
      reinterpret_cast<vtkOrientationRepresentation*>(widget->GetRepresentation());
    this->Actor->SetOrientation(repr->GetOrientation());
  }

  vtkActor* Actor = nullptr;
};

int TestOrientationWidget(int, char*[])
{
  // Create source, mapper and actor
  vtkNew<vtkCubeSource> cubeSource;
  vtkNew<vtkPolyDataMapper> cubeMapper;
  cubeMapper->SetInputConnection(cubeSource->GetOutputPort());
  vtkNew<vtkActor> cubeActor;
  cubeActor->SetMapper(cubeMapper);

  // Setup renderer and render window
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(cubeActor);
  renderer->SetBackground(0.7, 0.7, 1.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);
  renderWindow->SetWindowName("orientationWidget");

  // An interactor
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Callback to sync object and widget
  vtkSmartPointer<vtkOrientationCallback> myCallback =
    vtkSmartPointer<vtkOrientationCallback>::New();
  myCallback->Actor = cubeActor;

  // Camera widget and its representation
  vtkNew<vtkOrientationRepresentation> orientationRepresentation;
  vtkNew<vtkOrientationWidget> orientationWidget;
  orientationWidget->SetInteractor(renderWindowInteractor);
  orientationWidget->SetRepresentation(orientationRepresentation);
  orientationRepresentation->PlaceWidget(cubeActor->GetBounds());
  orientationWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);

  // Render
  renderWindowInteractor->Initialize();
  renderWindow->Render();
  orientationWidget->On();

  // Begin mouse interaction
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
