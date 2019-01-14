/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitSum.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImplicitSum
 * @brief   implicit sum of other implicit functions
 *
 * vtkImplicitSum produces a linear combination of other implicit functions.
 * The contribution of each function is weighted by a scalar coefficient.
 * The NormalizeByWeight option normalizes the output so that the
 * scalar weights add up to 1. Note that this function gives accurate
 * sums and gradients only if the input functions are linear.
*/

#ifndef vtkImplicitSum_h
#define vtkImplicitSum_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class vtkDoubleArray;
class vtkImplicitFunctionCollection;

class VTKCOMMONDATAMODEL_EXPORT vtkImplicitSum : public vtkImplicitFunction
{
public:
  static vtkImplicitSum *New();

  vtkTypeMacro(vtkImplicitSum,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Evaluate implicit function using current functions and weights.
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;
  //@}

  /**
   * Evaluate gradient of the weighted sum of functions.  Input functions
   * should be linear.
   */
  void EvaluateGradient(double x[3], double g[3]) override;

  /**
   * Override modified time retrieval because of object dependencies.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Add another implicit function to the list of functions, along with a
   * weighting factor.
   */
  void AddFunction(vtkImplicitFunction *in, double weight);

  /**
   * Add another implicit function to the list of functions, weighting it by
   * a factor of 1.
   */
  void AddFunction(vtkImplicitFunction *in) { this->AddFunction(in, 1.0); }

  /**
   * Remove all functions from the list.
   */
  void RemoveAllFunctions();

  /**
   * Set the weight (coefficient) of the given function to be weight.
   */
  void SetFunctionWeight(vtkImplicitFunction *f, double weight);

  //@{
  /**
   * When calculating the function and gradient values of the
   * composite function, setting NormalizeByWeight on will divide the
   * final result by the total weight of the component functions.
   * This process does not otherwise normalize the gradient vector.
   * By default, NormalizeByWeight is off.
   */
  vtkSetMacro(NormalizeByWeight, vtkTypeBool);
  vtkGetMacro(NormalizeByWeight, vtkTypeBool);
  vtkBooleanMacro(NormalizeByWeight, vtkTypeBool);
  //@}

protected:
  vtkImplicitSum();
  ~vtkImplicitSum() override;

  vtkImplicitFunctionCollection *FunctionList;
  vtkDoubleArray *Weights;
  double TotalWeight;

  void CalculateTotalWeight(void);
  vtkTypeBool NormalizeByWeight;

private:
  vtkImplicitSum(const vtkImplicitSum&) = delete;
  void operator=(const vtkImplicitSum&) = delete;
};

#endif
