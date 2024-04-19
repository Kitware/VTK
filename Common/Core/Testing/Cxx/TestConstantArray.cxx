// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConstantArray.h"

#include "vtkDataArrayRange.h"
#include "vtkIntArray.h"
#include "vtkVTK_DISPATCH_IMPLICIT_ARRAYS.h"

#ifdef VTK_DISPATCH_CONSTANT_ARRAYS
#include "vtkArrayDispatch.h"
#endif // VTK_DISPATCH_CONSTANT_ARRAYS

#include <cstdlib>
#include <memory>

#ifdef VTK_DISPATCH_CONSTANT_ARRAYS
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
#endif // VTK_DISPATCH_CONSTANT_ARRAYS

int TestConstantArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int res = EXIT_SUCCESS;

  vtkNew<vtkConstantArray<int>> identity;
  identity->SetBackend(std::make_shared<vtkConstantImplicitBackend<int>>(1));
  identity->SetNumberOfTuples(100);
  identity->SetNumberOfComponents(1);

  for (int iArr = 0; iArr < 100; iArr++)
  {
    if (identity->GetValue(iArr) != 1)
    {
      res = EXIT_FAILURE;
      std::cout << "get value failed with vtkConstantArray" << std::endl;
    }
  }

  for (auto val : vtk::DataArrayValueRange<1>(identity))
  {
    if (val != 1)
    {
      res = EXIT_FAILURE;
      std::cout << "range iterator failed with vtkConstantArray" << std::endl;
    }
  }

#ifdef VTK_DISPATCH_CONSTANT_ARRAYS
  std::cout << "vtkConstantArray: performing dispatch tests" << std::endl;
  vtkNew<vtkIntArray> destination;
  destination->SetNumberOfTuples(100);
  destination->SetNumberOfComponents(1);
  using Dispatcher =
    vtkArrayDispatch::Dispatch2ByArray<vtkArrayDispatch::ReadOnlyArrays, vtkArrayDispatch::Arrays>;
  ::ScaleWorker worker;
  if (!Dispatcher::Execute(identity, destination, worker, 3.0))
  {
    res = EXIT_FAILURE;
    std::cout << "vtkArrayDispatch failed with vtkConstantArray" << std::endl;
    worker(identity.Get(), destination.Get(), 3.0);
  }

  for (auto val : vtk::DataArrayValueRange<1>(destination))
  {
    if (val != 3)
    {
      res = EXIT_FAILURE;
      std::cout << "dispatch failed to populate the array with the correct values" << std::endl;
    }
  }
#endif // VTK_DISPATCH_CONSTANT_ARRAYS
  return res;
}
