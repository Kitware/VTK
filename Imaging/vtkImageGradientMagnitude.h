/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradientMagnitude.h
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
// .NAME vtkImageGradientMagnitude - Computes magnitude of the gradient.

// .SECTION Description
// vtkImageGradientMagnitude computes the gradient magnitude of an image.
// Setting the dimensionality determines whether the gradient is computed on
// 2D images, or 3D volumes.  The default is two dimensional XY images.

// .SECTION See Also
// vtkImageGradient vtkImageMagnitude

#ifndef __vtkImageGradientMagnitude_h
#define __vtkImageGradientMagnitude_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageGradientMagnitude : public vtkImageToImageFilter
{
public:
  static vtkImageGradientMagnitude *New();
  vtkTypeRevisionMacro(vtkImageGradientMagnitude,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // If "HandleBoundariesOn" then boundary pixels are duplicated
  // So central differences can get values.
  vtkSetMacro(HandleBoundaries, int);
  vtkGetMacro(HandleBoundaries, int);
  vtkBooleanMacro(HandleBoundaries, int);

  // Description:
  // Determines how the input is interpreted (set of 2d slices ...)
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);
  
protected:
  vtkImageGradientMagnitude();
  ~vtkImageGradientMagnitude() {};

  int HandleBoundaries;
  int Dimensionality;
  
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageGradientMagnitude(const vtkImageGradientMagnitude&);  // Not implemented.
  void operator=(const vtkImageGradientMagnitude&);  // Not implemented.
};

#endif



