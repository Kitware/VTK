// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkYieldCriteria.h"

#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <iostream>

int TestYieldCriteria(int argc, char* argv[])
{
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/MinimalTensors.vtu");
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkNew<vtkYieldCriteria> yieldFilter;
  yieldFilter->SetInputConnection(reader->GetOutputPort());
  yieldFilter->Update();

  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(yieldFilter->GetOutput());

  // Check 3D tensor criteria on points
  if (!grid->GetPointData()->HasArray("3D Tensor - Tresca Criterion") ||
    !grid->GetPointData()->HasArray("3D Tensor - Von Mises Criterion"))
  {
    std::cerr << "Missing one of the yield criteria arrays on points." << std::endl;
    return EXIT_FAILURE;
  }

  vtkDoubleArray* array =
    vtkDoubleArray::SafeDownCast(grid->GetPointData()->GetArray("3D Tensor - Tresca Criterion"));

  if (!vtkMathUtilities::FuzzyCompare(array->GetValue(8), 25.6299, 0.0001))
  {
    std::cerr << "Wrong value. Expected 25.6299 but got " << array->GetValue(8) << "." << std::endl;
    return EXIT_FAILURE;
  }

  array =
    vtkDoubleArray::SafeDownCast(grid->GetPointData()->GetArray("3D Tensor - Von Mises Criterion"));

  if (!vtkMathUtilities::FuzzyCompare(array->GetValue(11), 25.6128, 0.0001))
  {
    std::cerr << "Wrong value. Expected 25.6128 but got " << array->GetValue(11) << "."
              << std::endl;
    return EXIT_FAILURE;
  }

  // Check 2D tensor criteria on cells
  if (!grid->GetCellData()->HasArray("2D Tensor - Tresca Criterion") ||
    !grid->GetCellData()->HasArray("2D Tensor - Von Mises Criterion"))
  {
    std::cerr << "Missing one of the yield criteria arrays on cells." << std::endl;
    return EXIT_FAILURE;
  }

  array =
    vtkDoubleArray::SafeDownCast(grid->GetCellData()->GetArray("2D Tensor - Von Mises Criterion"));

  if (!vtkMathUtilities::FuzzyCompare(array->GetValue(2), 25.6036, 0.0001))
  {
    std::cerr << "Wrong value. Expected 25.6036 but got " << array->GetValue(2) << "." << std::endl;
    return EXIT_FAILURE;
  }

  if (!std::isnan(array->GetValue(0)))
  {
    std::cerr << "Wrong value. Expected NaN but got " << array->GetValue(0) << "." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
