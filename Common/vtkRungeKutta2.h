/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRungeKutta2.h
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
// .NAME vtkRungeKutta2 - Integrate an initial value problem using 2nd
// order Runge-Kutta method.

// .SECTION Description
// This is a concrete sub-class of vtkInitialValueProblemSolver.
// It uses a 2nd order Runge-Kutta method to obtain the values of
// a set of functions at the next time step.

// .SECTION See Also
// vtkInitialValueProblemSolver vtkRungeKutta4 vtkFunctionSet

#ifndef __vtkRungeKutta2_h
#define __vtkRungeKutta2_h

#include "vtkInitialValueProblemSolver.h"

class VTK_COMMON_EXPORT vtkRungeKutta2 : public vtkInitialValueProblemSolver
{
public:
  vtkTypeMacro(vtkRungeKutta2,vtkInitialValueProblemSolver);

  // Description:
  // Construct a vtkRungeKutta2 with no initial FunctionSet.
  static vtkRungeKutta2 *New();

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
      return vtkRungeKutta2::New();
    }

protected:
  vtkRungeKutta2();
  ~vtkRungeKutta2();
private:
  vtkRungeKutta2(const vtkRungeKutta2&);  // Not implemented.
  void operator=(const vtkRungeKutta2&);  // Not implemented.
};

#endif








