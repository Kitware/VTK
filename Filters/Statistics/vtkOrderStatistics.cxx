// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkOrderStatistics.h"
#include "vtkStatisticsAlgorithmPrivate.h"

#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStatisticalModel.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkStringToken.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariantArray.h"

#include <cmath>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOrderStatistics);

//------------------------------------------------------------------------------
vtkOrderStatistics::vtkOrderStatistics()
{
  this->QuantileDefinition = vtkOrderStatistics::InverseCDFAveragedSteps;
  this->NumberOfIntervals = 4;       // By default, calculate 5-points statistics
  this->Quantize = false;            // By default, do not force quantization
  this->MaximumHistogramSize = 1000; // A large value by default
  // Number of primary tables is variable
  this->NumberOfPrimaryTables = -1;

  this->AssessNames->SetNumberOfValues(1);
  this->AssessNames->SetValue(0, "Quantile");
}

//------------------------------------------------------------------------------
vtkOrderStatistics::~vtkOrderStatistics() = default;

//------------------------------------------------------------------------------
void vtkOrderStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfIntervals: " << this->NumberOfIntervals << endl;
  os << indent << "QuantileDefinition: " << this->QuantileDefinition << endl;
  os << indent << "Quantize: " << this->Quantize << endl;
  os << indent << "MaximumHistogramSize: " << this->MaximumHistogramSize << endl;
}

//------------------------------------------------------------------------------
void vtkOrderStatistics::AppendAlgorithmParameters(std::string& algorithmParameters) const
{
  this->Superclass::AppendAlgorithmParameters(algorithmParameters);
  if (algorithmParameters.back() != '(')
  {
    algorithmParameters += ",";
  }
  // clang-format off
  algorithmParameters +=
    "number_of_intervals=" + vtk::to_string(this->NumberOfIntervals) + ","
    "quantile_definition=" + vtk::to_string(static_cast<int>(this->QuantileDefinition)) + ","
    "quantize=" + vtk::to_string(static_cast<int>(this->Quantize)) + ","
    "maximum_histogram_size=" + vtk::to_string(this->MaximumHistogramSize)
    ;
  // clang-format on
}

//------------------------------------------------------------------------------
std::size_t vtkOrderStatistics::ConsumeNextAlgorithmParameter(
  vtkStringToken parameterName, const std::string& algorithmParameters)
{
  using namespace vtk::literals;
  std::size_t consumed = 0;
  switch (parameterName.GetHash())
  {
    case "number_of_intervals"_hash:
    {
      int value;
      if ((consumed = this->ConsumeInt(algorithmParameters, value)))
      {
        this->SetNumberOfIntervals(value);
      }
    }
    break;
    case "quantile_definition"_hash:
    {
      int value;
      if ((consumed = this->ConsumeInt(algorithmParameters, value)))
      {
        this->SetQuantileDefinition(value);
      }
    }
    break;
    case "quantize"_hash:
    {
      int value;
      if ((consumed = this->ConsumeInt(algorithmParameters, value)))
      {
        this->SetQuantize(value);
      }
    }
    break;
    case "maximum_histogram_size"_hash:
    {
      int value;
      if ((consumed = this->ConsumeInt(algorithmParameters, value)))
      {
        this->SetMaximumHistogramSize(value);
      }
    }
    break;
    default:
      consumed =
        this->Superclass::ConsumeNextAlgorithmParameter(parameterName, algorithmParameters);
      break;
  }
  return consumed;
}

