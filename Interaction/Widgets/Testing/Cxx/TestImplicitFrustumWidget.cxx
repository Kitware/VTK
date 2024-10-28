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

const char eventLog[] = "# StreamVersion 1.2\n"
                        "ExposeEvent 0 299 0 0 0 0 0\n"
                        "EnterEvent 287 2 0 0 0 0 0\n"
                        "MouseMoveEvent 210 159 0 0 0 0 0\n"
                        "RenderEvent 210 159 0 0 0 0 0\n"

                        "MouseWheelBackwardEvent 210 159 0 0 0 0 0\n"
                        "MouseWheelBackwardEvent 209 153 0 0 1 0 0\n"
                        "MouseWheelBackwardEvent 200 147 0 0 0 0 0\n"
                        "MouseWheelBackwardEvent 199 146 0 0 1 0 0\n"
                        "MouseWheelBackwardEvent 199 146 0 0 0 0 0\n"
                        "MouseWheelBackwardEvent 199 146 0 0 1 0 0\n"
                        "MouseWheelBackwardEvent 163 262 0 0 0 0 0\n"
                        "MouseWheelBackwardEvent 163 262 0 0 1 0 0\n"

                        "MouseMoveEvent 163 262 0 0 0 0 0\n"
                        "MiddleButtonPressEvent 163 262 0 0 0 0 0\n"
                        "MouseMoveEvent 162 196 0 0 0 0 0\n"
                        "MiddleButtonReleaseEvent 162 196 0 0 0 0 0\n"

                        "MouseWheelBackwardEvent 162 196 0 0 0 0 0\n"

                        "MouseMoveEvent 247 259 0 0 0 0 0\n"
                        "LeftButtonPressEvent 247 259 0 0 0 0 0\n"
                        "MouseMoveEvent 1 257 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 1 257 0 0 0 0 0\n"

                        "MouseMoveEvent 173 78 0 0 0 0 0\n"
                        "LeftButtonPressEvent 173 78 0 0 0 0 0\n"
                        "MouseMoveEvent 157 62 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 157 62 0 0 0 0 0\n"

                        "MouseMoveEvent 140 81 0 0 0 0 0\n"
                        "LeftButtonPressEvent 140 81 0 0 0 0 0\n"
                        "MouseMoveEvent 121 75 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 121 75 0 0 0 0 0\n"

                        "MouseMoveEvent 150 67 0 0 0 0 0\n"
                        "LeftButtonPressEvent 150 67 0 0 0 0 0\n"
                        "MouseMoveEvent 151 67 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 151 67 0 0 0 0 0\n"

                        // Removing the MoveEvents cause the interaction to be wrong.
                        "MouseMoveEvent 151 76 0 0 0 0 0\n"
                        "LeftButtonPressEvent 151 76 0 0 0 0 0\n"
                        "MouseMoveEvent 150 76 0 0 0 0 0\n"
                        "MouseMoveEvent 149 76 0 0 0 0 0\n"
                        "MouseMoveEvent 145 77 0 0 0 0 0\n"
                        "MouseMoveEvent 143 78 0 0 0 0 0\n"
                        "MouseMoveEvent 142 78 0 0 0 0 0\n"
                        "MouseMoveEvent 141 79 0 0 0 0 0\n"
                        "MouseMoveEvent 140 79 0 0 0 0 0\n"
                        "MouseMoveEvent 140 80 0 0 0 0 0\n"
                        "MouseMoveEvent 139 80 0 0 0 0 0\n"
                        "MouseMoveEvent 138 80 0 0 0 0 0\n"
                        "MouseMoveEvent 138 80 0 0 0 0 0\n"
                        "MouseMoveEvent 137 80 0 0 0 0 0\n"
                        "MouseMoveEvent 137 81 0 0 0 0 0\n"
                        "MouseMoveEvent 136 81 0 0 0 0 0\n"
                        "MouseMoveEvent 136 82 0 0 0 0 0\n"
                        "MouseMoveEvent 135 82 0 0 0 0 0\n"
                        "MouseMoveEvent 134 82 0 0 0 0 0\n"
                        "MouseMoveEvent 133 84 0 0 0 0 0\n"
                        "MouseMoveEvent 131 85 0 0 0 0 0\n"
                        "MouseMoveEvent 130 86 0 0 0 0 0\n"
                        "MouseMoveEvent 129 88 0 0 0 0 0\n"
                        "MouseMoveEvent 128 89 0 0 0 0 0\n"
                        "MouseMoveEvent 127 90 0 0 0 0 0\n"
                        "MouseMoveEvent 124 92 0 0 0 0 0\n"
                        "MouseMoveEvent 123 93 0 0 0 0 0\n"
                        "MouseMoveEvent 122 94 0 0 0 0 0\n"
                        "MouseMoveEvent 122 95 0 0 0 0 0\n"
                        "MouseMoveEvent 121 97 0 0 0 0 0\n"
                        "MouseMoveEvent 120 99 0 0 0 0 0\n"
                        "MouseMoveEvent 119 101 0 0 0 0 0\n"
                        "MouseMoveEvent 119 102 0 0 0 0 0\n"
                        "MouseMoveEvent 118 104 0 0 0 0 0\n"
                        "MouseMoveEvent 118 105 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 118 105 0 0 0 0 0\n"

                        "MouseMoveEvent 180 119 0 0 0 0 0\n"
                        "LeftButtonPressEvent 180 119 0 0 0 0 0\n"
                        "MouseMoveEvent 181 128 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 181 128 0 0 0 0 0\n"

                        "MouseMoveEvent 163 268 0 0 0 0 0\n"
                        "LeftButtonPressEvent 163 268 0 0 0 0 0\n"
                        "MouseMoveEvent 363 129 0 0 0 0 0\n"
                        "LeftButtonReleaseEvent 363 129 0 0 0 0 0\n"

                        "MouseMoveEvent 212 149 0 0 0 0 0\n"
                        "MiddleButtonPressEvent 212 149 0 0 0 0 0\n"
                        "MouseMoveEvent 159 155 0 0 0 0 0\n"
                        "MiddleButtonReleaseEvent 159 155 0 0 0 0 0\n"

                        "RenderEvent 159 155 0 0 0 0 0\n";

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

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkNew<vtkTICWCallback> myCallback;
  myCallback->Frustum = frustum;
  myCallback->Actor = selectActor;

  vtkNew<vtkImplicitFrustumRepresentation> rep;
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(glyph->GetOutput()->GetBounds());

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
  renWin->Render();
  recorder->On();

  interactor->Start();
  recorder->Stop();

#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);

  // render the image
  interactor->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  interactor->Start();
#endif

  return EXIT_SUCCESS;
}
