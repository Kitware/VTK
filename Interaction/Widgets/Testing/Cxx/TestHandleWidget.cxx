// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// This example tests the vtkHandleWidget.
//
// The handle that you see is always constrained to lie on a plane (
// defined by a vtkImplicitPlaneWidget2). It goes to show that you can place
// constraints on the movement of the handle. You can move the plane around
// interactively. It exercises the class vtkBoundedPlanePointPlacer.

#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkCutter.h"
#include "vtkGlyph3D.h"
#include "vtkHandleWidget.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLODActor.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPlane.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

//------------------------------------------------------------------------------
// This does the actual work: updates the vtkPline implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTIPW3Callback : public vtkCommand
{
public:
  static vtkTIPW3Callback* New() { return new vtkTIPW3Callback; }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkImplicitPlaneWidget2* planeWidget = reinterpret_cast<vtkImplicitPlaneWidget2*>(caller);
    vtkImplicitPlaneRepresentation* rep =
      reinterpret_cast<vtkImplicitPlaneRepresentation*>(planeWidget->GetRepresentation());
    rep->GetPlane(this->Plane);
    this->Actor->VisibilityOn();
  }

  vtkTIPW3Callback()
    : Actor(nullptr)
  {
    this->Plane = vtkPlane::New();
  }
  ~vtkTIPW3Callback() override { this->Plane->Delete(); }

  vtkPlane* Plane;
  vtkActor* Actor;
};

//------------------------------------------------------------------------------
static char HandleWidgetLog[] = "# StreamVersion 1\n"
                                "MouseMoveEvent 232 170 0 0 0 0 0\n"
                                "LeftButtonPressEvent 232 170 0 0 0 0 0\n"
                                "MouseMoveEvent 203 173 0 0 0 0 0\n"
                                "MouseMoveEvent 183 194 0 0 0 0 0\n"
                                "LeftButtonReleaseEvent 183 194 0 0 0 0 0\n"

                                "MouseMoveEvent 143 175 0 0 0 0 0\n"
                                "LeftButtonPressEvent 143 175 0 0 0 0 0\n"
                                "MouseMoveEvent 130 161 0 0 0 0 0\n"
                                "LeftButtonReleaseEvent 130 161 0 0 0 0 0\n"

                                "MouseMoveEvent 147 149 0 0 0 0 0\n"
                                "LeftButtonPressEvent 147 149 0 0 0 0 0\n"
                                "MouseMoveEvent 180 147 0 0 0 0 0\n"
                                "LeftButtonReleaseEvent 180 147 0 0 0 0 0\n"

                                "MouseMoveEvent 161 158 0 0 0 0 0\n"
                                "LeftButtonPressEvent 161 158 0 0 0 0 0\n"
                                "MouseMoveEvent 175 149 0 0 0 0 0\n"
                                "LeftButtonReleaseEvent 175 149 0 0 0 0 0\n"

                                "MouseMoveEvent 135 159 0 0 0 0 0\n"
                                "LeftButtonPressEvent 135 159 0 0 0 0 0\n"
                                "MouseMoveEvent 160 174 0 0 0 0 0\n"
                                "LeftButtonReleaseEvent 160 174 0 0 0 0 0\n"

                                "MouseMoveEvent 228 194 0 0 0 0 0\n"
                                "LeftButtonPressEvent 228 194 0 0 0 0 0\n"
                                "MouseMoveEvent 203 163 0 0 0 0 0\n"
                                "LeftButtonReleaseEvent 203 163 0 0 0 0 0\n"

                                "MouseMoveEvent 157 176 0 0 0 0 0\n"
                                "LeftButtonPressEvent 157 176 0 0 0 0 0\n"
                                "MouseMoveEvent 141 163 0 0 0 0 0\n"
                                "LeftButtonReleaseEvent 141 163 0 0 0 0 0\n";

