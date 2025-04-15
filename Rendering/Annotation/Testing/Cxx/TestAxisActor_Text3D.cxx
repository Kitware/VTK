// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "TestAxisActorInternal.h"

int TestAxisActor_Text3D(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkAxisActor> axis;
  ::InitializeXAxis(axis);
  axis->SetUseTextActor3D(true);

  return TestAxisActorInternal(axis);
}
