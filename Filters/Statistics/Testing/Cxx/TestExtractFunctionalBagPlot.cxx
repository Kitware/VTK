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
const double densities[] = {
  0.00013383,
  0.000611902,
  0.00238409,
  0.00791545,
  0.0223945,
  0.053991,
  0.110921,
  0.194186,
  0.289692,
  0.36827,
  0.398942,
  0.368271,
  0.2896921,
  0.1941861,
  0.1109211,
  0.0539911,
  0.02239451,
  0.007915451,
  0.002384091,
  0.0006119021
};

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

  // Create a density table

  vtkNew<vtkDoubleArray> density;
  density->SetName("Density");
  density->SetNumberOfValues(numCols);

  vtkNew<vtkStringArray> varName;
  varName->SetName("ColName");
  varName->SetNumberOfValues(numCols);
  for (int j = 0; j < numCols; j++)
    {
    double d = densities[j];
    density->SetValue(j, d);
    varName->SetValue(j, table->GetColumn(j)->GetName());
    }

  vtkNew<vtkTable> inTableDensity;
  inTableDensity->AddColumn(density.GetPointer());
  inTableDensity->AddColumn(varName.GetPointer());

  vtkNew<vtkExtractFunctionalBagPlot> ebp;

   // First verify that absence of input does not cause trouble
  ebp->Update();

  ebp->SetInputData(0, table.GetPointer());
  ebp->SetInputData(1, inTableDensity.GetPointer());
  ebp->SetInputArrayToProcess(0, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, "Density");
  ebp->SetInputArrayToProcess(1, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, "ColName");
  ebp->Update();

  vtkTable* outBPTable = ebp->GetOutput();

  vtkDoubleArray* q3Points =
    vtkDoubleArray::SafeDownCast(outBPTable->GetColumnByName("Q3Points"));
  vtkDoubleArray* q2Points =
    vtkDoubleArray::SafeDownCast(outBPTable->GetColumnByName("QMedPoints"));

  if (!q3Points || !q2Points)
    {
    outBPTable->Dump();
    cout << "## Failure: Missing Q3Points or QMedPoints columns!" << endl;
    return EXIT_FAILURE;
    }

  if (q3Points->GetNumberOfTuples() != numPoints ||
    q2Points->GetNumberOfTuples() != numPoints)
    {
    outBPTable->Dump();
    cout << "## Failure: Bad number of tuples in Q3Points or QMedPoints columns!" << endl;
    return EXIT_FAILURE;
    }

  if (q3Points->GetNumberOfComponents() != 2 ||
    q2Points->GetNumberOfComponents() != 2)
    {
    outBPTable->Dump();
    cout << "## Failure: Q3Points or QMedPoints does not have 2 components!" << endl;
    return EXIT_FAILURE;
    }

  // Verify last values
  double q3v[2];
  q3Points->GetTuple(19, q3v);
  double q2v[2];
  q2Points->GetTuple(19, q2v);

  if (q3v[0] != 95 || q3v[1] != 304 || q2v[0] != 171 || q2v[1] != 209)
    {
    outBPTable->Dump();
    cout << "## Failure: bad values found in Q3Points or QMedPoints" << endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
