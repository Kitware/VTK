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
#ifndef viskores_filter_resampling_HistSampling_h
#define viskores_filter_resampling_HistSampling_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/resampling/viskores_filter_resampling_export.h>

#include <viskores/Deprecated.h>

namespace viskores
{
namespace filter
{
namespace resampling
{

/// @brief Adaptively sample points to preserve tail features.
///
/// This filter randomly samples the points of a `viskores::cont::DataSet` and generates
/// a new `viskores::cont::DataSet` with a subsampling of the points. The sampling is
/// adaptively selected to preserve tail and outlying features of the active field.
/// That is, the more rare a field value is, the more likely the point will be
/// selected in the sampling. This is done by creating a histogram of the field
/// and using that to derive the importance level of each field value. Details of
/// the algorithm can be found in the paper "In Situ Data-Driven Adaptive Sampling
/// for Large-scale Simulation Data Summarization" by Biswas, Dutta, Pulido, and Ahrens
/// as published in _In Situ Infrastructures for Enabling Extreme-scale Analysis and
/// Visualization_ (ISAV 2018).
///
/// The cell set of the input data is removed and replaced with a set with a vertex
/// cell for each point. This effectively converts the data to a point cloud.
class VISKORES_FILTER_RESAMPLING_EXPORT HistSampling : public viskores::filter::Filter
{
public:
  /// @brief Specify the number of bins used when computing the histogram.
  ///
  /// The histogram is used to select the importance of each field value.
  /// More rare field values are more likely to be selected.
  VISKORES_CONT void SetNumberOfBins(viskores::Id numberOfBins)
  {
    this->NumberOfBins = numberOfBins;
  }
  /// @copydoc SetNumberOfBins
  VISKORES_CONT viskores::Id GetNumberOfBins() { return this->NumberOfBins; }

  /// @brief Specify the fraction of points to create in the sampled data.
  ///
  /// A fraction of 1 means that all the points will be sampled and be in the output.
  /// A fraction of 0 means that none of the points will be sampled. A fraction of 0.5 means
  /// that half the points will be selected to be in the output.
  VISKORES_CONT void SetSampleFraction(viskores::FloatDefault fraction)
  {
    this->SampleFraction = fraction;
  }
  /// @copydoc SetSampleFraction
  VISKORES_CONT viskores::FloatDefault GetSampleFraction() const { return this->SampleFraction; }

  VISKORES_DEPRECATED(2.2, "Use SetSampleFraction().")
  VISKORES_CONT void SetSamplePercent(viskores::FloatDefault samplePercent)
  {
    this->SetSampleFraction(samplePercent);
  }
  VISKORES_DEPRECATED(2.2, "Use GetSampleFraction().")
  VISKORES_CONT viskores::FloatDefault GetSamplePercent() const
  {
    return this->GetSampleFraction();
  }

  /// @brief Specify the seed used for random number generation.
  ///
  /// The random numbers are used to select which points to pull from the input. If
  /// the same seed is used for multiple invocations, the results will be the same.
  VISKORES_CONT void SetSeed(viskores::UInt32 seed) { this->Seed = seed; }
  /// @copydoc SetSeed
  VISKORES_CONT viskores::UInt32 GetSeed() { return this->Seed; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
  viskores::Id NumberOfBins = 10;
  viskores::FloatDefault SampleFraction = 0.1f;
  viskores::UInt32 Seed = 0;
};

} // namespace resampling
} // namespace filter
} // namespace viskores

#endif // viskores_filter_resampling_HistSampling_h
