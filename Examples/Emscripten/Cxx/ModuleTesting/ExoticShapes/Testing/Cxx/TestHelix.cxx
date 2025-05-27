// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <cstdlib>

#include "HelixSource.h"

#include "vtkActor.h"
#include "vtkDataArray.h"
#include "vtkElevationFilter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLightKit.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTestingInteractor.h"
#include "vtkTubeFilter.h"

int TestHelix(int argc, char* argv[])
{
  vtkNew<HelixSource> helix;
  helix->SetRadius(40.0);
  helix->SetPitch(10.0);
  helix->SetResolutionPerTurn(80);
  helix->SetNumberOfTurns(40);

  vtkNew<vtkTubeFilter> tube;
  tube->SetNumberOfSides(8);
  tube->SetRadius(2.0);
  tube->SetInputConnection(helix->GetOutputPort());

  vtkNew<vtkElevationFilter> elevation;
  elevation->SetLowPoint(0, 0, 0);
  elevation->SetHighPoint(0, 0, helix->GetPitch() * helix->GetNumberOfTurns());
  elevation->SetInputConnection(tube->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(elevation->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetLineWidth(10.0);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderer->AutomaticLightCreationOff();

  vtkNew<vtkLightKit> lightKit;
  lightKit->AddLightsToRenderer(renderer);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(1920, 1080);
  window->AddRenderer(renderer);

  renderer->ResetCamera();
  window->Render();

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  vtkNew<vtkInteractorStyleTrackballCamera> trackball;
  interactor->SetInteractorStyle(trackball);
  interactor->Start();
  return EXIT_SUCCESS;
}
