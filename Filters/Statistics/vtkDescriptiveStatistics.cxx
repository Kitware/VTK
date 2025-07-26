// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkDescriptiveStatistics.h"
#include "vtkStatisticsAlgorithmPrivate.h"

#include "vtkCompiler.h"
#include "vtkDataAssembly.h"
#include "vtkDataObjectCollection.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLegacy.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkStatisticalModel.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkStringToken.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariantArray.h"

#include <limits>
#include <set>
#include <sstream>
#include <vector>

#ifdef VTK_COMPILER_MSVC
#pragma float_control(precise, on) // enable precise semantics
#pragma fp_contract(off)           // disable contractions
#endif

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkDescriptiveStatistics);

//------------------------------------------------------------------------------
vtkDescriptiveStatistics::vtkDescriptiveStatistics()
{
  this->AssessNames->SetNumberOfValues(1);
  this->AssessNames->SetValue(
    0, "d"); // relative deviation, i.e., when unsigned, 1D Mahalanobis distance

  this->SampleEstimate = true;
  this->SignedDeviations = 0; // By default, use unsigned deviation (1D Mahlanobis distance)
}

//------------------------------------------------------------------------------
vtkDescriptiveStatistics::~vtkDescriptiveStatistics() = default;

//------------------------------------------------------------------------------
void vtkDescriptiveStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Type of statistics: "
     << (this->SampleEstimate ? "Sample Statistics" : "Population Statistics") << "\n";
  os << indent << "SignedDeviations: " << this->SignedDeviations << "\n";
}

