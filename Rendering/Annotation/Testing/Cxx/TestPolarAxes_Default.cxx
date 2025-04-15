// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestPolarAxesInternal.h"

#include "vtkPolarAxesActor.h"
#include "vtkRenderWindowInteractor.h"

//------------------------------------------------------------------------------
int TestPolarAxes_Default(int argc, char* argv[])
{
  vtkNew<vtkPolarAxesActor> polarAxes;
  // just bring polaraxes in front of the loaded data
  polarAxes->SetPole(.5, 1., 3);
  vtkNew<vtkRenderWindowInteractor> interactor;
  ::CreatePolarAxesPipeline(argc, argv, polarAxes, interactor);
  polarAxes->GetCamera()->Zoom(3);
  interactor->Start();
  return EXIT_SUCCESS;
}
