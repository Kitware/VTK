// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestPolarAxesInternal.h"
#include "vtkPolarAxesActor.h"
#include "vtkRenderWindowInteractor.h"

//------------------------------------------------------------------------------
int TestPolarAxes_Mode2D(int argc, char* argv[])
{
  vtkNew<vtkPolarAxesActor> polarAxes;
  ::InitializeAxes(polarAxes);
  polarAxes->SetUse2DMode(true);
  vtkNew<vtkRenderWindowInteractor> interactor;
  ::CreatePolarAxesPipeline(argc, argv, polarAxes, interactor);
  interactor->Start();
  return EXIT_SUCCESS;
}
