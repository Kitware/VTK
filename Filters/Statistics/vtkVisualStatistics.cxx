// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkVisualStatistics.h"
#include "vtkStatisticsAlgorithmPrivate.h"

#include "vtkDataAssembly.h"
#include "vtkDataObjectCollection.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLegacy.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStatisticalModel.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"
#include "vtkStringToken.h"
#include "vtkSumTables.h"
#include "vtkTable.h"
#include "vtkTypeUInt64Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariantArray.h"

#include <limits>
#include <set>
#include <sstream>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{

struct HistogramWorker
{
  vtkDataArray* Data{ nullptr };
  vtkIdTypeArray* Histogram{ nullptr };
  vtkSMPThreadLocalObject<vtkIdTypeArray> ThreadHistogram;
  vtkSMPThreadLocal<std::size_t> ThreadSamplesInRange;
  double Lo;
  double Hi;
  vtkIdType NumberOfBins;
  vtkUnsignedCharArray* Ghosts;
  vtkTypeUInt64 SamplesInRange{ 0 };

  HistogramWorker() = delete;

  HistogramWorker(vtkDataArray* data, vtkIdTypeArray* histogram, double lo, double hi,
    vtkUnsignedCharArray* ghosts)
    : Data(data)
    , Histogram(histogram)
    , ThreadHistogram(histogram) // exemplar
    , Lo(lo)
    , Hi(hi)
    , NumberOfBins(histogram->GetNumberOfTuples() - 3)
    , Ghosts(ghosts)
  {
  }

  void Initialize()
  {
    auto localHisto = this->ThreadHistogram.Local();
    localHisto->SetNumberOfTuples(this->Histogram->GetNumberOfTuples());
    localHisto->FillComponent(0, 0);
    auto& localSamplesInRange = this->ThreadSamplesInRange.Local();
    localSamplesInRange = 0;
  }

  template <bool HaveGhosts, bool IsFloatingPt>
  void Process(vtkIdType begin, vtkIdType end, double delta, vtkIdTypeArray* histo)
  {
    auto& localSamplesInRange = this->ThreadSamplesInRange.Local();
    vtkTypeInt64 count;
    for (vtkIdType ii = begin; ii < end; ++ii)
    {
      if (HaveGhosts)
      {
        // Skip any entry with any ghost-bit marked:
        if (!!this->Ghosts->GetValue(ii))
        {
          continue;
        }
      }
      double val = this->Data->GetTuple1(ii);
      int bin;
      if (IsFloatingPt)
      {
        if (vtkMath::IsNan(val))
        {
          bin = this->NumberOfBins + 2;
        }
        else if (val < this->Lo)
        {
          bin = 0;
        }
        else if (val > this->Hi)
        {
          bin = this->NumberOfBins + 1;
        }
        else if (val == this->Hi)
        {
          bin = this->NumberOfBins;
          ++localSamplesInRange;
        }
        else
        {
          bin = static_cast<int>((this->NumberOfBins * (val - this->Lo)) / delta) + 1;
          ++localSamplesInRange;
        }
      }
      else
      {
        if (val < this->Lo)
        {
          bin = 0;
        }
        else if (val > this->Hi)
        {
          bin = this->NumberOfBins + 1;
        }
        else if (val == this->Hi)
        {
          bin = this->NumberOfBins;
          ++localSamplesInRange;
        }
        else
        {
          bin = static_cast<int>((this->NumberOfBins * (val - this->Lo)) / delta) + 1;
          ++localSamplesInRange;
        }
      }
      histo->GetIntegerTuple(bin, &count);
      ++count;
      histo->SetIntegerTuple(bin, &count);
    }
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    double delta = this->Hi - this->Lo;
    if (delta == 0.)
    {
      delta = 1.0;
    }
    auto& histo(this->ThreadHistogram.Local());

    bool isFP = IsFloatingPoint(this->Data->GetDataType());
    if (this->Ghosts && this->Ghosts->GetNumberOfTuples() == this->Data->GetNumberOfTuples())
    {
      if (isFP)
      {
        this->Process<true, true>(begin, end, delta, histo);
      }
      else
      {
        this->Process<true, false>(begin, end, delta, histo);
      }
    }
    else
    {
      if (isFP)
      {
        this->Process<false, true>(begin, end, delta, histo);
      }
      else
      {
        this->Process<false, false>(begin, end, delta, histo);
      }
    }
  }

  void Reduce()
  {
    // Add each thread's histogram to the final output histogram.
    vtkTypeInt64 count;
    vtkTypeInt64 localCount;
    for (const auto& localHistogram : this->ThreadHistogram)
    {
      for (vtkIdType bb = 0; bb < this->NumberOfBins + 3; ++bb)
      {
        this->Histogram->GetIntegerTuple(bb, &count);
        localHistogram->GetIntegerTuple(bb, &localCount);
        count += localCount;
        this->Histogram->SetIntegerTuple(bb, &count);
      }
    }
    this->SamplesInRange = 0;
    for (const auto& localSamplesInRange : this->ThreadSamplesInRange)
    {
      this->SamplesInRange += localSamplesInRange;
    }
  }
};

} // anonymous namespace

