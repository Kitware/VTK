// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// This example tests the vtkDistanceWidget

// First include the required header files for the VTK classes we are using.
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDistanceRepresentation3D.h"
#include "vtkDistanceWidget.h"
#include "vtkFollower.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"

constexpr char TestDistanceWidget3DEventLog[] = "# StreamVersion 1\n"
                                                "EnterEvent 292 123 0 0 0 0 0\n"

                                                "MouseMoveEvent 219 189 0 0 0 0 0\n"
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
class vtkDistanceWidget3DCallback : public vtkCommand
{
public:
  static vtkDistanceWidget3DCallback* New() { return new vtkDistanceWidget3DCallback; }
  void Execute(vtkObject* caller, unsigned long, void*) override;
  vtkDistanceWidget3DCallback()
    : Renderer(nullptr)
    , RenderWindow(nullptr)
    , DistanceWidget(nullptr)
    , Distance(nullptr)
  {
  }
  vtkRenderer* Renderer;
  vtkRenderWindow* RenderWindow;
  vtkDistanceWidget* DistanceWidget;
  vtkDistanceRepresentation3D* Distance;
};

// Method re-positions the points using random perturbation
void vtkDistanceWidget3DCallback::Execute(vtkObject*, unsigned long eid, void* callData)
{
  if (eid == vtkCommand::InteractionEvent || eid == vtkCommand::EndInteractionEvent)
  {
    double pos1[3], pos2[3];
    // Modify the measure axis
    this->Distance->GetPoint1WorldPosition(pos1);
    this->Distance->GetPoint2WorldPosition(pos2);
    // double dist = sqrt(vtkMath::Distance2BetweenPoints(pos1, pos2));
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
int TestDistanceWidget3D(int argc, char* argv[])
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
  vtkNew<vtkPointHandleRepresentation3D> handle;
  handle->GetProperty()->SetColor(1, 0, 0);
  vtkNew<vtkDistanceRepresentation3D> rep;
  rep->SetHandleRepresentation(handle);
  rep->RulerModeOn();
  rep->SetRulerDistance(0.1);
  rep->SetNumberOfRulerTicks(4);
  double glyphScale = rep->GetGlyphScale();
  rep->SetGlyphScale(2.0);
  if (rep->GetGlyphScale() != 2.0)
  {
    vtkLogF(ERROR, "Error setting glyph scale to 2.0, returned %lf", rep->GetGlyphScale());
    return EXIT_FAILURE;
  }
  rep->SetGlyphScale(glyphScale);
  if (rep->GetGlyphScale() != glyphScale)
  {
    vtkLogF(
      ERROR, "Error setting glyph scale to %lf, returned %lf", glyphScale, rep->GetGlyphScale());
    return EXIT_FAILURE;
  }
  rep->SetGlyphScale(0.1);
  if (rep->GetGlyphScale() != 0.1)
  {
    vtkLogF(ERROR, "Error setting glyph scale to 0.1, returned %lf", rep->GetGlyphScale());
    return EXIT_FAILURE;
  }

  if (!rep->GetLineProperty())
  {
    vtkLogF(ERROR, "Error getting representation line property");
    return EXIT_FAILURE;
  }
  rep->GetLineProperty()->SetColor(1.0, 0.0, 1.0);
  rep->SetLabelPosition(0.45);
  if (rep->GetLabelPosition() != 0.45)
  {
    vtkLogF(ERROR, "Error setting label position to 0.45, returned : %lf", rep->GetLabelPosition());
    return EXIT_FAILURE;
  }
  for (int maxTicks = 1; maxTicks < 100; maxTicks += 10)
  {
    rep->SetMaximumNumberOfRulerTicks(maxTicks);
    if (rep->GetMaximumNumberOfRulerTicks() != maxTicks)
    {
      vtkLogF(ERROR, "Error setting maximum number of ruler ticks to %i, get returned %i", maxTicks,
        rep->GetMaximumNumberOfRulerTicks());
      return EXIT_FAILURE;
    }
  }
  vtkActor* glyphActor = rep->GetGlyphActor();
  if (!glyphActor)
  {
    vtkLogF(ERROR, "Error getting glyph actor");
    return EXIT_FAILURE;
  }
  glyphActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  vtkFollower* labelActor = rep->GetLabelActor();
  if (!labelActor)
  {
    vtkLogF(ERROR, "Error getting label actor");
    return EXIT_FAILURE;
  }
  labelActor->GetProperty()->SetColor(0.0, 1.0, 0.0);

  vtkNew<vtkDistanceWidget> widget;
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  vtkNew<vtkDistanceWidget3DCallback> mcbk;
  mcbk->Renderer = ren1;
  mcbk->RenderWindow = renWin;
  mcbk->Distance = rep;
  mcbk->DistanceWidget = widget;

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  iren->Initialize();
  widget->On();
  return vtkTesting::InteractorEventLoop(argc, argv, iren, TestDistanceWidget3DEventLog);
}
