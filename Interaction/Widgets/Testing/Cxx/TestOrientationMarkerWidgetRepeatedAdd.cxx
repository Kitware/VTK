// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAxesActor.h"
#include "vtkNew.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestOrientationMarkerWidgetRepeatedAdd(int, char*[])
{
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  vtkNew<vtkAxesActor> axes;
  {
    vtkNew<vtkOrientationMarkerWidget> widget;
    widget->SetOrientationMarker(axes);
    widget->SetInteractor(interactor);
    widget->SetViewport(0, 0, 0.2, 0.2);
    widget->EnabledOn();
    widget->InteractiveOn();

    widget->EnabledOff();
  }
  std::cout << "After first widget removed\n";
  vtkNew<vtkOrientationMarkerWidget> widget;
  widget->SetOrientationMarker(axes);
  widget->SetInteractor(interactor);
  widget->SetViewport(0, 0, 0.2, 0.2);
  widget->EnabledOn();
  widget->InteractiveOn();

  interactor->Initialize();
  renderWindow->Render();
  interactor->Start();
  std::cout << "After second widget removed\n";
  return EXIT_SUCCESS;
}
