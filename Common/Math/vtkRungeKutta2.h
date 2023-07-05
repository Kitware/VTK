// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRungeKutta2
 * @brief   Integrate an initial value problem using 2nd
 * order Runge-Kutta method.
 *
 *
 * This is a concrete sub-class of vtkInitialValueProblemSolver.
 * It uses a 2nd order Runge-Kutta method to obtain the values of
 * a set of functions at the next time step.
 *
 * @sa
 * vtkInitialValueProblemSolver vtkRungeKutta4 vtkRungeKutta45 vtkFunctionSet
 */

#ifndef vtkRungeKutta2_h
#define vtkRungeKutta2_h

#include "vtkCommonMathModule.h" // For export macro
#include "vtkInitialValueProblemSolver.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONMATH_EXPORT vtkRungeKutta2 : public vtkInitialValueProblemSolver
{
public:
  vtkTypeMacro(vtkRungeKutta2, vtkInitialValueProblemSolver);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct a vtkRungeKutta2 with no initial FunctionSet.
   */
  static vtkRungeKutta2* New();

  using Superclass::ComputeNextStep;
  ///@{
  /**
   * Given initial values, xprev , initial time, t and a requested time
   * interval, delT calculate values of x at t+delT (xnext).
   * delTActual is always equal to delT.
   * Since this class can not provide an estimate for the error error
   * is set to 0.
   * maxStep, minStep and maxError are unused.
   * This method returns an error code representing the nature of
   * the failure:
   * OutOfDomain = 1,
   * NotInitialized = 2,
   * UnexpectedValue = 3
   */
  int ComputeNextStep(double* xprev, double* xnext, double t, double& delT, double maxError,
    double& error, void* userData) override
  {
    double minStep = delT;
    double maxStep = delT;
    double delTActual;
    return this->ComputeNextStep(
      xprev, nullptr, xnext, t, delT, delTActual, minStep, maxStep, maxError, error, userData);
  }
  int ComputeNextStep(double* xprev, double* dxprev, double* xnext, double t, double& delT,
    double maxError, double& error, void* userData) override
  {
    double minStep = delT;
    double maxStep = delT;
    double delTActual;
    return this->ComputeNextStep(
      xprev, dxprev, xnext, t, delT, delTActual, minStep, maxStep, maxError, error, userData);
  }
  int ComputeNextStep(double* xprev, double* xnext, double t, double& delT, double& delTActual,
    double minStep, double maxStep, double maxError, double& error, void* userData) override
  {
    return this->ComputeNextStep(
      xprev, nullptr, xnext, t, delT, delTActual, minStep, maxStep, maxError, error, userData);
  }
  int ComputeNextStep(double* xprev, double* dxprev, double* xnext, double t, double& delT,
    double& delTActual, double minStep, double maxStep, double maxError, double& error,
    void* userData) override;
  ///@}

protected:
  vtkRungeKutta2();
  ~vtkRungeKutta2() override;

private:
  vtkRungeKutta2(const vtkRungeKutta2&) = delete;
  void operator=(const vtkRungeKutta2&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
