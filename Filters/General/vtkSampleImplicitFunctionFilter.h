// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSampleImplicitFunctionFilter
 * @brief   sample an implicit function over a dataset,
 * generating scalar values and optional gradient vectors
 *
 *
 * vtkSampleImplicitFunctionFilter is a filter that evaluates an implicit function and
 * (optional) gradients at each point in an input vtkDataSet. The output
 * of the filter are new scalar values (the function values) and the
 * optional vector (function gradient) array.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkSampleFunction vtkImplicitModeller
 */

#ifndef vtkSampleImplicitFunctionFilter_h
#define vtkSampleImplicitFunctionFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitFunction;
class vtkDataArray;

class VTKFILTERSGENERAL_EXPORT vtkSampleImplicitFunctionFilter : public vtkDataSetAlgorithm
{
public:
  ///@{
  /**
   * Standard instantiation, type information, and print methods.
   */
  static vtkSampleImplicitFunctionFilter* New();
  vtkTypeMacro(vtkSampleImplicitFunctionFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the implicit function to use to generate data.
   */
  virtual void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * Turn on/off the computation of gradients.
   */
  vtkSetMacro(ComputeGradients, vtkTypeBool);
  vtkGetMacro(ComputeGradients, vtkTypeBool);
  vtkBooleanMacro(ComputeGradients, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get the scalar array name for this data set. The initial value is
   * "Implicit scalars".
   */
  vtkSetStringMacro(ScalarArrayName);
  vtkGetStringMacro(ScalarArrayName);
  ///@}

  ///@{
  /**
   * Set/get the gradient array name for this data set. The initial value is
   * "Implicit gradients".
   */
  vtkSetStringMacro(GradientArrayName);
  vtkGetStringMacro(GradientArrayName);
  ///@}

  /**
   * Return the MTime also taking into account the implicit function.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkSampleImplicitFunctionFilter();
  ~vtkSampleImplicitFunctionFilter() override;

  vtkImplicitFunction* ImplicitFunction;
  vtkTypeBool ComputeGradients;
  char* ScalarArrayName;
  char* GradientArrayName;

  void ReportReferences(vtkGarbageCollector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkSampleImplicitFunctionFilter(const vtkSampleImplicitFunctionFilter&) = delete;
  void operator=(const vtkSampleImplicitFunctionFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
