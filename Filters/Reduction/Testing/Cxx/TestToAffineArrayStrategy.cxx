// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkToAffineArrayStrategy.h"

#include "vtkAffineArray.h"
#include "vtkDataArrayRange.h"
#include "vtkUnsignedLongArray.h"

#include <cstdlib>

int TestToAffineArrayStrategy(int, char*[])
{
  vtkNew<vtkUnsignedLongArray> base;
  base->SetNumberOfComponents(5);
  base->SetNumberOfTuples(100);
  auto range = vtk::DataArrayValueRange<5>(base);
  std::iota(range.begin(), range.end(), 3);
  std::transform(
    range.begin(), range.end(), range.begin(), [](unsigned long val) { return 42 * val; });

  vtkNew<vtkToAffineArrayStrategy> strat;
  auto opt = strat->EstimateReduction(base);

  if (!opt.IsSome)
  {
    std::cout << "Could not successfully identify affine array" << std::endl;
    return EXIT_FAILURE;
  }

  if (opt.Value != 2.0 / 500)
  {
    std::cout << "Did not evaluate reduction factor correctly" << std::endl;
    return EXIT_FAILURE;
  }

  vtkSmartPointer<vtkDataArray> result = strat->Reduce(base);

  if (!result)
  {
    std::cout << "Result of reduction is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  vtkSmartPointer<vtkAffineArray<unsigned long>> affine =
    vtkArrayDownCast<vtkAffineArray<unsigned long>>(result);

  if (!affine)
  {
    std::cout << "Could not cast result to affine array" << std::endl;
    return EXIT_FAILURE;
  }

  if (affine->GetNumberOfComponents() != base->GetNumberOfComponents())
  {
    std::cout << "Number of components not agreeing with base array" << std::endl;
    return EXIT_FAILURE;
  }

  if (affine->GetNumberOfTuples() != base->GetNumberOfTuples())
  {
    std::cout << "Number of tuples not agreeing with base array" << std::endl;
    return EXIT_FAILURE;
  }

  auto affRange = vtk::DataArrayValueRange<5>(affine);
  auto itAff = affRange.begin();
  for (auto itBase = range.begin(); itBase != range.end(); ++itBase, ++itAff)
  {
    if (*itBase != *itAff)
    {
      std::cout << "Base and affine values don't match up" << std::endl;
      return EXIT_FAILURE;
    }
  }

  base->SetValue(42, 0);

  auto badOpt = strat->EstimateReduction(base);
  if (badOpt.IsSome)
  {
    std::cout << "Identified non affine array as affine" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
