// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIndexedArray.h"

#include "vtkDataArrayRange.h"
#include "vtkIdList.h"
#include "vtkIntArray.h"
#include "vtkVTK_DISPATCH_IMPLICIT_ARRAYS.h"

#ifdef VTK_DISPATCH_INDEXED_ARRAYS
#include "vtkArrayDispatch.h"
#endif // VTK_DISPATCH_INDEXED_ARRAYS

#include <cstdlib>
#include <numeric>
#include <random>

#ifdef VTK_DISPATCH_INDEXED_ARRAYS
namespace
{
struct ScaleWorker
{
  template <typename SrcArray, typename DstArray>
  void operator()(SrcArray* srcArr, DstArray* dstArr, double scale)
  {
    using SrcType = vtk::GetAPIType<SrcArray>;
    using DstType = vtk::GetAPIType<DstArray>;

    const auto srcRange = vtk::DataArrayValueRange(srcArr);
    auto dstRange = vtk::DataArrayValueRange(dstArr);

    if (srcRange.size() != dstRange.size())
    {
      std::cout << "Different array sizes in ScaleWorker" << std::endl;
      return;
    }

    auto dstIter = dstRange.begin();
    for (SrcType srcVal : srcRange)
    {
      *dstIter++ = static_cast<DstType>(srcVal * scale);
    }
  }
};
}
#endif // VTK_DISPATCH_INDEXED_ARRAYS

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

#ifdef VTK_DISPATCH_INDEXED_ARRAYS
  std::cout << "vtkIndexedArray: performing dispatch tests" << std::endl;
  vtkNew<vtkIntArray> destination;
  destination->SetNumberOfTuples(100);
  destination->SetNumberOfComponents(1);
  using Dispatcher =
    vtkArrayDispatch::Dispatch2ByArray<vtkArrayDispatch::ReadOnlyArrays, vtkArrayDispatch::Arrays>;
  ::ScaleWorker worker;
  if (!Dispatcher::Execute(indexed, destination, worker, 3.0))
  {
    res = EXIT_FAILURE;
    std::cout << "vtkArrayDispatch failed with vtkIndexedArray" << std::endl;
    worker(indexed.Get(), destination.Get(), 3.0);
  }

  iArr = 0;
  for (auto val : vtk::DataArrayValueRange<1>(destination))
  {
    if (val != 3 * static_cast<int>(handles->GetId(iArr)))
    {
      res = EXIT_FAILURE;
      std::cout << "dispatch failed to populate the array with the correct values" << std::endl;
    }
    iArr++;
  }
#endif // VTK_DISPATCH_INDEXED_ARRAYS
  return res;
};
