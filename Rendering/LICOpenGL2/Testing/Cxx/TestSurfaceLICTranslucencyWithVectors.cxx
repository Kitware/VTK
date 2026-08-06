// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Reproduces surface LIC rendering with and without dual depth peeling.
// LIC with DDP uses opacity=0.5, does not work.
// ./bin/vtkRenderingLICOpenGL2CxxTests TestSurfaceLICTranslucencyWithVectors
// LIC without DDP uses opacity=1.0
// ./bin/vtkRenderingLICOpenGL2CxxTests TestSurfaceLICTranslucencyWithVectors --no-ddp
#include "vtkActor.h"
#include "vtkArrayCalculator.h"
#include "vtkCamera.h"
#include "vtkDualDepthPeelingPass.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceLICInterface.h"
#include "vtkSurfaceLICMapper.h"
#include "vtkTesting.h"

#include <cstring>

int TestSurfaceLICTranslucencyWithVectors(int argc, char* argv[])
{
  bool ddp = true;
  for (int i = 1; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--no-ddp"))
    {
      ddp = false;
    }
  }

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(64);
  sphere->SetPhiResolution(64);

  // Tangential swirl field V = (-y, x, 0), which lies (mostly) on the surface
  // and produces clear LIC streaks around the z axis.
  vtkNew<vtkArrayCalculator> calc;
  calc->SetInputConnection(sphere->GetOutputPort());
  calc->SetAttributeTypeToPointData();
  calc->AddCoordinateScalarVariable("x", 0);
  calc->AddCoordinateScalarVariable("y", 1);
  calc->AddCoordinateScalarVariable("z", 2);
  calc->SetResultArrayName("V");
  calc->SetFunction("(-y)*iHat + x*jHat + 0*kHat");

  vtkNew<vtkSurfaceLICMapper> mapper;
  mapper->SetInputConnection(calc->GetOutputPort());
  mapper->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "V");

  auto* li = mapper->GetLICInterface();
  li->SetEnable(1);
  li->SetNumberOfSteps(40);
  li->SetStepSize(0.4);
  li->SetLICIntensity(0.8);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.2, 0.2, 0.2);
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> window;
  window->SetMultiSamples(0);
  window->SetSize(500, 500);
  window->AddRenderer(renderer);

  if (ddp)
  {
    actor->GetProperty()->SetOpacity(0.5);
    vtkNew<vtkRenderStepsPass> steps;
    vtkNew<vtkDualDepthPeelingPass> peel;
    peel->SetMaximumNumberOfPeels(1);
    peel->SetTranslucentPass(steps->GetTranslucentPass());
    steps->SetTranslucentPass(peel);
    renderer->SetPass(steps);
  }

  window->Render();
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  if (testing->IsInteractiveModeSpecified())
  {
    vtkNew<vtkInteractorStyleTrackballCamera> style;
    interactor->SetInteractorStyle(style);
    interactor->Start();
  }
  return 0;
}
