// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Exercises the widgets a view holds: that adding one wires it to the view and
// places it over the scene, that the set can be read and emptied, and that a
// widget the component knows nothing about is wired up the same way.

#include "ScivisTestUtilities.h"
#include "vtkBoxWidget2.h"
#include "vtkDistanceWidget.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkNew.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScivisView.h"
#include "vtkScivisWidgets.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceRepresentation.h"
#include "vtkWidgetRepresentation.h"

#include <cmath>

namespace
{

// A view showing one sphere, somewhere other than the origin so that placing a
// widget over the scene is visibly different from leaving it where it started.
vtkSmartPointer<vtkScivisView> MakeView(double center)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(7.0);
  sphere->SetCenter(center, 0.0, 0.0);

  vtkNew<vtkSurfaceRepresentation> surface;
  surface->SetInputConnection(sphere->GetOutputPort());

  auto view = vtkSmartPointer<vtkScivisView>::New();
  view->AddRepresentation(surface);
  view->ResetCamera();
  view->Render();
  return view;
}

int TestAddingWiresAWidgetToTheView()
{
  auto view = MakeView(0.0);
  vtkScivisWidgets* widgets = view->GetWidgets();

  CHECK(widgets->GetNumberOfWidgets() == 0, "a new view is holding widgets");

  vtkDistanceWidget* ruler = widgets->AddDistance();
  CHECK(ruler != nullptr, "no widget came back");
  CHECK(widgets->GetNumberOfWidgets() == 1, "the widget was not kept");
  CHECK(widgets->GetWidget(0) == ruler, "the widget held is not the one handed back");
  CHECK(ruler->GetInteractor() == view->GetInteractor(),
    "the widget was not given the view's interactor");
  CHECK(ruler->GetDefaultRenderer() == view->GetRenderer(),
    "the widget was not given the view's renderer");
  CHECK(ruler->GetEnabled() != 0, "the widget was not enabled");

  return EXIT_SUCCESS;
}

// The view knows how big the scene is, which is what a widget that occupies a
// region needs in order to arrive around the data.
int TestWidgetsArePlacedOverTheScene()
{
  auto here = MakeView(0.0);
  auto faraway = MakeView(40.0);

  vtkImplicitPlaneWidget2* near = here->GetWidgets()->AddPlane();
  vtkImplicitPlaneWidget2* far = faraway->GetWidgets()->AddPlane();

  double nearOrigin[3];
  double farOrigin[3];
  near->GetImplicitPlaneRepresentation()->GetOrigin(nearOrigin);
  far->GetImplicitPlaneRepresentation()->GetOrigin(farOrigin);

  CHECK(
    farOrigin[0] > nearOrigin[0] + 10.0, "the plane did not follow the data it was placed over");

  // And it lands within the scene rather than beside it.
  double scene[6];
  CHECK(faraway->GetSceneBounds(scene), "the view reports no scene bounds");
  CHECK(
    farOrigin[0] >= scene[0] && farOrigin[0] <= scene[1], "the plane was placed outside the scene");

  return EXIT_SUCCESS;
}

// A widget placed over the scene spans all of it, not the middle half that a
// representation's default place factor of 0.5 would give.
int TestWidgetsSpanTheScene()
{
  auto view = MakeView(40.0);
  vtkBoxWidget2* box = view->GetWidgets()->AddBox();

  double scene[6];
  CHECK(view->GetSceneBounds(scene), "the view reports no scene bounds");
  const double* placed = box->GetRepresentation()->GetBounds();
  for (int i = 0; i < 3; ++i)
  {
    const double extent = scene[2 * i + 1] - scene[2 * i];
    CHECK(std::abs(placed[2 * i] - scene[2 * i]) < 0.01 * extent &&
        std::abs(placed[2 * i + 1] - scene[2 * i + 1]) < 0.01 * extent,
      "the box does not span the scene");
  }

  return EXIT_SUCCESS;
}

// Add() is the general case: a widget this component has no method for is wired
// up the same way.
int TestAnyWidgetCanBeAdded()
{
  auto view = MakeView(0.0);
  vtkScivisWidgets* widgets = view->GetWidgets();

  vtkNew<vtkBoxWidget2> box;
  widgets->Add(box);
  CHECK(widgets->GetNumberOfWidgets() == 1, "the widget was not kept");
  CHECK(box->GetInteractor() == view->GetInteractor(), "the widget was not wired to the view");
  CHECK(box->GetEnabled() != 0, "the widget was not enabled");

  // Adding it twice is not two widgets.
  widgets->Add(box);
  CHECK(widgets->GetNumberOfWidgets() == 1, "the same widget was added twice");

  return EXIT_SUCCESS;
}

int TestRemoving()
{
  auto view = MakeView(0.0);
  vtkScivisWidgets* widgets = view->GetWidgets();

  vtkDistanceWidget* ruler = widgets->AddDistance();
  vtkImplicitPlaneWidget2* plane = widgets->AddPlane();
  CHECK(widgets->GetNumberOfWidgets() == 2, "both widgets were not kept");

  widgets->Remove(ruler);
  CHECK(widgets->GetNumberOfWidgets() == 1, "the widget was not removed");
  CHECK(widgets->GetWidget(0) == plane, "the wrong widget was removed");
  CHECK(ruler->GetEnabled() == 0, "the removed widget is still enabled");

  // Removing something that was never added is not an error.
  widgets->Remove(ruler);
  CHECK(widgets->GetNumberOfWidgets() == 1, "removing an absent widget changed the set");

  widgets->RemoveAll();
  CHECK(widgets->GetNumberOfWidgets() == 0, "the set was not emptied");
  CHECK(plane->GetEnabled() == 0, "a widget was left enabled after the set was emptied");
  CHECK(widgets->GetWidget(0) == nullptr, "an index past the end returned a widget");

  return EXIT_SUCCESS;
}

// The widgets are reached through the view, but can outlive it.
int TestOutlivingTheView()
{
  vtkSmartPointer<vtkScivisWidgets> widgets;
  {
    auto view = MakeView(0.0);
    widgets = view->GetWidgets();
    widgets->AddDistance();
  }
  widgets->SetGlobalWarningDisplay(0);

  vtkNew<vtkBoxWidget2> box;
  widgets->Add(box);
  CHECK(widgets->GetNumberOfWidgets() == 1, "a widget was added with no view to add it to");

  return EXIT_SUCCESS;
}

}

int TestScivisWidgets(int, char*[])
{
  if (TestAddingWiresAWidgetToTheView() != EXIT_SUCCESS ||
    TestWidgetsArePlacedOverTheScene() != EXIT_SUCCESS ||
    TestWidgetsSpanTheScene() != EXIT_SUCCESS || TestAnyWidgetCanBeAdded() != EXIT_SUCCESS ||
    TestRemoving() != EXIT_SUCCESS || TestOutlivingTheView() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
