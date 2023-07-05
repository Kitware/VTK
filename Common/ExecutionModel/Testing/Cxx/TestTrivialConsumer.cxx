// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNew.h"
#include "vtkSphereSource.h"
#include "vtkTrivialConsumer.h"

int TestTrivialConsumer(int, char*[])
{
  vtkNew<vtkSphereSource> spheres;
  vtkNew<vtkTrivialConsumer> consumer;

  consumer->SetInputConnection(spheres->GetOutputPort());
  consumer->Update();

  return 0;
}
