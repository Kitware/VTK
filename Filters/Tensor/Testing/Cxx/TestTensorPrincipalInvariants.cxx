// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTensorPrincipalInvariants.h"

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

int TestTensorPrincipalInvariants(int argc, char* argv[])
{
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/MinimalTensors.vtu");
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkNew<vtkTensorPrincipalInvariants> invariantFilter;
  invariantFilter->SetInputConnection(reader->GetOutputPort());
  invariantFilter->Update();

  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(invariantFilter->GetOutput());

  // Check 3D tensor invariants on points
  if (!grid->GetPointData()->HasArray("3D Tensor - Sigma 1 (Vector)") ||
    !grid->GetPointData()->HasArray("3D Tensor - Sigma 2 (Vector)") ||
    !grid->GetPointData()->HasArray("3D Tensor - Sigma 3 (Vector)") ||
    !grid->GetPointData()->HasArray("3D Tensor - Sigma 1") ||
    !grid->GetPointData()->HasArray("3D Tensor - Sigma 2") ||
    !grid->GetPointData()->HasArray("3D Tensor - Sigma 3"))
  {
    std::cerr << "Missing one of the principal invariants arrays on points." << std::endl;
    return EXIT_FAILURE;
  }

  vtkDoubleArray* array =
    vtkDoubleArray::SafeDownCast(grid->GetPointData()->GetArray("3D Tensor - Sigma 1 (Vector)"));
  double* values = array->GetTuple3(0);

  if (!vtkMathUtilities::FuzzyCompare(values[0], 0.980516, 0.0001) ||
    !vtkMathUtilities::FuzzyCompare(values[1], 0.196437, 0.0001) ||
    !vtkMathUtilities::FuzzyCompare(values[2], 5.78099e-05, 0.0001))
  {
    std::cerr << "Wrong value(s). Expected (0.980516, 0.196437, 5.78099e-05) but got (" << values[0]
              << ", " << values[1] << ", " << values[2] << ")." << std::endl;
    return EXIT_FAILURE;
  }

  array = vtkDoubleArray::SafeDownCast(grid->GetPointData()->GetArray("3D Tensor - Sigma 2"));

  if (!vtkMathUtilities::FuzzyCompare(array->GetValue(5), 0.00133645, 0.0001))
  {
    std::cerr << "Wrong value. Expected 0.00133645 but got " << array->GetValue(5) << "."
              << std::endl;
    return EXIT_FAILURE;
  }

  // Check 2D tensor criteria on cells
  if (!grid->GetCellData()->HasArray("2D Tensor - Sigma 1 (Vector)") ||
    !grid->GetCellData()->HasArray("2D Tensor - Sigma 2 (Vector)") ||
    !grid->GetCellData()->HasArray("2D Tensor - Sigma 3 (Vector)") ||
    !grid->GetCellData()->HasArray("2D Tensor - Sigma 1") ||
    !grid->GetCellData()->HasArray("2D Tensor - Sigma 2") ||
    !grid->GetCellData()->HasArray("2D Tensor - Sigma 3"))
  {
    std::cerr << "Missing one of the principal invariants arrays on cells." << std::endl;
    return EXIT_FAILURE;
  }

  array = vtkDoubleArray::SafeDownCast(grid->GetCellData()->GetArray("2D Tensor - Sigma 2"));

  if (!vtkMathUtilities::FuzzyCompare(array->GetValue(2), 0.0, 0.0001))
  {
    std::cerr << "Wrong value. Expected 0.0 but got " << array->GetValue(2) << "." << std::endl;
    return EXIT_FAILURE;
  }

  if (!std::isnan(array->GetValue(0)))
  {
    std::cerr << "Wrong value. Expected NaN but got " << array->GetValue(0) << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Scale principal vectors by principal values
  invariantFilter->SetScaleVectors(true);
  invariantFilter->Update();
  grid = vtkUnstructuredGrid::SafeDownCast(invariantFilter->GetOutput());

  array =
    vtkDoubleArray::SafeDownCast(grid->GetCellData()->GetArray("2D Tensor - Sigma 3 (Vector)"));
  values = array->GetTuple3(2);

  if (!vtkMathUtilities::FuzzyCompare(values[0], -25.5966, 0.0001) ||
    !vtkMathUtilities::FuzzyCompare(values[1], -5.55154e-05, 0.0001) ||
    !vtkMathUtilities::FuzzyCompare(values[2], 0.0, 0.0001))
  {
    std::cerr << "Wrong value(s). Expected (-25.5966, -5.55154e-05, 0.0) but got (" << values[0]
              << ", " << values[1] << ", " << values[2] << ")." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
