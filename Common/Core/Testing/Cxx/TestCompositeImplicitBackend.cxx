// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeImplicitBackend.h"
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
  vtkCompositeImplicitBackend<int> composite(std::vector<vtkDataArray*>({ left, right }));

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

  vtkNew<vtkIntArray> leftMulti;
  leftMulti->SetNumberOfComponents(3);
  leftMulti->SetNumberOfTuples(10);
  auto leftMultiRange = vtk::DataArrayValueRange<3>(leftMulti);
  std::iota(leftMultiRange.begin(), leftMultiRange.end(), 0);

  vtkNew<vtkIntArray> rightMulti;
  rightMulti->SetNumberOfComponents(3);
  rightMulti->SetNumberOfTuples(10);
  auto rightMultiRange = vtk::DataArrayValueRange<3>(rightMulti);
  std::iota(rightMultiRange.begin(), rightMultiRange.end(), 30);

  vtkCompositeImplicitBackend<int> compositeMulti(
    std::vector<vtkDataArray*>({ leftMulti, rightMulti }));

  for (int i = 0; i < 60; ++i)
  {
    if (i != compositeMulti(i))
    {
      std::cout << "Composite backend operator not functioning: " << i
                << " != " << compositeMulti(i) << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
