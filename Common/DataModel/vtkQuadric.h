// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQuadric
 * @brief   evaluate implicit quadric function
 *
 * vtkQuadric evaluates the quadric function F(x,y,z) = a0*x^2 + a1*y^2 +
 * a2*z^2 + a3*x*y + a4*y*z + a5*x*z + a6*x + a7*y + a8*z + a9. vtkQuadric is
 * a concrete implementation of vtkImplicitFunction.
 */

#ifndef vtkQuadric_h
#define vtkQuadric_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkQuadric : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkQuadric, vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct quadric with all coefficients = 1.
   */
  static vtkQuadric* New();

  ///@{
  /**
   * Evaluate quadric equation.
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;
  ///@}

  /**
   * Evaluate the gradient to the quadric equation.
   */
  void EvaluateGradient(double x[3], double g[3]) override;

  ///@{
  /**
   * Set / get the 10 coefficients of the quadric equation.
   */
  void SetCoefficients(double a[10]);
  void SetCoefficients(double a0, double a1, double a2, double a3, double a4, double a5, double a6,
    double a7, double a8, double a9);
  vtkGetVectorMacro(Coefficients, double, 10);
  ///@}

protected:
  vtkQuadric();
  ~vtkQuadric() override = default;

  double Coefficients[10];

private:
  vtkQuadric(const vtkQuadric&) = delete;
  void operator=(const vtkQuadric&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
