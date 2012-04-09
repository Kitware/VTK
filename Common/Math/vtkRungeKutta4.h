/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRungeKutta4.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRungeKutta4 - Integrate an initial value problem using 4th
// order Runge-Kutta method.

// .SECTION Description
// This is a concrete sub-class of vtkInitialValueProblemSolver.
// It uses a 4th order Runge-Kutta method to obtain the values of
// a set of functions at the next time step.

// .SECTION See Also
// vtkInitialValueProblemSolver vtkRungeKutta45 vtkRungeKutta2 vtkFunctionSet

#ifndef __vtkRungeKutta4_h
#define __vtkRungeKutta4_h

#include "vtkCommonMathModule.h" // For export macro
#include "vtkInitialValueProblemSolver.h"

class VTKCOMMONMATH_EXPORT vtkRungeKutta4 : public vtkInitialValueProblemSolver
{
public:
  vtkTypeMacro(vtkRungeKutta4,vtkInitialValueProblemSolver);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a vtkRungeKutta4 with no initial FunctionSet.
  static vtkRungeKutta4 *New();


  // Description:
  // Given initial values, xprev , initial time, t and a requested time
  // interval, delT calculate values of x at t+delT (xnext).
  // delTActual is always equal to delT.
  // Since this class can not provide an estimate for the error error
  // is set to 0.
  // maxStep, minStep and maxError are unused.
  // This method returns an error code representing the nature of
  // the failure:
  // OutOfDomain = 1,
  // NotInitialized = 2,
  // UnexpectedValue = 3
  virtual int ComputeNextStep(double* xprev, double* xnext, double t,
                              double& delT, double maxError, double& error)
    {
      double minStep = delT;
      double maxStep = delT;
      double delTActual;
      return this->ComputeNextStep(xprev, 0, xnext, t, delT, delTActual,
                                   minStep, maxStep, maxError, error);
    }
  virtual int ComputeNextStep(double* xprev, double* dxprev, double* xnext,
                              double t, double& delT,
                              double maxError, double& error)
    {
      double minStep = delT;
      double maxStep = delT;
      double delTActual;
      return this->ComputeNextStep(xprev, dxprev, xnext, t, delT, delTActual,
                                   minStep, maxStep, maxError, error);
    }
  virtual int ComputeNextStep(double* xprev, double* xnext,
                              double t, double& delT, double& delTActual,
                              double minStep, double maxStep,
                              double maxError, double& error)
    {
      return this->ComputeNextStep(xprev, 0, xnext, t, delT, delTActual,
                                   minStep, maxStep, maxError, error);
    }
  virtual int ComputeNextStep(double* xprev, double* dxprev, double* xnext,
                              double t, double& delT, double& delTActual,
                              double minStep, double maxStep,
                              double maxError, double& error);

protected:
  vtkRungeKutta4();
  ~vtkRungeKutta4();

  virtual void Initialize();

  double* NextDerivs[3];
private:
  vtkRungeKutta4(const vtkRungeKutta4&);  // Not implemented.
  void operator=(const vtkRungeKutta4&);  // Not implemented.
};

#endif








