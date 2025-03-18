// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "TestAxisActorInternal.h"

int TestAxisActor_X(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkAxisActor> axis;
  ::InitializeXAxis(axis);
  return TestAxisActorInternal(axis);
}
