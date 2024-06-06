// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInterpolateCalculator
 * @brief   Interpolate a field's value and possibly derivatives at a point in a cell.
 */

#ifndef vtkInterpolateCalculator_h
#define vtkInterpolateCalculator_h

#include "vtkCellAttributeCalculator.h"
#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkVector.h"                // For API.

#include <array>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkIdTypeArray;

/**\brief Calculate field values at a point in a cell's parametric space.
 *
 */
class VTKFILTERSCELLGRID_EXPORT vtkInterpolateCalculator : public vtkCellAttributeCalculator
{
public:
  vtkTypeMacro(vtkInterpolateCalculator, vtkCellAttributeCalculator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// @name InterpolatingValues Interpolating Values
  ///@{
  /// Subclasses must override this method to perform evaluation.
  ///
  /// You must resize \a value before calling Evaluate so there is enough
  /// space to hold the resulting value(s).
  virtual void Evaluate(vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& value) = 0;

  /// Subclasses may override this method to perform multiple evaluations at a time.
  virtual void Evaluate(vtkIdTypeArray* cellIds, vtkDataArray* rst, vtkDataArray* result) = 0;

  /// Return true if the function has an analytic derivative.
  virtual bool AnalyticDerivative() const { return false; }

  /// Return the derivative of the function at \a rst.
  ///
  /// The derivative should be ordered as d/dx, d/dy, then d/dz.
  ///
  /// If you do not override \a AnalyticDerivative and \a EvaluateDerivative, this
  /// class will approximate the derivative by perturbing along each axis to compute
  /// a finite difference.
  ///
  /// In the case of a non-analytic derivative, you can control the magnitude of the
  /// difference along each axis by passing a different \a neighborhood value.
  virtual void EvaluateDerivative(vtkIdType cellId, const vtkVector3d& rst,
    std::vector<double>& jacobian, double neighborhood = 1e-3);

  /// Subclasses may override this method to perform multiple derivative-evaluations at a time.
  virtual void EvaluateDerivative(
    vtkIdTypeArray* cellIds, vtkDataArray* rst, vtkDataArray* result) = 0;

#if 0
  /// Return true if the given parametric coordinates lie inside the cell (or
  /// its closure) and false otherwise.
  ///
  /// Cells with a fixed shape are expected to ignore \a cellId.
  virtual bool IsInside(vtkIdType cellId, const vtkVector3d& rst) = 0;
#endif

protected:
  vtkInterpolateCalculator() = default;
  ~vtkInterpolateCalculator() override = default;

private:
  vtkInterpolateCalculator(const vtkInterpolateCalculator&) = delete;
  void operator=(const vtkInterpolateCalculator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkInterpolateCalculator_h
