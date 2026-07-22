// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkAnnotationLink.h"
#include "vtkCommand.h"
#include "vtkCubeSource.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkNew.h"
#include "vtkRenderView.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderedSurfaceRepresentation.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"

#include <vector>

class TestRenderViewUpdater : public vtkCommand
{
public:
  static TestRenderViewUpdater* New() { return new TestRenderViewUpdater; }

  void AddView(vtkView* view)
  {
    this->Views.push_back(view);
    view->AddObserver(vtkCommand::SelectionChangedEvent, this);
  }

  void Execute(vtkObject*, unsigned long, void*) override
  {
    for (unsigned int i = 0; i < this->Views.size(); i++)
    {
      this->Views[i]->Update();
    }
  }

private:
  TestRenderViewUpdater() = default;
  ~TestRenderViewUpdater() override = default;
  std::vector<vtkView*> Views;
};

static char RenderViewEventLog[] = "# StreamVersion 1\n"
                                   "RenderEvent 0 0 0 0 0 0 0\n"
                                   "EnterEvent 299 49 0 0 0 0 0\n"
                                   "LeftButtonPressEvent 215 192 0 0 0 0 0\n"
                                   "StartInteractionEvent 215 192 0 0 0 0 0\n"
                                   "MouseMoveEvent 213 192 0 0 0 0 0\n"
                                   "MouseMoveEvent 139 141 0 0 0 0 0\n"
                                   "LeftButtonReleaseEvent 139 141 0 0 0 0 0\n"
                                   "EndInteractionEvent 139 141 0 0 0 0 0\n"

                                   "LeftButtonPressEvent 65 182 0 0 0 0 0\n"
                                   "StartInteractionEvent 65 182 0 0 0 0 0\n"
                                   "MouseMoveEvent 65 181 0 0 0 0 0\n"
                                   "MouseMoveEvent 91 153 0 0 0 0 0\n"
                                   "LeftButtonReleaseEvent 91 153 0 0 0 0 0\n"
                                   "EndInteractionEvent 91 153 0 0 0 0 0\n"

                                   "LeftButtonPressEvent 39 190 0 0 0 0 0\n"
                                   "StartInteractionEvent 39 190 0 0 0 0 0\n"
                                   "MouseMoveEvent 40 190 0 0 0 0 0\n"
                                   "MouseMoveEvent 245 79 0 0 0 0 0\n"
                                   "LeftButtonReleaseEvent 245 79 0 0 0 0 0\n"
                                   "EndInteractionEvent 245 79 0 0 0 0 0\n"

                                   "MiddleButtonPressEvent 176 186 0 0 0 0 0\n"
                                   "StartInteractionEvent 176 186 0 0 0 0 0\n"
                                   "MouseMoveEvent 175 186 0 0 0 0 0\n"
                                   "MouseMoveEvent 161 192 0 0 0 0 0\n"
                                   "RenderEvent 161 192 0 0 0 0 0\n"
                                   "MiddleButtonReleaseEvent 161 192 0 0 0 0 0\n"
                                   "EndInteractionEvent 161 192 0 0 0 0 0\n"

                                   "RightButtonPressEvent 172 199 0 0 0 0 0\n"
                                   "StartInteractionEvent 172 199 0 0 0 0 0\n"
                                   "MouseMoveEvent 172 200 0 0 0 0 0\n"
                                   "MouseMoveEvent 126 155 0 0 0 0 0\n"
                                   "RightButtonReleaseEvent 126 155 0 0 0 0 0\n"
                                   "EndInteractionEvent 126 155 0 0 0 0 0\n"

                                   "StartInteractionEvent 141 132 0 0 0 0 0\n"
                                   "MouseWheelForwardEvent 141 132 0 0 0 0 0\n"
                                   "MouseWheelForwardEvent 141 132 0 0 0 0 0\n"
                                   "MouseWheelBackwardEvent 141 132 0 0 0 0 0\n"
                                   "MouseWheelBackwardEvent 141 132 0 0 0 0 0\n"
                                   "MouseWheelBackwardEvent 141 132 0 0 0 0 0\n"
                                   "MouseWheelBackwardEvent 141 132 0 0 0 0 0\n"
                                   "MouseWheelForwardEvent 142 133 0 0 0 0 0\n"
                                   "MouseWheelForwardEvent 142 133 0 0 0 0 0\n"
                                   "EndInteractionEvent 142 133 0 0 0 0 0\n"

                                   "LeaveEvent 251 299 0 0 0 0 0\n"
                                   "ExitEvent 251 299 0 0 0 0 0\n";
// #define RECORD

int TestRenderView(int argc, char* argv[])
{
  vtkNew<vtkAnnotationLink> link;
  vtkNew<TestRenderViewUpdater> updater;

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkCubeSource> cube;
  cube->SetCenter(2, 0, 0);

  vtkNew<vtkTransformFilter> transform;
  vtkNew<vtkTransform> trans;
  trans->Translate(0, 2, 0);
  transform->SetTransform(trans);
  transform->SetInputConnection(sphere->GetOutputPort());

  // Render view 1
  vtkNew<vtkRenderView> view;
  view->DisplayHoverTextOff();
  view->GetRenderWindow()->SetMultiSamples(0);
  updater->AddView(view);

  // Sphere 1
  vtkNew<vtkRenderedSurfaceRepresentation> sphereRep1;
  sphereRep1->SetInputConnection(sphere->GetOutputPort());
  sphereRep1->SetAnnotationLink(link);
  view->AddRepresentation(sphereRep1);
  view->Update();

  // Cube 1
  vtkNew<vtkRenderedSurfaceRepresentation> cubeRep1;
  cubeRep1->SetInputConnection(cube->GetOutputPort());
  view->AddRepresentation(cubeRep1);
  view->Update();

  view->ResetCamera();
  view->GetRenderer()->GradientBackgroundOff();

  // record events
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(view->GetInteractor());

  return vtkTesting::InteractorEventLoop(argc, argv, view->GetInteractor(), RenderViewEventLog);
}