vtkObjectFactoryNewMacro(vtkVisualStatistics);

vtkVisualStatistics::vtkVisualStatistics() = default;

vtkVisualStatistics::~vtkVisualStatistics() = default;

void vtkVisualStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldRanges: " << this->FieldRanges.size() << " entries.\n";
  vtkIndent i2 = indent.GetNextIndent();
  for (const auto& entry : this->FieldRanges)
  {
    os << i2 << "\"" << entry.first << "\" [" << entry.second.first << ", " << entry.second.second
       << "]\n";
  }
  os << indent << "NumberOfBins: " << this->NumberOfBins << "\n";
}

void vtkVisualStatistics::SetFieldRange(const char* field, double lo, double hi)
{
  if (!field || !field[0])
  {
    return;
  }
  std::string fieldStr(field);
  this->SetFieldRange(fieldStr, lo, hi);
}

void vtkVisualStatistics::SetFieldRange(const std::string& field, double lo, double hi)
{
  if (field.empty())
  {
    vtkErrorMacro("Empty field name.");
    return;
  }
  if (lo > hi)
  {
    vtkErrorMacro(
      "Field invalid field range [" << lo << ", " << hi << "] for \"" << field << "\".");
    return;
  }
  auto it = this->FieldRanges.find(field);
  if (it != this->FieldRanges.end())
  {
    if (it->second.first == lo && it->second.second == hi)
    {
      // Range is identical to the one already set.
      return;
    }
    it->second.first = lo;
    it->second.second = hi;
  }
  else
  {
    this->FieldRanges[field] = std::make_pair(lo, hi);
  }
  this->Modified();
}

bool vtkVisualStatistics::Aggregate(
  vtkDataObjectCollection* inMetaColl, vtkStatisticalModel* outMeta)
{
  if (!this->Superclass::Aggregate(inMetaColl, outMeta))
  {
    return false;
  }

  // Get hold of the first model (data object) in the collection
  int itemIndex;
  int numItems = inMetaColl->GetNumberOfItems();
  // Skip collection entries until we find one with a histogram table:
  vtkTable* histogramTab = nullptr;
  vtkTable* summaryTab = nullptr;
  for (itemIndex = 0; itemIndex < numItems; ++itemIndex)
  {
    // Verify that the first primary statistics are present
    auto* inMeta = vtkStatisticalModel::SafeDownCast(inMetaColl->GetItemAsObject(itemIndex));
    histogramTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Learned, 1) : nullptr;
    summaryTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Learned, 2) : nullptr;
    if (histogramTab && summaryTab && histogramTab->GetNumberOfRows() > 0)
    {
      break;
    }
  }
  if (!histogramTab)
  {
    return true;
  }

  // Use this first model to initialize the aggregated one
  vtkNew<vtkTable> aggregatedTab;
  aggregatedTab->DeepCopy(histogramTab);
  vtkNew<vtkTable> aggregatedSummaryTab;
  aggregatedSummaryTab->DeepCopy(summaryTab);

  // Now, loop over all remaining models and update aggregated each time
  for (++itemIndex; itemIndex < numItems; ++itemIndex)
  {
    // Verify that the current primary statistics are indeed contained in a table
    auto* inMeta = vtkStatisticalModel::SafeDownCast(inMetaColl->GetItemAsObject(itemIndex));
    histogramTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Learned, 1) : nullptr;
    summaryTab = inMeta ? inMeta->GetTable(vtkStatisticalModel::Learned, 2) : nullptr;
    if (!histogramTab || !summaryTab ||
      histogramTab->GetNumberOfRows() != aggregatedTab->GetNumberOfRows())
    {
      continue;
    }

    // This will print an error if aggregatedTab and histogramTab are not conformal.
    vtkSumTables::SumTables(aggregatedTab, histogramTab);
    vtkSumTables::SumTables(
      aggregatedSummaryTab, summaryTab, /*checkOnly*/ false, /*allowAbstractColumns*/ true);
  }

  // Finally set the output histogram statistics table.
  outMeta->SetNumberOfTables(vtkStatisticalModel::Learned, 3);
  outMeta->SetTable(vtkStatisticalModel::Learned, 1, aggregatedTab, "Histogram Statistics");
  outMeta->SetTable(vtkStatisticalModel::Learned, 2, aggregatedSummaryTab, "Histogram Summary");

  // Clean up
  return true;
}

