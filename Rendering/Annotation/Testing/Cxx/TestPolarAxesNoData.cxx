// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPolarAxesActor.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"

#include "vtkTestUtilities.h"

//------------------------------------------------------------------------------
int TestPolarAxesNoData(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(0., .5, 0.);
  camera->SetPosition(5., 6., 14.);

  vtkNew<vtkLight> light;
  light->SetFocalPoint(0.21406, 1.5, 0.0);
  light->SetPosition(7., 7., 4.);

  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->AddLight(light);

  vtkNew<vtkPolarAxesActor> polarAxes;
  polarAxes->SetPole(.5, 1., 3.);
  polarAxes->SetMaximumRadius(3.);
  polarAxes->SetMinimumAngle(-60.);
  polarAxes->SetMaximumAngle(210.);
  polarAxes->SetRequestedNumberOfRadialAxes(10);
  polarAxes->SetCamera(renderer->GetActiveCamera());
  polarAxes->SetPolarLabelFormat("%6.1f");
  polarAxes->GetLastRadialAxisProperty()->SetColor(0.0, 1.0, 0.0);
  polarAxes->GetSecondaryRadialAxesProperty()->SetColor(0.0, 0.0, 1.0);
  polarAxes->GetPolarArcsProperty()->SetColor(1.0, 0.0, 0.0);
  polarAxes->GetSecondaryPolarArcsProperty()->SetColor(1.0, 0.0, 1.0);
  polarAxes->GetPolarAxisProperty()->SetColor(1.0, 0.5, 0.0);
  polarAxes->GetPolarAxisTitleTextProperty()->SetColor(0.0, 0.0, 0.0);
  polarAxes->GetPolarAxisLabelTextProperty()->SetColor(1.0, 1.0, 0.0);
  polarAxes->GetLastRadialAxisTextProperty()->SetColor(0.0, 0.5, 0.0);
  polarAxes->GetSecondaryRadialAxesTextProperty()->SetColor(0.0, 1.0, 1.0);
  polarAxes->SetScreenSize(9.0);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  renWin->SetWindowName("VTK - Polar Axes");
  renWin->SetSize(600, 600);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renderer->SetBackground(.8, .8, .8);
  renderer->AddViewProp(polarAxes);
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
