// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAutoCorrelativeStatistics.h"
#include "vtkStatisticsAlgorithmPrivate.h"

#include "vtkDataObjectCollection.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStatisticalModel.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTableFFT.h"
#include "vtkVariantArray.h"

#include <limits>
#include <set>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAutoCorrelativeStatistics);

//------------------------------------------------------------------------------
vtkAutoCorrelativeStatistics::vtkAutoCorrelativeStatistics()
{
  this->AssessNames->SetNumberOfValues(1);
  this->AssessNames->SetValue(0, "d^2"); // Squared Mahalanobis distance

  this->SliceCardinality = 0; // Invalid value by default. Correct value must be specified.
}

//------------------------------------------------------------------------------
vtkAutoCorrelativeStatistics::~vtkAutoCorrelativeStatistics() = default;

//------------------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SliceCardinality: " << this->SliceCardinality << "\n";
}

//------------------------------------------------------------------------------
bool vtkAutoCorrelativeStatistics::Aggregate(
  vtkDataObjectCollection* inMetaColl, vtkStatisticalModel* outMeta)
{
  if (!outMeta)
  {
    return false;
  }
  // We do not call outMeta->Initialize() because outMeta is allowed to be a member of inMetaColl.

  // Get hold of the first model in the collection
  int itemIndex;
  int numItems = inMetaColl->GetNumberOfItems();
  // Skip collection entries until we find one with a primary table:
  vtkStatisticalModel* inMeta0 = nullptr;
  for (itemIndex = 0; itemIndex < numItems; ++itemIndex)
  {
    // Verify that the first primary statistics are present
    inMeta0 = vtkStatisticalModel::SafeDownCast(inMetaColl->GetItemAsObject(itemIndex));
    auto* primaryTab = inMeta0 ? inMeta0->GetTable(vtkStatisticalModel::Learned, 0) : nullptr;
    if (primaryTab)
    {
      ++itemIndex;
      break;
    }
  }
  if (itemIndex >= numItems)
  {
    // No models to aggregate. Leave the output model in an empty state.
    return true;
  }

  // Iterate over variable partitions.
  // Each model is a set of tables in the same order; one per variable.
  int nParts = inMeta0->GetNumberOfTables(vtkStatisticalModel::Learned);
  outMeta->SetNumberOfTables(vtkStatisticalModel::Learned, nParts);
  for (int b = 0; b < nParts; ++b)
  {
    auto* currentTab = inMeta0->GetTable(vtkStatisticalModel::Learned, b);
    if (!currentTab)
    {
      // Model is empty.
      continue;
    }

    // Skip FFT partition if already present in the model
    auto varName = inMeta0->GetTableName(vtkStatisticalModel::Learned, b);
    if (varName == "Autocorrelation FFT")
    {
      continue;
    }

    vtkIdType nRow = currentTab->GetNumberOfRows();
    if (!nRow)
    {
      // No statistics were calculated.
      continue;
    }

    // Use this first model to initialize the aggregated one
    auto aggregatedTab = vtkSmartPointer<vtkTable>::New();
    aggregatedTab->DeepCopy(currentTab);

    // Now, loop over all remaining models and update aggregated each time
    for (int ii = itemIndex; ii < numItems; ++ii)
    {
      auto* inMeta = vtkStatisticalModel::SafeDownCast(inMetaColl->GetItemAsObject(ii));
      if (!inMeta)
      {
        continue;
      }

      // Verify that the current model is indeed contained in a table
      currentTab = inMeta->GetTable(vtkStatisticalModel::Learned, b);
      if (!currentTab)
      {
        vtkWarningMacro("Model " << ii << "'s " << b << "-th table is null. Skipping.");
        continue;
      }

      if (currentTab->GetNumberOfRows() != nRow)
      {
        // Models do not match
        vtkWarningMacro("Model " << b << " has mismatched number of rows. Skipping.");
        continue;
      }

      // Iterate over all model rows
      for (int r = 0; r < nRow; ++r)
      {
        // Verify that variable names match each other
        if (currentTab->GetValueByName(r, "Variable") !=
          aggregatedTab->GetValueByName(r, "Variable"))
        {
          // Models do not match
          vtkErrorMacro("Model has mismatched variables. Skipping.");
          continue;
        }

        // Get aggregated statistics
        int n = aggregatedTab->GetValueByName(r, "Cardinality").ToInt();
        double meanXs = aggregatedTab->GetValueByName(r, "Mean Xs").ToDouble();
        double meanXt = aggregatedTab->GetValueByName(r, "Mean Xt").ToDouble();
        double M2Xs = aggregatedTab->GetValueByName(r, "M2 Xs").ToDouble();
        double M2Xt = aggregatedTab->GetValueByName(r, "M2 Xt").ToDouble();
        double MXsXt = aggregatedTab->GetValueByName(r, "M XsXt").ToDouble();

        // Get current model statistics
        int n_c = currentTab->GetValueByName(r, "Cardinality").ToInt();
        double meanXs_c = currentTab->GetValueByName(r, "Mean Xs").ToDouble();
        double meanXt_c = currentTab->GetValueByName(r, "Mean Xt").ToDouble();
        double M2Xs_c = currentTab->GetValueByName(r, "M2 Xs").ToDouble();
        double M2Xt_c = currentTab->GetValueByName(r, "M2 Xt").ToDouble();
        double MXsXt_c = currentTab->GetValueByName(r, "M XsXt").ToDouble();

        // Update global statistics
        int N = n + n_c;

        double invN = 1. / static_cast<double>(N);

        double deltaXs = meanXs_c - meanXs;
        double deltaXs_sur_N = deltaXs * invN;

        double deltaXt = meanXt_c - meanXt;
        double deltaXt_sur_N = deltaXt * invN;

        int prod_n = n * n_c;

        M2Xs += M2Xs_c + prod_n * deltaXs * deltaXs_sur_N;

        M2Xt += M2Xt_c + prod_n * deltaXt * deltaXt_sur_N;

        MXsXt += MXsXt_c + prod_n * deltaXs * deltaXt_sur_N;

        meanXs += n_c * deltaXs_sur_N;

        meanXt += n_c * deltaXt_sur_N;

        // Store updated model
        aggregatedTab->SetValueByName(r, "Cardinality", N);
        aggregatedTab->SetValueByName(r, "Mean Xs", meanXs);
        aggregatedTab->SetValueByName(r, "Mean Xt", meanXt);
        aggregatedTab->SetValueByName(r, "M2 Xs", M2Xs);
        aggregatedTab->SetValueByName(r, "M2 Xt", M2Xt);
        aggregatedTab->SetValueByName(r, "M XsXt", MXsXt);
      } // r
    }

    // Replace initial meta with aggregated table for current variable
    //    const char* varName = inMeta->GetMetaData( b )->Get( vtkCompositeDataSet::NAME() );
    outMeta->SetTable(vtkStatisticalModel::Learned, b, aggregatedTab, varName);
  } // b
  return true;
}

