// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause AND Apache-2.0
#include <vtkCommand.h>
#include <vtkCylinderSource.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>

#include <cassert>

int TestCapsuleSource(int, char*[])
{
  vtkSmartPointer<vtkCylinderSource> capsuleSource = vtkSmartPointer<vtkCylinderSource>::New();
  capsuleSource->CappingOn();
  capsuleSource->CapsuleCapOn();
  capsuleSource->SetResolution(10);
  capsuleSource->SetHeight(10.0);
  capsuleSource->SetRadius(10.0);
  capsuleSource->SetLatLongTessellation(false);
  capsuleSource->Update();

  auto capsule = capsuleSource->GetOutput();
  std::cout << *capsule << std::endl;
  assert(capsule->GetNumberOfPoints() == 182);
  assert(capsule->GetNumberOfCells() == 310);

  capsuleSource->SetResolution(6);
  capsuleSource->SetHeight(0.3);
  capsuleSource->SetRadius(21.0);
  capsuleSource->SetLatLongTessellation(true);
  capsuleSource->Update();

  capsule = capsuleSource->GetOutput();
  std::cout << *capsule << std::endl;
  assert(capsule->GetNumberOfPoints() == 62);
  assert(capsule->GetNumberOfCells() == 54);

  return EXIT_SUCCESS;
}
