/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRungeKutta45.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRungeKutta45
 * @brief   Integrate an initial value problem using 5th
 * order Runge-Kutta method with adaptive stepsize control.
 *
 *
 * This is a concrete sub-class of vtkInitialValueProblemSolver.
 * It uses a 5th order Runge-Kutta method with stepsize control to obtain
 * the values of a set of functions at the next time step. The stepsize
 * is adjusted by calculating an estimated error using an embedded 4th
 * order Runge-Kutta formula:
 * Press, W. H. et al., 1992, Numerical Recipes in Fortran, Second
 * Edition, Cambridge University Press
 * Cash, J.R. and Karp, A.H. 1990, ACM Transactions on Mathematical
 * Software, vol 16, pp 201-222
 *
 * @sa
 * vtkInitialValueProblemSolver vtkRungeKutta4 vtkRungeKutta2 vtkFunctionSet
*/

#ifndef vtkRungeKutta45_h
#define vtkRungeKutta45_h

#include "vtkCommonMathModule.h" // For export macro
#include "vtkInitialValueProblemSolver.h"

class VTKCOMMONMATH_EXPORT vtkRungeKutta45 : public vtkInitialValueProblemSolver
{
public:
  vtkTypeMacro(vtkRungeKutta45,vtkInitialValueProblemSolver);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct a vtkRungeKutta45 with no initial FunctionSet.
   */
  static vtkRungeKutta45 *New();

  //@{
  /**
   * Given initial values, xprev , initial time, t and a requested time
   * interval, delT calculate values of x at t+delTActual (xnext).
   * Possibly delTActual != delT. This may occur
   * because this solver supports adaptive stepsize control. It tries
   * to change to stepsize such that
   * the (estimated) error of the integration is less than maxError.
   * The solver will not set the stepsize smaller than minStep or
   * larger than maxStep (note that maxStep and minStep should both
   * be positive, whereas delT can be negative).
   * Also note that delT is an in/out argument. vtkRungeKutta45
   * will modify delT to reflect the best (estimated) size for the next
   * integration step.
   * An estimated value for the error is returned (by reference) in error.
   * This is the norm of the error vector if there are more than
   * one function to be integrated.
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
  vtkRungeKutta45();
  ~vtkRungeKutta45() VTK_OVERRIDE;

  void Initialize() VTK_OVERRIDE;

  // Cash-Karp parameters
  static double A[5];
  static double B[5][5];
  static double C[6];
  static double DC[6];

  double* NextDerivs[6];

  int ComputeAStep(double* xprev, double* dxprev, double* xnext, double t,
                   double& delT,  double& delTActual, double& error);

private:
  vtkRungeKutta45(const vtkRungeKutta45&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRungeKutta45&) VTK_DELETE_FUNCTION;
};

#endif