//------------------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::Learn(
  vtkTable* inData, vtkTable* inPara, vtkStatisticalModel* outMeta)
{
  if (!inData || !inPara || !outMeta)
  {
    return;
  }

  outMeta->Initialize();
  outMeta->SetAlgorithmParameters(this->GetAlgorithmParameters());

  // Verify that a cardinality was specified for the time slices
  if (!this->SliceCardinality)
  {
    vtkErrorMacro("No time slice cardinality was set. Cannot calculate model.");
    return;
  }

  // Process lparameter table and determine maximum time lag
  vtkIdType nRowPara = inPara->GetNumberOfRows();
  vtkIdType maxLag = 0;
  for (vtkIdType p = 0; p < nRowPara; ++p)
  {
    vtkIdType lag = inPara->GetValue(p, 0).ToInt();
    maxLag = std::max(lag, maxLag);
  } // p

  // Verify that a slice cardinality, maximum lag, and data size are consistent
  vtkIdType nRowData = inData->GetNumberOfRows();
  vtkIdType quo = nRowData / this->SliceCardinality;
  if (maxLag >= quo || nRowData != quo * this->SliceCardinality)
  {
    vtkErrorMacro("Incorrect specification of time slice cardinality: "
      << this->SliceCardinality << " with maximum time lag " << maxLag
      << " and data set cardinality " << nRowData << ". Exiting.");
    return;
  }

  // Rows of the model tables have 6 primary statistics
  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues(7);

  // Loop over requests
  int nParts = 0;
  outMeta->SetNumberOfTables(
    vtkStatisticalModel::Learned, static_cast<int>(this->Internals->Requests.size()));
  for (std::set<std::set<vtkStdString>>::const_iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit, ++nParts)
  {
    // Each request contains only one column of interest (if there are others, they are ignored)
    std::set<vtkStdString>::const_iterator it = rit->begin();
    std::string const& varName = *it;
    if (!inData->GetColumnByName(varName.c_str()))
    {
      vtkWarningMacro("InData table does not have a column " << varName << ". Ignoring it.");
      continue;
    }

    // Create primary statistics table for this variable
    vtkTable* modelTab = vtkTable::New();

    vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
    idTypeCol->SetName("Time Lag");
    modelTab->AddColumn(idTypeCol);
    idTypeCol->Delete();

    idTypeCol = vtkIdTypeArray::New();
    idTypeCol->SetName("Cardinality");
    modelTab->AddColumn(idTypeCol);
    idTypeCol->Delete();

    vtkDoubleArray* doubleCol = vtkDoubleArray::New();
    doubleCol->SetName("Mean Xs");
    modelTab->AddColumn(doubleCol);
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName("Mean Xt");
    modelTab->AddColumn(doubleCol);
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName("M2 Xs");
    modelTab->AddColumn(doubleCol);
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName("M2 Xt");
    modelTab->AddColumn(doubleCol);
    doubleCol->Delete();

    doubleCol = vtkDoubleArray::New();
    doubleCol->SetName("M XsXt");
    modelTab->AddColumn(doubleCol);
    doubleCol->Delete();

    // Loop over parameter table
    for (vtkIdType p = 0; p < nRowPara; ++p)
    {
      double meanXs = 0.;
      double meanXt = 0.;
      double mom2Xs = 0.;
      double mom2Xt = 0.;
      double momXsXt = 0.;

      // Retrieve current time lag
      vtkIdType lag = inPara->GetValue(p, 0).ToInt();

      // Offset into input data table for current time lag
      vtkIdType rowOffset = lag * this->SliceCardinality;

      // Calculate primary statistics
      double inv_n, xs, xt, delta, deltaXsn;
      for (vtkIdType r = 0; r < this->SliceCardinality; ++r)
      {
        inv_n = 1. / (r + 1.);

        xs = inData->GetValueByName(r, varName.c_str()).ToDouble();
        delta = xs - meanXs;
        meanXs += delta * inv_n;
        deltaXsn = xs - meanXs;
        mom2Xs += delta * deltaXsn;

        xt = inData->GetValueByName(r + rowOffset, varName.c_str()).ToDouble();
        delta = xt - meanXt;
        meanXt += delta * inv_n;
        mom2Xt += delta * (xt - meanXt);

        momXsXt += delta * deltaXsn;
      }

      // Store primary statistics
      row->SetValue(0, lag);
      row->SetValue(1, this->SliceCardinality);
      row->SetValue(2, meanXs);
      row->SetValue(3, meanXt);
      row->SetValue(4, mom2Xs);
      row->SetValue(5, mom2Xt);
      row->SetValue(6, momXsXt);
      modelTab->InsertNextRow(row);
    } // p

    // Resize output meta and append model table for current variable
    outMeta->SetTable(vtkStatisticalModel::Learned, nParts, modelTab, varName);

    // Clean up
    modelTab->Delete();
  } // rit

  // Clean up
  row->Delete();
}

