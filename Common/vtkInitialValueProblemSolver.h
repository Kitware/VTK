/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInitialValueProblemSolver.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  vtkTypeMacro(vtkInitialValueProblemSolver,vtkObject);
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




