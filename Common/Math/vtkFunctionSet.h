// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFunctionSet
 * @brief   Abstract interface for sets of functions
 *
 * vtkFunctionSet specifies an abstract interface for set of functions
 * of the form F_i = F_i(x_j) where F (with i=1..m) are the functions
 * and x (with j=1..n) are the independent variables.
 * The only supported operation is the function evaluation at x_j.
 *
 * @sa
 * vtkImplicitDataSet vtkCompositeInterpolatedVelocityField vtkAMRInterpolatedVelocityField
 * vtkInitialValueProblemSolver
 */

#ifndef vtkFunctionSet_h
#define vtkFunctionSet_h

#include "vtkCommonMathModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONMATH_EXPORT vtkFunctionSet : public vtkObject
{
public:
  vtkTypeMacro(vtkFunctionSet, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Evaluate functions at x_j.
   * x and f have to point to valid double arrays of appropriate
   * sizes obtained with GetNumberOfFunctions() and
   * GetNumberOfIndependentVariables.
   * If you inherit this class, make sure to reimplement at least one of the two
   * FunctionValues signatures.
   */
  virtual int FunctionValues(double* x, double* f) { return this->FunctionValues(x, f, nullptr); }
  virtual int FunctionValues(double* x, double* f, void* vtkNotUsed(userData))
  {
    return this->FunctionValues(x, f);
  }

  /**
   * Return the number of functions. Note that this is constant for
   * a given type of set of functions and can not be changed at
   * run time.
   */
  virtual int GetNumberOfFunctions() { return this->NumFuncs; }

  /**
   * Return the number of independent variables. Note that this is
   * constant for a given type of set of functions and can not be changed
   * at run time.
   */
  virtual int GetNumberOfIndependentVariables() { return this->NumIndepVars; }

protected:
  vtkFunctionSet();
  ~vtkFunctionSet() override = default;

  int NumFuncs;
  int NumIndepVars;

private:
  vtkFunctionSet(const vtkFunctionSet&) = delete;
  void operator=(const vtkFunctionSet&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
