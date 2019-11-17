/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOTDensityMap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkOTDensityMap.h"
#include "vtkTable.h"
#include "vtkTestErrorObserver.h"

#include <iostream>

//----------------------------------------------------------------------------
int TestOTDensityMap(int, char*[])
{
  vtkNew<vtkDoubleArray> arrFirstVariable;
  arrFirstVariable->SetName("Math");

  vtkNew<vtkDoubleArray> arrSecondVariable;
  arrSecondVariable->SetName("French");

  // Create a two columns table
  vtkNew<vtkTable> table;
  table->AddColumn(arrFirstVariable);
  table->AddColumn(arrSecondVariable);

  const int numNotes = 20;
  table->SetNumberOfRows(numNotes);

  const double MathValue[] = { 18, 20, 20, 16, 12, 14, 16, 14, 14, 13, 16, 18, 6, 10, 16, 14, 4, 16,
    16, 14 };

  const double FrenchValue[] = { 14, 12, 14, 16, 12, 14, 16, 4, 4, 10, 6, 20, 14, 16, 14, 14, 12, 2,
    14, 8 };

  for (int i = 0; i < numNotes; ++i)
  {
    table->SetValue(i, 0, MathValue[i]);
    table->SetValue(i, 1, FrenchValue[i]);
  }

  // Run Compute Quantiles
  vtkNew<vtkOTDensityMap> density;

  vtkNew<vtkTest::ErrorObserver> errorObserver1;
  // First verify that absence of input does not cause trouble
  density->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
  density->Update();
  errorObserver1->CheckErrorMessage("Input port 0 of algorithm vtkOTDensityMap");

  // Now set the real input table
  density->SetInputData(table);
  density->SetNumberOfContours(3);
  density->SetValue(0, 0.1);
  density->SetValue(1, 0.5);
  density->SetValue(2, 0.9);
  density->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "Math");
  density->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "French");
  density->Update();

  vtkMultiBlockDataSet* mbTable = density->GetOutput();

  if (mbTable->GetNumberOfBlocks() != 3)
  {
    cout << "Unexpected Number of Contour Blocks" << std::endl;
    return EXIT_FAILURE;
  }

  unsigned int childsNBlock[] = { 1, 2, 4 };

  int tablesNRows[] = { 44, 89, 52, 94, 36, 48, 10 };

  double tablesYValues[] = { 12.80000019073486328125, 14.934099197387695312, 11.056828498840332031,
    19.480913162231445312, 8.7593898773193359375, 15.193044662475585938, 19.2800006866455078125 };

  int nTable = 0;
  for (unsigned int i = 0; i < mbTable->GetNumberOfBlocks(); i++)
  {
    vtkMultiBlockDataSet* childBlock = vtkMultiBlockDataSet::SafeDownCast(mbTable->GetBlock(i));
    if (!childBlock || childBlock->GetNumberOfBlocks() != childsNBlock[i])
    {
      cout << "Unexpected Child Block format" << std::endl;
      return EXIT_FAILURE;
    }
    for (unsigned int j = 0; j < childsNBlock[i]; j++)
    {
      vtkTable* childTable = vtkTable::SafeDownCast(childBlock->GetBlock(j));
      if (!childTable || childTable->GetNumberOfColumns() != 2)
      {
        cout << "Unexpected Table format" << std::endl;
        return EXIT_FAILURE;
      }
      if (childTable->GetNumberOfRows() != tablesNRows[nTable])
      {
        cout << "Unexpected Number of rows : " << childTable->GetNumberOfRows()
             << " Expecting : " << tablesNRows[nTable] << std::endl;
        return EXIT_FAILURE;
      }
      vtkVariant tableYValue = childTable->GetValue(0, 1);
      if (!vtkMathUtilities::FuzzyCompare(tableYValue.ToDouble(), tablesYValues[nTable]))
      {
        std::cout << std::setprecision(20) << "Unexpected Table Value:" << tableYValue.ToDouble()
                  << " Expecting : " << tablesYValues[nTable] << std::endl;
      }
      nTable++;
    }
  }
  return EXIT_SUCCESS;
}
