// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkContourFilter.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGradient.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"

int TestHyperTreeGridBinary2DGradient(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(2);
  htGrid->SetDimensions(3, 3, 1);
  htGrid->SetBranchFactor(2);
  htGrid->SetDescriptor("R...|....");
  htGrid->Update();
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(htGrid->GetOutput());
  htg->GetCellData()->SetScalars(htg->GetCellData()->GetArray("Depth"));

  vtkNew<vtkHyperTreeGridGradient> gradient;
  gradient->SetInputConnection(htGrid->GetOutputPort());
  gradient->SetMode(vtkHyperTreeGridGradient::UNLIMITED);
  gradient->SetInputArrayToProcess(0, 0, 0, vtkDataSet::CELL, "Depth");
  gradient->ComputeGradientOn();
  gradient->ComputeDivergenceOff();
  gradient->ComputeVorticityOff();
  gradient->ComputeQCriterionOff();

  gradient->Update();

  vtkDoubleArray* gradientArray =
    vtkDoubleArray::SafeDownCast(vtkHyperTreeGrid::SafeDownCast(gradient->GetOutput())
                                   ->GetCellData()
                                   ->GetAbstractArray("Gradient"));

  if (gradientArray->GetNumberOfTuples() != 8)
  {
    vtkLogF(ERROR, "Expected %d tuples but got %lld.", 8, gradientArray->GetNumberOfTuples());
    return EXIT_FAILURE;
  }

  std::unordered_map<vtkIdType, std::array<double, 3>> expectedValues;
  expectedValues.emplace(0, std::array<double, 3>{ 0, 0, 0 });
  expectedValues.emplace(3, std::array<double, 3>{ -1, -1, 0 });
  expectedValues.emplace(7, std::array<double, 3>{ -3, -3, 0 });

  for (const auto& [id, coords] : expectedValues)
  {
    for (int i = 0; i < 3; ++i)
    {
      if (!vtkMathUtilities::FuzzyCompare<double>(gradientArray->GetTuple3(id)[i], coords[i]))
      {
        vtkLogF(ERROR, "Expected tuple %lld to be %f but got %f.", id, coords[i],
          gradientArray->GetTuple1(0));
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
