/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHighestDensityRegionsStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkHighestDensityRegionsStatistics.h"

#include <sstream>

//----------------------------------------------------------------------------
int TestHighestDensityRegionsStatistics(int , char * [])
{
  vtkNew<vtkTable> table;

  vtkNew<vtkDoubleArray> arrFirstVariable;
  const char *namev1 = "Math";
  arrFirstVariable->SetName(namev1);
  table->AddColumn(arrFirstVariable.GetPointer());

  vtkNew<vtkDoubleArray> arrSecondVariable;
  const char *namev2 = "French";
  arrSecondVariable->SetName(namev2);
  table->AddColumn(arrSecondVariable.GetPointer());

  vtkNew<vtkDoubleArray> arrThirdVariable;
  const char *namev3 = "MG";
  arrThirdVariable->SetName(namev3);
  table->AddColumn(arrThirdVariable.GetPointer());

  int numPoints = 20;
  table->SetNumberOfRows(numPoints);

  double MathValue[] =
    {
    18, 20, 20, 16,
    12, 14, 16, 14,
    14, 13, 16, 18,
    6, 10, 16, 14,
    4, 16, 16, 14
    };

  double FrenchValue[] =
    {
    14, 12, 14, 16,
    12, 14, 16, 4,
    4, 10, 6, 20,
    14, 16, 14, 14,
    12, 2, 14, 8
    };

  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i); // Known Test Values
    table->SetValue(i, 1, MathValue[i]);
    table->SetValue(i, 2, FrenchValue[i]);
    table->SetValue(i, 3, (MathValue[i] + FrenchValue[i]) / 2.0);
    table->SetValue(i, 4, MathValue[i] - FrenchValue[i]);
  }

  // Run HDR
  // Set HDR statistics algorithm and its input data port
  vtkNew<vtkHighestDensityRegionsStatistics> hdrs;

   // First verify that absence of input does not cause trouble
  cout << "## Verifying that absence of input does not cause trouble... ";
  hdrs->Update();
  cout << "done.\n";

  hdrs->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, table.GetPointer());
  // Select Column Pairs of Interest ( Learn Mode )
  // 1: a valid pair
  hdrs->AddColumnPair(namev1, namev2);
  // 2: another valid pair
  hdrs->AddColumnPair(namev2, namev3);
  // 3: an invalid pair
  hdrs->AddColumnPair(namev2, "M3");

  hdrs->SetLearnOption(true);
  hdrs->SetDeriveOption(true);
  hdrs->SetAssessOption(false);
  hdrs->SetTestOption(false);
  hdrs->Update();

  cout << "\n## Result:\n";
  vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast(
      hdrs->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL ) );

  vtkTable* outputMetaLearn = vtkTable::SafeDownCast(outputMetaDS->GetBlock(0));
  outputMetaLearn->Dump();

  std::stringstream ss;
  ss << "HDR (" << namev1 << "," << namev2 << ")";
  vtkDoubleArray* HDRArray = vtkArrayDownCast<vtkDoubleArray>(
    outputMetaLearn->GetColumnByName(ss.str().c_str()));
  if (!HDRArray)
  {
    cout << "Fail! The HDR column is missing from the result table!" << endl;
    return EXIT_FAILURE;
  }
  cout << "## Done." << endl;

  return EXIT_SUCCESS;
}
