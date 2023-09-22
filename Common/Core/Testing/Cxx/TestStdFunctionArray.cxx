// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStdFunctionArray.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkIntArray.h"
#include "vtkVTK_DISPATCH_IMPLICIT_ARRAYS.h"

#ifdef VTK_DISPATCH_STD_FUNCTION_ARRAYS
#include "vtkArrayDispatch.h"
#endif // VTK_DISPATCH_STD_FUNCTION_ARRAYS

#include <cstdlib>
#include <memory>

#ifdef VTK_DISPATCH_STD_FUNCTION_ARRAYS
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
#endif // VTK_DISPATCH_STD_FUNCTION_ARRAYS

int TestStdFunctionArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int res = EXIT_SUCCESS;

  vtkNew<vtkStdFunctionArray<int>> identity;
  auto identity_func = [](int idx) { return idx; };
  identity->SetBackend(std::make_shared<std::function<int(int)>>(identity_func));
  identity->SetNumberOfTuples(100);
  identity->SetNumberOfComponents(1);

  for (int iArr = 0; iArr < 100; iArr++)
  {
    if (identity->GetValue(iArr) != iArr)
    {
      res = EXIT_FAILURE;
      std::cout << "get value failed with vtkStdFunctionArray" << std::endl;
    }
  }

  int iArr = 0;
  for (auto val : vtk::DataArrayValueRange<1>(identity))
  {
    if (val != iArr)
    {
      res = EXIT_FAILURE;
      std::cout << "range iterator failed with vtkStdFunctionArray" << std::endl;
    }
    iArr++;
  }

#ifdef VTK_DISPATCH_STD_FUNCTION_ARRAYS
  std::cout << "vtkStdFunctionArray: performing dispatch tests" << std::endl;
  vtkNew<vtkIntArray> destination;
  destination->SetNumberOfTuples(100);
  destination->SetNumberOfComponents(1);
  using Dispatcher =
    vtkArrayDispatch::Dispatch2ByArray<vtkArrayDispatch::ReadOnlyArrays, vtkArrayDispatch::Arrays>;
  ::ScaleWorker worker;
  if (!Dispatcher::Execute(identity, destination, worker, 3.0))
  {
    res = EXIT_FAILURE;
    std::cout << "vtkArrayDispatch failed with vtkStdFunctionArray" << std::endl;
  }
  iArr = 0;
  for (auto val : vtk::DataArrayValueRange<1>(destination))
  {
    if (val != 3 * iArr)
    {
      res = EXIT_FAILURE;
      std::cout << "dispatch failed to populate the array with the correct values" << std::endl;
      worker(identity.Get(), destination.Get(), 3.0);
    }
    iArr++;
  }
#endif // VTK_DISPATCH_STD_FUNCTION_ARRAYS
  return res;
}
