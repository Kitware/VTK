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
#ifndef viskores_filter_field_transform_LogValues_h
#define viskores_filter_field_transform_LogValues_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/field_transform/viskores_filter_field_transform_export.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{

/// \brief Adds field to a `viskores::cont::DataSet` that gives the log values for the user specified field.
///
/// By default, `LogValues` takes a natural logarithm (of base e). The base of the
/// logarithm can be set to one of the bases listed in `LogBase` with `SetBaseValue()`.
///
/// Logarithms are often used to rescale data to simultaneously show data at different
/// orders of magnitude. It allows small changes in small numbers be visible next to
/// much larger numbers with less precision. One problem with this approach is if there
/// exist numbers very close to zero, the scale at the low range could make all but the
/// smallest numbers comparatively hard to see. Thus, `LogValues` supports setting a
/// minimum value (with `SetMinValue()`) that will clamp any smaller values to that.
///
class VISKORES_FILTER_FIELD_TRANSFORM_EXPORT LogValues : public viskores::filter::Filter
{
public:
  /// @brief Identifies a type of logarithm as specified by its base.
  enum struct LogBase
  {
    /// @copydoc LogValues::SetBaseValueToE
    E,
    /// @copydoc LogValues::SetBaseValueTo2
    TWO,
    /// @copydoc LogValues::SetBaseValueTo10
    TEN
  };

  /// @brief Specify the base of the logarithm.
  VISKORES_CONT const LogBase& GetBaseValue() const { return this->BaseValue; }
  /// @brief Specify the base of the logarithm.
  VISKORES_CONT void SetBaseValue(const LogBase& base) { this->BaseValue = base; }

  /// @brief Take the natural logarithm.
  ///
  /// The logarithm is set to the mathematical constant e (about 2.718).
  /// This is a constant that has many uses in calculus and other mathematics,
  /// and a logarithm of base e is often referred to as the "natural" logarithm.
  VISKORES_CONT void SetBaseValueToE() { this->SetBaseValue(LogBase::E); }
  /// @brief Take the base 2 logarithm.
  ///
  /// The base 2 logarithm is particularly useful for estimating the depth
  /// of a binary hierarchy.
  VISKORES_CONT void SetBaseValueTo2() { this->SetBaseValue(LogBase::TWO); }
  /// @brief Take the base 10 logarithm.
  ///
  /// The base 10 logarithm is handy to convert a number to its order of magnitude
  /// based on our standard base 10 human counting system.
  VISKORES_CONT void SetBaseValueTo10() { this->SetBaseValue(LogBase::TEN); }

  /// @brief Specifies the minimum value to take the logarithm of.
  ///
  /// Before taking the logarithm, this filter will check the value to this minimum
  /// value and clamp it to the minimum value if it is lower. This is useful to
  /// prevent values from approching negative infinity.
  ///
  /// By default, no minimum value is used.
  viskores::FloatDefault GetMinValue() const { return this->MinValue; }
  /// @copydoc GetMinValue
  void SetMinValue(const viskores::FloatDefault& value) { this->MinValue = value; }

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
  LogBase BaseValue = LogBase::E;
  viskores::FloatDefault MinValue = std::numeric_limits<FloatDefault>::min();
};
} // namespace field_transform
} // namespace viskores::filter
} // namespace viskores

#endif //viskores_filter_field_transform_LogValues_h
