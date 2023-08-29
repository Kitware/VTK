// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIndexedImplicitBackend.h"

#include "vtkDataArrayRange.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"

#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>

namespace
{

int LoopAndTest(vtkIdList* handles, vtkIndexedImplicitBackend<int>& backend)
{
  for (int idx = 0; idx < handles->GetNumberOfIds(); idx++)
  {
    if (backend(idx) != static_cast<int>(handles->GetId(idx)))
    {
      std::cout << "Indexed backend evaluation failed with: " << backend(idx)
                << " != " << handles->GetId(idx) << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int TestWithIDList()
{
  vtkNew<vtkIntArray> baseArray;
  baseArray->SetNumberOfComponents(1);
  baseArray->SetNumberOfTuples(100);
  auto range = vtk::DataArrayValueRange<1>(baseArray);
  std::iota(range.begin(), range.end(), 0);

  vtkNew<vtkIdList> handles;
  handles->SetNumberOfIds(100);
  {
    std::vector<vtkIdType> buffer(100);
    std::iota(buffer.begin(), buffer.end(), 0);
    std::random_device randdev;
    std::mt19937 generator(randdev());
    std::shuffle(buffer.begin(), buffer.end(), generator);
    for (vtkIdType idx = 0; idx < 100; idx++)
    {
      handles->SetId(idx, buffer[idx]);
    }
  }

  vtkIndexedImplicitBackend<int> backend(handles, baseArray);

  int res = LoopAndTest(handles, backend);

  vtkNew<vtkIntArray> baseMultiArray;
  baseMultiArray->SetNumberOfComponents(3);
  baseMultiArray->SetNumberOfTuples(100);
  auto multiRange = vtk::DataArrayValueRange<3>(baseMultiArray);
  std::iota(multiRange.begin(), multiRange.end(), 0);

  vtkNew<vtkIdList> multiHandles;
  multiHandles->SetNumberOfIds(300);
  {
    std::vector<vtkIdType> buffer(100);
    std::iota(buffer.begin(), buffer.end(), 0);
    std::random_device randdev;
    std::mt19937 generator(randdev());
    std::shuffle(buffer.begin(), buffer.end(), generator);
    for (vtkIdType idx = 0; idx < 100; idx++)
    {
      for (vtkIdType comp = 0; comp < 3; comp++)
      {
        multiHandles->SetId(3 * idx + comp, 3 * buffer[idx] + comp);
      }
    }
  }

  vtkIndexedImplicitBackend<int> multiBackend(multiHandles, baseMultiArray);
  return LoopAndTest(multiHandles, multiBackend) == EXIT_SUCCESS ? res : EXIT_FAILURE;
}

int LoopAndTest(vtkIntArray* handles, vtkIndexedImplicitBackend<int>& backend)
{
  for (int idx = 0; idx < handles->GetNumberOfTuples(); idx++)
  {
    if (backend(idx) != static_cast<int>(handles->GetValue(idx)))
    {
      std::cout << "Indexed backend evaluation failed with: " << backend(idx)
                << " != " << handles->GetValue(idx) << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int TestWithDataArrayIndexing()
{
  vtkNew<vtkIntArray> baseArray;
  baseArray->SetNumberOfComponents(1);
  baseArray->SetNumberOfTuples(100);
  auto range = vtk::DataArrayValueRange<1>(baseArray);
  std::iota(range.begin(), range.end(), 0);

  vtkNew<vtkIntArray> handles;
  handles->SetNumberOfComponents(1);
  handles->SetNumberOfTuples(100);
  {
    std::vector<int> buffer(100);
    std::iota(buffer.begin(), buffer.end(), 0);
    std::random_device randdev;
    std::mt19937 generator(randdev());
    std::shuffle(buffer.begin(), buffer.end(), generator);
    auto handleRange = vtk::DataArrayValueRange<1>(handles);
    std::copy(buffer.begin(), buffer.end(), handleRange.begin());
  }

  vtkIndexedImplicitBackend<int> backend(handles, baseArray);
  int res = LoopAndTest(handles, backend);

  // and with multi component entries

  vtkNew<vtkIntArray> baseMultiArray;
  baseMultiArray->SetNumberOfComponents(3);
  baseMultiArray->SetNumberOfTuples(100);
  auto multiRange = vtk::DataArrayValueRange<3>(baseMultiArray);
  std::iota(multiRange.begin(), multiRange.end(), 0);

  vtkNew<vtkIntArray> multiHandles;
  multiHandles->SetNumberOfComponents(1);
  multiHandles->SetNumberOfTuples(300);
  {
    std::vector<vtkIdType> buffer(100);
    std::iota(buffer.begin(), buffer.end(), 0);
    std::random_device randdev;
    std::mt19937 generator(randdev());
    std::shuffle(buffer.begin(), buffer.end(), generator);
    for (vtkIdType idx = 0; idx < 100; idx++)
    {
      for (vtkIdType comp = 0; comp < 3; comp++)
      {
        multiHandles->SetValue(3 * idx + comp, 3 * buffer[idx] + comp);
      }
    }
  }

  vtkIndexedImplicitBackend<int> multiBackend(multiHandles, baseMultiArray);
  return LoopAndTest(multiHandles, multiBackend) == EXIT_SUCCESS ? res : EXIT_FAILURE;
}
}

int TestIndexedImplicitBackend(int, char*[])
{
  int res = ::TestWithIDList();
  return ::TestWithDataArrayIndexing() == EXIT_SUCCESS ? res : EXIT_FAILURE;
}
