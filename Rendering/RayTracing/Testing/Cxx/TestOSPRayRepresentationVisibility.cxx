// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This test creates a widget, manipulates it and hides its representation to test that the OSPRay
// scenegraph nodes follow representation visibility changes.

#include "vtkAppendPolyData.h"
#include "vtkCamera.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkDisplaySizedImplicitPlaneRepresentation.h"
#include "vtkDisplaySizedImplicitPlaneWidget.h"
#include "vtkFeatureEdges.h"
#include "vtkGlyph3D.h"
#include "vtkMagnifierRepresentation.h"
#include "vtkMagnifierWidget.h"
#include "vtkNew.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOSPRayTestInteractor.h"
#include "vtkPlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"

const char TestOSPRayRepresentationVisibilityLog[] = "# StreamVersion 1.2\n"
                                                     "RenderEvent 226 43 0 0 0 0 0\n"
                                                     "LeftButtonPressEvent 151 122 0 0 0 0 0\n"
                                                     "RenderEvent 151 122 0 0 0 0 0\n"
                                                     "MouseMoveEvent 151 123 0 0 0 0 0\n"
                                                     "RenderEvent 151 123 0 0 0 0 0\n"
                                                     "MouseMoveEvent 151 131 0 0 0 0 0\n"
                                                     "RenderEvent 151 131 0 0 0 0 0\n"
                                                     "MouseMoveEvent 151 135 0 0 0 0 0\n"
                                                     "RenderEvent 151 135 0 0 0 0 0\n"
                                                     "MouseMoveEvent 149 146 0 0 0 0 0\n"
                                                     "RenderEvent 149 146 0 0 0 0 0\n"
                                                     "MouseMoveEvent 148 156 0 0 0 0 0\n"
                                                     "RenderEvent 148 156 0 0 0 0 0\n"
                                                     "MouseMoveEvent 147 158 0 0 0 0 0\n"
                                                     "RenderEvent 147 158 0 0 0 0 0\n"
                                                     "LeftButtonReleaseEvent 147 158 0 0 0 0 0\n"
                                                     "RenderEvent 147 158 0 0 0 0 0\n"
                                                     "MouseMoveEvent 438 138 0 0 0 0 0\n"
                                                     "InteractionEvent 438 138 0 0 0 0 0\n"
                                                     "TimerEvent 438 138 0 0 0 0 0\n"
                                                     "RenderEvent 438 138 0 0 0 0 0\n"
                                                     "TimerEvent 438 138 0 0 0 0 0\n"
                                                     "RenderEvent 438 138 0 0 0 0 0\n"
                                                     "TimerEvent 438 138 0 0 0 0 0\n"
                                                     "RenderEvent 438 138 0 0 0 0 0\n"
                                                     "TimerEvent 438 138 0 0 0 0 0\n"
                                                     "RenderEvent 438 138 0 0 0 0 0\n"
                                                     "TimerEvent 438 138 0 0 0 0 0\n"
                                                     "RenderEvent 438 138 0 0 0 0 0\n"
                                                     "TimerEvent 438 138 0 0 0 0 0\n"
                                                     "RenderEvent 438 138 0 0 0 0 0\n"
                                                     "TimerEvent 438 138 0 0 0 0 0\n"
                                                     "RenderEvent 438 138 0 0 0 0 0\n"
                                                     "TimerEvent 438 138 0 0 0 0 0\n"
                                                     "RenderEvent 438 138 0 0 0 0 0\n"
                                                     "TimerEvent 438 138 0 0 0 0 0\n"
                                                     "RenderEvent 438 138 0 0 0 0 0\n"
                                                     "LeftButtonReleaseEvent 438 138 0 0 0 0 0\n"
                                                     "EndInteractionEvent 438 138 0 0 0 0 0\n"
                                                     "RenderEvent 438 138 0 0 0 0 0\n"
                                                     "EnterEvent 137 0 0 0 0 0 0\n"
                                                     "MouseMoveEvent 137 0 0 0 0 0 0\n"
                                                     "RenderEvent 137 0 0 0 0 0 0\n"
                                                     "StartInteractionEvent 468 137 0 0 0 0 0\n"
                                                     "TimerEvent 468 137 0 0 0 0 0\n"
                                                     "RenderEvent 468 137 0 0 0 0 0\n"
                                                     "TimerEvent 468 137 0 0 0 0 0\n"
                                                     "RenderEvent 468 137 0 0 0 0 0\n"
                                                     "TimerEvent 468 137 0 0 0 0 0\n"
                                                     "RenderEvent 468 137 0 0 0 0 0\n"
                                                     "TimerEvent 468 137 0 0 0 0 0\n"
                                                     "RenderEvent 468 137 0 0 0 0 0\n"
                                                     "TimerEvent 468 137 0 0 0 0 0\n"
                                                     "RenderEvent 468 137 0 0 0 0 0\n"
                                                     "LeftButtonReleaseEvent 468 137 0 0 0 0 0\n"
                                                     "EndInteractionEvent 468 137 0 0 0 0 0\n"
                                                     "RenderEvent 468 137 0 0 0 0 0\n"
                                                     "MouseMoveEvent 467 137 0 0 0 0 0\n"
                                                     "RenderEvent 467 137 0 0 0 0 0\n"
                                                     "MouseMoveEvent 466 137 0 0 0 0 0\n"
                                                     "RenderEvent 466 137 0 0 0 0 0\n"
                                                     "MouseMoveEvent 464 137 0 0 0 0 0\n"
                                                     "RenderEvent 464 137 0 0 0 0 0\n"
                                                     "MouseMoveEvent 454 140 0 0 0 0 0\n"
                                                     "RenderEvent 454 140 0 0 0 0 0\n"
                                                     "MouseMoveEvent 450 140 0 0 0 0 0\n"
                                                     "RenderEvent 450 140 0 0 0 0 0\n"
                                                     "";