//------------------------------------------------------------------------------
bool vtkDescriptiveStatistics::Aggregate(
  vtkDataObjectCollection* inMetaColl, vtkStatisticalModel* outMeta)
{
  if (!outMeta)
  {
    return false;
  }
  // Do not call outMeta->Initialize() since outMeta may be a member of inMetaColl.
  outMeta->SetAlgorithmParameters(this->GetAlgorithmParameters());

  // Get hold of the first model (data object) in the collection
  int itemIndex;
  int numItems = inMetaColl->GetNumberOfItems();
  // Skip collection entries until we find one with a primary table:
  vtkTable* primaryTab = nullptr;
  for (itemIndex = 0; itemIndex < numItems; ++itemIndex)
  {
    // Verify that the first primary statistics are present
    auto* inMeta = vtkStatisticalModel::SafeDownCast(inMetaColl->GetItemAsObject(itemIndex));
    primaryTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Learned, 0) : nullptr;
    if (primaryTab)
    {
      break;
    }
  }
  if (!primaryTab)
  {
    return true;
  }

  vtkIdType nRow = primaryTab->GetNumberOfRows();
  if (!nRow)
  {
    // No statistics were calculated.
    return true;
  }

  // Use this first model to initialize the aggregated one
  vtkTable* aggregatedTab = vtkTable::New();
  aggregatedTab->DeepCopy(primaryTab);

  vtkDataArray* aggCardinality =
    vtkArrayDownCast<vtkDataArray>(aggregatedTab->GetColumnByName("Cardinality"));
  vtkDataArray* aggMinimum =
    vtkArrayDownCast<vtkDataArray>(aggregatedTab->GetColumnByName("Minimum"));
  vtkDataArray* aggMaximum =
    vtkArrayDownCast<vtkDataArray>(aggregatedTab->GetColumnByName("Maximum"));
  vtkDataArray* aggMean = vtkArrayDownCast<vtkDataArray>(aggregatedTab->GetColumnByName("Mean"));
  vtkDataArray* aggM2 = vtkArrayDownCast<vtkDataArray>(aggregatedTab->GetColumnByName("M2"));
  vtkDataArray* aggM3 = vtkArrayDownCast<vtkDataArray>(aggregatedTab->GetColumnByName("M3"));
  vtkDataArray* aggM4 = vtkArrayDownCast<vtkDataArray>(aggregatedTab->GetColumnByName("M4"));

  // Now, loop over all remaining models and update aggregated each time
  for (++itemIndex; itemIndex < numItems; ++itemIndex)
  {
    // Verify that the current primary statistics are indeed contained in a table
    auto* inMeta = vtkStatisticalModel::SafeDownCast(inMetaColl->GetItemAsObject(itemIndex));
    primaryTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Learned, 0) : nullptr;
    if (!primaryTab)
    {
      continue;
    }

    vtkDataArray* primCardinality =
      vtkArrayDownCast<vtkDataArray>(primaryTab->GetColumnByName("Cardinality"));
    vtkDataArray* primMinimum =
      vtkArrayDownCast<vtkDataArray>(primaryTab->GetColumnByName("Minimum"));
    vtkDataArray* primMaximum =
      vtkArrayDownCast<vtkDataArray>(primaryTab->GetColumnByName("Maximum"));
    vtkDataArray* primMean = vtkArrayDownCast<vtkDataArray>(primaryTab->GetColumnByName("Mean"));
    vtkDataArray* primM2 = vtkArrayDownCast<vtkDataArray>(primaryTab->GetColumnByName("M2"));
    vtkDataArray* primM3 = vtkArrayDownCast<vtkDataArray>(primaryTab->GetColumnByName("M3"));
    vtkDataArray* primM4 = vtkArrayDownCast<vtkDataArray>(primaryTab->GetColumnByName("M4"));

    if (primaryTab->GetNumberOfRows() != nRow)
    {
      vtkWarningMacro("Mismatched number of rows "
        << nRow << " vs " << primaryTab->GetNumberOfRows() << " at " << itemIndex << ". Skipping.");
      continue;
    }

    // Iterate over all model rows
    for (int r = 0; r < nRow; ++r)
    {
      // Verify that variable names match each other
      if (primaryTab->GetValueByName(r, "Variable") != aggregatedTab->GetValueByName(r, "Variable"))
      {
        // Models do not match. We may already have partially processed data. Error out.
        vtkErrorMacro("Model at " << itemIndex << " has mismatched variable " << r << ".");
        aggregatedTab->Delete();
        return false;
      }

      // It is important for n and n_c to be double, as later on they are multiplied by themselves,
      // which can produce an overflow if they were integer types.

      // Get aggregated statistics
      double n = aggCardinality->GetComponent(r, 0);
      double min = aggMinimum->GetComponent(r, 0);
      double max = aggMaximum->GetComponent(r, 0);
      double mean = aggMean->GetComponent(r, 0);
      double M2 = aggM2->GetComponent(r, 0);
      double M3 = aggM3->GetComponent(r, 0);
      double M4 = aggM4->GetComponent(r, 0);

      // Get current model statistics
      double n_c = primCardinality->GetComponent(r, 0);
      double min_c = primMinimum->GetComponent(r, 0);
      double max_c = primMaximum->GetComponent(r, 0);
      double mean_c = primMean->GetComponent(r, 0);
      double M2_c = primM2->GetComponent(r, 0);
      double M3_c = primM3->GetComponent(r, 0);
      double M4_c = primM4->GetComponent(r, 0);

      // Update global statistics
      double N = n + n_c;

      if (min_c < min)
      {
        aggregatedTab->SetValueByName(r, "Minimum", min_c);
      }

      if (max_c > max)
      {
        aggregatedTab->SetValueByName(r, "Maximum", max_c);
      }

      double delta = mean_c - mean;
      double delta_sur_N = delta / N;
      double delta2_sur_N2 = delta_sur_N * delta_sur_N;

      double n2 = n * n;
      double n_c2 = n_c * n_c;
      double prod_n = n * n_c;

      M4 += M4_c + delta2_sur_N2 * delta2_sur_N2 * prod_n * (n * n2 + n_c * n_c2) +
        6. * (n2 * M2_c + n_c2 * M2) * delta2_sur_N2 + 4. * (n * M3_c - n_c * M3) * delta_sur_N;

      M3 += M3_c + prod_n * (n - n_c) * delta * delta2_sur_N2 +
        3. * (n * M2_c - n_c * M2) * delta_sur_N;

      M2 += M2_c + prod_n * delta * delta_sur_N;

      mean += n_c * delta_sur_N;

      // Store updated model
      aggregatedTab->SetValueByName(r, "Cardinality", N);
      aggregatedTab->SetValueByName(r, "Mean", mean);
      aggregatedTab->SetValueByName(r, "M2", M2);
      aggregatedTab->SetValueByName(r, "M3", M3);
      aggregatedTab->SetValueByName(r, "M4", M4);
    }
  }

  // Finally set output primary statistics table.
  outMeta->SetNumberOfTables(vtkStatisticalModel::Learned, 1);
  outMeta->SetTable(vtkStatisticalModel::Learned, 0, aggregatedTab, "Primary Statistics");

  // Clean up
  aggregatedTab->Delete();
  return true;
}

