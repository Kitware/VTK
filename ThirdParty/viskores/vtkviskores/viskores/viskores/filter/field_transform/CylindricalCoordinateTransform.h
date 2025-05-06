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

#ifndef viskores_filter_field_transform_CylindricalCoordinateTransform_h
#define viskores_filter_field_transform_CylindricalCoordinateTransform_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/field_transform/viskores_filter_field_transform_export.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{

/// @brief Transform coordinates between Cartesian and cylindrical.
///
/// By default, this filter will transform the first coordinate system, but
/// this can be changed by setting the active field.
///
/// The resulting transformation will be set as the first coordinate system
/// in the output.
///
class VISKORES_FILTER_FIELD_TRANSFORM_EXPORT CylindricalCoordinateTransform
  : public viskores::filter::Filter
{
public:
  VISKORES_CONT CylindricalCoordinateTransform();

  /// @brief Establish a transformation from Cartesian to cylindrical coordinates.
  VISKORES_CONT void SetCartesianToCylindrical() { CartesianToCylindrical = true; }
  /// @brief Establish a transformation from cylindrical to Cartesian coordiantes.
  VISKORES_CONT void SetCylindricalToCartesian() { CartesianToCylindrical = false; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  bool CartesianToCylindrical = true;
};
} // namespace field_transform
} // namespace filter
} // namespace viskores

#endif //viskores_filter_field_transform_CylindricalCoordinateTransform_h