//------------------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::Derive(vtkStatisticalModel* inMeta)
{
  if (!inMeta || inMeta->GetNumberOfTables(vtkStatisticalModel::Learned) < 1)
  {
    return;
  }

  // Storage for time series table
  vtkTable* timeTable = vtkTable::New();

  // Iterate over variable partitions
  vtkIdType nLags = 0;
  int nParts = inMeta->GetNumberOfTables(vtkStatisticalModel::Learned);
  for (int b = 0; b < nParts; ++b)
  {
    vtkTable* modelTab = inMeta->GetTable(vtkStatisticalModel::Learned, b);
    if (!modelTab)
    {
      continue;
    }

    // Verify that number of time lags is consistent
    std::string varName = inMeta->GetTableName(vtkStatisticalModel::Learned, b);
    vtkIdType nRow = modelTab->GetNumberOfRows();
    if (b)
    {
      if (nRow != nLags)
      {
        vtkErrorMacro("Variable " << varName << " has " << nRow << " time lags but should have "
                                  << nLags << ". Exiting.");
        return;
      }
    }
    else // if ( b )
    {
      nLags = nRow;
    }
    if (!nRow)
    {
      continue;
    }

    int numDerived = 9;
    std::string derivedNames[] = { "Variance Xs", "Variance Xt", "Covariance", "Determinant",
      "Slope Xt/Xs", "Intercept Xt/Xs", "Slope Xs/Xt", "Intercept Xs/Xt", "Autocorrelation" };

    // Find or create columns for derived statistics
    vtkDoubleArray* derivedCol;
    for (int j = 0; j < numDerived; ++j)
    {
      if (!modelTab->GetColumnByName(derivedNames[j].c_str()))
      {
        derivedCol = vtkDoubleArray::New();
        derivedCol->SetName(derivedNames[j].c_str());
        derivedCol->SetNumberOfTuples(nRow);
        modelTab->AddColumn(derivedCol);
        derivedCol->Delete();
      }
    }

    // Storage for derived values
    double* derivedVals = new double[numDerived];
    vtkDoubleArray* timeArray = vtkDoubleArray::New();
    timeArray->SetName(varName.c_str());

    for (int i = 0; i < nRow; ++i)
    {
      double m2Xs = modelTab->GetValueByName(i, "M2 Xs").ToDouble();
      double m2Xt = modelTab->GetValueByName(i, "M2 Xt").ToDouble();
      double mXsXt = modelTab->GetValueByName(i, "M XsXt").ToDouble();

      double varXs, varXt, covXsXt;
      int numSamples = modelTab->GetValueByName(i, "Cardinality").ToInt();
      if (numSamples == 1)
      {
        varXs = 0.;
        varXt = 0.;
        covXsXt = 0.;
      }
      else
      {
        double inv_nm1;
        double n = static_cast<double>(numSamples);
        inv_nm1 = 1. / (n - 1.);
        varXs = m2Xs * inv_nm1;
        varXt = m2Xt * inv_nm1;
        covXsXt = mXsXt * inv_nm1;
      }

      // Store derived values
      derivedVals[0] = varXs;
      derivedVals[1] = varXt;
      derivedVals[2] = covXsXt;
      derivedVals[3] = varXs * varXt - covXsXt * covXsXt;

      // There will be NaN values in linear regression if covariance matrix is not positive definite
      double meanXs = modelTab->GetValueByName(i, "Mean Xs").ToDouble();
      double meanXt = modelTab->GetValueByName(i, "Mean Xt").ToDouble();

      // variable Xt on variable Xs:
      //   slope (explicitly handle degenerate cases)
      if (varXs < VTK_DBL_MIN)
      {
        derivedVals[4] = vtkMath::Nan();
      }
      else
      {
        derivedVals[4] = covXsXt / varXs;
      }
      //   intercept
      derivedVals[5] = meanXt - derivedVals[4] * meanXs;

      // variable Xs on variable Xt:
      //   slope (explicitly handle degenerate cases)
      if (varXt < VTK_DBL_MIN)
      {
        derivedVals[6] = vtkMath::Nan();
      }
      else
      {
        derivedVals[6] = covXsXt / varXt;
      }
      //   intercept
      derivedVals[7] = meanXs - derivedVals[6] * meanXt;

      // correlation coefficient (be consistent with degenerate cases detected above)
      if (varXs < VTK_DBL_MIN || varXt < VTK_DBL_MIN)
      {
        derivedVals[8] = vtkMath::Nan();
      }
      else
      {
        derivedVals[8] = covXsXt / sqrt(varXs * varXt);
      }

      // Update time series array
      timeArray->InsertNextValue(derivedVals[8]);

      for (int j = 0; j < numDerived; ++j)
      {
        modelTab->SetValueByName(i, derivedNames[j].c_str(), derivedVals[j]);
      }
    } // nRow

    // Append correlation coefficient to time series table
    timeTable->AddColumn(timeArray);

    // Clean up
    delete[] derivedVals;
    timeArray->Delete();
  } // for ( unsigned int b = 0; b < nParts; ++ b )

  // Now calculate FFT of time series
  vtkTableFFT* fft = vtkTableFFT::New();
  fft->SetInputData(timeTable);
  vtkTable* outt = fft->GetOutput();
  fft->Update();

  // Set auto-correlation FFT table
  inMeta->SetNumberOfTables(vtkStatisticalModel::Derived, 1);
  inMeta->SetTable(vtkStatisticalModel::Derived, 0, outt, "Autocorrelation FFT");
  inMeta->SetAlgorithmParameters(this->GetAlgorithmParameters());

  // Clean up
  fft->Delete();
  timeTable->Delete();
}

