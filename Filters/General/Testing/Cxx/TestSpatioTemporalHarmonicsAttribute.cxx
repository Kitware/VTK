// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetMapper.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSpatioTemporalHarmonicsAttribute.h"
#include "vtkSphereSource.h"

int TestSpatioTemporalHarmonicsAttribute(int, char*[])
{
  // Create source
  const int SPHERE_RADIUS = 10;
  const int SPHERE_RESOLUTION = 64;
  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(SPHERE_RADIUS);
  sphere->SetPhiResolution(SPHERE_RESOLUTION);
  sphere->SetThetaResolution(SPHERE_RESOLUTION);

  // Create filter
  vtkNew<vtkSpatioTemporalHarmonicsAttribute> harmonics;
  harmonics->SetInputConnection(sphere->GetOutputPort());

  harmonics->AddHarmonic(1.0, 1.0, 1.0, 1.0, 1.0, 0.0);
  harmonics->ClearHarmonics();

  harmonics->AddHarmonic(1.0, 1.0, 1.0, 0.0, 0.0, 0.0);
  harmonics->AddHarmonic(2.0, 1.0, 0.0, 1.0, 0.0, 0.0);
  harmonics->AddHarmonic(4.0, 1.0, 0.0, 0.0, 1.0, 0.0);

  // Create mapper and actor
  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(harmonics->GetOutputPort());
  mapper->SetScalarRange(-6.0, 6.0);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Create a renderer, render window, and interactor
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(.5, .5, .5);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->ResetCamera();
  renderer->GetActiveCamera()->SetPosition(40.0, 30.0, 20.0);
  renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
  renderer->ResetCameraClippingRange();

  // Add the actor, render and interact
  renderer->AddActor(actor);
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
