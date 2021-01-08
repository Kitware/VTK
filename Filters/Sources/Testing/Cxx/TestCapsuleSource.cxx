/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCapsuleSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.

  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

  Program: Bender
  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

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
