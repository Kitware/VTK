/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFunctionSet.h
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
// .NAME vtkFunctionSet - Abstract inteface for sets of functions
// .SECTION Description
// vtkFunctionSet specifies an abstract interface for set of functions
// of the form F_i = F_i(x_j) where F (with i=1..m) are the functions
// and x (with j=1..n) are the independent variables.
// The only supported operation is the  function evaluation at x_j.

// .SECTION See Also
// vtkImplicitDataSet vtkInterpolatedVelocityField 
// vtkInitialValueProblemSolver

#ifndef __vtkFunctionSet_h
#define __vtkFunctionSet_h

#include "vtkObject.h"

class VTK_EXPORT vtkFunctionSet : public vtkObject
{
public:
  vtkTypeMacro(vtkFunctionSet,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Evaluate functions at x_j.
  // x and f have to point to valid float arrays of appropriate 
  // sizes obtained with GetNumberOfFunctions() and
  // GetNumberOfIndependentVariables.
  virtual int FunctionValues(float* x, float* f) = 0;

  // Description:
  // Return the number of functions. Note that this is constant for 
  // a given type of set of functions and can not be changed at 
  // run time.
  virtual int GetNumberOfFunctions() { 
    return this->NumFuncs; }

  // Description:
  // Return the number of independent variables. Note that this is 
  // constant for a given type of set of functions and can not be changed  
  // at run time.
  virtual int GetNumberOfIndependentVariables() {
    return this->NumIndepVars; }

protected:
  vtkFunctionSet();
  ~vtkFunctionSet() {};
  vtkFunctionSet(const vtkFunctionSet&);
  void operator=(const vtkFunctionSet&);

  int NumFuncs;
  int NumIndepVars;

};

#endif





