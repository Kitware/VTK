// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDoubleArray.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkOTKernelSmoothing.h"
#include "vtkTable.h"

#include "vtkExecutive.h"
#include "vtkTestErrorObserver.h"

#include <iostream>
//------------------------------------------------------------------------------
int TestOTKernelSmoothing(int, char*[])
{
  vtkNew<vtkDoubleArray> arrFirstVariable;
  arrFirstVariable->SetName("Math");

  vtkNew<vtkTable> table;
  table->AddColumn(arrFirstVariable);

  const int numNotes = 20;
  table->SetNumberOfRows(numNotes);

  const double MathValue[] = { 18, 20, 20, 16, 12, 14, 16, 14, 14, 13, 16, 18, 6, 10, 16, 14, 4, 16,
    16, 14 };

  for (int i = 0; i < numNotes; ++i)
  {
    table->SetValue(i, 0, MathValue[i]);
  }

  // Run Compute Quantiles
  vtkNew<vtkOTKernelSmoothing> kernel;

  vtkNew<vtkTest::ErrorObserver> errorObserver1;
  // First verify that absence of input does not cause trouble
  kernel->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
  kernel->Update();
  errorObserver1->CheckErrorMessage("Input port 0 of algorithm vtkOTKernelSmoothing");

  // Now set the real input table
  kernel->SetInputData(table);
  kernel->SetPointNumber(100);
  kernel->SetBoundaryCorrection(true);
  kernel->SetTriangularPDF(true);
  kernel->SetEpanechnikovPDF(true);
  kernel->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "Math");
  kernel->Update();

  vtkTable* outputTable = kernel->GetOutput();

  if (outputTable->GetNumberOfColumns() != 3 || outputTable->GetNumberOfRows() != 100)
  {
    std::cout << "OutputTable has an unexpected format" << std::endl;
    return EXIT_FAILURE;
  }

  double tablesValues[] = { 0.064423254877735508517, 0.065363016055031350482,
    0.062847537553276378031 };
  for (int i = 0; i < 3; i++)
  {
    vtkVariant tableValue = outputTable->GetValue(50, i);
    if (!vtkMathUtilities::FuzzyCompare(tableValue.ToDouble(), tablesValues[i]))
    {
      std::cout << std::setprecision(20) << "Unexpected Table Value: " << tableValue.ToDouble()
                << " Expecting: " << tablesValues[i] << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
