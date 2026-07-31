// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkCamera.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkFrustum.h"
#include "vtkGlyph3D.h"
#include "vtkImplicitFrustumRepresentation.h"
#include "vtkImplicitFrustumWidget.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

namespace
{

constexpr char eventLog[] = "# StreamVersion 1.2\n"
                            "ExposeEvent 0 299 0 0 0 0 0\n"
                            "RenderEvent 0 299 0 0 0 0 0\n"
                            "MouseWheelBackwardEvent 176 156 0 0 0 0 0\n"
                            "MouseWheelBackwardEvent 176 156 0 0 0 0 0\n"
                            "MouseWheelBackwardEvent 176 156 0 0 0 0 0\n"
                            "RenderEvent 78 109 0 0 0 0 0\n"
                            "LeftButtonPressEvent 78 109 0 0 0 0 0\n"
                            "MouseMoveEvent 78 109 0 0 0 0 0\n"
                            "MouseMoveEvent 95 94 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 95 95 0 0 0 0 0\n"
                            "MouseMoveEvent 95 96 0 0 0 0 0\n"
                            "MouseMoveEvent 174 261 0 0 0 0 0\n"
                            "LeftButtonPressEvent 174 261 0 0 0 0 0\n"
                            "MouseMoveEvent 174 261 0 0 0 0 0\n"
                            "MouseMoveEvent 195 242 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 195 242 0 0 0 0 0\n"
                            "MouseMoveEvent 195 241 0 0 0 0 0\n"
                            "MouseMoveEvent 153 213 0 0 0 0 0\n"
                            "LeftButtonPressEvent 153 213 0 0 0 0 0\n"
                            "MouseMoveEvent 154 213 0 0 0 0 0\n"
                            "MouseMoveEvent 120 204 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 120 204 0 0 0 0 0\n"
                            "MouseMoveEvent 120 204 0 0 0 0 0\n"
                            "MouseMoveEvent 92 96 0 0 0 0 0\n"
                            "LeftButtonPressEvent 92 96 0 0 0 0 0\n"
                            "MouseMoveEvent 93 96 0 0 0 0 0\n"
                            "MouseMoveEvent 134 93 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 134 93 0 0 0 0 0\n"
                            "MouseMoveEvent 134 93 0 0 0 0 0\n"
                            "MouseMoveEvent 160 104 0 0 0 0 0\n"
                            "LeftButtonPressEvent 160 104 0 0 0 0 0\n"
                            "MouseMoveEvent 161 104 0 0 0 0 0\n"
                            "MouseMoveEvent 197 130 0 0 0 0 0\n"
                            "LeftButtonReleaseEvent 197 130 0 0 0 0 0\n"
                            "RenderEvent 197 130 0 0 0 0 0\n";

// This does the actual work: updates the vtkFrustum implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTICWCallback : public vtkCommand
{
public:
  static vtkTICWCallback* New() { return new vtkTICWCallback; }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkImplicitFrustumWidget* frustumWidget = reinterpret_cast<vtkImplicitFrustumWidget*>(caller);
    vtkImplicitFrustumRepresentation* rep =
      reinterpret_cast<vtkImplicitFrustumRepresentation*>(frustumWidget->GetRepresentation());
    rep->GetFrustum(this->Frustum);
    this->Actor->VisibilityOn();
  }

  vtkFrustum* Frustum = nullptr;
  vtkActor* Actor = nullptr;
};

} // anonymous namespace

int TestImplicitFrustumWidget(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
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

  vtkNew<vtkActor> maceActor;
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();

  // This portion of the code clips the mace with the vtkFrustums
  // implicit function. The clipped region is colored green.
  vtkNew<vtkFrustum> frustum;

  vtkNew<vtkClipPolyData> clipper;
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(frustum);
  clipper->InsideOutOn();

  vtkNew<vtkPolyDataMapper> selectMapper;
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  vtkNew<vtkActor> selectActor;
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

  renWin->Render();
  renderer->ResetCamera();

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkNew<vtkTICWCallback> myCallback;
  myCallback->Frustum = frustum;
  myCallback->Actor = selectActor;

  vtkNew<vtkImplicitFrustumRepresentation> rep;
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(glyph->GetOutput()->GetBounds());
  // position origin outside of displayed geometry, for easier interactions
  rep->SetOrigin(-0.8, -0.8, 0);
  rep->SetOrientation(0, 0, -45);

  vtkNew<vtkImplicitFrustumWidget> frustumWidget;
  frustumWidget->SetInteractor(interactor);
  frustumWidget->SetRepresentation(rep);
  frustumWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);
  frustumWidget->SetEnabled(true);

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(interactor);

#if 0 // change to 1 if recording
  recorder->SetFileName("./record.log");
  recorder->Record();

  interactor->Initialize();
  recorder->On();

  interactor->Start();
  recorder->Stop();

#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);

  // render the image
  interactor->Initialize();
  renWin->Render();
  // uncomment to see cursor
  // recorder->ShowCursorOn();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  interactor->Start();
#endif

  return EXIT_SUCCESS;
}