//------------------------------------------------------------------------------
void vtkOrderStatistics::SetQuantileDefinition(int qd)
{
  switch (qd)
  {
    case vtkOrderStatistics::InverseCDF:
      break;
    case vtkOrderStatistics::InverseCDFAveragedSteps:
      break;
    default:
      vtkWarningMacro("Incorrect type of quantile definition: " << qd << ". Ignoring it.");
      return;
  }

  this->QuantileDefinition = static_cast<vtkOrderStatistics::QuantileDefinitionType>(qd);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkOrderStatistics::SetParameter(
  const char* parameter, int vtkNotUsed(index), vtkVariant value)
{
  if (!strcmp(parameter, "NumberOfIntervals"))
  {
    this->SetNumberOfIntervals(value.ToInt());

    return true;
  }

  if (!strcmp(parameter, "QuantileDefinition"))
  {
    this->SetQuantileDefinition(value.ToInt());

    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
void vtkOrderStatistics::Learn(
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
  outMeta->SetAlgorithmParameters(this->GetAlgorithmParameters());
  // Store 1 "learned" model table per request
  outMeta->SetNumberOfTables(
    vtkStatisticalModel::Learned, static_cast<int>(this->Internals->Requests.size()));

  vtkUnsignedCharArray* ghosts = inData->GetRowData()->GetGhostArray();

  // Loop over requests
  vtkIdType nRow = inData->GetNumberOfRows();
  int requestIndex = 0;
  for (std::set<std::set<vtkStdString>>::iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit, ++requestIndex)
  {
    // Each request contains only one column of interest (if there are others, they are ignored)
    std::set<vtkStdString>::const_iterator it = rit->begin();
    std::string const& col = *it;
    if (!inData->GetColumnByName(col.c_str()))
    {
      vtkWarningMacro("InData table does not have a column " << col << ". Ignoring it.");
      continue;
    }

    // Get hold of data for this variable
    vtkAbstractArray* vals = inData->GetColumnByName(col.c_str());

    // Create histogram table for this variable
    vtkTable* histogramTab = vtkTable::New();

    // Row to be used to insert into histogram table
    vtkVariantArray* row = vtkVariantArray::New();
    row->SetNumberOfValues(2);

    // Switch depending on data type
    if (vals->IsA("vtkDataArray"))
    {
      vtkDoubleArray* doubleCol = vtkDoubleArray::New();
      doubleCol->SetName("Value");
      histogramTab->AddColumn(doubleCol);
      doubleCol->Delete();
    }
    else if (vals->IsA("vtkStringArray"))
    {
      vtkStringArray* stringCol = vtkStringArray::New();
      stringCol->SetName("Value");
      histogramTab->AddColumn(stringCol);
      stringCol->Delete();
    }
    else if (vals->IsA("vtkVariantArray"))
    {
      vtkVariantArray* variantCol = vtkVariantArray::New();
      variantCol->SetName("Value");
      histogramTab->AddColumn(variantCol);
      variantCol->Delete();
    }
    else
    {
      vtkWarningMacro("Unsupported data type for column " << col << ". Ignoring it.");

      continue;
    }

    vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
    idTypeCol->SetName("Cardinality");
    histogramTab->AddColumn(idTypeCol);
    idTypeCol->Delete();

    // Switch depending on data type
    if (vals->IsA("vtkDataArray"))
    {
      // Downcast column to data array for efficient data access
      vtkDataArray* dvals = vtkArrayDownCast<vtkDataArray>(vals);

      // Calculate histogram
      std::map<double, vtkIdType> histogram;
      for (vtkIdType r = 0; r < nRow; ++r)
      {
        if (!ghosts || !(ghosts->GetValue(r) & this->GhostsToSkip))
        {
          ++histogram[dvals->GetTuple1(r)];
        }
      }

      // If maximum size was requested, make sure it is satisfied
      if (this->Quantize)
      {
        // Retrieve achieved histogram size
        vtkIdType Nq = static_cast<vtkIdType>(histogram.size());

        // If histogram is too big, quantization will have to occur
        while (Nq > this->MaximumHistogramSize)
        {
          // Retrieve extremal values
          double mini = histogram.begin()->first;
          double maxi = histogram.rbegin()->first;

          // Create bucket width based on target histogram size
          // FIXME: .5 is arbitrary at this point
          double width = (maxi - mini) / std::round(Nq / 2.);

          // Now re-calculate histogram by quantizing values
          histogram.clear();
          double reading;
          double quantum;
          for (vtkIdType r = 0; r < nRow; ++r)
          {
            if (ghosts && (ghosts->GetValue(r) & this->GhostsToSkip))
            {
              continue;
            }
            reading = dvals->GetTuple1(r);
            quantum = mini + std::round((reading - mini) / width) * width;
            ++histogram[quantum];
          }

          // Update histogram size for conditional clause
          Nq = static_cast<vtkIdType>(histogram.size());
        }
      }

      // Store histogram
      for (std::map<double, vtkIdType>::iterator mit = histogram.begin(); mit != histogram.end();
           ++mit)
      {
        row->SetValue(0, mit->first);
        row->SetValue(1, mit->second);
        histogramTab->InsertNextRow(row);
      }
    } // if ( vals->IsA("vtkDataArray") )
    else if (vals->IsA("vtkStringArray"))
    {
      // Downcast column to string array for efficient data access
      vtkStringArray* svals = vtkArrayDownCast<vtkStringArray>(vals);

      // Calculate histogram
      std::map<vtkStdString, vtkIdType> histogram;
      for (vtkIdType r = 0; r < nRow; ++r)
      {
        if (!ghosts || !(ghosts->GetValue(r) & this->GhostsToSkip))
        {
          ++histogram[svals->GetValue(r)];
        }
      }

      // Store histogram
      for (std::map<vtkStdString, vtkIdType>::iterator mit = histogram.begin();
           mit != histogram.end(); ++mit)
      {
        row->SetValue(0, mit->first);
        row->SetValue(1, mit->second);
        histogramTab->InsertNextRow(row);
      }
    } // else if ( vals->IsA("vtkStringArray") )
    else if (vals->IsA("vtkVariantArray"))
    {
      // Downcast column to variant array for efficient data access
      vtkVariantArray* vvals = vtkArrayDownCast<vtkVariantArray>(vals);

      // Calculate histogram
      std::map<vtkVariant, vtkIdType> histogram;
      for (vtkIdType r = 0; r < nRow; ++r)
      {
        if (!ghosts || !(ghosts->GetValue(r) & this->GhostsToSkip))
        {
          ++histogram[vvals->GetVariantValue(r)];
        }
      }

      // Store histogram
      for (std::map<vtkVariant, vtkIdType>::iterator mit = histogram.begin();
           mit != histogram.end(); ++mit)
      {
        row->SetValue(0, mit->first);
        row->SetValue(1, mit->second);
        histogramTab->InsertNextRow(row);
      }
    } // else if ( vals->IsA("vtkVariantArray") )
    else
    {
      vtkWarningMacro("Unsupported data type for column " << col << ". Ignoring it.");

      continue;
    } // else

    // Add the histogram to the output model:
    outMeta->SetTable(vtkStatisticalModel::Learned, requestIndex, histogramTab, col);

    // Clean up
    histogramTab->Delete();
    row->Delete();
  } // rit
}

//------------------------------------------------------------------------------
void vtkOrderStatistics::Derive(vtkStatisticalModel* inMeta)
{
  if (!inMeta || inMeta->GetNumberOfTables(vtkStatisticalModel::Learned) < 1)
  {
    return;
  }
  inMeta->SetAlgorithmParameters(this->GetAlgorithmParameters());
  inMeta->SetNumberOfTables(vtkStatisticalModel::Derived, 2);

  // Create cardinality table
  vtkTable* cardinalityTab = vtkTable::New();

  vtkStringArray* stringCol = vtkStringArray::New();
  stringCol->SetName("Variable");
  cardinalityTab->AddColumn(stringCol);
  stringCol->Delete();

  vtkIdTypeArray* idTypeCol = vtkIdTypeArray::New();
  idTypeCol->SetName("Cardinality");
  cardinalityTab->AddColumn(idTypeCol);
  idTypeCol->Delete();

  // Create quantile table
  vtkTable* quantileTab = vtkTable::New();

  stringCol = vtkStringArray::New();
  stringCol->SetName("Quantile");
  quantileTab->AddColumn(stringCol);
  stringCol->Delete();

  double dq = 1. / static_cast<double>(this->NumberOfIntervals);
  for (vtkIdType i = 0; i <= this->NumberOfIntervals; ++i)
  {

    // Handle special case of quartiles and median for convenience
    ldiv_t q = ldiv(i << 2, this->NumberOfIntervals);
    if (q.rem)
    {
      // General case
      stringCol->InsertNextValue((vtkVariant(i * dq).ToString() + "-quantile").c_str());
    }
    else
    {
      // Case where q is a multiple of 4
      switch (q.quot)
      {
        case 0:
          stringCol->InsertNextValue("Minimum");
          break;
        case 1:
          stringCol->InsertNextValue("First Quartile");
          break;
        case 2:
          stringCol->InsertNextValue("Median");
          break;
        case 3:
          stringCol->InsertNextValue("Third Quartile");
          break;
        case 4:
          stringCol->InsertNextValue("Maximum");
          break;
        default:
          stringCol->InsertNextValue((vtkVariant(i * dq).ToString() + "-quantile").c_str());
          break;
      }
    }
  }

  // Prepare row for insertion into cardinality table
  vtkVariantArray* row = vtkVariantArray::New();
  row->SetNumberOfValues(2);

  // Iterate over primary tables
  int nParts = inMeta->GetNumberOfTables(vtkStatisticalModel::Learned);
  for (int b = 0; b < nParts; ++b)
  {
    vtkTable* histogramTab = inMeta->GetTable(vtkStatisticalModel::Learned, b);
    if (!histogramTab)
    {
      continue;
    }

    // Downcast columns to typed arrays for efficient data access
    vtkAbstractArray* vals = histogramTab->GetColumnByName("Value");
    vtkIdTypeArray* card =
      vtkArrayDownCast<vtkIdTypeArray>(histogramTab->GetColumnByName("Cardinality"));

    // The CDF will be used for quantiles calculation (effectively as a reverse look-up table)
    vtkIdType nRowHist = histogramTab->GetNumberOfRows();
    std::vector<vtkIdType> cdf(nRowHist);

    // Calculate variable cardinality and CDF
    vtkIdType c;
    vtkIdType n = 0;
    for (vtkIdType r = 0; r < nRowHist; ++r)
    {
      // Update cardinality and CDF
      c = card->GetValue(r);
      n += c;
      cdf[r] = n;
    }

    // Get partition variable name
    std::string varName = inMeta->GetTableName(vtkStatisticalModel::Learned, b);

    // Store cardinality
    row->SetValue(0, varName.c_str());
    row->SetValue(1, n);
    cardinalityTab->InsertNextRow(row);

    // Find or create column of probability mass function of histogram table
    std::string probaName("P");
    vtkDoubleArray* probaCol;
    vtkAbstractArray* abstrCol = histogramTab->GetColumnByName(probaName.c_str());
    if (!abstrCol)
    {
      probaCol = vtkDoubleArray::New();
      probaCol->SetName(probaName.c_str());
      probaCol->SetNumberOfTuples(nRowHist);
      histogramTab->AddColumn(probaCol);
      probaCol->Delete();
    }
    else
    {
      probaCol = vtkArrayDownCast<vtkDoubleArray>(abstrCol);
    }

    // Finally calculate and store probabilities
    double inv_n = 1. / n;
    double p;
    for (vtkIdType r = 0; r < nRowHist; ++r)
    {
      c = card->GetValue(r);
      p = inv_n * c;

      probaCol->SetValue(r, p);
    }

    // Storage for quantile indices
    std::vector<std::pair<vtkIdType, vtkIdType>> quantileIndices;
    std::pair<vtkIdType, vtkIdType> qIdxPair;

    // First quantile index is always 0 with no jump (corresponding to the first and the smallest
    // value)
    qIdxPair.first = 0;
    qIdxPair.second = 0;
    quantileIndices.push_back(qIdxPair);

    // Calculate all interior quantiles (i.e. for 0 < k < q)
    vtkIdType rank = 0;
    double dh = n / static_cast<double>(this->NumberOfIntervals);
    for (vtkIdType k = 1; k < this->NumberOfIntervals; ++k)
    {
      // Calculate np value
      double np = k * dh;

      // Calculate first quantile index
      vtkIdType qIdx1;
      if (this->QuantileDefinition == vtkOrderStatistics::InverseCDFAveragedSteps)
      {
        qIdx1 = static_cast<vtkIdType>(std::round(np));
      }
      else
      {
        qIdx1 = static_cast<vtkIdType>(ceil(np));
      }

      // Find rank of the entry where first quantile index is reached using the CDF
      while (qIdx1 > cdf[rank])
      {
        ++rank;

        if (rank >= nRowHist)
        {
          vtkErrorMacro("Inconsistent quantile table: at last rank "
            << rank << " the CDF is  " << cdf[rank - 1] << " < " << qIdx1
            << " the quantile index. Cannot derive model.");
          return;
        }
      }

      // Store rank in histogram of first quantile index
      qIdxPair.first = rank;

      // Decide whether midpoint interpolation will be used for this numeric type input
      if (this->QuantileDefinition == vtkOrderStatistics::InverseCDFAveragedSteps)
      {
        // Calculate second quantile index for mid-point interpolation
        vtkIdType qIdx2 = static_cast<vtkIdType>(floor(np + 1.));

        // If the two quantile indices differ find rank where second is reached
        if (qIdx1 != qIdx2)
        {
          while (qIdx2 > cdf[rank])
          {
            ++rank;

            if (rank >= nRowHist)
            {
              vtkErrorMacro("Inconsistent quantile table: at last rank "
                << rank << " the CDF is  " << cdf[rank - 1] << " < " << qIdx2
                << " the quantile index. Cannot derive model.");
              return;
            }
          }
        }
      } // if ( this->QuantileDefinition == vtkOrderStatistics::InverseCDFAveragedSteps )

      // Store rank in histogram of second quantile index
      qIdxPair.second = rank;

      // Store pair of ranks
      quantileIndices.push_back(qIdxPair);
    }

    // Last quantile index is always cardinality with no jump (corresponding to the last and thus
    // largest value)
    qIdxPair.first = nRowHist - 1;
    qIdxPair.second = nRowHist - 1;
    quantileIndices.push_back(qIdxPair);

    // Finally prepare quantile values column depending on data type
    if (vals->IsA("vtkDataArray"))
    {
      // Downcast column to data array for efficient data access
      vtkDataArray* dvals = vtkArrayDownCast<vtkDataArray>(vals);

      // Create column for quantiles of the same type as the values
      vtkDataArray* quantCol = vtkDataArray::CreateDataArray(dvals->GetDataType());
      quantCol->SetName(varName.c_str());
      quantCol->SetNumberOfTuples(this->NumberOfIntervals + 1);
      quantileTab->AddColumn(quantCol);
      quantCol->Delete();

      // Decide whether midpoint interpolation will be used for this numeric type input
      if (this->QuantileDefinition == vtkOrderStatistics::InverseCDFAveragedSteps)
      {
        // Compute and store quantile values
        vtkIdType k = 0;
        for (std::vector<std::pair<vtkIdType, vtkIdType>>::iterator qit = quantileIndices.begin();
             qit != quantileIndices.end(); ++qit, ++k)
        {
          // Retrieve data values from rank into histogram and interpolate
          double Qp = .5 * (dvals->GetTuple1(qit->first) + dvals->GetTuple1(qit->second));

          // Store quantile value
          quantCol->SetTuple1(k, Qp);
        } // qit
      }
      else // if ( this->QuantileDefinition == vtkOrderStatistics::InverseCDFAveragedSteps )
      {
        // Compute and store quantile values
        vtkIdType k = 0;
        for (std::vector<std::pair<vtkIdType, vtkIdType>>::iterator qit = quantileIndices.begin();
             qit != quantileIndices.end(); ++qit, ++k)
        {
          // Retrieve data value from rank into histogram
          double Qp = dvals->GetTuple1(qit->first);

          // Store quantile value
          quantCol->SetTuple1(k, Qp);
        } // qit
      }   // else ( this->QuantileDefinition == vtkOrderStatistics::InverseCDFAveragedSteps )
    }     // if ( vals->IsA("vtkDataArray") )
    else if (vals->IsA("vtkStringArray"))
    {
      // Downcast column to string array for efficient data access
      vtkStringArray* svals = vtkArrayDownCast<vtkStringArray>(vals);

      // Create column for quantiles of the same type as the values
      vtkStringArray* quantCol = vtkStringArray::New();
      quantCol->SetName(varName.c_str());
      quantCol->SetNumberOfTuples(this->NumberOfIntervals + 1);
      quantileTab->AddColumn(quantCol);
      quantCol->Delete();

      // Compute and store quantile values
      vtkIdType k = 0;
      for (std::vector<std::pair<vtkIdType, vtkIdType>>::iterator qit = quantileIndices.begin();
           qit != quantileIndices.end(); ++qit, ++k)
      {
        // Retrieve data value from rank into histogram
        std::string Qp = svals->GetValue(qit->first);

        // Store quantile value
        quantCol->SetValue(k, Qp);
      }
    } // else if ( vals->IsA("vtkStringArray") )
    else if (vals->IsA("vtkVariantArray"))
    {
      // Downcast column to variant array for efficient data access
      vtkVariantArray* vvals = vtkArrayDownCast<vtkVariantArray>(vals);

      // Create column for quantiles of the same type as the values
      vtkVariantArray* quantCol = vtkVariantArray::New();
      quantCol->SetName(varName.c_str());
      quantCol->SetNumberOfTuples(this->NumberOfIntervals + 1);
      quantileTab->AddColumn(quantCol);
      quantCol->Delete();

      // Compute and store quantile values
      vtkIdType k = 0;
      for (std::vector<std::pair<vtkIdType, vtkIdType>>::iterator qit = quantileIndices.begin();
           qit != quantileIndices.end(); ++qit, ++k)
      {
        // Retrieve data value from rank into histogram
        vtkVariant Qp = vvals->GetValue(qit->first);

        // Store quantile value
        quantCol->SetValue(k, Qp);
      }
    } // else if ( vals->IsA("vtkVariantArray") )
    else
    {
      vtkWarningMacro(
        "Unsupported data type for column " << varName << ". Cannot calculate quantiles for it.");

      continue;
    } // else

  } // for ( unsigned int b = 0; b < nParts; ++ b )

  // Add cardinality and quantile tables.
  inMeta->SetTable(vtkStatisticalModel::Derived, 0, cardinalityTab, "Cardinalities");
  inMeta->SetTable(vtkStatisticalModel::Derived, 1, quantileTab, "Quantiles");

  // Clean up
  row->Delete();
  cardinalityTab->Delete();
  quantileTab->Delete();
}

//------------------------------------------------------------------------------
void vtkOrderStatistics::Test(vtkTable* inData, vtkStatisticalModel* inMeta, vtkTable* outMeta)
{
  if (!inMeta)
  {
    return;
  }

  int nParts = inMeta->GetNumberOfTables(vtkStatisticalModel::Learned);
  if (nParts < 1)
  {
    return;
  }

  auto* quantileTab = inMeta->FindTableByName(vtkStatisticalModel::Derived, "Quantiles");
  if (!quantileTab)
  {
    return;
  }

  if (!outMeta)
  {
    return;
  }

  // Prepare columns for the test:
  // 0: variable name
  // 1: Maximum vertical distance between CDFs
  // 2: Kolmogorov-Smirnov test statistic (the above times the square root of the cardinality)
  // NB: These are not added to the output table yet, for they will be filled individually first
  //     in order that R be invoked only once.
  vtkStringArray* nameCol = vtkStringArray::New();
  nameCol->SetName("Variable");

  vtkDoubleArray* distCol = vtkDoubleArray::New();
  distCol->SetName("Maximum Distance");

  vtkDoubleArray* statCol = vtkDoubleArray::New();
  statCol->SetName("Kolmogorov-Smirnov");

  // Prepare storage for quantiles and model CDFs
  vtkIdType nQuant = quantileTab->GetNumberOfRows();
  std::vector<std::string> quantiles(nQuant);

  // Loop over requests
  vtkIdType nRowData = inData->GetNumberOfRows();
  double inv_nq = 1. / nQuant;
  double inv_card = 1. / nRowData;
  double sqrt_card = sqrt(static_cast<double>(nRowData));
  for (std::set<std::set<vtkStdString>>::const_iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit)
  {
    // Each request contains only one column of interest (if there are others, they are ignored)
    std::set<vtkStdString>::const_iterator it = rit->begin();
    std::string const& varName = *it;
    if (!inData->GetColumnByName(varName.c_str()))
    {
      vtkWarningMacro("InData table does not have a column " << varName << ". Ignoring it.");
      continue;
    }

    // Find the quantile column that corresponds to the variable of the request
    vtkAbstractArray* quantCol = quantileTab->GetColumnByName(varName.c_str());
    if (!quantCol)
    {
      vtkWarningMacro(
        "Quantile table table does not have a column " << varName << ". Ignoring it.");
      continue;
    }

    // First iterate over all observations to calculate empirical PDF
    typedef std::map<std::string, double> CDF;
    CDF cdfEmpirical;
    for (vtkIdType j = 0; j < nRowData; ++j)
    {
      // Read observation and update PDF
      cdfEmpirical[inData->GetValueByName(j, varName.c_str()).ToString()] += inv_card;
    }

    // Now integrate to obtain empirical CDF
    double sum = 0.;
    for (CDF::iterator cit = cdfEmpirical.begin(); cit != cdfEmpirical.end(); ++cit)
    {
      sum += cit->second;
      cit->second = sum;
    }

    // Sanity check: verify that empirical CDF = 1
    if (fabs(sum - 1.) > 1.e-6)
    {
      vtkWarningMacro("Incorrect empirical CDF for variable:" << varName << ". Ignoring it.");

      continue;
    }

    // Retrieve quantiles to calculate model CDF and insert value into empirical CDF
    for (vtkIdType i = 0; i < nQuant; ++i)
    {
      // Read quantile and update CDF
      quantiles[i] = quantileTab->GetValueByName(i, varName.c_str()).ToString();

      // Update empirical CDF if new value found (with unknown ECDF)
      std::pair<CDF::iterator, bool> result =
        cdfEmpirical.insert(std::pair<std::string, double>(quantiles[i], -1));
      if (result.second)
      {
        CDF::iterator eit = result.first;
        // Check if new value has no predecessor, in which case CDF = 0
        if (eit == cdfEmpirical.begin())
        {
          result.first->second = 0.;
        }
        else
        {
          --eit;
          result.first->second = eit->second;
        }
      }
    }

    // Iterate over all CDF jump values
    int currentQ = 0;
    double mcdf = 0.;
    double Dmn = 0.;
    for (CDF::iterator cit = cdfEmpirical.begin(); cit != cdfEmpirical.end(); ++cit)
    {
      // If observation is smaller than minimum then there is nothing to do
      if (cit->first >= quantiles[0])
      {
        while (currentQ < nQuant && cit->first >= quantiles[currentQ])
        {
          ++currentQ;
        }

        // Calculate model CDF at observation
        mcdf = currentQ * inv_nq;
      }

      // Calculate vertical distance between CDFs and update maximum if needed
      double d = fabs(cit->second - mcdf);
      Dmn = std::max(d, Dmn);
    }

    // Insert variable name and calculated Kolmogorov-Smirnov statistic
    // NB: R will be invoked only once at the end for efficiency
    nameCol->InsertNextValue(varName);
    distCol->InsertNextTuple1(Dmn);
    statCol->InsertNextTuple1(sqrt_card * Dmn);
  } // rit

  // Now, add the already prepared columns to the output table
  outMeta->AddColumn(nameCol);
  outMeta->AddColumn(distCol);
  outMeta->AddColumn(statCol);

  // Clean up
  nameCol->Delete();
  distCol->Delete();
  statCol->Delete();
}

//------------------------------------------------------------------------------
class DataArrayQuantizer : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkDataArray* Data;
  vtkDataArray* Quantiles;

  DataArrayQuantizer(vtkAbstractArray* vals, vtkAbstractArray* quantiles)
  {
    this->Data = vtkArrayDownCast<vtkDataArray>(vals);
    this->Quantiles = vtkArrayDownCast<vtkDataArray>(quantiles);
  }
  ~DataArrayQuantizer() override = default;
  void operator()(vtkDoubleArray* result, vtkIdType id) override
  {
    result->SetNumberOfValues(1);

    double dval = this->Data->GetTuple1(id);
    if (dval < this->Quantiles->GetTuple1(0))
    {
      // dval is smaller than lower bound
      result->SetValue(0, 0);

      return;
    }

    vtkIdType q = 1;
    vtkIdType n = this->Quantiles->GetNumberOfTuples();
    while (q < n && dval > this->Quantiles->GetTuple1(q))
    {
      ++q;
    }

    result->SetValue(0, q);
  }
};

//------------------------------------------------------------------------------
class StringArrayQuantizer : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkStringArray* Data;
  vtkStringArray* Quantiles;

  StringArrayQuantizer(vtkAbstractArray* vals, vtkAbstractArray* quantiles)
  {
    this->Data = vtkArrayDownCast<vtkStringArray>(vals);
    this->Quantiles = vtkArrayDownCast<vtkStringArray>(quantiles);
  }
  ~StringArrayQuantizer() override = default;
  void operator()(vtkDoubleArray* result, vtkIdType id) override
  {
    result->SetNumberOfValues(1);

    std::string sval = this->Data->GetValue(id);
    if (sval < this->Quantiles->GetValue(0))
    {
      // sval is smaller than lower bound
      result->SetValue(0, 0);

      return;
    }

    vtkIdType q = 1;
    vtkIdType n = this->Quantiles->GetNumberOfValues();
    while (q < n && sval > this->Quantiles->GetValue(q))
    {
      ++q;
    }

    result->SetValue(0, q);
  }
};

//------------------------------------------------------------------------------
class VariantArrayQuantizer : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  vtkVariantArray* Data;
  vtkVariantArray* Quantiles;

  VariantArrayQuantizer(vtkAbstractArray* vals, vtkAbstractArray* quantiles)
  {
    this->Data = vtkArrayDownCast<vtkVariantArray>(vals);
    this->Quantiles = vtkArrayDownCast<vtkVariantArray>(quantiles);
  }
  ~VariantArrayQuantizer() override = default;
  void operator()(vtkDoubleArray* result, vtkIdType id) override
  {
    result->SetNumberOfValues(1);

    vtkVariant vval = this->Data->GetValue(id);
    if (vval < this->Quantiles->GetValue(0))
    {
      // vval is smaller than lower bound
      result->SetValue(0, 0);

      return;
    }

    vtkIdType q = 1;
    vtkIdType n = this->Quantiles->GetNumberOfValues();
    while (q < n && vval > this->Quantiles->GetValue(q))
    {
      ++q;
    }

    result->SetValue(0, q);
  }
};