//------------------------------------------------------------------------------
void vtkDescriptiveStatistics::Learn(
  vtkTable* inData, vtkTable* vtkNotUsed(inParameters), vtkStatisticalModel* outMeta)
{
  if (!inData)
  {
    return;
  }

  if (!outMeta)
  {
    return;
  }

  outMeta->Initialize();
  outMeta->SetNumberOfTables(vtkStatisticalModel::Learned, 1);
  outMeta->SetAlgorithmParameters(this->GetAlgorithmParameters());

  // The primary statistics table
  vtkTable* primaryTab = vtkTable::New();

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName("Variable");
  primaryTab->AddColumn(stringCol);
  stringCol->Delete();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName("Cardinality");
  primaryTab->AddColumn(idTypeCol);
  idTypeCol->Delete();

  vtkDoubleArray* doubleCol = vtkDoubleArray::New();
  doubleCol->SetName("Minimum");
  primaryTab->AddColumn(doubleCol);
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName("Maximum");
  primaryTab->AddColumn(doubleCol);
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName("Mean");
  primaryTab->AddColumn(doubleCol);
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName("M2");
  primaryTab->AddColumn(doubleCol);
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName("M3");
  primaryTab->AddColumn(doubleCol);
  doubleCol->Delete();

  doubleCol = vtkDoubleArray::New();
  doubleCol->SetName("M4");
  primaryTab->AddColumn(doubleCol);
  doubleCol->Delete();

  vtkDataSetAttributes* dsa = inData->GetRowData();
  vtkUnsignedCharArray* ghosts = dsa->GetGhostArray();

  // Loop over requests
  vtkIdType nRow = inData->GetNumberOfRows();
  vtkIdType numberOfGhostlessRow = 0;
  if (ghosts)
  {
    for (vtkIdType id = 0; id < ghosts->GetNumberOfValues(); ++id)
    {
      if (!(ghosts->GetValue(id) & this->GhostsToSkip))
      {
        ++numberOfGhostlessRow;
      }
    }
  }
  else
  {
    numberOfGhostlessRow = nRow;
  }
  for (std::set<std::set<vtkStdString>>::const_iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit)
  {
    // Each request contains only one column of interest (if there are others, they are ignored)
    std::set<vtkStdString>::const_iterator it = rit->begin();
    vtkStdString const& varName = *it;
    if (!inData->GetColumnByName(varName.c_str()))
    {
      vtkWarningMacro("InData table does not have a column " << varName << ". Ignoring it.");
      continue;
    }

    double minVal, maxVal, mean, mom2, mom3, mom4;
    if (numberOfGhostlessRow == 0)
    {
      minVal = std::numeric_limits<double>::quiet_NaN();
      maxVal = std::numeric_limits<double>::quiet_NaN();
      mean = std::numeric_limits<double>::quiet_NaN();
      mom2 = std::numeric_limits<double>::quiet_NaN();
      mom3 = std::numeric_limits<double>::quiet_NaN();
      mom4 = std::numeric_limits<double>::quiet_NaN();
    }
    else
    {
      minVal = std::numeric_limits<double>::max();
      maxVal = std::numeric_limits<double>::min();
      mean = 0.;
      mom2 = 0.;
      mom3 = 0.;
      mom4 = 0.;
    }

    if (numberOfGhostlessRow)
    {
      double n, inv_n, val, delta, A, B;
      vtkIdType numberOfSkippedElements = 0;
      for (vtkIdType r = 0; r < nRow; ++r)
      {
        if (ghosts && (ghosts->GetValue(r) & this->GhostsToSkip))
        {
          ++numberOfSkippedElements;
          continue;
        }
        n = r + 1. - numberOfSkippedElements;
        inv_n = 1. / n;

        val = inData->GetValueByName(r, varName.c_str()).ToDouble();
        delta = val - mean;

        A = delta * inv_n;
        mean += A;
        mom4 += A *
          (A * A * delta * (r - numberOfSkippedElements) * (n * (n - 3.) + 3.) + 6. * A * mom2 -
            4. * mom3);

        B = val - mean;
        mom3 += A * (B * delta * (n - 2.) - 3. * mom2);
        mom2 += delta * B;

        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);
      }
    }

    vtkVariantArray* row = vtkVariantArray::New();

    row->SetNumberOfValues(8);

    row->SetValue(0, varName);
    row->SetValue(1, numberOfGhostlessRow);
    row->SetValue(2, minVal);
    row->SetValue(3, maxVal);
    row->SetValue(4, mean);
    row->SetValue(5, mom2);
    row->SetValue(6, mom3);
    row->SetValue(7, mom4);

    primaryTab->InsertNextRow(row);

    row->Delete();
  } // rit

  outMeta->SetTable(vtkStatisticalModel::Learned, 0, primaryTab, "Primary Statistics");

  // Clean up
  primaryTab->Delete();
}

