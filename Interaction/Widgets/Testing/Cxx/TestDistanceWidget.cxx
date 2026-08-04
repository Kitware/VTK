// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// This example tests the vtkDistanceWidget

#include "vtkActor.h"
#include "vtkAxisActor2D.h"
#include "vtkCommand.h"
#include "vtkDistanceRepresentation2D.h"
#include "vtkDistanceWidget.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStringFormatter.h"
#include "vtkTesting.h"
#include "vtkTextProperty.h"

constexpr char TestDistanceWidgetEventLog[] = "# StreamVersion 1\n"
                                              "RenderEvent 0 0 0 0 0 0 0\n"

                                              "LeftButtonPressEvent 219 189 0 0 0 0 0\n"
                                              "LeftButtonReleaseEvent 219 189 0 0 0 0 0\n"

                                              "MouseMoveEvent 89 82 0 0 0 0 0\n"
                                              "LeftButtonPressEvent 89 82 0 0 0 0 0\n"
                                              "LeftButtonReleaseEvent 89 82 0 0 0 0 0\n"

                                              "MouseMoveEvent 216 189 0 0 0 0 0\n"
                                              "LeftButtonPressEvent 216 189 0 0 0 0 0\n"
                                              "MouseMoveEvent 62 217 0 0 0 0 0\n"
                                              "LeftButtonReleaseEvent 62 217 0 0 0 0 0\n"

                                              "MouseMoveEvent 92 81 0 0 0 0 0\n"
                                              "LeftButtonPressEvent 92 81 0 0 0 0 0\n"
                                              "MouseMoveEvent 256 153 0 0 0 0 0\n"
                                              "LeftButtonReleaseEvent 256 153 0 0 0 0 0\n";

// This callback is responsible for adjusting the point position.
// It looks in the region around the point and finds the maximum or
// minimum value.
class vtkDistanceCallback : public vtkCommand
{
public:
  static vtkDistanceCallback* New() { return new vtkDistanceCallback; }
  void Execute(vtkObject* caller, unsigned long, void*) override;
  vtkDistanceCallback()
    : Renderer(nullptr)
    , RenderWindow(nullptr)
    , DistanceWidget(nullptr)
    , Distance(nullptr)
  {
  }
  vtkRenderer* Renderer;
  vtkRenderWindow* RenderWindow;
  vtkDistanceWidget* DistanceWidget;
  vtkDistanceRepresentation2D* Distance;
};

// Method re-positions the points using random perturbation
void vtkDistanceCallback::Execute(vtkObject*, unsigned long eid, void* callData)
{
  if (eid == vtkCommand::InteractionEvent || eid == vtkCommand::EndInteractionEvent)
  {
    double pos1[3], pos2[3];
    // Modify the measure axis
    this->Distance->GetPoint1WorldPosition(pos1);
    this->Distance->GetPoint2WorldPosition(pos2);
    double dist = sqrt(vtkMath::Distance2BetweenPoints(pos1, pos2));

    this->Distance->GetAxis()->SetRange(0.0, dist);
    auto title = vtk::format("{:<#6.3g}", dist);
    this->Distance->GetAxis()->SetTitle(title.c_str());
  }
  else
  {
    int pid = *(reinterpret_cast<int*>(callData));

    // From the point id, get the display coordinates
    double pos1[3], pos2[3], *pos;
    this->Distance->GetPoint1DisplayPosition(pos1);
    this->Distance->GetPoint2DisplayPosition(pos2);
    if (pid == 0)
    {
      pos = pos1;
    }
    else
    {
      pos = pos2;
    }

    // Okay, render without the widget, and get the color buffer
    int enabled = this->DistanceWidget->GetEnabled();
    if (enabled)
    {
      this->DistanceWidget->SetEnabled(0); // does a Render() as a side effect
    }

    // Pretend we are doing something serious....just randomly bump the
    // location of the point.
    double p[3];
    p[0] = pos[0] + static_cast<int>(vtkMath::Random(-5.5, 5.5));
    p[1] = pos[1] + static_cast<int>(vtkMath::Random(-5.5, 5.5));
    p[2] = 0.0;

    // Set the new position
    if (pid == 0)
    {
      this->Distance->SetPoint1DisplayPosition(p);
    }
    else
    {
      this->Distance->SetPoint2DisplayPosition(p);
    }

    // Side effect of a render here
    if (enabled)
    {
      this->DistanceWidget->SetEnabled(1);
    }
  }
}

