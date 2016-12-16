/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestComputeQuartiles.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkComputeQuartiles.h"
#include "vtkDoubleArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkStatisticsAlgorithm.h"
#include "vtkTable.h"

#include "vtkTestErrorObserver.h"
#include "vtkExecutive.h"

//----------------------------------------------------------------------------
int TestComputeQuartiles(int , char * [])
{
  vtkNew<vtkDoubleArray> arrFirstVariable;
  arrFirstVariable->SetName("Math");

  vtkNew<vtkDoubleArray> arrSecondVariable;
  arrSecondVariable->SetName("French");

  // Create a two columns table
  vtkNew<vtkTable> table;
  table->AddColumn(arrFirstVariable.GetPointer());
  table->AddColumn(arrSecondVariable.GetPointer());

  const int numNotes = 20;
  table->SetNumberOfRows(numNotes);

  const double MathValue[] =
    {
    18, 20, 20, 16,
    12, 14, 16, 14,
    14, 13, 16, 18,
    6, 10, 16, 14,
    4, 16, 16, 14
    };

  const double FrenchValue[] =
    {
    14, 12, 14, 16,
    12, 14, 16, 4,
    4, 10, 6, 20,
    14, 16, 14, 14,
    12, 2, 14, 8
    };

  for (int i = 0; i < numNotes; ++i)
  {
    table->SetValue(i, 0, MathValue[i]);
    table->SetValue(i, 1, FrenchValue[i]);
  }

  // Run Compute Quantiles
  vtkNew<vtkComputeQuartiles> quartiles;

  vtkNew<vtkTest::ErrorObserver> errorObserver1;
  // First verify that absence of input does not cause trouble
  quartiles->GetExecutive()->AddObserver(vtkCommand::ErrorEvent,errorObserver1.GetPointer());
  quartiles->Update();
  errorObserver1->CheckErrorMessage("Input port 0 of algorithm vtkComputeQuartiles");

  // Now set the real input table
  quartiles->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, table.GetPointer());
  quartiles->Update();

  vtkTable *outTable = quartiles->GetOutput();

  const double MathQuartiles[] =
    {
    4, 13.5, 15, 16, 20
    };
  const double FrenchQuartiles[] =
    {
    2, 9, 14, 14, 20
    };

  bool ret = EXIT_SUCCESS;

  for (int i = 0; i < 5; i++)
  {
    if (outTable->GetValue(i, 0).ToFloat() != MathQuartiles[i] ||
      outTable->GetValue(i, 1).ToFloat() != FrenchQuartiles[i])
    {
      ret = EXIT_FAILURE;
    }
  }

  if (ret != EXIT_SUCCESS)
  {
    cout << "Failure!" << endl;
    outTable->Dump();
  }

  return ret;
}
