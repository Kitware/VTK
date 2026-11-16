// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGFindCriticalPointsResponder
 * @brief   Locate a vtkDGCell attribute's interior critical points.
 *
 * This responder searches one cell type's cells for the interior critical
 * points (minima, maxima, saddles) of a scalar Bernstein-Bezier
 * cell-attribute, appending one point to the query's output per critical
 * point found.
 */

#ifndef vtkDGFindCriticalPointsResponder_h
#define vtkDGFindCriticalPointsResponder_h

#include "vtkCellGridFindCriticalPoints.h" // for inheritance
#include "vtkCellGridResponder.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellAttribute;
class vtkCellMetadata;
class vtkDGCell;

class VTKFILTERSCELLGRID_EXPORT vtkDGFindCriticalPointsResponder
  : public vtkCellGridResponder<vtkCellGridFindCriticalPoints::Query>
{
public:
  static vtkDGFindCriticalPointsResponder* New();
  vtkTypeMacro(
    vtkDGFindCriticalPointsResponder, vtkCellGridResponder<vtkCellGridFindCriticalPoints::Query>);

  bool Query(vtkCellGridFindCriticalPoints::Query* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGFindCriticalPointsResponder() = default;
  ~vtkDGFindCriticalPointsResponder() override = default;

  /// CountOutputs pass: solve for cellType's critical points, classify each
  /// one, and cache both on \a query (SetCriticalPoints()/
  /// SetCriticalPointTypes()).
  bool CountCriticalPoints(vtkCellGridFindCriticalPoints::Query* query, vtkDGCell* cellType);
  /// GenerateOutputs pass: write cellType's cached critical points and
  /// classifications (from CountCriticalPoints()) into the query's output.
  bool GenerateCriticalPoints(vtkCellGridFindCriticalPoints::Query* query, vtkDGCell* cellType);

private:
  vtkDGFindCriticalPointsResponder(const vtkDGFindCriticalPointsResponder&) = delete;
  void operator=(const vtkDGFindCriticalPointsResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGFindCriticalPointsResponder_h
// VTK-HeaderTest-Exclude: vtkDGFindCriticalPointsResponder.h
