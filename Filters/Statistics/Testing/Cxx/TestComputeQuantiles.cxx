// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkComputeQuantiles.h"
#include "vtkExecutive.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkTable.h"
#include "vtkTestErrorObserver.h"

#include <vtksys/SystemTools.hxx>

//------------------------------------------------------------------------------
// randomly sampled data
const int N_random_list = 100;
const int random_list[] = { 73, 8, 67, 84, 28, 75, 20, 75, 38, 38, 39, 94, 58, 89, 91, 3, 91, 76,
  18, 70, 18, 69, 87, 25, 81, 24, 6, 81, 67, 98, 9, 24, 40, 13, 30, 93, 46, 65, 67, 55, 56, 74, 48,
  28, 28, 13, 21, 33, 98, 20, 84, 69, 40, 2, 41, 70, 20, 71, 14, 35, 68, 47, 59, 86, 41, 53, 57, 55,
  26, 47, 44, 89, 46, 35, 34, 20, 10, 77, 55, 28, 33, 70, 30, 10, 9, 34, 10, 77, 39, 35, 4, 20, 53,
  44, 1, 60, 77, 80, 39, 14 };

// the correct solutions for the sampled data; includes the extrema
const double quartile_solution[] = { 1, 24.5, 44, 70, 98 };
const double decile_solution[] = { 1, 10, 20, 28, 36.5, 44, 55, 67.5, 75, 85, 98 };
//------------------------------------------------------------------------------

bool ComputeQuantiles(vtkTable* table, int N_intervals, const double* solution)
{
  vtkNew<vtkComputeQuantiles> computeQuantiles;
  computeQuantiles->SetNumberOfIntervals(N_intervals);
  computeQuantiles->SetInputData(table);
  computeQuantiles->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, table->GetColumn(0)->GetName());
  computeQuantiles->Update();

  vtkDataArray* solutionArray =
    vtkDataArray::SafeDownCast(computeQuantiles->GetOutput()->GetColumn(0));
  if (!solutionArray || (solutionArray->GetNumberOfTuples() != N_intervals + 1))
  {
    return false;
  }
  for (int i = 0; i <= N_intervals; i++)
  {
    double s = solutionArray->GetTuple1(i);
    if (s != solution[i])
    {
      return false;
    }
  }
  return true;
}

int TestComputeQuantiles(int, char*[])
{
  // Create the table containing the raw random samples
  vtkNew<vtkIntArray> sample_array;
  sample_array->SetName("samples");
  sample_array->SetNumberOfComponents(1);
  sample_array->SetNumberOfTuples(N_random_list);
  for (int i = 0; i < N_random_list; i++)
  {
    sample_array->SetTuple1(i, random_list[i]);
  }
  vtkNew<vtkTable> table;
  table->AddColumn(sample_array);

  if (!ComputeQuantiles(table, 4, quartile_solution))
  {
    cout << "## Failure: Computation of quartiles does not match solution data!" << endl;
    return EXIT_FAILURE;
  }

  if (!ComputeQuantiles(table, 10, decile_solution))
  {
    cout << "## Failure: Computation of deciles does not match solution data!" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
