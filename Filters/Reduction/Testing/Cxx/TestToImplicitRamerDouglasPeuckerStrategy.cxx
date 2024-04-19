// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkToImplicitRamerDouglasPeuckerStrategy.h"

#include "vtkCompositeArray.h"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"

#include <cstdlib>

namespace
{
constexpr double TEST_TOL = 1e-8;
}

int TestToImplicitRamerDouglasPeuckerStrategy(int, char*[])
{
  vtkNew<vtkFloatArray> base;
  base->SetName("ByParts");
  base->SetNumberOfComponents(4);
  base->SetNumberOfTuples(300);
  base->Fill(42.0);
  auto range = vtk::DataArrayValueRange<4>(base);
  std::iota(range.begin() + 10, range.begin() + 100, 0);
  std::transform(range.begin() + 10, range.begin() + 100, range.begin() + 10,
    [](float val) { return 2.5 * val; });
  std::fill(range.begin() + 100, range.begin() + 444, 0.42);
  std::iota(range.begin() + 444, range.end(), 0);
  std::transform(range.begin() + 444, range.end(), range.begin() + 444,
    [](float val) { return 3.14 * val + 67; });
  base->SetValue(342, 7.0);

  vtkNew<vtkToImplicitRamerDouglasPeuckerStrategy> strat;
  auto opt = strat->EstimateReduction(base);

  if (!opt.IsSome)
  {
    std::cout << "Could not identify constant / affine by part array" << std::endl;
    return EXIT_FAILURE;
  }

  double theoreticalReduction = 8.0 / 1200;
  if (std::abs(opt.Value - theoreticalReduction) > ::TEST_TOL)
  {
    std::cout << "Could not estimate reduced size correctly: " << theoreticalReduction
              << " != " << opt.Value << std::endl;
    return EXIT_FAILURE;
  }

  vtkSmartPointer<vtkDataArray> arr = strat->Reduce(base);

  if (!arr)
  {
    std::cout << "Could not get reduced array" << std::endl;
    return EXIT_FAILURE;
  }

  vtkCompositeArray<float>* composite = vtkArrayDownCast<vtkCompositeArray<float>>(arr);

  if (!composite)
  {
    std::cout << "Returned array is not composite" << std::endl;
    return EXIT_FAILURE;
  }

  if (composite->GetNumberOfComponents() != base->GetNumberOfComponents())
  {
    std::cout << "Number of components does not correspond to base array" << std::endl;
    return EXIT_FAILURE;
  }

  if (composite->GetNumberOfTuples() != base->GetNumberOfTuples())
  {
    std::cout << "Number of tuples does not correspond to base array" << std::endl;
    return EXIT_FAILURE;
  }

  for (vtkIdType iArr = 0; iArr < base->GetNumberOfValues(); iArr++)
  {
    if (std::abs(composite->GetValue(iArr) - base->GetValue(iArr)) > strat->GetTolerance())
    {
      std::cout << "GetValue consistency failed at position: " << iArr << "("
                << base->GetValue(iArr) << " != " << composite->GetValue(iArr) << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
