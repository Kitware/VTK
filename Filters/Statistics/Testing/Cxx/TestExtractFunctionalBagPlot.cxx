/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractFunctionalBagPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDoubleArray.h"
#include "vtkExtractFunctionalBagPlot.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <sstream>

//----------------------------------------------------------------------------
int TestExtractFunctionalBagPlot(int , char * [])
{
  // Create a table with some points in it...
  vtkNew<vtkTable> table;

  const int numCols = 20;
  const int numPoints = 20;

  for (int j = 0; j < numCols; j++)
    {
    vtkNew<vtkDoubleArray> arr;
    std::stringstream ss;
    ss << "Var" << j;
    arr->SetName(ss.str().c_str());
    arr->SetNumberOfValues(numPoints);
    table->AddColumn(arr.GetPointer());
    }

  table->SetNumberOfRows(numPoints);

  for (int j = 0; j < numCols; j++)
    {
    for (int i = 0; i < numPoints; i++)
      {
      table->SetValue(i, j, i * j);
      }
    }

  cout << "\n## Input data table:\n";
  table->Dump();

  // Create a density table

  vtkNew<vtkDoubleArray> density;
  density->SetName("Density");
  density->SetNumberOfValues(numCols);

  vtkNew<vtkStringArray> varName;
  varName->SetName("ColName");
  varName->SetNumberOfValues(numCols);
  for (int j = 0; j < numCols; j++)
    {
    double x = j * 8. / static_cast<double>(numCols) - 4.;
    double y = (1. / sqrt(vtkMath::Pi() * 2.)) * exp(-(x*x) / 2.);
    density->SetValue(j, y);

    varName->SetValue(j, table->GetColumn(j)->GetName());
    }

  vtkNew<vtkTable> inTableDensity;
  inTableDensity->AddColumn(density.GetPointer());
  inTableDensity->AddColumn(varName.GetPointer());

  cout << "\n## Input density table:\n";
  inTableDensity->Dump();

  vtkNew<vtkExtractFunctionalBagPlot> ebp;

   // First verify that absence of input does not cause trouble
  cout << "## Verifying that absence of input does not cause trouble... ";
  ebp->Update();
  cout << "done.\n";

  ebp->SetInputData(0, table.GetPointer());
  ebp->SetInputData(1, inTableDensity.GetPointer());
  ebp->SetInputArrayToProcess(0, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, "Density");
  ebp->SetInputArrayToProcess(1, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, "ColName");
  ebp->Update();

  cout << "\n## Results:" << endl;
  vtkTable* outBPTable = ebp->GetOutput();
  outBPTable->Dump();

  vtkDoubleArray* q3Points =
    vtkDoubleArray::SafeDownCast(outBPTable->GetColumnByName("Q3Points"));
  vtkDoubleArray* q2Points =
    vtkDoubleArray::SafeDownCast(outBPTable->GetColumnByName("QMedPoints"));

  if (!q3Points || !q2Points)
    {
    cout << "## Failure: Missing Q3Points or QMedPoints columns!" << endl;
    return EXIT_FAILURE;
    }

  if (q3Points->GetNumberOfTuples() != numPoints ||
    q2Points->GetNumberOfTuples() != numPoints)
    {
    cout << "## Failure: Bad number of tuples in Q3Points or QMedPoints columns!" << endl;
    return EXIT_FAILURE;
    }

  if (q3Points->GetNumberOfComponents() != 2 ||
    q2Points->GetNumberOfComponents() != 2)
    {
    cout << "## Failure: Q3Points or QMedPoints does not have 2 components!" << endl;
    return EXIT_FAILURE;
    }

  // Verify last values
  double q3v[2];
  q3Points->GetTuple(19, q3v);
  double q2v[2];
  q2Points->GetTuple(19, q2v);

  if (q3v[0] != 38 || q3v[1] != 323 || q2v[0] != 95 || q2v[1] != 285)
    {
    cout << "## Failure: bad values found in Q3Points or QMedPoints" << endl;
    return EXIT_FAILURE;
    }
  cout << "## Success!" << endl;
  return EXIT_SUCCESS;
}
