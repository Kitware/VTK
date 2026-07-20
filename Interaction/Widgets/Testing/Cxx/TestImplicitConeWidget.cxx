// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkCamera.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkCone.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkImplicitConeRepresentation.h"
#include "vtkImplicitConeWidget.h"
#include "vtkLODActor.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"

namespace
{

constexpr char eventLog[] = "# StreamVersion 1.2\n"
                            "StartInteractionEvent 167 132 0 0 0 0 0\n"
                            "MouseWheelBackwardEvent 167 132 0 0 0 0 0\n"
                            "MouseWheelBackwardEvent 167 132 0 0 1 0 0\n"
                            "MouseWheelBackwardEvent 167 132 0 0 0 0 0\n"
                            "MouseWheelBackwardEvent 167 132 0 0 1 0 0\n"
                            "MouseWheelBackwardEvent 167 132 0 0 0 0 0\n"
                            "MouseWheelBackwardEvent 167 132 0 0 1 0 0\n"
                            "MouseWheelBackwardEvent 167 132 0 0 0 0 0\n"
                            "MouseWheelBackwardEvent 167 132 0 0 1 0 0\n"
                            "MouseWheelBackwardEvent 167 132 0 0 0 0 0\n"
                            "MouseWheelBackwardEvent 167 132 0 0 1 0 0\n"

                            "MouseMoveEvent 217 138 0 0 0 0 0\n"
                            "LeftButtonPressEvent 217 138 0 0 0 0 0\n"
                            "MouseMoveEvent 194 146 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 194 146 0 0 0 0 0\n"

                            "MouseMoveEvent 211 146 0 0 0 0 0\n"
                            "LeftButtonPressEvent 211 146 0 0 0 0 0\n"
                            "MouseMoveEvent 164 215 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 164 215 0 0 0 0 0\n"

                            // Interact with the widget circle
                            // Needs 3 Mouse Move events to move the widget circle back and forth
                            "MouseMoveEvent 191 181 0 0 0 0 0\n"
                            "LeftButtonPressEvent 191 181 0 0 0 0 0\n"
                            "MouseMoveEvent 176 181 0 0 0 0 0\n"
                            "MouseMoveEvent 198 181 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 198 181 0 0 0 0 0\n";

// This does the actual work: updates the vtkCone implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTICWCallback : public vtkCommand
{
public:
  static vtkTICWCallback* New() { return new vtkTICWCallback; }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkImplicitConeWidget* coneWidget = reinterpret_cast<vtkImplicitConeWidget*>(caller);
    vtkImplicitConeRepresentation* rep =
      reinterpret_cast<vtkImplicitConeRepresentation*>(coneWidget->GetRepresentation());
    rep->GetCone(this->Cone);
    this->Actor->VisibilityOn();
  }

  vtkCone* Cone = nullptr;
  vtkActor* Actor = nullptr;
};

} // anonymous namespace

int TestImplicitConeWidget(int argc, char* argv[])
{
  // Create a mace out of filters.
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkConeSource> coneSource;
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(coneSource->GetOutputPort());
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

  vtkNew<vtkLODActor> maceActor;
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();

  // This portion of the code clips the mace with the vtkCones
  // implicit function. The clipped region is colored green.
  vtkNew<vtkCone> cone;
  cone->SetIsDoubleCone(false);

  vtkNew<vtkClipPolyData> clipper;
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(cone);
  clipper->InsideOutOn();

  vtkNew<vtkPolyDataMapper> selectMapper;
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  vtkNew<vtkLODActor> selectActor;
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0, 1, 0);
  selectActor->VisibilityOff();
  selectActor->SetScale(1.01, 1.01, 1.01);

  // Create the RenderWindow, Renderer and both Actors
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(maceActor);
  renderer->AddActor(selectActor);
  renderer->SetBackground(0.1, 0.2, 0.4);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  renWin->SetInteractor(interactor);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkNew<vtkTICWCallback> myCallback;
  myCallback->Cone = cone;
  myCallback->Actor = selectActor;

  vtkNew<vtkImplicitConeRepresentation> rep;
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(glyph->GetOutput()->GetBounds());

  vtkNew<vtkImplicitConeWidget> coneWidget;
  coneWidget->SetInteractor(interactor);
  coneWidget->SetRepresentation(rep);
  coneWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);
  coneWidget->SetEnabled(true);

  interactor->Initialize();
  return vtkTesting::InteractorEventLoop(argc, argv, interactor, ::eventLog);
}
