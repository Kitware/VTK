// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestPolarAxesInternal.h"
#include "vtkPolarAxesActor.h"

//------------------------------------------------------------------------------
int TestPolarAxes_Text3D(int argc, char* argv[])
{
  vtkNew<vtkPolarAxesActor> polarAxes;
  ::InitializeAxes(polarAxes);
  polarAxes->SetUseTextActor3D(true);
  vtkNew<vtkRenderWindowInteractor> interactor;
  ::CreatePolarAxesPipeline(argc, argv, polarAxes, interactor);
  interactor->Start();
  return EXIT_SUCCESS;
}
