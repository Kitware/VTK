// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInterpolatedVelocityField
 * @brief   A concrete class for obtaining
 *  the interpolated velocity values at a point.
 *
 *
 * vtkInterpolatedVelocityField acts as a continuous velocity field via
 * cell interpolation on a vtkDataSet, NumberOfIndependentVariables = 4
 * (x,y,z,t) and NumberOfFunctions = 3 (u,v,w). As a concrete sub-class
 * of vtkCompositeInterpolatedVelocityField, this class adopts two levels
 * of cell caching for faster though less robust cell location than its
 * sibling class vtkCellLocatorInterpolatedVelocityField. Level #0 begins
 * with intra-cell caching. Specifically, if the previous cell is valid
 * and the next point is still within it, ( vtkCell::EvaluatePosition()
 * returns 1, coupled with the new parametric coordinates and weights ),
 * the function values are interpolated and vtkCell::EvaluatePosition()
 * is invoked only. If it fails, level #1 follows by inter-cell location
 * of the target cell (that contains the next point). By inter-cell, the
 * previous cell gives an important clue / guess or serves as an immediate
 * neighbor to aid in the location of the target cell (as is typically the
 * case with integrating a streamline across cells) by means of vtkDataSet::
 * FindCell(). If this still fails, a global cell search is invoked via
 * vtkDataSet::FindCell().
 *
 * Regardless of inter-cell or global search, a point locator is employed as
 * a crucial tool underlying the interpolation process. The use of a point
 * locator, while faster than a cell locator, is not optimal and may cause
 * vtkInterpolatedVelocityField to return incorrect results (i.e., premature
 * streamline termination) for datasets defined on complex grids (especially
 * those this discontinuous/incompatible cells). In these cases, try
 * vtkCellLocatorInterpolatedVelocityField which produces the best results at
 * the cost of speed.
 *
 * @warning
 * vtkInterpolatedVelocityField is not thread safe. A new instance should be
 * created by each thread.
 *
 * @sa
 *  vtkCompositeInterpolatedVelocityField vtkCellLocatorInterpolatedVelocityField
 *  vtkGenericInterpolatedVelocityField vtkCachingInterpolatedVelocityField
 *  vtkTemporalInterpolatedVelocityField vtkFunctionSet vtkStreamTracer
 */

#ifndef vtkInterpolatedVelocityField_h
#define vtkInterpolatedVelocityField_h

#include "vtkCompositeInterpolatedVelocityField.h"
#include "vtkDeprecation.h"            // For VTK_DEPRECATED_IN_9_2_0
#include "vtkFiltersFlowPathsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTK_DEPRECATED_IN_9_2_0(
  "Use vtkCompositeInterpolatedVelocityField instead of vtkCellLocatorInterpolatedVelocityField "
  "and set the desired strategy.") VTKFILTERSFLOWPATHS_EXPORT vtkInterpolatedVelocityField
  : public vtkCompositeInterpolatedVelocityField
{
public:
  /**
   * Construct a vtkCompositeInterpolatedVelocityField subclass.
   */
  static vtkInterpolatedVelocityField* New();

  ///@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkInterpolatedVelocityField, vtkCompositeInterpolatedVelocityField);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

protected:
  vtkInterpolatedVelocityField();
  ~vtkInterpolatedVelocityField() override = default;

private:
  vtkInterpolatedVelocityField(const vtkInterpolatedVelocityField&) = delete;
  void operator=(const vtkInterpolatedVelocityField&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkInterpolatedVelocityField.h