//------------------------------------------------------------------------------
// Use the invalid value of -1 for p-values since R is absent
vtkDoubleArray* vtkAutoCorrelativeStatistics::CalculatePValues(vtkDoubleArray* statCol)
{
  // A column must be created first
  vtkDoubleArray* testCol = vtkDoubleArray::New();

  // Fill this column
  vtkIdType n = statCol->GetNumberOfTuples();
  testCol->SetNumberOfTuples(n);
  testCol->FillComponent(0, -1);

  return testCol;
}

//------------------------------------------------------------------------------
void vtkAutoCorrelativeStatistics::SelectAssessFunctor(
  vtkTable* outData, vtkDataObject* inMetaDO, vtkStringArray* rowNames, AssessFunctor*& dfunc)
{
  dfunc = nullptr;
  vtkStatisticalModel* inMeta = vtkStatisticalModel::SafeDownCast(inMetaDO);
  vtkTable* modelTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Learned, 0) : nullptr;
  vtkTable* derivedTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Derived, 0) : nullptr;
  if (!modelTab || !derivedTab)
  {
    return;
  }

  vtkIdType nRowPrim = modelTab->GetNumberOfRows();
  if (nRowPrim != derivedTab->GetNumberOfRows())
  {
    return;
  }

  const auto& varName = rowNames->GetValue(0);

  // Downcast meta columns to string arrays for efficient data access
  vtkStringArray* vars = vtkArrayDownCast<vtkStringArray>(modelTab->GetColumnByName("Variable"));
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

      // For auto-correlative statistics, type must be convertible to DataArray
      // E.g., StringArrays do not fit here
      vtkDataArray* vals = vtkArrayDownCast<vtkDataArray>(arr);
      if (!vals)
      {
        return;
      }

      // FIXME: Fetch necessary values here and do something with them

      return;
    }
  }

  // If arrived here it means that the variable of interest was not found in the parameter table
}
VTK_ABI_NAMESPACE_END
