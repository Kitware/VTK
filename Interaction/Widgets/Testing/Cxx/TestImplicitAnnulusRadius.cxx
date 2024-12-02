// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkImplicitAnnulusRepresentation.h"
#include "vtkImplicitAnnulusWidget.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLogger.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

namespace records
{
void RecordInteractions(vtkRenderWindowInteractor* interactor)
{
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(interactor);
  recorder->SetFileName("./record.log");
  recorder->Record();

  interactor->Initialize();
  interactor->GetRenderWindow()->Render();
  recorder->On();

  interactor->Start();
  recorder->Stop();
}

const char outerRadiusUpper[] = "# StreamVersion 1.2\n"
                                "LeftButtonPressEvent 153 255 0 0 0 0 0\n"
                                "MouseMoveEvent 153 255 0 0 0 0 0\n"
                                "MouseMoveEvent 153 256 0 0 0 0 0\n"
                                "MouseMoveEvent 153 260 0 0 0 0 0\n"
                                "MouseMoveEvent 153 262 0 0 0 0 0\n"
                                "MouseMoveEvent 153 265 0 0 0 0 0\n"
                                "MouseMoveEvent 153 267 0 0 0 0 0\n"
                                "MouseMoveEvent 153 269 0 0 0 0 0\n"
                                "MouseMoveEvent 153 271 0 0 0 0 0\n"
                                "MouseMoveEvent 153 272 0 0 0 0 0\n"
                                "MouseMoveEvent 153 273 0 0 0 0 0\n"
                                "MouseMoveEvent 153 274 0 0 0 0 0\n"
                                "MouseMoveEvent 153 276 0 0 0 0 0\n"
                                "MouseMoveEvent 153 279 0 0 0 0 0\n"
                                "MouseMoveEvent 153 280 0 0 0 0 0\n"
                                "MouseMoveEvent 153 282 0 0 0 0 0\n"
                                "MouseMoveEvent 153 284 0 0 0 0 0\n"
                                "MouseMoveEvent 153 285 0 0 0 0 0\n"
                                "MouseMoveEvent 153 287 0 0 0 0 0\n"
                                "MouseMoveEvent 153 288 0 0 0 0 0\n"
                                "MouseMoveEvent 153 289 0 0 0 0 0\n"
                                "LeftButtonReleaseEvent 153 289 0 0 0 0 0\n"
                                "RenderEvent 153 289 0 0 0 0 0\n";

const char outerRadiusLower[] = "# StreamVersion 1.2\n"
                                "RenderEvent 177 33 0 0 0 0 0\n"
                                "LeftButtonPressEvent 152 46 0 0 0 0 0\n"
                                "MouseMoveEvent 152 45 0 0 0 0 0\n"
                                "MouseMoveEvent 152 43 0 0 0 0 0\n"
                                "MouseMoveEvent 153 42 0 0 0 0 0\n"
                                "MouseMoveEvent 153 39 0 0 0 0 0\n"
                                "MouseMoveEvent 154 38 0 0 0 0 0\n"
                                "MouseMoveEvent 154 36 0 0 0 0 0\n"
                                "MouseMoveEvent 154 34 0 0 0 0 0\n"
                                "MouseMoveEvent 155 32 0 0 0 0 0\n"
                                "MouseMoveEvent 155 31 0 0 0 0 0\n"
                                "MouseMoveEvent 155 29 0 0 0 0 0\n"
                                "MouseMoveEvent 155 28 0 0 0 0 0\n"
                                "MouseMoveEvent 155 27 0 0 0 0 0\n"
                                "MouseMoveEvent 156 24 0 0 0 0 0\n"
                                "MouseMoveEvent 156 23 0 0 0 0 0\n"
                                "MouseMoveEvent 156 21 0 0 0 0 0\n"
                                "MouseMoveEvent 156 19 0 0 0 0 0\n"
                                "MouseMoveEvent 156 16 0 0 0 0 0\n"
                                "MouseMoveEvent 156 10 0 0 0 0 0\n"
                                "MouseMoveEvent 156 6 0 0 0 0 0\n"
                                "MouseMoveEvent 156 4 0 0 0 0 0\n"
                                "LeftButtonReleaseEvent 156 4 0 0 0 0 0\n"
                                "RenderEvent 177 33 0 0 0 0 0\n";

const char outerRadiusSide[] = "# StreamVersion 1.2\n"
                               "RenderEvent 177 33 0 0 0 0 0\n"
                               "LeftButtonPressEvent 255 143 0 0 0 0 0\n"
                               "MouseMoveEvent 255 143 0 0 0 0 0\n"
                               "MouseMoveEvent 256 143 0 0 0 0 0\n"
                               "MouseMoveEvent 260 143 0 0 0 0 0\n"
                               "MouseMoveEvent 263 143 0 0 0 0 0\n"
                               "MouseMoveEvent 266 143 0 0 0 0 0\n"
                               "MouseMoveEvent 268 143 0 0 0 0 0\n"
                               "MouseMoveEvent 271 143 0 0 0 0 0\n"
                               "MouseMoveEvent 273 143 0 0 0 0 0\n"
                               "MouseMoveEvent 275 143 0 0 0 0 0\n"
                               "MouseMoveEvent 277 143 0 0 0 0 0\n"
                               "MouseMoveEvent 278 143 0 0 0 0 0\n"
                               "MouseMoveEvent 280 143 0 0 0 0 0\n"
                               "MouseMoveEvent 282 143 0 0 0 0 0\n"
                               "MouseMoveEvent 284 143 0 0 0 0 0\n"
                               "MouseMoveEvent 285 143 0 0 0 0 0\n"
                               "MouseMoveEvent 288 143 0 0 0 0 0\n"
                               "MouseMoveEvent 291 143 0 0 0 0 0\n"
                               "MouseMoveEvent 293 143 0 0 0 0 0\n"
                               "MouseMoveEvent 294 143 0 0 0 0 0\n"
                               "LeftButtonReleaseEvent 294 143 0 0 0 0 0\n"
                               "RenderEvent 287 138 0 0 0 0 0\n";

const char innerRadiusUpper[] = "# StreamVersion 1.2\n"
                                "ExposeEvent 0 299 0 0 0 0 0\n"
                                "RenderEvent 0 299 0 0 0 0 0\n"
                                "LeftButtonPressEvent 147 203 0 0 0 0 0\n"
                                "MouseMoveEvent 147 203 0 0 0 0 0\n"
                                "MouseMoveEvent 147 204 0 0 0 0 0\n"
                                "MouseMoveEvent 147 206 0 0 0 0 0\n"
                                "MouseMoveEvent 147 208 0 0 0 0 0\n"
                                "MouseMoveEvent 147 210 0 0 0 0 0\n"
                                "MouseMoveEvent 147 212 0 0 0 0 0\n"
                                "MouseMoveEvent 147 213 0 0 0 0 0\n"
                                "MouseMoveEvent 147 215 0 0 0 0 0\n"
                                "MouseMoveEvent 147 218 0 0 0 0 0\n"
                                "MouseMoveEvent 147 220 0 0 0 0 0\n"
                                "MouseMoveEvent 147 223 0 0 0 0 0\n"
                                "LeftButtonReleaseEvent 147 223 0 0 0 0 0\n"
                                "RenderEvent 147 223 0 0 0 0 0\n";

const char innerRadiusLower[] = "# StreamVersion 1.2\n"
                                "ExposeEvent 0 299 0 0 0 0 0\n"
                                "RenderEvent 0 299 0 0 0 0 0\n"
                                "LeftButtonPressEvent 199 152 0 0 0 0 0\n"
                                "MouseMoveEvent 199 152 0 0 0 0 0\n"
                                "MouseMoveEvent 199 150 0 0 0 0 0\n"
                                "MouseMoveEvent 199 148 0 0 0 0 0\n"
                                "MouseMoveEvent 199 147 0 0 0 0 0\n"
                                "MouseMoveEvent 199 144 0 0 0 0 0\n"
                                "MouseMoveEvent 199 142 0 0 0 0 0\n"
                                "MouseMoveEvent 199 139 0 0 0 0 0\n"
                                "MouseMoveEvent 199 137 0 0 0 0 0\n"
                                "MouseMoveEvent 199 136 0 0 0 0 0\n"
                                "MouseMoveEvent 199 133 0 0 0 0 0\n"
                                "MouseMoveEvent 199 131 0 0 0 0 0\n"
                                "LeftButtonReleaseEvent 199 131 0 0 0 0 0\n"
                                "RenderEvent 199 131 0 0 0 0 0\n";

const char innerRadiusSide[] = "# StreamVersion 1.2\n"
                               "ExposeEvent 0 299 0 0 0 0 0\n"
                               "RenderEvent 0 299 0 0 0 0 0\n"
                               "LeftButtonPressEvent 151 98 0 0 0 0 0\n"
                               "MouseMoveEvent 151 98 0 0 0 0 0\n"
                               "MouseMoveEvent 153 98 0 0 0 0 0\n"
                               "MouseMoveEvent 156 98 0 0 0 0 0\n"
                               "MouseMoveEvent 159 98 0 0 0 0 0\n"
                               "MouseMoveEvent 161 98 0 0 0 0 0\n"
                               "MouseMoveEvent 163 98 0 0 0 0 0\n"
                               "MouseMoveEvent 165 98 0 0 0 0 0\n"
                               "MouseMoveEvent 166 98 0 0 0 0 0\n"
                               "MouseMoveEvent 170 98 0 0 0 0 0\n"
                               "MouseMoveEvent 172 98 0 0 0 0 0\n"
                               "MouseMoveEvent 175 98 0 0 0 0 0\n"
                               "LeftButtonReleaseEvent 175 98 0 0 0 0 0\n"
                               "RenderEvent 175 98 0 0 0 0 0\n";
};