//------------------------------------------------------------------------------
int TestHandleWidget(int argc, char* argv[])
{
  // Create a mace out of filters.
  //
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkNew<vtkAppendPolyData> apd;
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphere->GetOutputPort());

  // This portion of the code clips the mace with the vtkPlanes
  // implicit function. The cut region is colored green.
  vtkNew<vtkTIPW3Callback> myCallback;
  vtkNew<vtkCutter> cutter;
  cutter->SetInputConnection(apd->GetOutputPort());
  cutter->SetCutFunction(myCallback->Plane);

  vtkNew<vtkPolyDataMapper> selectMapper;
  selectMapper->SetInputConnection(cutter->GetOutputPort());

  vtkNew<vtkLODActor> selectActor;
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0, 1, 0);
  selectActor->VisibilityOff();
  selectActor->SetScale(1.01, 1.01, 1.01);

  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputConnection(apd->GetOutputPort());
  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outline->GetOutputPort());
  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper);

  double repBounds[6] = { -0.7, 0.7, -0.7, 0.7, -0.7, 0.7 };
  vtkNew<vtkImplicitPlaneRepresentation> rep;
  rep->SetPlaceFactor(1.0);
  rep->GetPlaneProperty()->SetAmbientColor(0.0, 0.5, 0.5);
  rep->GetPlaneProperty()->SetOpacity(0.3);
  rep->PlaceWidget(repBounds);
  vtkNew<vtkImplicitPlaneWidget2> planeWidget;
  planeWidget->SetRepresentation(rep);

  myCallback->Actor = selectActor;

  planeWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // VTK widgets consist of two parts: the widget part that handles event processing;
  // and the widget representation that defines how the widget appears in the scene
  // (i.e., matters pertaining to geometry).
  vtkNew<vtkPointHandleRepresentation3D> handleRep;
  handleRep->SetPlaceFactor(2.5);
  handleRep->PlaceWidget(outlineActor->GetBounds());
  handleRep->SetHandleSize(30);

  vtkSmartPointer<vtkHandleWidget> handleWidget = vtkSmartPointer<vtkHandleWidget>::New();
  handleWidget->SetInteractor(iren);
  planeWidget->SetInteractor(iren);
  handleWidget->SetRepresentation(handleRep);

  ren1->AddActor(selectActor);
  ren1->AddActor(outlineActor);

  // Add the actors to the renderer, set the background and size
  //

  // record events
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
  //  recorder->SetFileName("c:/record.log");
  //  recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(HandleWidgetLog);
  recorder->EnabledOn();

  // Should we constrain the handles to the oblique plane ?
  bool constrainHandlesToObliquePlane = false;
  for (int i = 0; i < argc; i++)
  {
    if (strcmp("-ConstrainHandlesToPlane", argv[i]) == 0)
    {
      constrainHandlesToObliquePlane = true;
      break;
    }
  }

  // Set some defaults.
  //
  rep->SetNormal(0.942174, 0.25322, 0.219519);
  double worldPos[3] = { -0.0417953, 0.202206, -0.0538641 };
  handleRep->SetWorldPosition(worldPos);
  rep->GetPlane(myCallback->Plane);

  if (constrainHandlesToObliquePlane)
  {
    vtkNew<vtkBoundedPlanePointPlacer> placer;

    // Define the plane as the image plane widget's plane
    placer->SetProjectionNormalToOblique();
    placer->SetObliquePlane(myCallback->Plane);

    // Also add bounding planes for the bounds of the dataset.
    double bounds[6];
    outline->GetOutput()->GetBounds(bounds);
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(bounds[0], bounds[2], bounds[4]);
    plane->SetNormal(1.0, 0.0, 0.0);
    placer->AddBoundingPlane(plane);

    plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(bounds[1], bounds[3], bounds[5]);
    plane->SetNormal(-1.0, 0.0, 0.0);
    placer->AddBoundingPlane(plane);

    plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(bounds[0], bounds[2], bounds[4]);
    plane->SetNormal(0.0, 1.0, 0.0);
    placer->AddBoundingPlane(plane);

    plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(bounds[1], bounds[3], bounds[5]);
    plane->SetNormal(0.0, -1.0, 0.0);
    placer->AddBoundingPlane(plane);

    plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(bounds[0], bounds[2], bounds[4]);
    plane->SetNormal(0.0, 0.0, 1.0);
    placer->AddBoundingPlane(plane);

    plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(bounds[1], bounds[3], bounds[5]);
    plane->SetNormal(0.0, 0.0, -1.0);
    placer->AddBoundingPlane(plane);

    handleRep->SetPointPlacer(placer);
  }

  iren->Initialize();
  renWin->Render();
  handleWidget->EnabledOn();
  planeWidget->EnabledOn();
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();
  renWin->Render();

  recorder->Play();

  iren->Start();

  return EXIT_SUCCESS;
}