//------------------------------------------------------------------------------
void vtkDescriptiveStatistics::Derive(vtkStatisticalModel* modelData)
{
  auto* primaryTab = modelData ? modelData->GetTable(vtkStatisticalModel::Learned, 0) : nullptr;
  if (!primaryTab)
  {
    return;
  }

  int numDoubles = 5;
  std::string doubleNames[] = { "Standard Deviation", "Variance", "Skewness", "Kurtosis", "Sum" };

  // Create table for derived statistics
  vtkIdType nRow = primaryTab->GetNumberOfRows();
  vtkTable* derivedTab = vtkTable::New();
  vtkDoubleArray* doubleCol;
  for (int j = 0; j < numDoubles; ++j)
  {
    if (!derivedTab->GetColumnByName(doubleNames[j].c_str()))
    {
      doubleCol = vtkDoubleArray::New();
      doubleCol->SetName(doubleNames[j].c_str());
      doubleCol->SetNumberOfTuples(nRow);
      derivedTab->AddColumn(doubleCol);
      doubleCol->Delete();
    }
  }

  // Storage for standard deviation, variance, skewness,  kurtosis, sum
  std::vector<double> derivedVals(numDoubles);

  for (int i = 0; i < nRow; ++i)
  {
    double mom2 = primaryTab->GetValueByName(i, "M2").ToDouble();
    double mom3 = primaryTab->GetValueByName(i, "M3").ToDouble();
    double mom4 = primaryTab->GetValueByName(i, "M4").ToDouble();

    vtkTypeInt64 numSamples = primaryTab->GetValueByName(i, "Cardinality").ToTypeInt64();

    if (!numSamples)
    {
      derivedVals[0] = std::numeric_limits<double>::quiet_NaN();
      derivedVals[1] = std::numeric_limits<double>::quiet_NaN();
      derivedVals[2] = std::numeric_limits<double>::quiet_NaN();
      derivedVals[3] = std::numeric_limits<double>::quiet_NaN();
      derivedVals[4] = std::numeric_limits<double>::quiet_NaN();

      for (int j = 0; j < numDoubles; ++j)
      {
        derivedTab->SetValueByName(i, doubleNames[j].c_str(), derivedVals[j]);
      }

      continue;
    }

    double mean = primaryTab->GetValueByName(i, "Mean").ToDouble();

    if (mom2 * mom2 <= FLT_EPSILON * std::abs(mean))
    {
      derivedVals[0] = 0.0;
      derivedVals[1] = 0.0;
      derivedVals[2] = std::numeric_limits<double>::quiet_NaN();
      derivedVals[3] = std::numeric_limits<double>::quiet_NaN();
      derivedVals[4] = numSamples * mean;

      for (int j = 0; j < numDoubles; ++j)
      {
        derivedTab->SetValueByName(i, doubleNames[j].c_str(), derivedVals[j]);
      }

      continue;
    }

    double n = static_cast<double>(numSamples);

    // Variance
    if (this->SampleEstimate)
    {
      if (n > 1)
      {
        derivedVals[1] = mom2 / (n - 1.);
      }
      else
      {
        derivedVals[1] = std::numeric_limits<double>::quiet_NaN();
      }
    }
    else // PopulationStatistics
    {
      derivedVals[1] = mom2 / n;
    }

    // Standard deviation
    derivedVals[0] = sqrt(derivedVals[1]);

    // Skewness
    if (this->SampleEstimate)
    {
      if (n > 2)
      {
        derivedVals[2] = n / ((n - 1.) * (n - 2.)) * mom3 / (derivedVals[1] * derivedVals[0]);
      }
      else
      {
        derivedVals[2] = std::numeric_limits<double>::quiet_NaN();
      }
    }
    else // PopulationStatistics
    {
      derivedVals[2] = mom3 / (n * derivedVals[1] * derivedVals[0]);
    }

    // Kurtosis
    if (this->SampleEstimate)
    {
      if (n > 3)
      {
        derivedVals[3] = (n / (n - 1.)) * ((n + 1.) / (n - 2.)) / (n - 3.) * mom4 /
            (derivedVals[1] * derivedVals[1]) -
          3. * ((n - 1.) / (n - 2.)) * ((n - 1.) / (n - 3.));
      }
      else
      {
        derivedVals[3] = std::numeric_limits<double>::quiet_NaN();
      }
    }
    else // PopulationStatistics
    {
      derivedVals[3] = mom4 / n / (derivedVals[1] * derivedVals[1]) - 3.;
    }

    // Sum
    derivedVals[4] = numSamples * mean;

    for (int j = 0; j < numDoubles; ++j)
    {
      derivedTab->SetValueByName(i, doubleNames[j].c_str(), derivedVals[j]);
    }
  }

  // Finally append the derived statistics table to the partitioned dataset and data assembly.
  modelData->SetNumberOfTables(vtkStatisticalModel::Derived, 1);
  modelData->SetTable(vtkStatisticalModel::Derived, 0, derivedTab, "Derived Statistics");
  modelData->SetAlgorithmParameters(this->GetAlgorithmParameters());

  // Clean up
  derivedTab->Delete();
}

