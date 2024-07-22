// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeArray.h"

#include "vtkAffineArray.h"
#include "vtkDataArrayRange.h"
#include "vtkIntArray.h"

#include <cstdlib>
#include <memory>

namespace
{

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
      std::cerr << "get value failed with vtkCompositeArray: " << iArr
                << " != " << composite->GetValue(iArr) << std::endl;
    }
  }

  int iArr = 0;
  for (auto val : vtk::DataArrayValueRange<1>(composite))
  {
    if (val != iArr)
    {
      res = EXIT_FAILURE;
      std::cerr << "range iterator failed with vtkCompositerray" << std::endl;
    }
    iArr++;
  }

  // test a 1 composite
  vtkSmartPointer<vtkCompositeArray<int>> oneComposite =
    vtk::ConcatenateDataArrays<int>(std::vector<vtkDataArray*>({ composite }));
  for (iArr = 0; iArr < 100; iArr++)
  {
    if (oneComposite->GetValue(iArr) != iArr)
    {
      res = EXIT_FAILURE;
      std::cerr << "get value failed with vtkCompositeArray for composite with one array: " << iArr
                << " != " << composite->GetValue(iArr) << std::endl;
    }
  }

  // test memory size measurement
  vtkSmartPointer<vtkCompositeArray<int>> largeComposite = ::SetupCompositeArray(2000 * 20);
  if (largeComposite->GetActualMemorySize() != 2000 * 2)
  {
    res = EXIT_FAILURE;
    std::cerr << "Wrong value memory size value for large vtkCompositeArray: "
              << largeComposite->GetActualMemorySize() << " KiB instead of " << 20 * sizeof(int)
              << std::endl;
  }

  return res;
}
