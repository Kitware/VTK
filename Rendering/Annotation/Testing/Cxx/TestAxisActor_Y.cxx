// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "TestAxisActorInternal.h"

int TestAxisActor_Y(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkAxisActor> axis;
  ::InitializeYAxis(axis);
  return TestAxisActorInternal(axis);
}