//------------------------------------------------------------------------------
// Use the invalid value of -1 for p-values if R is absent
vtkDoubleArray* vtkDescriptiveStatistics::CalculatePValues(vtkDoubleArray* statCol)
{
  // A column must be created first
  vtkDoubleArray* testCol = vtkDoubleArray::New();

  // Fill this column
  vtkIdType n = statCol->GetNumberOfTuples();
  testCol->SetNumberOfTuples(n);
  for (vtkIdType r = 0; r < n; ++r)
  {
    testCol->SetTuple1(r, -1);
  }

  return testCol;
}

//------------------------------------------------------------------------------
void vtkDescriptiveStatistics::Test(
  vtkTable* inData, vtkStatisticalModel* inMeta, vtkTable* outMeta)
{
  vtkTable* primaryTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Learned, 0) : nullptr;
  vtkTable* derivedTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Derived, 0) : nullptr;
  if (!primaryTab || !derivedTab)
  {
    return;
  }

  vtkIdType nRowPrim = primaryTab->GetNumberOfRows();
  if (nRowPrim != derivedTab->GetNumberOfRows())
  {
    vtkErrorMacro("Inconsistent input: primary model has "
      << nRowPrim << " rows but derived model has " << derivedTab->GetNumberOfRows()
      << ". Cannot test.");
    return;
  }

  if (!outMeta)
  {
    return;
  }

  // Prepare columns for the test:
  // 0: variable name
  // 1: Jarque-Bera statistic
  // 2: Jarque-Bera p-value (calculated only if R is available, filled with -1 otherwise)
  // NB: These are not added to the output table yet, for they will be filled individually first
  //     in order that R be invoked only once.
  vtkStringArray* nameCol = vtkStringArray::New();
  nameCol->SetName("Variable");

  vtkDoubleArray* statCol = vtkDoubleArray::New();
  statCol->SetName("Jarque-Bera");

  // Downcast columns to string arrays for efficient data access
  vtkStringArray* vars = vtkArrayDownCast<vtkStringArray>(primaryTab->GetColumnByName("Variable"));

  // Loop over requests
  for (std::set<std::set<vtkStdString>>::const_iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit)
  {
    // Each request contains only one column of interest (if there are others, they are ignored)
    std::set<vtkStdString>::const_iterator it = rit->begin();
    vtkStdString const& varName = *it;
    if (!inData->GetColumnByName(varName.c_str()))
    {
      vtkWarningMacro("InData table does not have a column " << varName << ". Ignoring it.");
      continue;
    }

    // Find the model row that corresponds to the variable of the request
    vtkIdType r = 0;
    while (r < nRowPrim && vars->GetValue(r) != varName)
    {
      ++r;
    }
    if (r >= nRowPrim)
    {
      vtkWarningMacro(
        "Incomplete input: model does not have a row " << varName << ". Cannot test.");
      continue;
    }

    // Retrieve model statistics necessary for Jarque-Bera testing
    double n = primaryTab->GetValueByName(r, "Cardinality").ToDouble();
    double skew = derivedTab->GetValueByName(r, "Skewness").ToDouble();
    double kurt = derivedTab->GetValueByName(r, "Kurtosis").ToDouble();

    // Now calculate Jarque-Bera statistic
    double jb = n * (skew * skew + .25 * kurt * kurt) / 6.;

    // Insert variable name and calculated Jarque-Bera statistic
    // NB: R will be invoked only once at the end for efficiency
    nameCol->InsertNextValue(varName);
    statCol->InsertNextTuple1(jb);
  } // rit

  // Now, add the already prepared columns to the output table
  outMeta->AddColumn(nameCol);
  outMeta->AddColumn(statCol);

  // Last phase: compute the p-values or assign invalid value if they cannot be computed
  // If available, use R to obtain the p-values for the Chi square distribution with 2 DOFs
  vtkDoubleArray* testCol = this->CalculatePValues(statCol);

  // The test column name can only be set after the column has been obtained from R
  testCol->SetName("P");

  // Now add the column of invalid values to the output table
  outMeta->AddColumn(testCol);

  testCol->Delete();

  // Clean up
  nameCol->Delete();
  statCol->Delete();
}

