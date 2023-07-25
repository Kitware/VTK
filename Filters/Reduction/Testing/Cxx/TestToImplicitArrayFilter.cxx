// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkToImplicitArrayFilter.h"

#include "vtkConstantArray.h"
#include "vtkDataArraySelection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkToConstantArrayStrategy.h"

#include <cstdlib>

namespace
{
int TestCompressible()
{
  vtkNew<vtkSphereSource> sphere;
  sphere->Update();

  vtkPolyData* output = vtkPolyData::SafeDownCast(sphere->GetOutput());

  auto pointData = output->GetPointData();

  vtkNew<vtkIntArray> constArr;
  constArr->SetNumberOfComponents(2);
  constArr->SetNumberOfTuples(output->GetNumberOfPoints());
  constArr->Fill(42);
  constArr->SetName("Constant");
  pointData->AddArray(constArr);

  vtkNew<vtkToImplicitArrayFilter> toImpArr;
  vtkNew<vtkToConstantArrayStrategy> strat;
  toImpArr->SetStrategy(strat);
  toImpArr->SetInputConnection(sphere->GetOutputPort());
  auto select = toImpArr->GetPointDataArraySelection();
  select->EnableArray("Constant");
  toImpArr->Update();

  output = vtkPolyData::SafeDownCast(toImpArr->GetOutput());
  if (!output)
  {
    std::cout << "Output of filter is not poly data" << std::endl;
    return EXIT_FAILURE;
  }

  pointData = output->GetPointData();

  vtkConstantArray<int>* arr =
    vtkArrayDownCast<vtkConstantArray<int>>(pointData->GetArray("Constant"));
  if (!arr)
  {
    std::cout << "Does not have constant array in output" << std::endl;
    return EXIT_FAILURE;
  }

  if (arr->GetNumberOfComponents() != constArr->GetNumberOfComponents())
  {
    std::cout << "Resulting compressed array does not have correct number of components"
              << std::endl;
    return EXIT_FAILURE;
  }

  if (arr->GetNumberOfTuples() != constArr->GetNumberOfTuples())
  {
    std::cout << "Resulting compressed array does not have correct number of tuples" << std::endl;
    return EXIT_FAILURE;
  }

  for (int iV = 0; iV < output->GetNumberOfPoints() * 2; ++iV)
  {
    if (constArr->GetValue(iV) != arr->GetValue(iV))
    {
      std::cout << "Failed to attribute correct value to constant array." << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int TestNotCompressible()
{
  vtkNew<vtkSphereSource> sphere;
  sphere->Update();

  vtkPolyData* output = vtkPolyData::SafeDownCast(sphere->GetOutput());

  auto pointData = output->GetPointData();

  vtkNew<vtkIntArray> constArr;
  constArr->SetNumberOfComponents(2);
  constArr->SetNumberOfTuples(output->GetNumberOfPoints());
  constArr->Fill(42);
  constArr->SetValue(1, 43);
  constArr->SetName("NotConstant");
  pointData->AddArray(constArr);

  vtkNew<vtkToImplicitArrayFilter> toImpArr;
  vtkNew<vtkToConstantArrayStrategy> strat;
  toImpArr->SetStrategy(strat);
  toImpArr->SetInputConnection(sphere->GetOutputPort());
  auto select = toImpArr->GetPointDataArraySelection();
  select->EnableArray("NotConstant");
  toImpArr->Update();

  output = vtkPolyData::SafeDownCast(toImpArr->GetOutput());
  if (!output)
  {
    std::cout << "Output of filter is not poly data" << std::endl;
    return EXIT_FAILURE;
  }

  pointData = output->GetPointData();

  vtkIntArray* arr = vtkArrayDownCast<vtkIntArray>(pointData->GetArray("NotConstant"));
  if (!arr)
  {
    std::cout << "Does not have not constant array in output" << std::endl;
    return EXIT_FAILURE;
  }

  if (arr != constArr)
  {
    std::cout << "Original array was not shallow copied into input" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
}

int TestToImplicitArrayFilter(int, char*[])
{
  return (::TestCompressible() == EXIT_SUCCESS ? ::TestNotCompressible() : EXIT_FAILURE);
}
