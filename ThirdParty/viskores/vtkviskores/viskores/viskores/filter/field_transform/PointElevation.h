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

#ifndef viskores_filter_field_transform_PointElevation_h
#define viskores_filter_field_transform_PointElevation_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/field_transform/viskores_filter_field_transform_export.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{
/// \brief Generate a scalar field along a specified direction
///
/// The filter will take a data set and a field of 3 dimensional vectors and compute the
/// distance along a line defined by a low point and a high point. Any point in the plane
/// touching the low point and perpendicular to the line is set to the minimum range value
/// in the elevation whereas any point in the plane touching the high point and
/// perpendicular to the line is set to the maximum range value. All other values are
/// interpolated linearly between these two planes. This filter is commonly used to compute
/// the elevation of points in some direction, but can be repurposed for a variety of measures.
///
/// The default name for the output field is ``elevation'', but that can be
/// overridden as always using the `SetOutputFieldName()` method.
class VISKORES_FILTER_FIELD_TRANSFORM_EXPORT PointElevation : public viskores::filter::Filter
{
public:
  VISKORES_CONT PointElevation();

  /// @brief Specify the coordinate of the low point.
  ///
  /// The plane of low values is defined by the plane that contains the low point and
  /// is normal to the direction from the low point to the high point. All vector
  /// values on this plane are assigned the low value.
  VISKORES_CONT void SetLowPoint(const viskores::Vec3f_64& point) { this->LowPoint = point; }
  /// @copydoc SetLowPoint
  VISKORES_CONT void SetLowPoint(viskores::Float64 x, viskores::Float64 y, viskores::Float64 z)
  {
    this->SetLowPoint({ x, y, z });
  }

  /// @brief Specify the coordinate of the high point.
  ///
  /// The plane of high values is defined by the plane that contains the high point and
  /// is normal to the direction from the low point to the high point. All vector
  /// values on this plane are assigned the high value.
  VISKORES_CONT void SetHighPoint(const viskores::Vec3f_64& point) { this->HighPoint = point; }
  /// @copydoc SetHighPoint
  VISKORES_CONT void SetHighPoint(viskores::Float64 x, viskores::Float64 y, viskores::Float64 z)
  {
    this->SetHighPoint({ x, y, z });
  }

  /// @brief Specify the range of values to output.
  ///
  /// Values at the low plane are given @p low and values at the high plane are given
  /// @p high. Values in between the planes have a linearly interpolated value based
  /// on the relative distance between the two planes.
  VISKORES_CONT void SetRange(viskores::Float64 low, viskores::Float64 high)
  {
    this->RangeLow = low;
    this->RangeHigh = high;
  }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::Vec3f_64 LowPoint = { 0.0, 0.0, 0.0 };
  viskores::Vec3f_64 HighPoint = { 0.0, 0.0, 1.0 };
  viskores::Float64 RangeLow = 0.0, RangeHigh = 1.0;
};
} // namespace field_transform
} // namespace filter
} // namespace viskores

#endif // viskores_filter_field_transform_PointElevation_h
