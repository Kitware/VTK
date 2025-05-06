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

#ifndef viskores_filter_geometry_refinement_Tube_h
#define viskores_filter_geometry_refinement_Tube_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/geometry_refinement/viskores_filter_geometry_refinement_export.h>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{

/// @brief Generate a tube around each line and polyline.
///
/// The radius, number of sides, and end capping can be specified for each tube.
/// The orientation of the geometry of the tube are computed automatically using
/// a heuristic to minimize the twisting along the input data set.
///
class VISKORES_FILTER_GEOMETRY_REFINEMENT_EXPORT Tube : public viskores::filter::Filter
{
public:
  /// @brief Specify the radius of each tube.
  VISKORES_CONT void SetRadius(viskores::FloatDefault r) { this->Radius = r; }

  /// @brief Specify the number of sides for each tube.
  ///
  /// The tubes are generated using a polygonal approximation. This option determines
  /// how many facets will be generated around the tube.
  VISKORES_CONT void SetNumberOfSides(viskores::Id n) { this->NumberOfSides = n; }

  /// The `Tube` filter can optionally add a cap at the ends of each tube. This option
  /// specifies whether that cap is generated.
  VISKORES_CONT void SetCapping(bool v) { this->Capping = v; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::FloatDefault Radius{};
  viskores::Id NumberOfSides = 6;
  bool Capping = false;
};
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores

#endif // viskores_filter_geometry_refinement_Tube_h
