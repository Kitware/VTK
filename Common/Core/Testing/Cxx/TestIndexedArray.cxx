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

  std::uniform_int_distribution<vtkIdType> dist(0, 999);
  auto index_rand = [&]() { return dist(generator); };
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
      std::cerr << "get value failed with vtkIndexedArray" << std::endl;
    }
  }

  int iArr = 0;
  for (auto val : vtk::DataArrayValueRange<1>(indexed))
  {
    if (val != static_cast<int>(handles->GetId(iArr)))
    {
      res = EXIT_FAILURE;
      std::cerr << "range iterator failed with vtkIndexedArray" << std::endl;
    }
    iArr++;
  }

  // Test memory size measurement for a large array
  vtkNew<vtkIdList> largeHandles;
  largeHandles->SetNumberOfIds(1024 * 3);

  vtkNew<vtkIntArray> largeArray;
  largeArray->SetNumberOfComponents(4);
  largeArray->SetNumberOfTuples(1024 * 5);

  vtkNew<vtkIndexedArray<int>> largeIndexed;
  largeIndexed->SetBackend(
    std::make_shared<vtkIndexedImplicitBackend<int>>(largeHandles, largeArray));

  unsigned long expectedSizeInKib = 3 * sizeof(vtkIdType) + largeArray->GetActualMemorySize();
  if (largeIndexed->GetActualMemorySize() != expectedSizeInKib)
  {
    res = EXIT_FAILURE;
    std::cerr << "Wrong value memory size value for large vtkIndexedArray: "
              << largeIndexed->GetActualMemorySize() << " KiB instead of " << expectedSizeInKib
              << std::endl;
  }

  // Test memory size for an array smaller than 1KiB
  vtkNew<vtkIdList> smallHandles;
  smallHandles->SetNumberOfIds(5);

  vtkNew<vtkIntArray> smallArray;
  smallArray->SetNumberOfComponents(5);
  smallArray->SetNumberOfTuples(5);

  vtkNew<vtkIndexedArray<int>> smallIndexed;
  smallIndexed->SetBackend(
    std::make_shared<vtkIndexedImplicitBackend<int>>(smallHandles, smallArray));

  if (smallIndexed->GetActualMemorySize() != 2)
  {
    res = EXIT_FAILURE;
    std::cerr << "Wrong value memory size value for large vtkIndexedArray: "
              << smallIndexed->GetActualMemorySize() << " KiB instead of 2" << std::endl;
  }

  return res;
}
