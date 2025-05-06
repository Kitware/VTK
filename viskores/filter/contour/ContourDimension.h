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
#ifndef viskores_filter_contour_ContourDimension_h
#define viskores_filter_contour_ContourDimension_h

namespace viskores
{
namespace filter
{
namespace contour
{

/// @brief Identifies what type cells will be contoured.
///
/// The `ContourDimension` enum is used by the contour filters to specify which
/// dimension of cell to contour by.
enum struct ContourDimension
{
  /// @copydoc viskores::filter::contour::AbstractContour::SetInputCellDimensionToAuto
  Auto,
  /// @copydoc viskores::filter::contour::AbstractContour::SetInputCellDimensionToAll
  All,
  /// @copydoc viskores::filter::contour::AbstractContour::SetInputCellDimensionToPolyhedra
  Polyhedra,
  /// @copydoc viskores::filter::contour::AbstractContour::SetInputCellDimensionToPolygons
  Polygons,
  /// @copydoc viskores::filter::contour::AbstractContour::SetInputCellDimensionToLines
  Lines
};

}
}
} // namespace viskores::filter::contour

#endif // viskores_filter_contour_ContourDimension_h
