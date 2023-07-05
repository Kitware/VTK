// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRungeKutta4
 * @brief   Integrate an initial value problem using 4th
 * order Runge-Kutta method.
 *
 *
 * This is a concrete sub-class of vtkInitialValueProblemSolver.
 * It uses a 4th order Runge-Kutta method to obtain the values of
 * a set of functions at the next time step.
 *
 * @sa
 * vtkInitialValueProblemSolver vtkRungeKutta45 vtkRungeKutta2 vtkFunctionSet
 */

#ifndef vtkRungeKutta4_h
#define vtkRungeKutta4_h

#include "vtkCommonMathModule.h" // For export macro
#include "vtkInitialValueProblemSolver.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONMATH_EXPORT vtkRungeKutta4 : public vtkInitialValueProblemSolver
{
public:
  vtkTypeMacro(vtkRungeKutta4, vtkInitialValueProblemSolver);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct a vtkRungeKutta4 with no initial FunctionSet.
   */
  static vtkRungeKutta4* New();

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
  vtkRungeKutta4();
  ~vtkRungeKutta4() override;

  void Initialize() override;

  double* NextDerivs[3];

private:
  vtkRungeKutta4(const vtkRungeKutta4&) = delete;
  void operator=(const vtkRungeKutta4&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
