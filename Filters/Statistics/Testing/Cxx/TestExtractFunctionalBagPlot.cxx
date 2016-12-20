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

#include "vtkTestErrorObserver.h"
#include "vtkExecutive.h"

#include <algorithm>
#include <sstream>
#include <vector>

#include <vtksys/SystemTools.hxx>

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
  std::vector<double> sortedDensities(
    densities, densities + sizeof(densities) / sizeof(double));
  double totalSumOfDensities = 0.;
  for (std::size_t i = 0; i < sortedDensities.size(); i++)
  {
    totalSumOfDensities += sortedDensities[i];
  }

  std::sort(sortedDensities.begin(), sortedDensities.end());
  double sumOfDensities = 0.;
  double sumForP50 = totalSumOfDensities * 0.5;
  double sumForP95 = totalSumOfDensities * ((100. - 95.)  / 100.);
  double p50 = 0.;
  double p95 = 0.;
  for (std::size_t i = 0; i < sortedDensities.size(); i++)
  {
    sumOfDensities += sortedDensities[i];
    if (sumOfDensities >= sumForP50 && p50 == 0.)
    {
      p50 = sortedDensities[i];
    }
    if (sumOfDensities >= sumForP95 && p95 == 0.)
    {
      p95 = sortedDensities[i];
    }
  }

  vtkNew<vtkExtractFunctionalBagPlot> ebp;
  ebp->SetDensityForP50(p50);
  ebp->SetDensityForPUser(p95);
  ebp->SetPUser(95);

  vtkNew<vtkTest::ErrorObserver> errorObserver1;
   // First verify that absence of input does not cause trouble
  ebp->GetExecutive()->AddObserver(vtkCommand::ErrorEvent,errorObserver1.GetPointer());
  ebp->Update();
  int status = errorObserver1->CheckErrorMessage("Input port 0 of algorithm vtkExtractFunctionalBagPlot");

  ebp->SetInputData(0, table.GetPointer());
  ebp->SetInputData(1, inTableDensity.GetPointer());
  ebp->SetInputArrayToProcess(0, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, "Density");
  ebp->SetInputArrayToProcess(1, 1, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, "ColName");
  ebp->Update();

  vtkTable* outBPTable = ebp->GetOutput();
  vtkDoubleArray* q3Points = 0;
  for (vtkIdType i = 0; i < outBPTable->GetNumberOfColumns(); i++)
  {
    const char* colName = outBPTable->GetColumnName(i);
    if (vtksys::SystemTools::StringStartsWith(colName, "Q3Points"))
    {
      q3Points =
        vtkArrayDownCast<vtkDoubleArray>(outBPTable->GetColumn(i));
      break;
    }
  }
  vtkDoubleArray* q2Points =
    vtkArrayDownCast<vtkDoubleArray>(outBPTable->GetColumnByName("QMedPoints"));

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

  if (q3v[0] != 114 || q3v[1] != 285 || q2v[0] != 171 || q2v[1] != 209)
  {
    outBPTable->Dump();
    cout << "## Failure: bad values found in Q3Points or QMedPoints" << endl;
    return EXIT_FAILURE;
  }
  return status;
}
