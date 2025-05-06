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
#ifndef viskores_filter_density_estimate_NDHistogram_h
#define viskores_filter_density_estimate_NDHistogram_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/density_estimate/viskores_filter_density_estimate_export.h>

namespace viskores
{
namespace filter
{
namespace density_estimate
{
/// \brief Generate a N-Dims histogram from input fields
///
/// This filter takes a data set and with target fields and bins defined,
/// it would generate a N-Dims histogram from input fields. The result is stored
/// in a field named as "Frequency". This filed contains all the frequencies of
/// the N-Dims histogram in sparse representation. That being said, the result
/// field does not store 0 frequency bins. Meanwhile all input fields now
/// would have the same length and store bin ids instead.
/// E.g. (FieldA[i], FieldB[i], FieldC[i], Frequency[i]) is a bin in the histogram.
/// The first three numbers are binIDs for FieldA, FieldB and FieldC. Frequency[i] stores
/// the frequency for this bin (FieldA[i], FieldB[i], FieldC[i]).
///
class VISKORES_FILTER_DENSITY_ESTIMATE_EXPORT NDHistogram : public viskores::filter::Filter
{
public:
  VISKORES_CONT
  void AddFieldAndBin(const std::string& fieldName, viskores::Id numOfBins);

  // This index is the field position in FieldNames
  // (or the input _fieldName string vector of SetFields() Function)
  VISKORES_CONT
  viskores::Float64 GetBinDelta(size_t fieldIdx);

  // This index is the field position in FieldNames
  // (or the input _fieldName string vector of SetFields() Function)
  VISKORES_CONT
  viskores::Range GetDataRange(size_t fieldIdx);

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  std::vector<viskores::Id> NumOfBins;
  std::vector<std::string> FieldNames;
  std::vector<viskores::Float64> BinDeltas;
  std::vector<viskores::Range> DataRanges; //Min Max of the field
};
} // namespace density_estimate
} // namespace filter
} // namespace vtm

#endif //viskores_filter_density_estimate_NDHistogram_h
