/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFunctionSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFunctionSet - Abstract interface for sets of functions
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

#include "vtkCommonMathModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONMATH_EXPORT vtkFunctionSet : public vtkObject
{
public:
  vtkTypeMacro(vtkFunctionSet,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Evaluate functions at x_j.
  // x and f have to point to valid double arrays of appropriate
  // sizes obtained with GetNumberOfFunctions() and
  // GetNumberOfIndependentVariables.
  virtual int FunctionValues(double* x, double* f) = 0;

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
  ~vtkFunctionSet() {}

  int NumFuncs;
  int NumIndepVars;

private:
  vtkFunctionSet(const vtkFunctionSet&);  // Not implemented.
  void operator=(const vtkFunctionSet&);  // Not implemented.
};

#endif





