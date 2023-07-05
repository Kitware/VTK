// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause AND Apache-2.0
#include <vtkCapsuleSource.h>
#include <vtkCommand.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>

#include <cassert>

int TestCapsuleSource(int, char*[])
{
  vtkSmartPointer<vtkCapsuleSource> capsuleSource = vtkSmartPointer<vtkCapsuleSource>::New();
  capsuleSource->SetThetaResolution(10);
  capsuleSource->SetPhiResolution(8);
  capsuleSource->SetCylinderLength(10.0);
  capsuleSource->SetRadius(10.0);
  capsuleSource->SetLatLongTessellation(false);
  capsuleSource->Update();

  auto capsule = capsuleSource->GetOutput();
  std::cout << *capsule << std::endl;
  assert(capsule->GetNumberOfPoints() == 124);
  assert(capsule->GetNumberOfCells() == 232);

  capsuleSource->SetThetaResolution(6);
  capsuleSource->SetPhiResolution(12);
  capsuleSource->SetCylinderLength(0.3);
  capsuleSource->SetRadius(21.0);
  capsuleSource->SetLatLongTessellation(true);
  capsuleSource->Update();

  capsule = capsuleSource->GetOutput();
  std::cout << *capsule << std::endl;
  assert(capsule->GetNumberOfPoints() == 164);
  assert(capsule->GetNumberOfCells() == 196);

  return EXIT_SUCCESS;
}
