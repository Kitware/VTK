// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDummyController.h"

#include "ExerciseMultiProcessController.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int DummyController(int argc, char* argv[])
{
  VTK_CREATE(vtkDummyController, controller);

  controller->Initialize(&argc, &argv, 1);

  int retval = ExerciseMultiProcessController(controller);

  controller->Finalize();

  return retval;
}
