// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImplicitWindowFunction
 * @brief   implicit function maps another implicit function to lie within a specified range
 *
 * vtkImplicitWindowFunction is used to modify the output of another
 * implicit function to lie within a specified "window", or function
 * range. This can be used to add "thickness" to cutting or clipping
 * functions.
 *
 * This class works as follows. First, it evaluates the function value of the
 * user-specified implicit function. Then, based on the window range specified,
 * it maps the function value into the window values specified.
 *
 *
 * @sa
 * vtkImplicitFunction
 */

#ifndef vtkImplicitWindowFunction_h
#define vtkImplicitWindowFunction_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkImplicitWindowFunction : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkImplicitWindowFunction, vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with window range (0,1) and window values (0,1).
   */
  static vtkImplicitWindowFunction* New();

  ///@{
  /**
   * Evaluate window function.
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;
  ///@}

  /**
   * Evaluate window function gradient. Just return implicit function gradient.
   */
  void EvaluateGradient(double x[3], double n[3]) override;

  ///@{
  /**
   * Specify an implicit function to operate on.
   */
  virtual void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * Specify the range of function values which are considered to lie within
   * the window. WindowRange[0] is assumed to be less than WindowRange[1].
   */
  vtkSetVector2Macro(WindowRange, double);
  vtkGetVectorMacro(WindowRange, double, 2);
  ///@}

  ///@{
  /**
   * Specify the range of output values that the window range is mapped
   * into. This is effectively a scaling and shifting of the original
   * function values.
   */
  vtkSetVector2Macro(WindowValues, double);
  vtkGetVectorMacro(WindowValues, double, 2);
  ///@}

  /**
   * Override modified time retrieval because of object dependencies.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Participate in garbage collection.
   */
  bool UsesGarbageCollector() const override { return true; }
  ///@}

protected:
  vtkImplicitWindowFunction();
  ~vtkImplicitWindowFunction() override;

  void ReportReferences(vtkGarbageCollector*) override;

  vtkImplicitFunction* ImplicitFunction;
  double WindowRange[2];
  double WindowValues[2];

private:
  vtkImplicitWindowFunction(const vtkImplicitWindowFunction&) = delete;
  void operator=(const vtkImplicitWindowFunction&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
