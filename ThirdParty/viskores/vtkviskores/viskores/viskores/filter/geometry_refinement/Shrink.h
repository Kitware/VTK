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

#ifndef viskores_filter_geometry_refinement_Shrink_h
#define viskores_filter_geometry_refinement_Shrink_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/geometry_refinement/viskores_filter_geometry_refinement_export.h>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{
/// \brief Shrink cells of an arbitrary dataset by a constant factor.
///
/// The Shrink filter shrinks the cells of a DataSet towards their centroid,
/// computed as the average position of the cell points.
/// This filter disconnects the cells, duplicating the points connected to multiple cells.
/// The resulting CellSet is always an `ExplicitCellSet`.
class VISKORES_FILTER_GEOMETRY_REFINEMENT_EXPORT Shrink : public viskores::filter::Filter
{
public:
  /// @brief Specify the scale factor to size each cell.
  ///
  /// The shrink factor specifies the ratio of the shrunk cell to its original size.
  /// This value must be between 0 and 1.
  /// A value of 1 is the same size as the input, and a value of 0 shrinks each cell to a point.
  VISKORES_CONT void SetShrinkFactor(viskores::FloatDefault factor)
  {
    this->ShrinkFactor = viskores::Min(viskores::Max(0, factor), 1); // Clamp shrink factor value
  }

  /// @copydoc SetShrinkFactor
  VISKORES_CONT viskores::FloatDefault GetShrinkFactor() const { return this->ShrinkFactor; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
  viskores::FloatDefault ShrinkFactor = 0.5f;
};
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores

#endif // viskores_filter_geometry_refinement_Shrink_h
