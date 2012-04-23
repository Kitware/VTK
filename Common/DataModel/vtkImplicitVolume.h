/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitVolume.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImplicitVolume - treat a volume as if it were an implicit function
// .SECTION Description
// vtkImplicitVolume treats a volume (e.g., structured point dataset)
// as if it were an implicit function. This means it computes a function
// value and gradient. vtkImplicitVolume is a concrete implementation of
// vtkImplicitFunction.
//
// vtkImplicitDataSet computes the function (at the point x) by performing
// cell interpolation. That is, it finds the cell containing x, and then
// uses the cell's interpolation functions to compute an interpolated
// scalar value at x. (A similar approach is used to find the
// gradient, if requested.) Points outside of the dataset are assigned
// the value of the ivar OutValue, and the gradient value OutGradient.

// .SECTION Caveats
// The input volume data is only updated when GetMTime() is called.
// Works for 3D structured points datasets, 0D-2D datasets won't work properly.

// .SECTION See Also
// vtkImplicitFunction vtkImplicitDataSet vtkClipPolyData vtkCutter
// vtkImplicitWindowFunction

#ifndef __vtkImplicitVolume_h
#define __vtkImplicitVolume_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class vtkIdList;
class vtkImageData;

class VTKCOMMONDATAMODEL_EXPORT vtkImplicitVolume : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkImplicitVolume,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct an vtkImplicitVolume with no initial volume; the OutValue
  // set to a large negative number; and the OutGradient set to (0,0,1).
  static vtkImplicitVolume *New();

  // Description:
  // Returns the mtime also considering the volume.  This also calls Update
  // on the volume, and it therefore must be called before the function is
  // evaluated.
  unsigned long GetMTime();

  // Description
  // Evaluate the ImplicitVolume. This returns the interpolated scalar value
  // at x[3].
  double EvaluateFunction(double x[3]);
  double EvaluateFunction(double x, double y, double z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate ImplicitVolume gradient.
  void EvaluateGradient(double x[3], double n[3]);

  // Description:
  // Specify the volume for the implicit function.
  virtual void SetVolume(vtkImageData*);
  vtkGetObjectMacro(Volume,vtkImageData);

  // Description:
  // Set the function value to use for points outside of the dataset.
  vtkSetMacro(OutValue,double);
  vtkGetMacro(OutValue,double);

  // Description:
  // Set the function gradient to use for points outside of the dataset.
  vtkSetVector3Macro(OutGradient,double);
  vtkGetVector3Macro(OutGradient,double);

protected:
  vtkImplicitVolume();
  ~vtkImplicitVolume();

  vtkImageData *Volume; // the structured points
  double OutValue;
  double OutGradient[3];
  // to replace a static
  vtkIdList *PointIds;

private:
  vtkImplicitVolume(const vtkImplicitVolume&);  // Not implemented.
  void operator=(const vtkImplicitVolume&);  // Not implemented.
};

#endif


