// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkKochanekSpline
 * @brief   computes an interpolating spline using a Kochanek basis.
 *
 * Implements the Kochanek interpolating spline described in: Kochanek, D.,
 * Bartels, R., "Interpolating Splines with Local Tension, Continuity, and
 * Bias Control," Computer Graphics, vol. 18, no. 3, pp. 33-41, July 1984.
 * These splines give the user more control over the shape of the curve than
 * the cardinal splines implemented in vtkCardinalSpline. Three parameters
 * can be specified. All have a range from -1 to 1.
 *
 * Tension controls how sharply the curve bends at an input point. A
 * value of -1 produces more slack in the curve. A value of 1 tightens
 * the curve.
 *
 * Continuity controls the continuity of the first derivative at input
 * points.
 *
 * Bias controls the direction of the curve at it passes through an input
 * point. A value of -1 undershoots the point while a value of 1
 * overshoots the point.
 *
 * These three parameters give the user broad control over the shape of
 * the interpolating spline. The original Kochanek paper describes the
 * effects nicely and is recommended reading.
 *
 * @sa
 * vtkSpline vtkCardinalSpline
 */

#ifndef vtkKochanekSpline_h
#define vtkKochanekSpline_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkSpline.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkKochanekSpline : public vtkSpline
{
public:
  vtkTypeMacro(vtkKochanekSpline, vtkSpline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct a KochanekSpline with the following defaults: DefaultBias = 0,
   * DefaultTension = 0, DefaultContinuity = 0.
   */
  static vtkKochanekSpline* New();

  /**
   * Compute Kochanek Spline coefficients.
   */
  void Compute() override;

  /**
   * Evaluate a 1D Kochanek spline.
   */
  double Evaluate(double t) override;

  ///@{
  /**
   * Set the bias for all points. Default is 0.
   */
  vtkSetMacro(DefaultBias, double);
  vtkGetMacro(DefaultBias, double);
  ///@}

  ///@{
  /**
   * Set the tension for all points. Default is 0.
   */
  vtkSetMacro(DefaultTension, double);
  vtkGetMacro(DefaultTension, double);
  ///@}

  ///@{
  /**
   * Set the continuity for all points. Default is 0.
   */
  vtkSetMacro(DefaultContinuity, double);
  vtkGetMacro(DefaultContinuity, double);
  ///@}

  /**
   * Deep copy of cardinal spline data.
   */
  void DeepCopy(vtkSpline* s) override;

protected:
  vtkKochanekSpline();
  ~vtkKochanekSpline() override = default;

  void Fit1D(int size, double* x, double* y, double tension, double bias, double continuity,
    double coefficients[][4], int leftConstraint, double leftValue, int rightConstraint,
    double rightValue);

  double DefaultBias;
  double DefaultTension;
  double DefaultContinuity;

private:
  vtkKochanekSpline(const vtkKochanekSpline&) = delete;
  void operator=(const vtkKochanekSpline&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
