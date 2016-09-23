/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImplicitDataSet
 * @brief   treat a dataset as if it were an implicit function
 *
 * vtkImplicitDataSet treats any type of dataset as if it were an
 * implicit function. This means it computes a function value and
 * gradient. vtkImplicitDataSet is a concrete implementation of
 * vtkImplicitFunction.
 *
 * vtkImplicitDataSet computes the function (at the point x) by performing
 * cell interpolation. That is, it finds the cell containing x, and then
 * uses the cell's interpolation functions to compute an interpolated
 * scalar value at x. (A similar approach is used to find the
 * gradient, if requested.) Points outside of the dataset are assigned
 * the value of the ivar OutValue, and the gradient value OutGradient.
 *
 * @warning
 * Any type of dataset can be used as an implicit function as long as it
 * has scalar data associated with it.
 *
 * @sa
 * vtkImplicitFunction vtkImplicitVolume vtkClipPolyData vtkCutter
 * vtkImplicitWindowFunction
*/

#ifndef vtkImplicitDataSet_h
#define vtkImplicitDataSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkImplicitDataSet : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkImplicitDataSet,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct an vtkImplicitDataSet with no initial dataset; the OutValue
   * set to a large negative number; and the OutGradient set to (0,0,1).
   */
  static vtkImplicitDataSet *New();

  /**
   * Return the MTime also considering the DataSet dependency.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Evaluate the implicit function. This returns the interpolated scalar value
   * at x[3].
   */
  double EvaluateFunction(double x[3]) VTK_OVERRIDE;
  double EvaluateFunction(double x, double y, double z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;
  //@}

  /**
   * Evaluate implicit function gradient.
   */
  void EvaluateGradient(double x[3], double n[3]) VTK_OVERRIDE;

  //@{
  /**
   * Set / get the dataset used for the implicit function evaluation.
   */
  virtual void SetDataSet(vtkDataSet*);
  vtkGetObjectMacro(DataSet,vtkDataSet);
  //@}

  //@{
  /**
   * Set / get the function value to use for points outside of the dataset.
   */
  vtkSetMacro(OutValue,double);
  vtkGetMacro(OutValue,double);
  //@}

  //@{
  /**
   * Set / get the function gradient to use for points outside of the dataset.
   */
  vtkSetVector3Macro(OutGradient,double);
  vtkGetVector3Macro(OutGradient,double);
  //@}

protected:
  vtkImplicitDataSet();
  ~vtkImplicitDataSet() VTK_OVERRIDE;

  void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;

  vtkDataSet *DataSet;
  double OutValue;
  double OutGradient[3];

  double *Weights; //used to compute interpolation weights
  int Size; //keeps track of length of weights array

private:
  vtkImplicitDataSet(const vtkImplicitDataSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImplicitDataSet&) VTK_DELETE_FUNCTION;
};

#endif