namespace test
{
bool TestInnerOuterRadius(vtkImplicitAnnulusRepresentation* rep)
{
  const double inner = rep->GetInnerRadius();
  const double outer = rep->GetOuterRadius();

  if (inner > outer)
  {
    vtkLog(ERROR, "Inner radius is expected to be lower than outer one.");
    return false;
  }

  const double newInner = outer + 1;
  rep->SetInnerRadius(newInner);
  if (newInner != rep->GetInnerRadius())
  {
    vtkLog(ERROR, "Getter should return previously set value");
    return false;
  }
  // restore for further testing.
  rep->SetInnerRadius(inner);

  return true;
}

bool TestOuterRadiusInteractions(vtkInteractorEventRecorder* recorder,
  vtkImplicitAnnulusRepresentation* annulusRep, const char* log)
{
  const double initialRadius = annulusRep->GetOuterRadius();

  // interact with upper part of the cylinder
  recorder->SetInputString(log);
  recorder->Play();
  recorder->Clear();
  double finalRadius = annulusRep->GetOuterRadius();

  // reset to previous radius for further testing
  annulusRep->SetOuterRadius(initialRadius);
  annulusRep->GetRenderer()->Render();
  return finalRadius > initialRadius;
}

bool TestInnerRadiusInteractions(vtkInteractorEventRecorder* recorder,
  vtkImplicitAnnulusRepresentation* annulusRep, const char* log)
{
  const double initialRadius = annulusRep->GetInnerRadius();

  // interact with upper part of the cylinder
  recorder->SetInputString(log);
  recorder->Play();
  recorder->Clear();
  double finalRadius = annulusRep->GetInnerRadius();

  // reset to previous radius for further testing
  annulusRep->SetInnerRadius(initialRadius);
  annulusRep->GetRenderer()->Render();
  return finalRadius > initialRadius;
}

bool TestInteractions(
  vtkRenderWindowInteractor* interactor, vtkImplicitAnnulusRepresentation* annulusRep)
{
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(interactor);
  recorder->ReadFromInputStringOn();

  if (!TestOuterRadiusInteractions(recorder, annulusRep, records::outerRadiusUpper))
  {
    vtkLog(ERROR, "Outer radius should have increased with upper interaction.");
  }
  if (!TestOuterRadiusInteractions(recorder, annulusRep, records::outerRadiusLower))
  {
    vtkLog(ERROR, "Outer radius should have increased with lower interaction.");
  }
  if (!TestOuterRadiusInteractions(recorder, annulusRep, records::outerRadiusSide))
  {
    vtkLog(ERROR, "Outer radius should have increased with side interaction.");
  }
  if (!TestInnerRadiusInteractions(recorder, annulusRep, records::innerRadiusUpper))
  {
    vtkLog(ERROR, "Inner radius should have increased with upper interaction.");
  }
  if (!TestInnerRadiusInteractions(recorder, annulusRep, records::innerRadiusLower))
  {
    vtkLog(ERROR, "Inner radius should have increased with lower interaction.");
  }
  if (!TestInnerRadiusInteractions(recorder, annulusRep, records::innerRadiusSide))
  {
    vtkLog(ERROR, "Inner radius should have increased with side interaction.");
  }

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();
  return true;
}

}

int TestImplicitAnnulusRadius(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->Update();
  double sphereBounds[6];
  sphere->GetOutput()->GetBounds(sphereBounds);
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);
  sphereActor->VisibilityOn();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(sphereActor);
  // orient camera to get the circles in the screen plane.
  renderer->ResetCamera();
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->Azimuth(90);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  renWin->SetInteractor(interactor);
  renWin->Render();

  // Set Annulus Widget
  vtkNew<vtkImplicitAnnulusRepresentation> rep;
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(sphereBounds);

  vtkNew<vtkImplicitAnnulusWidget> annulusWidget;
  annulusWidget->SetInteractor(interactor);
  annulusWidget->SetRepresentation(rep);
  annulusWidget->SetEnabled(true);

#if 0 // set to 1 if recording
  records::RecordInteractions(interactor);
#else
  test::TestInteractions(interactor, rep);

  // run baseline comparison
  renWin->Render();
  interactor->Start();
#endif

  return EXIT_SUCCESS;
}
