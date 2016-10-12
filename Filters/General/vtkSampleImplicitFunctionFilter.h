/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSampleImplicitFunctionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkImplicitFunction;
class vtkDataArray;

class VTKFILTERSGENERAL_EXPORT vtkSampleImplicitFunctionFilter : public vtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard instantiation, type information, and print methods.
   */
  static vtkSampleImplicitFunctionFilter *New();
  vtkTypeMacro(vtkSampleImplicitFunctionFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Specify the implicit function to use to generate data.
   */
  virtual void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  //@}

  //@{
  /**
   * Turn on/off the computation of gradients.
   */
  vtkSetMacro(ComputeGradients,int);
  vtkGetMacro(ComputeGradients,int);
  vtkBooleanMacro(ComputeGradients,int);
  //@}

  //@{
  /**
   * Set/get the scalar array name for this data set. The initial value is
   * "Implicit scalars".
   */
  vtkSetStringMacro(ScalarArrayName);
  vtkGetStringMacro(ScalarArrayName);
  //@}

  //@{
  /**
   * Set/get the gradient array name for this data set. The initial value is
   * "Implicit gradients".
   */
  vtkSetStringMacro(GradientArrayName);
  vtkGetStringMacro(GradientArrayName);
  //@}

  /**
   * Return the MTime also taking into account the implicit function.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkSampleImplicitFunctionFilter();
  ~vtkSampleImplicitFunctionFilter() VTK_OVERRIDE;

  vtkImplicitFunction *ImplicitFunction;
  int ComputeGradients;
  char *ScalarArrayName;
  char *GradientArrayName;

  void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;


private:
  vtkSampleImplicitFunctionFilter(const vtkSampleImplicitFunctionFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSampleImplicitFunctionFilter&) VTK_DELETE_FUNCTION;
};

#endif
