// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// This example tests the vtkAngleWidget.

#include "vtkActor.h"
#include "vtkAngleRepresentation3D.h"
#include "vtkAngleWidget.h"
#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"

#include <iostream>

namespace
{

constexpr char TestAngleWidget3DEventLog[] = "# StreamVersion 1\n"
                                             "MouseMoveEvent 136 208 0 0 0 0 0\n"
                                             "LeftButtonPressEvent 136 208 0 0 0 0 0\n"
                                             "LeftButtonReleaseEvent 136 208 0 0 0 0 0\n"

                                             "MouseMoveEvent 20 33 0 0 0 0 0\n"
                                             "LeftButtonPressEvent 20 33 0 0 0 0 0\n"
                                             "LeftButtonReleaseEvent 20 33 0 0 0 0 0\n"

                                             "MouseMoveEvent 161 29 0 0 0 0 0\n"
                                             "LeftButtonPressEvent 161 29 0 0 0 0 0\n"
                                             "LeftButtonReleaseEvent 161 29 0 0 0 0 0\n"

                                             "KeyPressEvent 184 66 0 0 116 1 t\n"
                                             "CharEvent 184 66 0 0 116 1 t\n"
                                             "KeyReleaseEvent 184 67 0 0 116 1 t\n"

                                             "MouseMoveEvent 177 129 0 0 0 0 t\n"
                                             "LeftButtonPressEvent 177 129 0 0 0 0 t\n"
                                             "StartInteractionEvent 177 129 0 0 0 0 t\n"
                                             "MouseMoveEvent 107 174 0 0 0 0 t\n"
                                             "LeftButtonReleaseEvent 107 174 0 0 0 0 t\n"
                                             "EndInteractionEvent 107 174 0 0 0 0 t\n";

// This callback is responsible for setting the angle label.
class vtkAngleCallback : public vtkCommand
{
public:
  static vtkAngleCallback* New() { return new vtkAngleCallback; }
  void Execute(vtkObject*, unsigned long eid, void*) override
  {
    if (eid == vtkCommand::PlacePointEvent)
    {
      std::cout << "point placed\n";
    }
    else if (eid == vtkCommand::InteractionEvent)
    {
      double point1[3], center[3], point2[3];
      this->Rep->GetPoint1WorldPosition(point1);
      this->Rep->GetCenterWorldPosition(center);
      this->Rep->GetPoint2WorldPosition(point2);
      std::cout << "Angle between "
                << "(" << point1[0] << "," << point1[1] << "," << point1[2] << "), (" << center[0]
                << "," << center[1] << "," << center[2] << ") and (" << point2[0] << ","
                << point2[1] << "," << point2[2] << ") is " << this->Rep->GetAngle() << " radians."
                << std::endl;
    }
  }
  vtkAngleRepresentation3D* Rep;
  vtkAngleCallback()
    : Rep(nullptr)
  {
  }
};

}

// The actual test function
int TestAngleWidget3D(int argc, char* argv[])
{
  vtkSmartPointer<vtkSphereSource> ss = vtkSmartPointer<vtkSphereSource>::New();

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Create a test pipeline
  //
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(ss->GetOutputPort());
  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  // Create the widget and its representation
  vtkSmartPointer<vtkPointHandleRepresentation3D> handle =
    vtkSmartPointer<vtkPointHandleRepresentation3D>::New();
  handle->GetProperty()->SetColor(1, 0, 0);
  vtkSmartPointer<vtkAngleRepresentation3D> rep = vtkSmartPointer<vtkAngleRepresentation3D>::New();
  rep->SetHandleRepresentation(handle);
  rep->SetScale(vtkMath::Pi() / 180.0);
  rep->SetLabelFormat("{:<#6.3g} rad");

  vtkSmartPointer<vtkAngleWidget> widget = vtkSmartPointer<vtkAngleWidget>::New();
  widget->SetInteractor(iren);
  widget->CreateDefaultRepresentation();
  widget->SetRepresentation(rep);

  vtkSmartPointer<vtkAngleCallback> mcbk = vtkSmartPointer<vtkAngleCallback>::New();
  mcbk->Rep = rep;
  widget->AddObserver(vtkCommand::PlacePointEvent, mcbk);
  widget->AddObserver(vtkCommand::InteractionEvent, mcbk);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(actor);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  widget->On();
  renWin->Render();

  return vtkTesting::InteractorEventLoop(argc, argv, iren, TestAngleWidget3DEventLog);
}
