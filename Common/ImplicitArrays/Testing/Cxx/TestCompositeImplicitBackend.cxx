/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositeImplicitBackend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeImplicitBackend.h"
#include "vtkCompositeImplicitBackend.txx"
#include "vtkDataArrayRange.h"

#include "vtkIntArray.h"

#include <algorithm>
#include <numeric>

int TestCompositeImplicitBackend(int, char*[])
{
  // Setup branches
  vtkNew<vtkIntArray> left;
  left->SetNumberOfComponents(1);
  left->SetNumberOfTuples(10);
  auto leftRange = vtk::DataArrayValueRange<1>(left);
  std::iota(leftRange.begin(), leftRange.end(), 0);

  vtkNew<vtkIntArray> right;
  right->SetNumberOfComponents(1);
  right->SetNumberOfTuples(10);
  auto rightRange = vtk::DataArrayValueRange<1>(right);
  std::iota(rightRange.begin(), rightRange.end(), 10);

  // Make structure
  vtkCompositeImplicitBackend<int> composite(left, right);

  // Do checks on structure
  for (int i = 0; i < 20; ++i)
  {
    if (i != composite(i))
    {
      std::cout << "Composite backend operator not functioning: " << i << " != " << composite(i)
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