//------------------------------------------------------------------------------
class TableColumnDeviantFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkDataArray* Data;
  double Nominal;
  double Deviation;
};

// When the deviation is 0, we can't normalize. Instead, a non-zero value (1)
// is returned only when the nominal value is matched exactly.
class ZedDeviationDeviantFunctor : public TableColumnDeviantFunctor
{
public:
  ZedDeviationDeviantFunctor(vtkDataArray* vals, double nominal)
  {
    this->Data = vals;
    this->Nominal = nominal;
  }
  ~ZedDeviationDeviantFunctor() override = default;
  void operator()(vtkDoubleArray* result, vtkIdType id) override
  {
    result->SetNumberOfValues(1);
    result->SetValue(0, (this->Data->GetComponent(id, 0) == this->Nominal) ? 0. : 1.);
  }
};

class SignedTableColumnDeviantFunctor : public TableColumnDeviantFunctor
{
public:
  SignedTableColumnDeviantFunctor(vtkDataArray* vals, double nominal, double deviation)
  {
    this->Data = vals;
    this->Nominal = nominal;
    this->Deviation = deviation;
  }
  ~SignedTableColumnDeviantFunctor() override = default;
  void operator()(vtkDoubleArray* result, vtkIdType id) override
  {
    result->SetNumberOfValues(1);
    result->SetValue(0, (this->Data->GetComponent(id, 0) - this->Nominal) / this->Deviation);
  }
};

class UnsignedTableColumnDeviantFunctor : public TableColumnDeviantFunctor
{
public:
  UnsignedTableColumnDeviantFunctor(vtkDataArray* vals, double nominal, double deviation)
  {
    this->Data = vals;
    this->Nominal = nominal;
    this->Deviation = deviation;
  }
  ~UnsignedTableColumnDeviantFunctor() override = default;
  void operator()(vtkDoubleArray* result, vtkIdType id) override
  {
    result->SetNumberOfValues(1);
    result->SetValue(0, fabs(this->Data->GetComponent(id, 0) - this->Nominal) / this->Deviation);
  }
};

