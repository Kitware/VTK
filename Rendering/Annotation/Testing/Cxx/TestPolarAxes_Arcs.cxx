// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestPolarAxesInternal.h"

#include "vtkPolarAxesActor.h"
#include "vtkRenderWindowInteractor.h"

//------------------------------------------------------------------------------
int TestPolarAxes_Arcs(int argc, char* argv[])
{
  vtkNew<vtkPolarAxesActor> polarAxes;
  ::InitializeAxes(polarAxes);
  polarAxes->SetMinimumAngle(120);
  polarAxes->SetMaximumAngle(-450); // clamped to -360, i.e. 0
  polarAxes->SetMinimumRadius(0.5);
  polarAxes->SetMaximumRadius(3.5);
  polarAxes->SetArcMinorTickVisibility(true);
  polarAxes->SetArcTickRatioSize(0.8);
  polarAxes->SetPolarArcResolutionPerDegree(0.05);
  vtkNew<vtkRenderWindowInteractor> interactor;
  ::CreatePolarAxesPipeline(argc, argv, polarAxes, interactor);
  interactor->Start();
  return EXIT_SUCCESS;
}
