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

#include <vtkActor.h>
#include <vtkCapsuleSource.h>
#include <vtkCell.h>
#include <vtkCommand.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTesting.h>
#include <vtkTestingInteractor.h>

bool CompareVector3(const double* v1, const double* v2)
{
  double diff[3];
  vtkMath::Subtract(v1, v2, diff);
  if (vtkMath::Dot(diff, diff) < 1e-6)
  {
    return true;
  }
  return false;
}

bool CompareCell(vtkCell* c1, vtkCell* c2)
{
  if (c1->GetCellDimension() != c2->GetCellDimension())
  {
    return false;
  }
  for (int i = 0; i < c1->GetCellDimension(); ++i)
  {
    if (c1->GetPointId(i) != c2->GetPointId(i))
    {
      return false;
    }
  }
  return true;
}

bool ComparePolyData(vtkPolyData* p1, vtkPolyData* p2)
{
  int pointPositionError = 0;
  for (vtkIdType i = 0; i < p1->GetNumberOfPoints(); ++i)
  {
    if (!CompareVector3(p1->GetPoint(i), p2->GetPoint(i)))
    {
      ++pointPositionError;
    }
  }
  if (pointPositionError > 0)
  {
    std::cerr << "There are " << pointPositionError << " points different." << std::endl;
  }
  int cellConnectionError = 0;
  for (vtkIdType i = 0; i < p1->GetNumberOfCells(); ++i)
  {
    if (!CompareCell(p1->GetCell(i), p2->GetCell(i)))
    {
      ++cellConnectionError;
    }
  }
  if (cellConnectionError > 0)
  {
    std::cerr << "There are " << pointPositionError << " cells different." << std::endl;
  }
  return pointPositionError == 0 && cellConnectionError == 0;
}

int TestCapsuleSource(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "The baseline filename is missing." << std::endl;
    return EXIT_FAILURE;
  }
  vtkSmartPointer<vtkCapsuleSource> capsuleSource = vtkSmartPointer<vtkCapsuleSource>::New();
  capsuleSource->SetThetaResolution(10);
  capsuleSource->SetPhiResolution(8);
  capsuleSource->SetCylinderLength(10.0);
  capsuleSource->SetRadius(10.0);
  capsuleSource->SetLatLongTessellation(false);
  capsuleSource->Update();
  std::string baselineCaspuleFilename = argv[1];
  baselineCaspuleFilename += "/TestTriangularCapsuleSource.vtp";
  vtkSmartPointer<vtkPolyDataReader> r = vtkSmartPointer<vtkPolyDataReader>::New();
  r->SetFileName(baselineCaspuleFilename.c_str());
  r->Update();
  bool success = ComparePolyData(r->GetOutput(), capsuleSource->GetOutput());
  if (!success)
  {
    std::cerr << "The generated capsule and the"
              << " baseline are different." << std::endl;
    return EXIT_FAILURE;
  }
  capsuleSource->SetThetaResolution(6);
  capsuleSource->SetPhiResolution(12);
  capsuleSource->SetCylinderLength(0.3);
  capsuleSource->SetRadius(21.0);
  capsuleSource->SetLatLongTessellation(true);
  capsuleSource->Update();
  baselineCaspuleFilename = argv[1];
  baselineCaspuleFilename += "/TestQuadrangleCapsuleSource.vtp";
  r->SetFileName(baselineCaspuleFilename.c_str());
  r->Update();
  success = ComparePolyData(r->GetOutput(), capsuleSource->GetOutput());
  if (!success)
  {
    std::cerr << "The generated capsule and the"
              << " baseline are different." << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
