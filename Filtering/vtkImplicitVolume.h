/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitVolume.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

#include "vtkImplicitFunction.h"
#include "vtkImageData.h"

class VTK_FILTERING_EXPORT vtkImplicitVolume : public vtkImplicitFunction
{
public:
  vtkTypeRevisionMacro(vtkImplicitVolume,vtkImplicitFunction);
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
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate ImplicitVolume gradient.
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Specify the volume for the implicit function.
  vtkSetObjectMacro(Volume,vtkImageData);
  vtkGetObjectMacro(Volume,vtkImageData);

  // Description:
  // Set the function value to use for points outside of the dataset.
  vtkSetMacro(OutValue,float);
  vtkGetMacro(OutValue,float);

  // Description:
  // Set the function gradient to use for points outside of the dataset.
  vtkSetVector3Macro(OutGradient,float);
  vtkGetVector3Macro(OutGradient,float);

protected:
  vtkImplicitVolume();
  ~vtkImplicitVolume();

  vtkImageData *Volume; // the structured points
  float OutValue;
  float OutGradient[3];
  // to replace a static
  vtkIdList *PointIds;
private:
  vtkImplicitVolume(const vtkImplicitVolume&);  // Not implemented.
  void operator=(const vtkImplicitVolume&);  // Not implemented.
};

#endif


