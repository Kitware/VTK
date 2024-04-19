// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkExecutive.h"
#include "vtkExtractHistogram.h"
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

// the correct histogram solution for the sampled data
const int N_histogram_bins = 10;
const int histogram_data[] = { 11, 11, 11, 12, 11, 9, 6, 14, 7, 8 };
//------------------------------------------------------------------------------

bool TestBasicHistogramExtraction(vtkTable* table)
{
  // Basic test if the result from vtkExtractHistogram with default settings matches the solution
  // data.
  vtkNew<vtkExtractHistogram> histogram;
  histogram->SetBinCount(N_histogram_bins);
  histogram->AccumulationOn();
  histogram->SetInputData(table);
  histogram->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, table->GetColumnName(0));
  histogram->Update();
  vtkTable* output = histogram->GetOutput();
  vtkDataArray* bin_values =
    vtkDataArray::SafeDownCast(output->GetColumnByName(histogram->GetBinValuesArrayName()));
  vtkDataArray* bin_accumulation =
    vtkDataArray::SafeDownCast(output->GetColumnByName(histogram->GetBinAccumulationArrayName()));
  int sum = 0;
  for (int i = 0; i < N_histogram_bins; i++)
  {
    // test the histogram bin values
    if (bin_values->GetTuple1(i) != histogram_data[i])
    {
      return false;
    }
    // test the accumulated histogram bin values
    sum += histogram_data[i];
    if (bin_accumulation->GetTuple1(i) != sum)
    {
      return false;
    }
  }
  return true;
}

bool TestHistogramNormalization(vtkTable* table)
{
  vtkNew<vtkExtractHistogram> histogram;
  histogram->SetBinCount(N_histogram_bins);
  histogram->AccumulationOn();
  histogram->NormalizeOn();
  histogram->SetInputData(table);
  histogram->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, table->GetColumnName(0));
  histogram->Update();
  vtkTable* output = histogram->GetOutput();
  // we make use of the fact that the value in the last accumulation bin should be 1.0 within
  // numerical tolerances
  vtkDataArray* bin_accumulation =
    vtkDataArray::SafeDownCast(output->GetColumnByName(histogram->GetBinAccumulationArrayName()));
  double last_val = bin_accumulation->GetTuple1(bin_accumulation->GetNumberOfTuples() - 1);
  double diff1 = fabs(last_val - 1.0);
  return diff1 <= std::numeric_limits<double>::epsilon() * N_histogram_bins;
}

int TestExtractHistogram(int, char*[])
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

  // First test the basic histogram extraction.
  // Subsequent tests then don't need to test for basic histogram correctness and can specialize
  // towards their tasks.
  if (!TestBasicHistogramExtraction(table))
  {
    cout << "## Failure: Basic histogram extraction does not match solution data!" << endl;
    return EXIT_FAILURE;
  }

  if (!TestHistogramNormalization(table))
  {
    cout << "## Failure: Histogram normalization failed!" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
