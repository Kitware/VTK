// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCardinalSpline
 * @brief   computes an interpolating spline using a
 * a Cardinal basis.
 *
 *
 * vtkCardinalSpline is a concrete implementation of vtkSpline using a
 * Cardinal basis.
 *
 * @sa
 * vtkSpline vtkKochanekSpline
 */

#ifndef vtkCardinalSpline_h
#define vtkCardinalSpline_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkSpline.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkCardinalSpline : public vtkSpline
{
public:
  static vtkCardinalSpline* New();

  vtkTypeMacro(vtkCardinalSpline, vtkSpline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Compute Cardinal Splines for each dependent variable
   */
  void Compute() override;

  /**
   * Evaluate a 1D cardinal spline.
   */
  double Evaluate(double t) override;

  /**
   * Deep copy of cardinal spline data.
   */
  void DeepCopy(vtkSpline* s) override;

protected:
  vtkCardinalSpline();
  ~vtkCardinalSpline() override = default;

  void Fit1D(int size, double* x, double* y, double* w, double coefficients[][4],
    int leftConstraint, double leftValue, int rightConstraint, double rightValue);

  void FitClosed1D(int size, double* x, double* y, double* w, double coefficients[][4]);

private:
  vtkCardinalSpline(const vtkCardinalSpline&) = delete;
  void operator=(const vtkCardinalSpline&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
