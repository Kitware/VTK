// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkToImplicitTypeErasureStrategy.h"

#include "vtkDataArrayRange.h"
#include "vtkIntArray.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <cstdlib>
#include <random>

int TestToImplicitTypeErasureStrategy(int, char*[])
{
  vtkNew<vtkIntArray> base;
  base->SetNumberOfComponents(3);
  base->SetNumberOfTuples(100);
  base->SetName("Base");
  auto range = vtk::DataArrayValueRange<3>(base);
  std::size_t quarterSize = base->GetNumberOfValues() / 4;
  std::fill(range.begin(), range.begin() + quarterSize, 0);
  std::fill(range.begin() + quarterSize, range.begin() + 2 * quarterSize, 1);
  std::fill(range.begin() + 2 * quarterSize, range.begin() + 3 * quarterSize, 2);
  std::fill(range.begin() + 3 * quarterSize, range.end(), 3);
  std::shuffle(range.begin(), range.end(), std::default_random_engine());

  vtkNew<vtkToImplicitTypeErasureStrategy> strat;
  auto opt = strat->EstimateReduction(base);
  if (!opt.IsSome)
  {
    std::cout << "Could not identify type erasure compressible array";
    return EXIT_FAILURE;
  }

  if (opt.Value != 0.25)
  {
    std::cout << "Did not identify correct reduction factor: 0.25 != " << opt.Value << std::endl;
    return EXIT_FAILURE;
  }

  vtkSmartPointer<vtkDataArray> result = strat->Reduce(base);

  if (!result)
  {
    std::cout << "Generated a nullptr result" << std::endl;
    return EXIT_FAILURE;
  }

  if (result->GetNumberOfComponents() != base->GetNumberOfComponents())
  {
    std::cout << "Result does not have same number of components as base" << std::endl;
    return EXIT_FAILURE;
  }

  if (result->GetNumberOfTuples() != base->GetNumberOfTuples())
  {
    std::cout << "Result does not have same number of tuples as base" << std::endl;
    return EXIT_FAILURE;
  }

  auto compressedRange = vtk::DataArrayValueRange<3>(result);
  auto cIt = compressedRange.begin();
  for (auto baseIt = range.begin(); baseIt != range.end(); ++baseIt, ++cIt)
  {
    if (*baseIt != *cIt)
    {
      std::cout << "Values in compressed array don't line up with base" << std::endl;
      return EXIT_FAILURE;
    }
  }

  vtkNew<vtkUnsignedCharArray> baseCopy;
  baseCopy->SetNumberOfComponents(base->GetNumberOfComponents());
  baseCopy->SetNumberOfTuples(base->GetNumberOfTuples());
  auto cpRange = vtk::DataArrayValueRange(baseCopy);
  std::copy(range.begin(), range.end(), cpRange.begin());

  opt = strat->EstimateReduction(baseCopy);

  if (opt.IsSome)
  {
    std::cout << "Should not be able to further compress unsigned char array using type erasure"
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
