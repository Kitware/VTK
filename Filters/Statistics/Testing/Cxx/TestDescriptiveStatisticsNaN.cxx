// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
// for implementing this test.

#include "vtkDataAssembly.h"
#include "vtkDataObjectCollection.h"
#include "vtkDataSetAttributes.h"
#include "vtkDescriptiveStatistics.h"
#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkStatisticalModel.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#include <iostream>

namespace
{

bool testStatsOutput(vtkDescriptiveStatistics* stats, double means[], double stdevs[])
{
  bool ok = true;

  // Get output data and meta tables
  auto* model = vtkStatisticalModel::SafeDownCast(
    stats->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
  if (!model || model->GetNumberOfTables() != 2)
  {
    std::cerr << "ERROR: Empty model or unexpected number of tables.\n";
  }
  auto* learned = model->GetTable(vtkStatisticalModel::Learned, 0);
  auto* derived = model->GetTable(vtkStatisticalModel::Derived, 0);

  std::cout << "\n## Calculated the following primary statistics:\n";
  for (vtkIdType r = 0; r < learned->GetNumberOfRows(); ++r)
  {
    std::cout << "   ";
    for (int i = 0; i < learned->GetNumberOfColumns(); ++i)
    {
      double val = learned->GetValue(r, i).ToDouble();
      std::cout << learned->GetColumnName(i) << "=" << val << "  ";
    }
    std::cout << "\n";

    // Verify some of the calculated learned statistics
    auto mean = learned->GetValueByName(r, "Mean").ToDouble();
    if (fabs(mean - means[r]) > 1.e-6 || (std::isnan(mean) && !std::isnan(means[r])))
    {
      std::cerr << "ERROR: Incorrect mean " << mean << ", expected " << means[r] << ".\n";
      ok = false;
    }
    auto card = learned->GetValueByName(r, "Cardinality").ToInt();
    // Skipped NaN values reduce cardinality of field 2, but if not skipping,
    // NaN values are counted.
    int numExpected = (r == 0 || !stats->GetSkipInvalidValues() ? 8 : 6);
    if (card != numExpected)
    {
      std::cerr << "ERROR: Incorrect cardinality " << card << ", expected " << numExpected << ".\n";
      ok = false;
    }
  }

  std::cout << "\n## Calculated the following derived statistics:\n";
  for (vtkIdType r = 0; r < derived->GetNumberOfRows(); ++r)
  {
    std::cout << "   ";
    for (int i = 0; i < derived->GetNumberOfColumns(); ++i)
    {
      double val = derived->GetValue(r, i).ToDouble();
      std::cout << derived->GetColumnName(i) << "=" << val << "  ";
    }
    std::cout << "\n";

    auto stdev = derived->GetValueByName(r, "Standard Deviation").ToDouble();
    if (fabs(stdev - stdevs[r]) > 1.e-6 || (std::isnan(stdevs[r]) && !std::isnan(stdev)))
    {
      ok = false;
      std::cerr << "ERROR: Expected standard deviation " << stdevs[r] << ", got " << stdev << ".\n";
    }
  }
  return ok;
}

} // anonymous namespace

int TestDescriptiveStatisticsNaN(int, char*[])
{
  bool ok = true;

  // clang-format off
  double field1[] = { 0, 1, 0, 1, 0, 1, 0, 1 };
  double field2[] = { 0, 1, 0, 1, 0, 1, NAN, NAN };
  constexpr int numValues = sizeof(field1)/sizeof(field1[0]);
  // clang-format on

  vtkNew<vtkDoubleArray> f1;
  vtkNew<vtkDoubleArray> f2;
  f1->SetNumberOfTuples(8);
  f2->SetNumberOfTuples(8);
  f1->SetName("field1");
  f2->SetName("field2");

  for (int ii = 0; ii < numValues; ++ii)
  {
    f1->SetValue(ii, field1[ii]);
    f2->SetValue(ii, field2[ii]);
  }

  vtkNew<vtkTable> tab;
  tab->AddColumn(f1);
  tab->AddColumn(f2);

  std::string columns[] = { "field1", "field2" };
  constexpr int numFields = sizeof(columns) / sizeof(columns[0]);

  // Expected means and standard deviations
  double means[] = { 0.5, 0.5 };
  double stdevs[] = { 0.5, 0.5 };
  double means_nan[] = { 0.5, NAN };
  double stdevs_nan[] = { 0.5, NAN };

  vtkNew<vtkDescriptiveStatistics> stats;
  stats->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, tab);

  // Select Columns of Interest
  for (int i = 0; i < numFields; ++i)
  {
    stats->AddColumn(columns[i].c_str());
  }

  // Test Learn, Derive, Test, and Assess options
  stats->SetLearnOption(true);
  stats->SetDeriveOption(true);
  stats->SetAssessOption(false);
  stats->SetTestOption(false);
  stats->SampleEstimateOff();
  stats->Update();

  std::cout << "\n# Test with SkipInvalidValues on\n";
  ok |= testStatsOutput(stats, means, stdevs);

  stats->SkipInvalidValuesOff();
  stats->Update();

  std::cout << "\n# Test with SkipInvalidValues off\n";
  ok |= testStatsOutput(stats, means_nan, stdevs_nan);

  return ok ? 0 : 1;
}
