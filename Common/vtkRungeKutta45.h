/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRungeKutta45.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRungeKutta45 - Integrate an initial value problem using 5th
// order Runge-Kutta method with adaptive stepsize control.

// .SECTION Description
// This is a concrete sub-class of vtkInitialValueProblemSolver.
// It uses a 5th order Runge-Kutta method with stepsize control to obtain 
// the values of a set of functions at the next time step. The stepsize
// is adjusted by calculating an estimated error using an embedded 4th
// order Runge-Kutta formula:
// Press, W. H. et al., 1992, Numerical Recipes in Fortran, Second
// Edition, Cambridge University Press
// Cash, J.R. and Karp, A.H. 1990, ACM Transactions on Mathematical
// Software, vol 16, pp 201-222

// .SECTION See Also
// vtkInitialValueProblemSolver vtkRungeKutta4 vtkRungeKutta2 vtkFunctionSet

#ifndef __vtkRungeKutta45_h
#define __vtkRungeKutta45_h

#include "vtkInitialValueProblemSolver.h"

class VTK_COMMON_EXPORT vtkRungeKutta45 : public vtkInitialValueProblemSolver
{
public:
  vtkTypeRevisionMacro(vtkRungeKutta45,vtkInitialValueProblemSolver);

  // Description:
  // Construct a vtkRungeKutta45 with no initial FunctionSet.
  static vtkRungeKutta45 *New();

  // Description:
  // Given initial values, xprev , initial time, t and a requested time 
  // interval, delT calculate values of x at t+delTActual (xnext).
  // Possibly delTActual != delT. This may occur
  // because this solver supports adaptive stepsize control. It tries 
  // to change to stepsize such that
  // the (estimated) error of the integration is less than maxError.
  // The solver will not set the stepsize smaller than minStep or
  // larger than maxStep (note that maxStep and minStep should both
  // be positive, whereas delT can be negative).
  // Also note that delT is an in/out argument. vtkRungeKutta45
  // will modify delT to reflect the best (estimated) size for the next
  // integration step.
  // An estimated value for the error is returned (by reference) in error.
  // This is the norm of the error vector if there are more than
  // one function to be integrated.
  // This method returns an error code representing the nature of
  // the failure:
  // OutOfDomain = 1,
  // NotInitialized = 2,
  // UnexpectedValue = 3
  virtual int ComputeNextStep(float* xprev, float* xnext, float t,
			      float& delT, float maxError, float& error) 
    {
      float minStep = delT;
      float maxStep = delT;
      float delTActual;
      return this->ComputeNextStep(xprev, 0, xnext, t, delT, delTActual,
				   minStep, maxStep, maxError, error);
    }
  virtual int ComputeNextStep(float* xprev, float* dxprev, float* xnext, 
			      float t, float& delT, 
			      float maxError, float& error)
    {
      float minStep = delT;
      float maxStep = delT;
      float delTActual;
      return this->ComputeNextStep(xprev, dxprev, xnext, t, delT, delTActual,
				   minStep, maxStep, maxError, error);
    }
  virtual int ComputeNextStep(float* xprev, float* xnext, 
			      float t, float& delT, float& delTActual,
			      float minStep, float maxStep,
			      float maxError, float& error)
    {
      return this->ComputeNextStep(xprev, 0, xnext, t, delT, delTActual,
				   minStep, maxStep, maxError, error);
    }
  virtual int ComputeNextStep(float* xprev, float* dxprev, float* xnext, 
			      float t, float& delT, float& delTActual,
			      float minStep, float maxStep, 
			      float maxError, float& error);

  // Description:
  // Create concrete instance of this object.
  virtual vtkInitialValueProblemSolver* MakeObject() 
    {
      return vtkRungeKutta45::New();
    }

protected:
  vtkRungeKutta45();
  ~vtkRungeKutta45();

  virtual void Initialize();

  // Cash-Karp parameters
  static double A[5];
  static double B[5][5];
  static double C[6];
  static double DC[6];

  float* NextDerivs[6];

  int ComputeAStep(float* xprev, float* dxprev, float* xnext, float t, 
		   float& delT,  float& error);

private:
  vtkRungeKutta45(const vtkRungeKutta45&);  // Not implemented.
  void operator=(const vtkRungeKutta45&);  // Not implemented.
};

#endif








