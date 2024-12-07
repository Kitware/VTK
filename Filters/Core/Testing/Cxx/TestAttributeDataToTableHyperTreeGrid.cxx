// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAttributeDataToTableFilter.h"

#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"
#include "vtkIntArray.h"
#include "vtkTable.h"

#include <cstdlib>
#include <numeric>

int TestAttributeDataToTableHyperTreeGrid(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkHyperTreeGridPreConfiguredSource> source;
  source->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::UNBALANCED_3DEPTH_2BRANCH_2X3);
  source->Update();

  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(source->GetOutput());
  if (!htg)
  {
    std::cout << "Problem generating HTG" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkDoubleArray> scalars;
  scalars->SetName("ScalarField");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(htg->GetNumberOfCells());
  auto scalRange = vtk::DataArrayValueRange<1>(scalars);
  std::iota(scalRange.begin(), scalRange.end(), 0);

  vtkNew<vtkDoubleArray> vectors;
  vectors->SetName("VectorField");
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples(htg->GetNumberOfCells());
  auto vecRange = vtk::DataArrayValueRange<3>(vectors);
  std::iota(vecRange.begin(), vecRange.end(), 0);

  htg->GetCellData()->AddArray(scalars);
  htg->GetCellData()->AddArray(vectors);

  vtkNew<vtkAttributeDataToTableFilter> toTable;
  toTable->SetInputData(htg);
  toTable->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
  toTable->Update();

  vtkTable* table = vtkTable::SafeDownCast(toTable->GetOutput());

  if (!table)
  {
    std::cout << "vtkAttributeDataToTableFilter did not return a table" << std::endl;
    return EXIT_FAILURE;
  }

  auto checkIota = [](vtkDataArray* arr)
  {
    if (!arr)
    {
      return false;
    }
    auto range = vtk::DataArrayValueRange(arr);
    for (vtkIdType iV = 0; iV < range.size(); ++iV)
    {
      if (range[iV] != static_cast<double>(iV))
      {
        return false;
      }
    }
    return true;
  };

  if (!checkIota(table->GetRowData()->GetArray("ScalarField")))
  {
    std::cout << "Problem checking iota of ScalarField" << std::endl;
    return EXIT_FAILURE;
  }
  if (!checkIota(table->GetRowData()->GetArray("VectorField")))
  {
    std::cout << "Problem checking iota of VectorField" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
