// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGEvaluator
 * @brief   Classify world points, evaluate cell parameters, and interpolate attributes.
 *
 * Given a set of input points in world coordinates,
 * classify these points (determine which cells they are inside);
 * evaluate these points (determine the parametric coordinates of the point inside each cell);
 * and interpolate an attribute (evaluate the value of an attribute at the parametric coords).
 */

#ifndef vtkDGEvaluator_h
#define vtkDGEvaluator_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkCellGridEvaluator.h" // for inheritance
#include "vtkCellGridResponder.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellAttribute;
class vtkCellMetadata;
class vtkDataArray;
class vtkTypeInt64Array;

class VTKFILTERSCELLGRID_EXPORT vtkDGEvaluator : public vtkCellGridResponder<vtkCellGridEvaluator>
{
public:
  static vtkDGEvaluator* New();
  vtkTypeMacro(vtkDGEvaluator, vtkCellGridResponder<vtkCellGridEvaluator>);

  bool Query(
    vtkCellGridEvaluator* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches) override;

protected:
  vtkDGEvaluator() = default;
  ~vtkDGEvaluator() override = default;

  /// Mark points that are potentially inside a cell.
  bool ClassifyPoints(
    vtkCellGridEvaluator* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches);
  /// Determine parametric coordinates of points inside or on a cell.
  bool EvaluatePositions(
    vtkCellGridEvaluator* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches);
  /// Interpolate cell-attributes onto points inside or on a cell.
  bool InterpolatePoints(
    vtkCellGridEvaluator* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches);

private:
  vtkDGEvaluator(const vtkDGEvaluator&) = delete;
  void operator=(const vtkDGEvaluator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGEvaluator_h
// VTK-HeaderTest-Exclude: vtkDGEvaluator.h
