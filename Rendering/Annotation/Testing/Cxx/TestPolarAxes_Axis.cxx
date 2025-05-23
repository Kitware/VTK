// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestPolarAxesInternal.h"

#include "vtkAxisActor.h"
#include "vtkPolarAxesActor.h"
#include "vtkRenderWindowInteractor.h"

//------------------------------------------------------------------------------
int TestPolarAxes_Axis(int argc, char* argv[])
{
  vtkNew<vtkPolarAxesActor> polarAxes;
  ::InitializeAxes(polarAxes);
  polarAxes->SetMinimumAngle(0);
  polarAxes->SetMaximumAngle(180);
  polarAxes->SetRequestedNumberOfRadialAxes(5);
  polarAxes->SetRadialUnits(false);                           // default true;
  polarAxes->SetTickLocation(vtkAxisActor::VTK_TICKS_INSIDE); // default BOTH
  polarAxes->SetAxisMinorTickVisibility(true);                // default off
  polarAxes->SetLastRadialAxisMajorTickSize(1.);              // default 0.
  polarAxes->SetPolarAxisMajorTickThickness(2.5);             // default 1
  polarAxes->SetPolarLabelOffset(22);                         // default 10
  polarAxes->SetPolarAxisTitle("Testing Polar Axes Title");
  polarAxes->SetDrawPolarArcsGridlines(false);
  vtkNew<vtkRenderWindowInteractor> interactor;
  ::CreatePolarAxesPipeline(argc, argv, polarAxes, interactor);
  interactor->Start();
  return EXIT_SUCCESS;
}
