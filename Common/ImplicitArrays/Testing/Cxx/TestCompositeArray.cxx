// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeArray.h"

#include "vtkAffineArray.h"
#include "vtkDataArrayRange.h"
#include "vtkIntArray.h"
#include "vtkVTK_DISPATCH_IMPLICIT_ARRAYS.h"

#ifdef VTK_DISPATCH_COMPOSITE_ARRAYS
#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchImplicitArrayList.h"
#endif // VTK_DISPATCH_AFFINE_ARRAYS

#include <cstdlib>
#include <memory>

namespace
{
#ifdef VTK_DISPATCH_COMPOSITE_ARRAYS
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
#endif // VTK_DISPATCH_COMPOSITE_ARRAYS

vtkSmartPointer<vtkCompositeArray<int>> SetupCompositeArray(int length)
{
  std::vector<vtkSmartPointer<vtkAffineArray<int>>> affArrays(length / 20);
  std::vector<vtkSmartPointer<vtkIntArray>> intArrays(length / 20);
  for (int i = 0; i < length / 20; ++i)
  {
    vtkNew<vtkAffineArray<int>> affine;
    affine->SetBackend(std::make_shared<vtkAffineImplicitBackend<int>>(1, i * 20));
    affine->SetNumberOfTuples(10);
    affine->SetNumberOfComponents(1);
    affArrays[i] = affine;
  }
  for (int i = 0; i < length / 20; ++i)
  {
    vtkNew<vtkIntArray> iota;
    iota->SetNumberOfTuples(10);
    iota->SetNumberOfComponents(1);
    auto range = vtk::DataArrayValueRange<1>(iota);
    std::iota(range.begin(), range.end(), 10 * (2 * i + 1));
    intArrays[i] = iota;
  }

  std::vector<vtkDataArray*> interleaf;
  for (int i = 0; i < length / 20; ++i)
  {
    interleaf.emplace_back(affArrays[i]);
    interleaf.emplace_back(intArrays[i]);
  }

  return vtk::ConcatenateDataArrays<int>(interleaf);
}

}

int TestCompositeArray(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int res = EXIT_SUCCESS;

  vtkSmartPointer<vtkCompositeArray<int>> composite = ::SetupCompositeArray(100);

  for (int iArr = 0; iArr < 100; iArr++)
  {
    if (composite->GetValue(iArr) != iArr)
    {
      res = EXIT_FAILURE;
      std::cout << "get value failed with vtkCompositeArray: " << iArr
                << " != " << composite->GetValue(iArr) << std::endl;
    }
  }

  int iArr = 0;
  for (auto val : vtk::DataArrayValueRange<1>(composite))
  {
    if (val != iArr)
    {
      res = EXIT_FAILURE;
      std::cout << "range iterator failed with vtkCompositerray" << std::endl;
    }
    iArr++;
  }

#ifdef VTK_DISPATCH_COMPOSITE_ARRAYS
  std::cout << "vtkCompositeArray: performing dispatch tests" << std::endl;
  vtkNew<vtkIntArray> destination;
  destination->SetNumberOfTuples(100);
  destination->SetNumberOfComponents(1);
  using Dispatcher =
    vtkArrayDispatch::Dispatch2ByArray<vtkArrayDispatch::ReadOnlyArrays, vtkArrayDispatch::Arrays>;
  ::ScaleWorker worker;
  if (!Dispatcher::Execute(composite, destination, worker, 3.0))
  {
    res = EXIT_FAILURE;
    std::cout << "vtkArrayDispatch failed with vtkCompositeArray" << std::endl;
    worker(composite.Get(), destination.Get(), 3.0);
  }

  iArr = 0;
  for (auto val : vtk::DataArrayValueRange<1>(destination))
  {
    if (val != 3 * iArr)
    {
      res = EXIT_FAILURE;
      std::cout << "dispatch failed to populate the array with the correct values" << std::endl;
    }
    iArr++;
  }
#endif // VTK_DISPATCH_COMPOSITE_ARRAYS

  // test a 1 composite
  vtkSmartPointer<vtkCompositeArray<int>> oneComposite =
    vtk::ConcatenateDataArrays<int>(std::vector<vtkDataArray*>({ composite }));
  for (iArr = 0; iArr < 100; iArr++)
  {
    if (oneComposite->GetValue(iArr) != iArr)
    {
      res = EXIT_FAILURE;
      std::cout << "get value failed with vtkCompositeArray for composite with one array: " << iArr
                << " != " << composite->GetValue(iArr) << std::endl;
    }
  }

  return res;
};
