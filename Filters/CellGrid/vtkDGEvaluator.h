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
#include "vtkVector.h" // for vtkVector3d

#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkCellAttribute;
class vtkCellMetadata;
class vtkDataArray;
class vtkDGCell;
class vtkInterpolateCalculator;
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

  /// Classify world-space points against cells using Newton-Raphson iteration.
  /// Convergence stores the parametric coordinates, eliminating a separate position-
  /// evaluation pass.
  bool ClassifyPoints(
    vtkCellGridEvaluator* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches);
  /// Interpolate cell-attributes onto points inside or on a cell.
  bool InterpolatePoints(
    vtkCellGridEvaluator* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches);

private:
  /// Invert the reference-to-world mapping for \a testPoint in cell \a cellId via
  /// Newton-Raphson. Returns true and sets \a rst to the parametric coordinates if
  /// Newton converged and the result lies inside the reference element.
  /// \a xyz and \a jacobian are caller-supplied working buffers reused across calls.
  static bool EvaluatePosition(vtkInterpolateCalculator* calc, vtkDGCell* dgCell, vtkIdType cellId,
    const vtkVector3d& testPoint, std::vector<double>& xyz, std::vector<double>& jacobian,
    vtkVector3d& rst);

  vtkDGEvaluator(const vtkDGEvaluator&) = delete;
  void operator=(const vtkDGEvaluator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGEvaluator_h
// VTK-HeaderTest-Exclude: vtkDGEvaluator.h
