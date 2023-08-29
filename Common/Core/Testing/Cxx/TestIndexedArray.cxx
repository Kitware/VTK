// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIndexedArray.h"

#include "vtkDataArrayRange.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkVTK_DISPATCH_IMPLICIT_ARRAYS.h"

#include <cstdlib>
#include <numeric>
#include <random>

int TestIndexedArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int res = EXIT_SUCCESS;

  vtkNew<vtkIntArray> baseArray;
  baseArray->SetNumberOfComponents(1);
  baseArray->SetNumberOfTuples(1000);
  auto range = vtk::DataArrayValueRange<1>(baseArray);
  std::iota(range.begin(), range.end(), 0);

  vtkNew<vtkIdList> handles;
  handles->SetNumberOfIds(100);
  std::random_device randdev;
  std::mt19937 generator(randdev());
  auto index_rand = std::bind(std::uniform_int_distribution<vtkIdType>(0, 999), generator);
  for (vtkIdType idx = 0; idx < 100; idx++)
  {
    handles->SetId(idx, index_rand());
  }

  vtkNew<vtkIndexedArray<int>> indexed;
  indexed->SetBackend(std::make_shared<vtkIndexedImplicitBackend<int>>(handles, baseArray));
  indexed->SetNumberOfComponents(1);
  indexed->SetNumberOfTuples(100);

  for (vtkIdType iArr = 0; iArr < 100; iArr++)
  {
    if (indexed->GetValue(iArr) != static_cast<int>(handles->GetId(iArr)))
    {
      res = EXIT_FAILURE;
      std::cout << "get value failed with vtkIndexedArray" << std::endl;
    }
  }

  int iArr = 0;
  for (auto val : vtk::DataArrayValueRange<1>(indexed))
  {
    if (val != static_cast<int>(handles->GetId(iArr)))
    {
      res = EXIT_FAILURE;
      std::cout << "range iterator failed with vtkIndexedArray" << std::endl;
    }
    iArr++;
  }

  return res;
};
