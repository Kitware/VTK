// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkImplicitPlaneWidget.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLODActor.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

constexpr char eventLog[] = "# StreamVersion 1\n"
                            "CharEvent 108 202 0 0 105 1 i\n"
                            "KeyReleaseEvent 108 202 0 0 105 1 i\n"
                            "LeftButtonPressEvent 251 148 0 0 0 0 i\n"
                            "MouseMoveEvent 251 149 0 0 0 0 i\n"
                            "MouseMoveEvent 217 237 0 0 0 0 i\n"
                            "LeftButtonReleaseEvent 217 237 0 0 0 0 i\n"
                            "MouseMoveEvent 213 233 0 0 0 0 i\n"
                            "MouseMoveEvent 167 79 0 0 0 0 i\n"
                            "LeftButtonPressEvent 167 79 0 0 0 0 i\n"
                            "MouseMoveEvent 167 80 0 0 0 0 i\n"
                            "MouseMoveEvent 181 103 0 0 0 0 i\n"
                            "LeftButtonReleaseEvent 181 103 0 0 0 0 i\n"
                            "MouseMoveEvent 181 103 0 0 0 0 i\n"
                            "MouseMoveEvent 185 99 0 0 0 0 i\n"
                            "MiddleButtonPressEvent 185 99 0 0 0 0 i\n"
                            "MouseMoveEvent 185 100 0 0 0 0 i\n"
                            "MouseMoveEvent 175 122 0 0 0 0 i\n"
                            "MiddleButtonReleaseEvent 175 122 0 0 0 0 i\n"
                            "MouseMoveEvent 175 122 0 0 0 0 i\n"
                            "MouseMoveEvent 258 44 0 0 0 0 i\n"
                            "LeftButtonPressEvent 258 44 0 0 0 0 i\n"
                            "MouseMoveEvent 258 45 0 0 0 0 i\n"
                            "MouseMoveEvent 236 41 0 0 0 0 i\n"
                            "LeftButtonReleaseEvent 236 41 0 0 0 0 i\n"
                            "MouseMoveEvent 236 40 0 0 0 0 i\n"
                            "MouseMoveEvent 231 43 0 0 0 0 i\n"
                            "RightButtonPressEvent 231 43 0 0 0 0 i\n"
                            "MouseMoveEvent 231 42 0 0 0 0 i\n"
                            "MouseMoveEvent 218 1 0 0 0 0 i\n"
                            "LeaveEvent 220 -1 0 0 0 0 i\n"
                            "MouseMoveEvent 220 -1 0 0 0 0 i\n"
                            "MouseMoveEvent 220 -11 0 0 0 0 i\n"
                            "RightButtonReleaseEvent 220 -11 0 0 0 0 i\n"
                            "EnterEvent 218 0 0 0 0 0 i\n"
                            "MouseMoveEvent 218 0 0 0 0 0 i\n"
                            "MouseMoveEvent 215 14 0 0 0 0 i\n";

// This does the actual work: updates the vtkPlane implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTIPWCallback : public vtkCommand
{
public:
  static vtkTIPWCallback* New() { return new vtkTIPWCallback; }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkImplicitPlaneWidget* planeWidget = reinterpret_cast<vtkImplicitPlaneWidget*>(caller);
    planeWidget->GetPlane(this->Plane);
    this->Actor->VisibilityOn();
  }
  vtkTIPWCallback()
    : Plane(nullptr)
    , Actor(nullptr)
  {
  }
  vtkPlane* Plane;
  vtkActor* Actor;
};

int TestImplicitPlaneWidget(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create a mace out of filters.
  //
  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkGlyph3D> glyph = vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkSmartPointer<vtkAppendPolyData> apd = vtkSmartPointer<vtkAppendPolyData>::New();
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphere->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> maceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(apd->GetOutputPort());

  vtkSmartPointer<vtkLODActor> maceActor = vtkSmartPointer<vtkLODActor>::New();
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();

  // This portion of the code clips the mace with the vtkPlanes
  // implicit function. The clipped region is colored green.
  vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
  vtkSmartPointer<vtkClipPolyData> clipper = vtkSmartPointer<vtkClipPolyData>::New();
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();

  vtkSmartPointer<vtkPolyDataMapper> selectMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  vtkSmartPointer<vtkLODActor> selectActor = vtkSmartPointer<vtkLODActor>::New();
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0, 1, 0);
  selectActor->VisibilityOff();
  selectActor->SetScale(1.01, 1.01, 1.01);

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkSmartPointer<vtkTIPWCallback> myCallback = vtkSmartPointer<vtkTIPWCallback>::New();
  myCallback->Plane = plane;
  myCallback->Actor = selectActor;

  vtkSmartPointer<vtkImplicitPlaneWidget> planeWidget =
    vtkSmartPointer<vtkImplicitPlaneWidget>::New();
  planeWidget->SetInteractor(iren);
  planeWidget->SetPlaceFactor(1.25);
  glyph->Update();
  planeWidget->SetInputConnection(glyph->GetOutputPort());
  planeWidget->PlaceWidget();
  planeWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);

  ren1->AddActor(maceActor);
  ren1->AddActor(selectActor);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  //  recorder->SetFileName("c:/record.log");
  //  recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);

  // render the image
  //
  renWin->SetMultiSamples(0);
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
