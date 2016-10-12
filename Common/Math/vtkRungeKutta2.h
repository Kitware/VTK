/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRungeKutta2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKCOMMONMATH_EXPORT vtkRungeKutta2 : public vtkInitialValueProblemSolver
{
public:
  vtkTypeMacro(vtkRungeKutta2,vtkInitialValueProblemSolver);

  /**
   * Construct a vtkRungeKutta2 with no initial FunctionSet.
   */
  static vtkRungeKutta2 *New();

  //@{
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
  int ComputeNextStep(double* xprev, double* xnext,
                      double t, double& delT,
                      double maxError, double& error) VTK_OVERRIDE
  {
      double minStep = delT;
      double maxStep = delT;
      double delTActual;
      return this->ComputeNextStep(xprev, 0, xnext, t, delT, delTActual,
                                   minStep, maxStep, maxError, error);
  }
  int ComputeNextStep(double* xprev, double* dxprev, double* xnext,
                      double t, double& delT,
                      double maxError, double& error) VTK_OVERRIDE
  {
      double minStep = delT;
      double maxStep = delT;
      double delTActual;
      return this->ComputeNextStep(xprev, dxprev, xnext, t, delT, delTActual,
                                   minStep, maxStep, maxError, error);
  }
  int ComputeNextStep(double* xprev, double* xnext,
                      double t, double& delT, double& delTActual,
                      double minStep, double maxStep,
                      double maxError, double& error) VTK_OVERRIDE
  {
      return this->ComputeNextStep(xprev, 0, xnext, t, delT, delTActual,
                                   minStep, maxStep, maxError, error);
  }
  int ComputeNextStep(double* xprev, double* dxprev, double* xnext,
                      double t, double& delT, double& delTActual,
                      double minStep, double maxStep,
                      double maxError, double& error) VTK_OVERRIDE;
  //@}

protected:
  vtkRungeKutta2();
  ~vtkRungeKutta2() VTK_OVERRIDE;
private:
  vtkRungeKutta2(const vtkRungeKutta2&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRungeKutta2&) VTK_DELETE_FUNCTION;
};

#endif








// VTK-HeaderTest-Exclude: vtkRungeKutta2.h
