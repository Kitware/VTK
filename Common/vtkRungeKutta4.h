/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRungeKutta4.h
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
// .NAME vtkRungeKutta4 - Integrate an initial value problem using 4th
// order Runge-Kutta method.

// .SECTION Description
// This is a concrete sub-class of vtkInitialValueProblemSolver.
// It uses a 4th order Runge-Kutta method to obtain the values of
// a set of functions at the next time step.

// .SECTION See Also
// vtkInitialValueProblemSolver vtkRungeKutta2 vtkFunctionSet

#ifndef __vtkRungeKutta4_h
#define __vtkRungeKutta4_h

#include "vtkInitialValueProblemSolver.h"

class VTK_COMMON_EXPORT vtkRungeKutta4 : public vtkInitialValueProblemSolver
{
public:
  vtkTypeRevisionMacro(vtkRungeKutta4,vtkInitialValueProblemSolver);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a vtkRungeKutta4 with no initial FunctionSet.
  static vtkRungeKutta4 *New();

  // Description:
  // Given initial values, xprev , initial time, t and time interval, delT
  // calculate values of x at t+delT (xnext)
  // It returns an estimated value for the error (not implemented yet)
  // or -1 on failure (for example, if the integration moves out of
  // a data set)
  virtual float ComputeNextStep(float* xprev, float* xnext, float t,
                                float delT) 
    {
      return this->ComputeNextStep(xprev, 0, xnext, t, delT);
    }
  virtual float ComputeNextStep(float* xprev, float* dxprev, float* xnext, 
                                float t, float delT);

  // Description:
  // Create concrete instance of this object.
  virtual vtkInitialValueProblemSolver* MakeObject() 
    {
      return vtkRungeKutta4::New();
    }


protected:
  vtkRungeKutta4();
  ~vtkRungeKutta4();

  virtual void Initialize();

  float* NextDerivs[3];
private:
  vtkRungeKutta4(const vtkRungeKutta4&);  // Not implemented.
  void operator=(const vtkRungeKutta4&);  // Not implemented.
};

#endif








