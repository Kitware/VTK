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

#ifndef viskores_filter_geometry_refinement_ConvertToPointCloud_h
#define viskores_filter_geometry_refinement_ConvertToPointCloud_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/geometry_refinement/viskores_filter_geometry_refinement_export.h>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{

/// @brief Convert a `DataSet` to a point cloud.
///
/// A point cloud in Viskores is represented as a data set with "vertex" shape cells.
/// This filter replaces the `CellSet` in a `DataSet` with a `CellSet` of only
/// vertex cells. There will be one cell per point.
///
/// This filter is useful for dropping the cells of any `DataSet` so that you can
/// operate on it as just a collection of points. It is also handy for completing
/// a `DataSet` that does not have a `CellSet` associated with it or has points
/// that do not belong to cells.
///
/// Note that all fields associated with cells are dropped. This is because the
/// cells are dropped.
///
class VISKORES_FILTER_GEOMETRY_REFINEMENT_EXPORT ConvertToPointCloud
  : public viskores::filter::Filter
{
  bool AssociateFieldsWithCells = false;

public:
  /// By default, all the input point fields are kept as point fields in the output.
  /// However, the output has exactly one cell per point and it might be easier to
  /// treat the fields as cell fields. When this flag is turned on, the point field
  /// association is changed to cell.
  ///
  /// Note that any field that is marked as point coordinates will remain as point
  /// fields. It is not valid to set a cell field as the point coordinates.
  ///
  VISKORES_CONT void SetAssociateFieldsWithCells(bool flag)
  {
    this->AssociateFieldsWithCells = flag;
  }
  /// @copydoc SetAssociateFieldsWithCells
  VISKORES_CONT bool GetAssociateFieldsWithCells() const { return this->AssociateFieldsWithCells; }

protected:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

}
}
} // namespace viskores::filter::geometry_refinement

#endif //viskores_filter_geometry_refinement_ConvertToPointCloud_h
