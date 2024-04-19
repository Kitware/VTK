// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkToConstantArrayStrategy.h"

#include "vtkCharArray.h"
#include "vtkConstantArray.h"

#include <cstdlib>

int TestToConstantArrayStrategy(int, char*[])
{
  vtkNew<vtkCharArray> baseArr;
  baseArr->SetName("Basic");
  baseArr->SetNumberOfComponents(1);
  baseArr->SetNumberOfTuples(100);
  baseArr->Fill(42);

  vtkNew<vtkToConstantArrayStrategy> strat;
  auto opt = strat->EstimateReduction(baseArr);
  if (opt.IsSome == false)
  {
    std::cout << "Could not successfully detect constant array." << std::endl;
    return EXIT_FAILURE;
  }

  if (opt.Value != 0.01)
  {
    std::cout << "Did not successfully identify reduction factor: " << opt.Value << " != 0.01"
              << std::endl;
    return EXIT_FAILURE;
  }

  vtkSmartPointer<vtkDataArray> compressed = strat->Reduce(baseArr);
  if (!compressed)
  {
    std::cout << "Did not successfully compress constant array" << std::endl;
    return EXIT_FAILURE;
  }

  vtkSmartPointer<vtkConstantArray<char>> typed =
    vtkArrayDownCast<vtkConstantArray<char>>(compressed);
  if (!typed)
  {
    std::cout << "Did not successfully identify type of constant array to use" << std::endl;
    return EXIT_FAILURE;
  }

  if (typed->GetNumberOfComponents() != baseArr->GetNumberOfComponents())
  {
    std::cout << "Did not set number of components correctly" << std::endl;
    return EXIT_FAILURE;
  }

  if (typed->GetNumberOfTuples() != baseArr->GetNumberOfTuples())
  {
    std::cout << "Did not set number of tuples correctly" << std::endl;
    return EXIT_FAILURE;
  }

  for (int iV = 0; iV < 100; ++iV)
  {
    if (typed->GetValue(iV) != baseArr->GetValue(iV))
    {
      std::cout << "Compressed array does not evaluate to base array" << std::endl;
      return EXIT_FAILURE;
    }
  }

  baseArr->SetValue(42, 43);

  auto badOpt = strat->EstimateReduction(baseArr);

  if (badOpt.IsSome)
  {
    std::cout << "False positive on non constant array" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
