/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInitialValueProblemSolver.h
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
// .NAME vtkInitialValueProblemSolver - Integrate a set of ordinary
// differential equations (initial value problem) in time.

// .SECTION Description
// Given a vtkFunctionSet which returns dF_i(x_j, t)/dt given x_j and
// t, vtkInitialValueProblemSolver computes the value of F_i at t+deltat.

// .SECTION Warning
// vtkInitialValueProblemSolver and it's subclasses are not thread-safe.
// You should create a new integrator for each thread.

// .SECTION See Also
// vtkRungeKutta2 vtkRungeKutta4

#ifndef __vtkInitialValueProblemSolver_h
#define __vtkInitialValueProblemSolver_h

#include "vtkFunctionSet.h"

class VTK_COMMON_EXPORT vtkInitialValueProblemSolver : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkInitialValueProblemSolver,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

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
                                float t, float delT) = 0;

  // Description:
  // Create concrete instance of the object.
  virtual vtkInitialValueProblemSolver *MakeObject()=0;
  
  // Description:
  // Set / get the dataset used for the implicit function evaluation.
  virtual void SetFunctionSet(vtkFunctionSet* functionset);
  vtkGetObjectMacro(FunctionSet,vtkFunctionSet);

protected:
  vtkInitialValueProblemSolver();
  ~vtkInitialValueProblemSolver();

  virtual void Initialize();

  vtkFunctionSet* FunctionSet;

  float* Vals;
  float* Derivs;
  int Initialized;
private:
  vtkInitialValueProblemSolver(const vtkInitialValueProblemSolver&);  // Not implemented.
  void operator=(const vtkInitialValueProblemSolver&);  // Not implemented.
};

#endif




