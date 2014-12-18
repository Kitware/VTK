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
// .NAME vtkImplicitDataSet - treat a dataset as if it were an implicit function
// .SECTION Description
// vtkImplicitDataSet treats any type of dataset as if it were an
// implicit function. This means it computes a function value and
// gradient. vtkImplicitDataSet is a concrete implementation of
// vtkImplicitFunction.
//
// vtkImplicitDataSet computes the function (at the point x) by performing
// cell interpolation. That is, it finds the cell containing x, and then
// uses the cell's interpolation functions to compute an interpolated
// scalar value at x. (A similar approach is used to find the
// gradient, if requested.) Points outside of the dataset are assigned
// the value of the ivar OutValue, and the gradient value OutGradient.

// .SECTION Caveats
// Any type of dataset can be used as an implicit function as long as it
// has scalar data associated with it.

// .SECTION See Also
// vtkImplicitFunction vtkImplicitVolume vtkClipPolyData vtkCutter
// vtkImplicitWindowFunction

#ifndef vtkImplicitDataSet_h
#define vtkImplicitDataSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkImplicitDataSet : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkImplicitDataSet,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct an vtkImplicitDataSet with no initial dataset; the OutValue
  // set to a large negative number; and the OutGradient set to (0,0,1).
  static vtkImplicitDataSet *New();

  // Description:
  // Return the MTime also considering the DataSet dependency.
  unsigned long GetMTime();

  // Description
  // Evaluate the implicit function. This returns the interpolated scalar value
  // at x[3].
  double EvaluateFunction(double x[3]);
  double EvaluateFunction(double x, double y, double z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate implicit function gradient.
  void EvaluateGradient(double x[3], double n[3]);

  // Description:
  // Set / get the dataset used for the implicit function evaluation.
  virtual void SetDataSet(vtkDataSet*);
  vtkGetObjectMacro(DataSet,vtkDataSet);

  // Description:
  // Set / get the function value to use for points outside of the dataset.
  vtkSetMacro(OutValue,double);
  vtkGetMacro(OutValue,double);

  // Description:
  // Set / get the function gradient to use for points outside of the dataset.
  vtkSetVector3Macro(OutGradient,double);
  vtkGetVector3Macro(OutGradient,double);

protected:
  vtkImplicitDataSet();
  ~vtkImplicitDataSet();

  virtual void ReportReferences(vtkGarbageCollector*);

  vtkDataSet *DataSet;
  double OutValue;
  double OutGradient[3];

  double *Weights; //used to compute interpolation weights
  int Size; //keeps track of length of weights array

private:
  vtkImplicitDataSet(const vtkImplicitDataSet&);  // Not implemented.
  void operator=(const vtkImplicitDataSet&);  // Not implemented.
};

#endif


