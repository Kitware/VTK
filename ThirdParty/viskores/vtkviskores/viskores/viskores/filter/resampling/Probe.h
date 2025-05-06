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
#ifndef viskores_filter_resampling_Probe_h
#define viskores_filter_resampling_Probe_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/resampling/viskores_filter_resampling_export.h>

namespace viskores
{
namespace filter
{
namespace resampling
{

/// @brief Sample the fields of a data set at specified locations.
///
/// The `viskores::filter::resampling::Probe` filter samples the fields of one
/// `viskores::cont::DataSet` and places them in the fields of another
/// `viskores::cont::DataSet`.
///
/// To use this filter, first specify a geometry to probe with with `SetGeometry()`.
/// The most important feature of this geometry is its coordinate system.
/// When you call `Execute()`, the output will be the data specified with
/// `SetGeometry()` but will have the fields of the input to `Execute()`
/// transferred to it. The fields are transfered by probing the input data
/// set at the point locations of the geometry.
///
class VISKORES_FILTER_RESAMPLING_EXPORT Probe : public viskores::filter::Filter
{
public:
  /// @brief Specify the geometry to probe with.
  ///
  /// When `Execute()` is called, the input data will be probed at all the point
  /// locations of this @p geometry as specified by its coordinate system.
  VISKORES_CONT void SetGeometry(const viskores::cont::DataSet& geometry)
  {
    this->Geometry = viskores::cont::DataSet();
    this->Geometry.CopyStructure(geometry);
  }

  /// @copydoc SetGeometry
  VISKORES_CONT const viskores::cont::DataSet& GetGeometry() const { return this->Geometry; }

  /// @brief Specify the value to use for points outside the bounds of the input.
  ///
  /// It is possible that the sampling geometry will have points outside the bounds of
  /// the input. When this happens, the field will be set to this "invalid" value.
  /// By default, the invalid value is NaN.
  VISKORES_CONT void SetInvalidValue(viskores::Float64 invalidValue)
  {
    this->InvalidValue = invalidValue;
  }
  /// @copydoc SetInvalidValue
  VISKORES_CONT viskores::Float64 GetInvalidValue() const { return this->InvalidValue; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::cont::DataSet Geometry;

  viskores::Float64 InvalidValue = viskores::Nan64();
};

} // namespace resampling
} // namespace filter
} // namespace viskores

#endif // viskores_filter_resampling_Probe_h