void vtkVisualStatistics::Learn(
  vtkTable* inData, vtkTable* inParameters, vtkStatisticalModel* outMeta)
{
  this->Superclass::Learn(inData, inParameters, outMeta);

  if (!inData)
  {
    return;
  }

  if (!outMeta)
  {
    return;
  }

  vtkDataSetAttributes* dsa = inData->GetRowData();
  vtkUnsignedCharArray* ghosts = dsa->GetGhostArray();

  std::map<std::string, vtkSmartPointer<vtkIdTypeArray>> histograms;
  vtkNew<vtkTable> learnedHistograms;
  vtkNew<vtkTable> learnedHistogramSummary;
  vtkNew<vtkStringArray> names;
  vtkNew<vtkTypeUInt64Array> totals;
  names->SetName("Name");
  totals->SetName("Totals");
  learnedHistogramSummary->AddColumn(names);
  learnedHistogramSummary->AddColumn(totals);
  for (const auto& entry : this->FieldRanges)
  {
    auto* vals = vtkDataArray::SafeDownCast(dsa->GetArray(entry.first.c_str()));
    if (!vals)
    {
      continue;
    }

    auto histo = vtkSmartPointer<vtkIdTypeArray>::New();
    histo->SetName(entry.first.c_str());
    histo->SetNumberOfTuples(this->NumberOfBins + 3);
    histo->FillComponent(0, 0);
    vtkIdType numTuples = vals->GetNumberOfTuples();
    HistogramWorker hw(vals, histo, entry.second.first, entry.second.second, ghosts);
    vtkSMPTools::For(0, numTuples, hw);
    histograms[entry.first] = hw.Histogram;
    learnedHistograms->AddColumn(hw.Histogram);
    names->InsertNextValue(entry.first.c_str());
    totals->InsertNextValue(hw.SamplesInRange);
  }

  outMeta->SetNumberOfTables(vtkStatisticalModel::Learned, 3);
  outMeta->SetAlgorithmParameters(this->GetAlgorithmParameters());
  outMeta->SetTable(vtkStatisticalModel::Learned, 1, learnedHistograms, "Histogram Statistics");
  outMeta->SetTable(vtkStatisticalModel::Learned, 2, learnedHistogramSummary, "Histogram Summary");
}

void vtkVisualStatistics::Derive(vtkStatisticalModel* modelData)
{
  this->Superclass::Derive(modelData);
  // TODO: We could normalize the histogram counts into empirical probabilities.
}

vtkDataArray* vtkVisualStatistics::GetHistogramForField(const std::string& fieldName)
{
  auto* model = vtkStatisticalModel::SafeDownCast(this->GetOutputDataObject(OUTPUT_MODEL));
  if (!model)
  {
    return nullptr;
  }
  auto* histograms = model->GetTable(vtkStatisticalModel::Learned, 1);
  if (!histograms)
  {
    return nullptr;
  }
  auto* data = vtkDataArray::SafeDownCast(histograms->GetColumnByName(fieldName.c_str()));
  return data;
}

std::string serializeFieldRanges(const std::map<std::string, std::pair<double, double>>& ranges)
{
  std::string result = "{";
  bool first = true;
  for (const auto& entry : ranges)
  {
    if (!first)
    {
      result += ",";
    }

    if (entry.first.find('"') == std::string::npos)
    {
      result += "\"" + entry.first + "\":";
    }
    else if (entry.first.find('\'') == std::string::npos)
    {
      result += "'" + entry.first + "':";
    }
    else
    {
      vtkGenericWarningMacro(
        "Field names (" << entry.first << ") cannot have both single- and double-quotes.");
      continue;
    }
    result +=
      "(" + vtk::to_string(entry.second.first) + "," + vtk::to_string(entry.second.second) + ")";
    first = false;
  }
  result += "}";
  return result;
}

void vtkVisualStatistics::AppendAlgorithmParameters(std::string& algorithmParameters) const
{
  this->Superclass::AppendAlgorithmParameters(algorithmParameters);
  if (algorithmParameters.back() != '(')
  {
    algorithmParameters += ",";
  }
  // clang-format off
  algorithmParameters +=
      "number_of_bins=" + vtk::to_string(this->NumberOfBins) + ","
      "field_ranges=" + serializeFieldRanges(this->FieldRanges)
      ;
  // clang-format on
}

std::size_t vtkVisualStatistics::ConsumeNextAlgorithmParameter(
  vtkStringToken parameterName, const std::string& algorithmParameters)
{
  using namespace vtk::literals;
  std::size_t consumed = 0;
  switch (parameterName.GetHash())
  {
    case "number_of_bins"_hash:
    {
      int value;
      if ((consumed = this->ConsumeInt(algorithmParameters, value)))
      {
        this->SetNumberOfBins(value);
      }
    }
    break;
    case "field_ranges"_hash:
    {
      std::map<std::string, std::vector<double>> map;
      if ((consumed = this->ConsumeStringToDoublesMap(algorithmParameters, map)))
      {
        this->ResetFieldRanges();
        for (const auto& entry : map)
        {
          if (entry.second.size() != 2 || entry.second[1] < entry.second[0])
          {
            vtkErrorMacro("Invalid range for \"" << entry.first << "\".");
            continue;
          }
          this->SetFieldRange(entry.first, entry.second[0], entry.second[1]);
        }
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

VTK_ABI_NAMESPACE_END
