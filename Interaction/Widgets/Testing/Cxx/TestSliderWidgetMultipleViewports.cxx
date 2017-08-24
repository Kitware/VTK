/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSliderWidgetMultipleViewports.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkSliderWidget with multiple viewports.

// First include the required header files for the VTK classes we are using.
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkLODActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSliderRepresentation2D.h"
#include "vtkSliderRepresentation3D.h"
#include "vtkSliderWidget.h"
#include "vtkSphere.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

const char TestSliderWidgetMultipleViewportsLog[] =
  "# StreamVersion 1\n"
  "EnterEvent 292 46 0 0 0 0 0\n"
  "MouseMoveEvent 273 65 0 0 0 0 0\n"
  "MouseMoveEvent 252 88 0 0 0 0 0\n"
  "MouseMoveEvent 148 299 0 0 0 0 0\n"
  "LeaveEvent 147 301 0 0 0 0 0\n"
  "EnterEvent 145 299 0 0 0 0 0\n"
  "MouseMoveEvent 145 299 0 0 0 0 0\n"
  "MouseMoveEvent 115 190 0 0 0 0 0\n"
  "LeftButtonPressEvent 115 190 0 0 0 0 0\n"
  "StartInteractionEvent 115 190 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 115 190 0 0 0 0 0\n"
  "EndInteractionEvent 115 190 0 0 0 0 0\n"
  "RenderEvent 115 190 0 0 0 0 0\n"
  "KeyPressEvent 115 190 0 0 114 1 r\n"
  "CharEvent 115 190 0 0 114 1 r\n"
  "RenderEvent 115 190 0 0 114 1 r\n"
  "KeyReleaseEvent 115 190 0 0 114 1 r\n"
  "MouseMoveEvent 194 163 0 0 0 0 r\n"
  "MouseMoveEvent 195 163 0 0 0 0 r\n"
  "LeftButtonPressEvent 195 163 0 0 0 0 r\n"
  "RenderEvent 195 163 0 0 0 0 r\n"
  "MouseMoveEvent 195 163 0 0 0 0 r\n"
  "MouseMoveEvent 201 151 0 0 0 0 r\n"
  "RenderEvent 201 151 0 0 0 0 r\n"
  "LeftButtonReleaseEvent 201 151 0 0 0 0 r\n"
  "RenderEvent 201 151 0 0 0 0 r\n"
  "LeftButtonPressEvent 204 29 0 0 0 0 r\n"
  "RenderEvent 204 29 0 0 0 0 r\n"
  "RenderEvent 210 30 0 0 0 0 r\n"
  "LeftButtonReleaseEvent 210 30 0 0 0 0 r\n"
  "LeftButtonPressEvent 158 159 0 0 0 0 r\n"
  "RenderEvent 158 159 0 0 0 0 r\n"
  "LeftButtonReleaseEvent 169 138 0 0 0 0 r\n"
  "RenderEvent 169 138 0 0 0 0 r\n"
  "RenderEvent 169 138 0 0 0 0 r\n"
  "MouseMoveEvent 251 159 0 0 0 0 r\n"
  "LeftButtonPressEvent 251 159 0 0 0 0 r\n"
  "StartInteractionEvent 251 159 0 0 0 0 r\n"
  "TimerEvent 251 159 0 0 0 0 r\n"
  "RenderEvent 251 159 0 0 0 0 r\n"
  "TimerEvent 251 159 0 0 0 0 r\n"
  "RenderEvent 251 159 0 0 0 0 r\n"
  "TimerEvent 251 159 0 0 0 0 r\n"
  "RenderEvent 251 159 0 0 0 0 r\n"
  "TimerEvent 251 159 0 0 0 0 r\n"
  "RenderEvent 251 159 0 0 0 0 r\n"
  "LeftButtonReleaseEvent 251 159 0 0 0 0 r\n"
  "EndInteractionEvent 251 159 0 0 0 0 r\n"
  "RenderEvent 251 159 0 0 0 0 r\n"
  "LeftButtonPressEvent 250 159 0 0 0 0 r\n"
  "StartInteractionEvent 250 159 0 0 0 0 r\n"
  "TimerEvent 250 159 0 0 0 0 r\n"
  "RenderEvent 250 159 0 0 0 0 r\n"
  "TimerEvent 250 159 0 0 0 0 r\n"
  "RenderEvent 250 159 0 0 0 0 r\n"
  "TimerEvent 250 159 0 0 0 0 r\n"
  "RenderEvent 250 159 0 0 0 0 r\n"
  "TimerEvent 250 159 0 0 0 0 r\n"
  "RenderEvent 250 159 0 0 0 0 r\n"
  "TimerEvent 250 159 0 0 0 0 r\n"
  "RenderEvent 250 159 0 0 0 0 r\n"
  "TimerEvent 250 159 0 0 0 0 r\n"
  "RenderEvent 250 159 0 0 0 0 r\n"
  "LeftButtonReleaseEvent 250 159 0 0 0 0 r\n"
  "EndInteractionEvent 250 159 0 0 0 0 r\n"
  "RenderEvent 250 159 0 0 0 0 r\n"
  "LeftButtonPressEvent 250 159 0 0 0 0 r\n"
  "RenderEvent 250 159 0 0 0 0 r\n"
  "LeftButtonReleaseEvent 250 159 0 0 0 0 r\n"
  "RenderEvent 250 159 0 0 0 0 r\n"
  "LeftButtonPressEvent 209 30 0 0 0 0 r\n"
  "RenderEvent 209 30 0 0 0 0 r\n"
  "MouseMoveEvent 209 30 0 0 0 0 r\n"
  "RenderEvent 209 30 0 0 0 0 r\n"
  "MouseMoveEvent 210 30 0 0 0 0 r\n"
  "RenderEvent 210 30 0 0 0 0 r\n"
  "MouseMoveEvent 210 30 0 0 0 0 r\n"
  "RenderEvent 210 30 0 0 0 0 r\n"
  "MouseMoveEvent 211 30 0 0 0 0 r\n"
  "RenderEvent 211 30 0 0 0 0 r\n"
  "MouseMoveEvent 212 30 0 0 0 0 r\n"
  "RenderEvent 212 30 0 0 0 0 r\n"
  "MouseMoveEvent 214 30 0 0 0 0 r\n"
  "RenderEvent 214 30 0 0 0 0 r\n"
  "MouseMoveEvent 214 30 0 0 0 0 r\n"
  "RenderEvent 214 30 0 0 0 0 r\n"
  "MouseMoveEvent 215 30 0 0 0 0 r\n"
  "RenderEvent 215 30 0 0 0 0 r\n"
  "MouseMoveEvent 233 30 0 0 0 0 r\n"
  "RenderEvent 233 30 0 0 0 0 r\n"
  "LeftButtonReleaseEvent 233 30 0 0 0 0 r\n"
  "MouseMoveEvent 204 30 0 0 0 0 r\n"
  "LeftButtonPressEvent 204 30 0 0 0 0 r\n"
  "RenderEvent 204 30 0 0 0 0 r\n"
  "LeftButtonReleaseEvent 204 30 0 0 0 0 r\n"
  "RenderEvent 204 30 0 0 0 0 r\n"
  "RenderEvent 204 30 0 0 0 0 r\n"
  "MouseMoveEvent 239 83 0 0 0 0 r\n";

