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

#ifndef viskores_filter_geometry_refinement_SplitSharpEdges_h
#define viskores_filter_geometry_refinement_SplitSharpEdges_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/geometry_refinement/viskores_filter_geometry_refinement_export.h>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{
/// @brief Split sharp polygon mesh edges with a large feature angle between the adjacent cells.
///
/// Split sharp manifold edges where the feature angle between the adjacent polygonal cells
/// are larger than a threshold value. The feature angle is the angle between the normals of
/// the two polygons. Two polygons on the same plane have a feature angle of 0. Perpendicular
/// polygons have a feature angle of 90 degrees.
///
/// When an edge is split, it adds a new point to the coordinates and updates the connectivity
/// of an adjacent surface. For example, consider two adjacent triangles (0,1,2) and (2,1,3)
/// where edge (1,2) needs to be split. Two new points 4 (duplication of point 1) and 5
/// (duplication of point 2) would be added and the later triangle's connectivity would be
/// changed to (5,4,3). By default, all old point's fields would be copied to the new point.
///
/// Note that "split" edges do not have space added between them. They are still adjacent
/// visually, but the topology becomes disconnectered there. Splitting sharp edges is most
/// useful to duplicate normal shading vectors to make a sharp shading effect.
///
class VISKORES_FILTER_GEOMETRY_REFINEMENT_EXPORT SplitSharpEdges : public viskores::filter::Filter
{
public:
  /// @brief Specify the feature angle threshold to split on.
  ///
  /// The feature angle is the angle between the normals of the two polygons. Two polygons on
  /// the same plane have a feature angle of 0. Perpendicular polygons have a feature angle
  /// of 90 degrees.
  ///
  /// Any edge with a feature angle larger than this threshold will be split. The feature
  /// angle is specified in degrees. The default value is 30 degrees.
  VISKORES_CONT void SetFeatureAngle(viskores::FloatDefault value) { this->FeatureAngle = value; }

  /// @copydoc SetFeatureAngle
  VISKORES_CONT viskores::FloatDefault GetFeatureAngle() const { return this->FeatureAngle; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::FloatDefault FeatureAngle = 30.0;
};
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores

#endif // viskores_filter_geometry_refinement_SplitSharpEdges_h
