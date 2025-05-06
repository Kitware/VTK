//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vector>

#include <viskores/filter/density_estimate/NDHistogram.h>
#include <viskores/filter/density_estimate/worklet/NDimsHistogram.h>

namespace viskores
{
namespace filter
{
namespace density_estimate
{
void NDHistogram::AddFieldAndBin(const std::string& fieldName, viskores::Id numOfBins)
{
  this->FieldNames.push_back(fieldName);
  this->NumOfBins.push_back(numOfBins);
}

viskores::Float64 NDHistogram::GetBinDelta(size_t fieldIdx)
{
  return BinDeltas[fieldIdx];
}

viskores::Range NDHistogram::GetDataRange(size_t fieldIdx)
{
  return DataRanges[fieldIdx];
}

VISKORES_CONT viskores::cont::DataSet NDHistogram::DoExecute(const viskores::cont::DataSet& inData)
{
  viskores::worklet::NDimsHistogram ndHistogram;

  // Set the number of data points
  ndHistogram.SetNumOfDataPoints(inData.GetField(0).GetNumberOfValues());

  // Add field one by one
  // (By using AddFieldAndBin(), the length of FieldNames and NumOfBins must be the same)
  for (size_t i = 0; i < this->FieldNames.size(); i++)
  {
    viskores::Range rangeField;
    viskores::Float64 deltaField;
    ndHistogram.AddField(
      inData.GetField(this->FieldNames[i]).GetData(), this->NumOfBins[i], rangeField, deltaField);
    DataRanges.push_back(rangeField);
    BinDeltas.push_back(deltaField);
  }

  std::vector<viskores::cont::ArrayHandle<viskores::Id>> binIds;
  viskores::cont::ArrayHandle<viskores::Id> freqs;
  ndHistogram.Run(binIds, freqs);

  viskores::cont::DataSet outputData;
  for (size_t i = 0; i < binIds.size(); i++)
  {
    outputData.AddField(
      { this->FieldNames[i], viskores::cont::Field::Association::WholeDataSet, binIds[i] });
  }
  outputData.AddField({ "Frequency", viskores::cont::Field::Association::WholeDataSet, freqs });
  // The output is a "summary" of the input, no need to map fields
  return outputData;
}

} // namespace density_estimate
} // namespace filter
} // namespace viskores