// This does the actual work: updates the vtkPlane implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkOSPRayRVCallback : public vtkCommand
{
public:
  static vtkOSPRayRVCallback* New() { return new vtkOSPRayRVCallback; }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    auto* planeWidget = reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(caller);
    auto* rep = reinterpret_cast<vtkDisplaySizedImplicitPlaneRepresentation*>(
      planeWidget->GetRepresentation());
    rep->GetPlane(this->Plane);
    this->Actor->VisibilityOn();
  }

  vtkOSPRayRVCallback()
    : Plane(nullptr)
    , Actor(nullptr)
  {
  }

  vtkPlane* Plane;
  vtkActor* Actor;
};

class vtkOSPRayRVCallbackT : public vtkCommand
{
public:
  static vtkOSPRayRVCallbackT* New() { return new vtkOSPRayRVCallbackT; }

  void Execute(vtkObject* caller, unsigned long, void*) override { this->Repr->SetVisibility(0); }

  vtkOSPRayRVCallbackT()
    : Repr(nullptr)
  {
  }
  vtkDisplaySizedImplicitPlaneRepresentation* Repr;
};

int TestOSPRayRepresentationVisibility(int argc, char* argv[])
{
  // Create a mace out of filters.
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkConeSource> cone;
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkNew<vtkAppendPolyData> apd;
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphere->GetOutputPort());

  vtkNew<vtkPolyDataMapper> maceMapper;
  maceMapper->SetInputConnection(apd->GetOutputPort());

  vtkNew<vtkActor> maceActor;
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();

  // This portion of the code clips the mace with the vtkPlanes
  // implicit function. The clipped region is colored green.
  vtkNew<vtkPlane> plane;
  vtkNew<vtkClipPolyData> clipper;
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();

  vtkNew<vtkPolyDataMapper> selectMapper;
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  vtkNew<vtkActor> selectActor;
  selectActor->SetMapper(selectMapper);
  selectActor->SetScale(1.01, 1.01, 1.01);
  selectActor->VisibilityOff();
  selectActor->GetProperty()->SetColor(0.4, 0.4, 1);

  // Special effect to see edges
  vtkNew<vtkFeatureEdges> edges;
  edges->SetInputConnection(sphere->GetOutputPort());
  edges->ExtractAllEdgeTypesOff();
  edges->ManifoldEdgesOn();

  vtkNew<vtkPolyDataMapper> edgeMapper;
  edgeMapper->SetInputConnection(edges->GetOutputPort());
  edgeMapper->SetScalarVisibility(false);
  vtkNew<vtkActor> edgeActor;
  edgeActor->SetMapper(edgeMapper);
  edgeActor->GetProperty()->SetColor(1, 0.4, 0.4);
  edgeActor->GetProperty()->SetLineWidth(2.0);

  // Create the RenderWindow, Renderer and both Actors
  vtkNew<vtkRenderer> ren1;
  ren1->SetBackground(0.5, 0.3, 0.2);
  ren1->SetViewport(0, 0, 0.5, 1);
  vtkNew<vtkRenderer> ren2;
  ren2->SetBackground(0.8, 0.8, 0.6);
  ren2->SetViewport(0.5, 0, 1, 1);
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 300);
  renWin->AddRenderer(ren1);
  renWin->AddRenderer(ren2);
  ren1->AddActor(maceActor);
  ren1->AddActor(selectActor);
  ren2->AddActor(maceActor);
  ren2->AddActor(edgeActor);
  vtkOSPRayRendererNode::SetSamplesPerPixel(16, ren1);
  vtkOSPRayRendererNode::SetSamplesPerPixel(16, ren2);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkOSPRayRVCallback> myCallback;
  myCallback->Plane = plane;
  myCallback->Actor = selectActor;

  vtkNew<vtkDisplaySizedImplicitPlaneRepresentation> rep;
  rep->ScaleEnabledOn();
  rep->SetPlaceFactor(1.25); // This must be set prior to placing the widget
  rep->PlaceWidget(selectActor->GetBounds());
  rep->SetNormal(plane->GetNormal());
  rep->DrawOutlineOn();
  rep->DrawIntersectionEdgesOn();

  vtkNew<vtkDisplaySizedImplicitPlaneWidget> planeWidget;
  planeWidget->SetInteractor(iren);
  planeWidget->SetRepresentation(rep);
  planeWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);
  planeWidget->SetDefaultRenderer(ren1);

  vtkNew<vtkMagnifierRepresentation> magRep;
  magRep->BorderOn();
  magRep->SetMagnificationFactor(15.0);
  magRep->GetBorderProperty()->SetColor(0.4, 0.4, 1);
  magRep->AddViewProp(maceActor);
  magRep->AddViewProp(edgeActor);

  vtkNew<vtkMagnifierWidget> magW;
  magW->SetInteractor(iren);
  magW->SetRepresentation(magRep);
  magW->SetDefaultRenderer(ren2);

  vtkNew<vtkOSPRayRVCallbackT> cbt;
  cbt->Repr = rep;
  iren->AddObserver(vtkCommand::TimerEvent, cbt);

  vtkNew<vtkOSPRayPass> ospray;
  ren1->SetPass(ospray);
  vtkNew<vtkOSPRayPass> ospray2;
  ren2->SetPass(ospray2);

  // Render
  ren1->ResetCamera();
  ren1->GetActiveCamera()->Azimuth(100);
  ren2->ResetCamera();
  renWin->Render();
  iren->Initialize();

  planeWidget->On();
  magW->On();

  return vtkTesting::InteractorEventLoop(argc, argv, iren, TestOSPRayRepresentationVisibilityLog);
}