// The actual test function
int TestDistanceWidget(int argc, char* argv[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  //
  vtkNew<vtkSphereSource> ss;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(ss->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Create the widget and its representation
  vtkNew<vtkPointHandleRepresentation2D> handle;
  handle->GetProperty()->SetColor(1, 0, 0);
  vtkNew<vtkDistanceRepresentation2D> rep;
  rep->SetHandleRepresentation(handle);
  vtkAxisActor2D* axis = rep->GetAxis();
  axis->UseFontSizeFromPropertyOn();
  vtkTextProperty* titleProp = axis->GetTitleTextProperty();
  titleProp->SetFontSize(40);
  if (!axis)
  {
    vtkLogF(ERROR, "Error getting representation's axis");
    return EXIT_FAILURE;
  }

  axis->SetNumberOfMinorTicks(4);
  axis->SetTickLength(9);
  axis->SetTitlePosition(0.2);
  rep->RulerModeOn();
  rep->SetRulerDistance(0.25);
  if (rep->GetRulerDistance() != 0.25)
  {
    vtkLogF(
      ERROR, "Error setting ruler distance to 0.25, get returned %lf", rep->GetRulerDistance());
    return EXIT_FAILURE;
  }

  vtkProperty2D* prop2D = rep->GetAxisProperty();
  if (!prop2D)
  {
    vtkLogF(ERROR, "Error getting widget axis property");
    return EXIT_FAILURE;
  }

  prop2D->SetColor(1.0, 0.0, 1.0);

  vtkNew<vtkDistanceWidget> widget;
  widget->SetInteractor(iren);
  widget->CreateDefaultRepresentation();
  widget->SetRepresentation(rep);

  vtkNew<vtkDistanceCallback> mcbk;
  mcbk->Renderer = ren1;
  mcbk->RenderWindow = renWin;
  mcbk->Distance = rep;
  mcbk->DistanceWidget = widget;

  rep->SetScale(0.5);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  iren->Initialize();
  widget->On();
  vtkTesting::InteractorEventLoop(argc, argv, iren, TestDistanceWidgetEventLog);

  double* p1w = rep->GetPoint1WorldPosition();
  if (p1w)
  {
    vtkLogF(INFO, "Point 1 World Position: %lf, %lf, %lf", p1w[0], p1w[1], p1w[2]);
  }
  else
  {
    vtkLogF(ERROR, "Error getting point 1 world position");
    return EXIT_FAILURE;
  }

  double* p2w = rep->GetPoint2WorldPosition();
  if (p2w)
  {
    vtkLogF(INFO, "Point 2 World Position: %lf, %lf, %lf", p2w[0], p2w[1], p2w[2]);
  }
  else
  {
    vtkLogF(ERROR, "Error getting point 2 world position");
    return EXIT_FAILURE;
  }

  double distance = rep->GetDistance();
  vtkLogF(INFO, "Distance: %lf", distance);
  double pointDistance = vtkMath::Distance2BetweenPoints(p1w, p2w);
  pointDistance = sqrt(pointDistance);
  if (fabs(pointDistance - distance) > 0.01)
  {
    vtkLogF(ERROR,
      "Error: distance between the world positions of the end points = %lf, while the "
      "representation's distance is %lf",
      pointDistance, distance);
    return EXIT_FAILURE;
  }
  // now set it and test again
  double p1wSet[3] = { 10.0, 10.0, 10.0 };
  double p2wSet[3] = { -10.0, -10.0, -10.0 };
  rep->SetPoint1WorldPosition(p1wSet);
  p1w = rep->GetPoint1WorldPosition();
  if (p1w)
  {
    vtkLogF(INFO, "Point 1 World Position: %lf, %lf, %lf", p1w[0], p1w[1], p1w[2]);
  }
  else
  {
    vtkLogF(ERROR, "Error getting point 1 world position");
    return EXIT_FAILURE;
  }
  rep->SetPoint2WorldPosition(p2wSet);
  p2w = rep->GetPoint2WorldPosition();
  if (p2w)
  {
    vtkLogF(INFO, "Point 1 World Position: %lf, %lf, %lf", p2w[0], p2w[1], p2w[2]);
  }
  else
  {
    vtkLogF(ERROR, "Error getting point 2 world position");
    return EXIT_FAILURE;
  }

  distance = rep->GetDistance();
  pointDistance = vtkMath::Distance2BetweenPoints(p1wSet, p2wSet);
  pointDistance = sqrt(pointDistance);
  if (fabs(pointDistance - distance) > 0.01)
  {
    vtkLogF(ERROR,
      "Error: distance between the world positions of the end points = %lf, while the "
      "representation's distance is %lf",
      pointDistance, distance);
    return EXIT_FAILURE;
  }
  vtkLogF(INFO, "New distance = %lf", distance);

  double p1d[3];
  rep->GetPoint1DisplayPosition(p1d);
  vtkLogF(INFO, "Point 1 Display Position: %lf, %lf, %lf", p1d[0], p1d[1], p1d[2]);

  double p2d[3];
  rep->GetPoint2DisplayPosition(p2d);
  vtkLogF(INFO, "Point 2 Display Position: %lf, %lf, %lf", p2d[0], p2d[1], p2d[2]);

  rep->SetScale(0.75);
  if (rep->GetScale() != 0.75)
  {
    vtkLogF(ERROR, "Error: GetScale() did not return value set by SetScale()");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