//------------------------------------------------------------------------------
void vtkOrderStatistics::SelectAssessFunctor(
  vtkTable* outData, vtkDataObject* inMetaDO, vtkStringArray* rowNames, AssessFunctor*& dfunc)
{
  dfunc = nullptr;
  auto* inMeta = vtkStatisticalModel::SafeDownCast(inMetaDO);
  if (!inMeta)
  {
    return;
  }

  int nParts = inMeta->GetNumberOfTables(vtkStatisticalModel::Learned);
  if (nParts < 1)
  {
    return;
  }

  auto* quantileTab = inMeta->FindTableByName(vtkStatisticalModel::Derived, "Quantiles");
  if (!quantileTab)
  {
    return;
  }

  // Retrieve name of variable of the request
  const auto& varName = rowNames->GetValue(0);

  // Grab the data for the requested variable
  vtkAbstractArray* vals = outData->GetColumnByName(varName.c_str());
  if (!vals)
  {
    return;
  }

  // Find the quantile column that corresponds to the variable of the request
  vtkAbstractArray* quantiles = quantileTab->GetColumnByName(varName.c_str());
  if (!quantiles)
  {
    vtkWarningMacro("Quantile table table does not have a column " << varName << ". Ignoring it.");
    return;
  }

  // Select assess functor depending on data and quantile type
  if (vals->IsA("vtkDataArray") && quantiles->IsA("vtkDataArray"))
  {
    dfunc = new DataArrayQuantizer(vals, quantiles);
  }
  else if (vals->IsA("vtkStringArray") && quantiles->IsA("vtkStringArray"))
  {
    dfunc = new StringArrayQuantizer(vals, quantiles);
  }
  else if (vals->IsA("vtkVariantArray") && quantiles->IsA("vtkVariantArray"))
  {
    dfunc = new VariantArrayQuantizer(vals, quantiles);
  }
  else
  {
    vtkWarningMacro("Unsupported (data,quantiles) type for column "
      << varName << ": data type is " << vals->GetClassName() << " and quantiles type is "
      << quantiles->GetClassName() << ". Ignoring it.");
  }
}
VTK_ABI_NAMESPACE_END
