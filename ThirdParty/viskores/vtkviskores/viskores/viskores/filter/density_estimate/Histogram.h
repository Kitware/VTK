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

#ifndef viskores_filter_density_estimate_Histogram_h
#define viskores_filter_density_estimate_Histogram_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/density_estimate/viskores_filter_density_estimate_export.h>

namespace viskores
{
namespace filter
{
namespace density_estimate
{
/// \brief Construct the histogram of a given field.
///
/// The range of the field is evenly split to a set number of bins (set by
/// `SetNumberOfBins()`). This filter then counts the number of values in the filter
/// that are in each bin.
///
/// The result of this filter is stored in a `viskores::cont::DataSet` with no points
/// or cells. It contains only a single field containing the histogram (bin counts).
/// The field has an association of `viskores::cont::Field::Association::WholeDataSet`.
/// The field contains an array of `viskores::Id` with the bin counts. By default, the
/// field is named "histogram", but that can be changed with the `SetOutputFieldName()`
/// method.
///
/// If this filter is run on a partitioned data set, the result will be a
/// `viskores::cont::PartitionedDataSet` containing a single
/// `viskores::cont::DataSet` as previously described.
///
class VISKORES_FILTER_DENSITY_ESTIMATE_EXPORT Histogram : public viskores::filter::Filter
{
public:
  VISKORES_CONT Histogram();

  /// @brief Set the number of bins for the resulting histogram.
  ///
  /// By default, a histogram with 10 bins is created.
  VISKORES_CONT void SetNumberOfBins(viskores::Id count) { this->NumberOfBins = count; }

  /// @brief Get the number of bins for the resulting histogram.
  VISKORES_CONT viskores::Id GetNumberOfBins() const { return this->NumberOfBins; }

  /// @brief Set the range to use to generate the histogram.
  ///
  /// If range is set to empty, the field's global range (computed using
  /// `viskores::cont::FieldRangeGlobalCompute`) will be used.
  VISKORES_CONT void SetRange(const viskores::Range& range) { this->Range = range; }

  /// @brief Get the range used to generate the histogram.
  ///
  /// If the returned range is empty, then the field's global range will be used.
  VISKORES_CONT const viskores::Range& GetRange() const { return this->Range; }

  /// @brief Returns the size of bin in the computed histogram.
  ///
  /// This value is only valid after a call to `Execute`.
  VISKORES_CONT viskores::Float64 GetBinDelta() const { return this->BinDelta; }

  /// @brief Returns the range used for most recent execute.
  ///
  /// If `SetRange` is used to specify a non-empty range, then this range will
  /// be returned. Otherwise, the coputed range is returned.
  /// This value is only valid after a call to `Execute`.
  VISKORES_CONT viskores::Range GetComputedRange() const { return this->ComputedRange; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
  VISKORES_CONT viskores::cont::PartitionedDataSet DoExecutePartitions(
    const viskores::cont::PartitionedDataSet& inData) override;

  ///@{
  /// when operating on viskores::cont::PartitionedDataSet, we
  /// want to do processing across ranks as well. Just adding pre/post handles
  /// for the same does the trick.
  VISKORES_CONT void PreExecute(const viskores::cont::PartitionedDataSet& input);
  VISKORES_CONT void PostExecute(const viskores::cont::PartitionedDataSet& input,
                                 viskores::cont::PartitionedDataSet& output);
  ///@}

  viskores::Id NumberOfBins = 10;
  viskores::Float64 BinDelta = 0;
  viskores::Range ComputedRange;
  viskores::Range Range;
  bool InExecutePartitions = false;
};
} // namespace density_estimate
} // namespace filter
} // namespace viskores

#endif // viskores_filter_density_estimate_Histogram_h