// This does the actual work: updates the probe.
// Callback for the interaction
class vtkSliderMultipleViewportsCallback : public vtkCommand
{
public:
  static vtkSliderMultipleViewportsCallback* New()
  {
    return new vtkSliderMultipleViewportsCallback;
  }
  void Execute(vtkObject* caller, unsigned long, void*) VTK_OVERRIDE
  {
    vtkSliderWidget* sliderWidget = reinterpret_cast<vtkSliderWidget*>(caller);
    this->Glyph->SetScaleFactor(
      static_cast<vtkSliderRepresentation*>(sliderWidget->GetRepresentation())
        ->GetValue());
  }
  vtkSliderMultipleViewportsCallback()
    : Glyph(0)
  {
  }
  vtkGlyph3D* Glyph;
};

int TestSliderWidgetMultipleViewports(int argc, char* argv[])
{
  // Create a mace out of filters.
  //
  vtkSmartPointer<vtkSphereSource> sphereSource =
    vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkGlyph3D> glyph = vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInputConnection(sphereSource->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkSmartPointer<vtkAppendPolyData> apd =
    vtkSmartPointer<vtkAppendPolyData>::New();
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphereSource->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> maceMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(apd->GetOutputPort());

  vtkSmartPointer<vtkLODActor> maceActor = vtkSmartPointer<vtkLODActor>::New();
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();
  maceActor->SetPosition(1, 1, 1);

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  ren1->SetViewport(0, 0, 0.5, 1.0);
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  vtkSmartPointer<vtkRenderer> ren2 = vtkSmartPointer<vtkRenderer>::New();
  ren2->SetViewport(0.5, 0, 1.0, 1.0);
  renWin->AddRenderer(ren2);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // VTK widgets consist of two parts: the widget part that handles event
  // processing; and the widget representation that defines how the widget
  // appears in the scene (i.e., matters pertaining to geometry).
  vtkSmartPointer<vtkSliderRepresentation2D> sliderRep =
    vtkSmartPointer<vtkSliderRepresentation2D>::New();
  sliderRep->SetValue(0.25);
  sliderRep->SetTitleText("Spike Size");
  sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint1Coordinate()->SetValue(0.1, 0.1);
  sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep->GetPoint2Coordinate()->SetValue(0.4, 0.1);
  sliderRep->SetSliderLength(0.02);
  sliderRep->SetSliderWidth(0.03);
  sliderRep->SetEndCapLength(0.01);
  sliderRep->SetEndCapWidth(0.03);
  sliderRep->SetTubeWidth(0.005);

  vtkSmartPointer<vtkSliderWidget> sliderWidget =
    vtkSmartPointer<vtkSliderWidget>::New();
  sliderWidget->SetInteractor(iren);
  sliderWidget->SetRepresentation(sliderRep);
  sliderWidget->SetCurrentRenderer(ren2);
  sliderWidget->SetAnimationModeToAnimate();

  vtkSmartPointer<vtkSliderMultipleViewportsCallback> callback =
    vtkSmartPointer<vtkSliderMultipleViewportsCallback>::New();
  callback->Glyph = glyph;
  sliderWidget->AddObserver(vtkCommand::InteractionEvent, callback);
  ren1->AddActor(maceActor);
  sliderWidget->EnabledOn();

  vtkSmartPointer<vtkSliderRepresentation3D> sliderRep3D =
    vtkSmartPointer<vtkSliderRepresentation3D>::New();
  sliderRep3D->SetValue(0.25);
  sliderRep3D->SetTitleText("Spike Size");
  sliderRep3D->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  sliderRep3D->GetPoint1Coordinate()->SetValue(0, 0, 0);
  sliderRep3D->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  sliderRep3D->GetPoint2Coordinate()->SetValue(2, 0, 0);
  sliderRep3D->SetSliderLength(0.075);
  sliderRep3D->SetSliderWidth(0.05);
  sliderRep3D->SetEndCapLength(0.05);

  vtkSmartPointer<vtkSliderWidget> sliderWidget3D =
    vtkSmartPointer<vtkSliderWidget>::New();
  sliderWidget3D->GetEventTranslator()->SetTranslation(
    vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Select);
  sliderWidget3D->GetEventTranslator()->SetTranslation(
    vtkCommand::RightButtonReleaseEvent, vtkWidgetEvent::EndSelect);
  sliderWidget3D->SetInteractor(iren);
  sliderWidget3D->SetRepresentation(sliderRep3D);
  sliderWidget3D->SetCurrentRenderer(ren2);
  sliderWidget3D->SetAnimationModeToAnimate();
  sliderWidget3D->EnabledOn();

  sliderWidget3D->AddObserver(vtkCommand::InteractionEvent, callback);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  ren2->SetBackground(0.9, 0.4, 0.2);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();

  return vtkTesting::InteractorEventLoop(
    argc, argv, iren, TestSliderWidgetMultipleViewportsLog);
}
