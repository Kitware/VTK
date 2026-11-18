// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGIsocontourPointsResponder
 * @brief   Locate points on a vtkDGCell attribute's isocontour.
 *
 * This responder searches one cell type's cells for points on the isocontour
 * of a scalar Bernstein-Bezier cell-attribute, appending them to the query's
 * output. See vtkCellGridIsocontourPoints for the algorithm.
 */

#ifndef vtkDGIsocontourPointsResponder_h
#define vtkDGIsocontourPointsResponder_h

#include "vtkCellGridIsocontourPoints.h" // for inheritance
#include "vtkCellGridResponder.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;
class vtkDGCell;

class VTKFILTERSCELLGRID_EXPORT vtkDGIsocontourPointsResponder
  : public vtkCellGridResponder<vtkCellGridIsocontourPoints::Query>
{
public:
  static vtkDGIsocontourPointsResponder* New();
  vtkTypeMacro(
    vtkDGIsocontourPointsResponder, vtkCellGridResponder<vtkCellGridIsocontourPoints::Query>);

  bool Query(vtkCellGridIsocontourPoints::Query* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGIsocontourPointsResponder() = default;
  ~vtkDGIsocontourPointsResponder() override = default;

  /// CountOutputs pass: find cellType's isocontour points and cache them on
  /// \a query (SetIsocontourPoints()).
  bool FindPoints(vtkCellGridIsocontourPoints::Query* query, vtkDGCell* cellType);
  /// GenerateOutputs pass: write cellType's cached points into the query's
  /// output and place them in world space.
  bool GeneratePoints(vtkCellGridIsocontourPoints::Query* query, vtkDGCell* cellType);
  /// InterpolateAttributes pass: evaluate the grid's other attributes at the
  /// points GeneratePoints() wrote, into the output's arrays named like them.
  bool InterpolateAttributes(vtkCellGridIsocontourPoints::Query* query, vtkDGCell* cellType);

private:
  vtkDGIsocontourPointsResponder(const vtkDGIsocontourPointsResponder&) = delete;
  void operator=(const vtkDGIsocontourPointsResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGIsocontourPointsResponder_h
