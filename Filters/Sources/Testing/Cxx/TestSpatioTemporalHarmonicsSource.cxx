// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetMapper.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSpatioTemporalHarmonicsSource.h"

int TestSpatioTemporalHarmonicsSource(int, char*[])
{
  // Create source
  const int MAX_EXTENT = 10;
  vtkNew<vtkSpatioTemporalHarmonicsSource> source;
  source->SetWholeExtent(-MAX_EXTENT, MAX_EXTENT, -MAX_EXTENT, MAX_EXTENT, -MAX_EXTENT, MAX_EXTENT);

  source->ClearHarmonics();
  source->AddHarmonic(1.0, 1.0, 1.0, 0.0, 0.0, 0.0);
  source->AddHarmonic(2.0, 1.0, 0.0, 1.0, 0.0, 0.0);
  source->AddHarmonic(4.0, 1.0, 0.0, 0.0, 1.0, 0.0);

  source->ClearTimeStepValues();
  source->AddTimeStepValue(0.0);
  source->AddTimeStepValue(1.0);
  source->AddTimeStepValue(2.0);

  source->Update();
  source->UpdateTimeStep(1.0);
  source->Update();

  // Create mapper and actor
  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());
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
  renderer->GetActiveCamera()->SetPosition(50.0, 40.0, 30.0);
  renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
  renderer->ResetCameraClippingRange();

  // Add the actor, render and interact
  renderer->AddActor(actor);
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