//------------------------------------------------------------------------------
void vtkDescriptiveStatistics::SelectAssessFunctor(
  vtkTable* outData, vtkDataObject* inMetaDO, vtkStringArray* rowNames, AssessFunctor*& dfunc)
{
  dfunc = nullptr;
  vtkStatisticalModel* inMeta = vtkStatisticalModel::SafeDownCast(inMetaDO);
  vtkTable* primaryTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Learned, 0) : nullptr;
  vtkTable* derivedTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Derived, 0) : nullptr;
  if (!primaryTab || !derivedTab)
  {
    return;
  }

  vtkIdType nRowPrim = primaryTab->GetNumberOfRows();
  if (nRowPrim != derivedTab->GetNumberOfRows())
  {
    return;
  }

  const auto& varName = rowNames->GetValue(0);

  // Downcast meta columns to string arrays for efficient data access
  vtkStringArray* vars = vtkArrayDownCast<vtkStringArray>(primaryTab->GetColumnByName("Variable"));
  if (!vars)
  {
    return;
  }

  // Loop over primary statistics table until the requested variable is found
  for (int r = 0; r < nRowPrim; ++r)
  {
    if (vars->GetValue(r) == varName)
    {
      // Grab the data for the requested variable
      vtkAbstractArray* arr = outData->GetColumnByName(varName.c_str());
      if (!arr)
      {
        return;
      }

      // For descriptive statistics, type must be convertible to DataArray
      // E.g., StringArrays do not fit here
      vtkDataArray* vals = vtkArrayDownCast<vtkDataArray>(arr);
      if (!vals)
      {
        return;
      }

      // Fetch necessary value from primary model
      double mean = primaryTab->GetValueByName(r, "Mean").ToDouble();

      // Fetch necessary value from derived model
      double stdv = derivedTab->GetValueByName(r, "Standard Deviation").ToDouble();
      // NB: If derived values were specified (and not calculated by Derive)
      //     and are inconsistent, then incorrect assessments will be produced

      if (stdv < VTK_DBL_MIN)
      {
        dfunc = new ZedDeviationDeviantFunctor(vals, mean);
      }
      else
      {
        if (this->GetSignedDeviations())
        {
          dfunc = new SignedTableColumnDeviantFunctor(vals, mean, stdv);
        }
        else
        {
          dfunc = new UnsignedTableColumnDeviantFunctor(vals, mean, stdv);
        }
      }

      return;
    }
  }

  // If arrived here it means that the variable of interest was not found in the parameter table
}

bool vtkDescriptiveStatistics::GetDistributionForField(const std::string& fieldName, double& mean,
  double& variance, double& stdev, double& skewness, double& kurtosis)
{
  auto* model = vtkStatisticalModel::SafeDownCast(this->GetOutputDataObject(OUTPUT_MODEL));
  if (!model)
  {
    return false;
  }
  auto* primary = model->GetTable(vtkStatisticalModel::Learned, 0);
  auto* derived = model->GetTable(vtkStatisticalModel::Derived, 0);
  if (!primary || !derived)
  {
    return false;
  }
  for (vtkIdType ii = 0; ii < primary->GetNumberOfRows(); ++ii)
  {
    if (fieldName == primary->GetValueByName(ii, "Variable").ToString())
    {
      mean = primary->GetValueByName(ii, "Mean").ToDouble();
      variance = derived->GetValueByName(ii, "Variance").ToDouble();
      stdev = derived->GetValueByName(ii, "Standard Deviation").ToDouble();
      skewness = derived->GetValueByName(ii, "Skewness").ToDouble();
      kurtosis = derived->GetValueByName(ii, "Kurtosis").ToDouble();
      return true;
    }
  }
  return false;
}

void vtkDescriptiveStatistics::AppendAlgorithmParameters(std::string& algorithmParameters) const
{
  this->Superclass::AppendAlgorithmParameters(algorithmParameters);
  if (algorithmParameters.back() != '(')
  {
    algorithmParameters += ",";
  }
  // clang-format off
  algorithmParameters +=
    "sample_estimate=" + vtk::to_string(static_cast<int>(this->SampleEstimate)) + ","
    "signed_deviations=" + vtk::to_string(static_cast<int>(this->SignedDeviations))
    ;
  // clang-format on
}

std::size_t vtkDescriptiveStatistics::ConsumeNextAlgorithmParameter(
  vtkStringToken parameterName, const std::string& algorithmParameters)
{
  using namespace vtk::literals;
  int value;
  std::size_t consumed = 0;
  switch (parameterName.GetHash())
  {
    case "sample_estimate"_hash:
      if ((consumed = this->ConsumeInt(algorithmParameters, value)))
      {
        this->SetSampleEstimate(value);
      }
      break;
    case "signed_deviations"_hash:
      if ((consumed = this->ConsumeInt(algorithmParameters, value)))
      {
        this->SetSignedDeviations(value);
      }
      break;
    default:
      consumed =
        this->Superclass::ConsumeNextAlgorithmParameter(parameterName, algorithmParameters);
      break;
  }
  return consumed;
}

VTK_ABI_NAMESPACE_END
