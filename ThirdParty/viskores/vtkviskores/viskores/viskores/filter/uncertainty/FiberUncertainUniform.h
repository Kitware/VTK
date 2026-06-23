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

//  This code is based on the algorithm presented in the following paper:
//  Hari, G., Joshi, N., Wang, Z., Gong, Q., Pugmire, D., Moreland, K.,
//  Johnson, C. R., Klasky, S., Podhorszki, N., and Athawale, T. M.
//  (2024). "FunM2C: A Filter for Uncertainty Visualization of Multivariate Data on Multi-Core Devices,"
//  in 2024 IEEE Workshop on Uncertainty Visualization: Applications, Techniques, Software, and Decision Frameworks,
//  St Pete Beach, FL, USA, 2024, pp. 43-47, doi: 10.1109/UncertaintyVisualization63963.2024.00010.

#ifndef viskores_filter_uncertainty_FiberUncertainUniform_h
#define viskores_filter_uncertainty_FiberUncertainUniform_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/uncertainty/viskores_filter_uncertainty_export.h>

namespace viskores
{
namespace filter
{
namespace uncertainty
{
/// @brief Visualize fiber uncertainty for uniform distributed data.
///
/// This filter computes the positional probability of fibers (bivariate features) as a
/// function of uncertainty in the input bivariate data (with variables denoted by X and Y).
/// The data are sampled on a regular grid, and the uncertainty in the data for each variable
/// is assumed to be uniformly distributed. The uniform distribution range is estimated from
/// the input datasets via the minimum and maximum values of variables X and Y observed across
/// the ensemble.
///
class VISKORES_FILTER_UNCERTAINTY_EXPORT FiberUncertainUniform : public viskores::filter::Filter
{
public:
  /// @brief Constructor
  VISKORES_CONT FiberUncertainUniform() = default;

  /// @brief Sets the trait rectangle's range for the first field.
  VISKORES_CONT void SetRange1(const viskores::Range& range) { this->RangeAxis1 = range; }

  /// @copydoc SetRange1
  VISKORES_CONT void SetRange1(viskores::Float64 min, viskores::Float64 max)
  {
    this->SetRange1({ min, max });
  }

  /// @brief Sets the trait rectangle's range for the second field.
  VISKORES_CONT void SetRange2(const viskores::Range& range) { this->RangeAxis2 = range; }

  /// @copydoc SetRange2
  VISKORES_CONT void SetRange2(viskores::Float64 min, viskores::Float64 max)
  {
    this->SetRange2({ min, max });
  }

  /// @brief Sets minimum X.
  /// Sets minimum value of the uniform distribution of the variable X at each grid point.
  VISKORES_CONT void SetField1Min(const std::string& fieldName)
  {
    this->SetActiveField(0, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets maxmimum X.
  /// Sets maximum value of the uniform distribution of the variable X at each grid point.
  VISKORES_CONT void SetField1Max(const std::string& fieldName)
  {
    this->SetActiveField(1, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets minimum Y.
  /// Sets minimum value of the uniform distribution of the variable Y at each grid point.
  VISKORES_CONT void SetField2Min(const std::string& fieldName)
  {
    this->SetActiveField(2, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets maxmimum Y.
  /// Sets maximum value of the uniform distribution of the variable Y at each grid point.
  VISKORES_CONT void SetField2Max(const std::string& fieldName)
  {
    this->SetActiveField(3, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets number of samples.
  /// Sets how many samples will be used when computing fiber uncertainty using the Monte Carlo approach.
  VISKORES_CONT void SetNumberOfSamples(const viskores::Id& numSamples)
  {
    this->NumSamples = numSamples;
  }

  /// @brief Supported approaches for uncertain fiber surfaces.
  enum struct ApproachEnum
  {
    MonteCarlo,
    ClosedForm,
    Mean,
    Truth
  };

  /// @brief Sets the approach for computing uncertainty (ClosedForm or  MonteCarlo).
  ///  Sets the approach for the corresponding filter that was selected.
  VISKORES_CONT void SetApproach(ApproachEnum approach) { this->Approach = approach; }

private:
  viskores::Range RangeAxis1;
  viskores::Range RangeAxis2;
  ApproachEnum Approach = ApproachEnum::ClosedForm;
  viskores::Id NumSamples = 5000;

  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

}
}
}

#endif
