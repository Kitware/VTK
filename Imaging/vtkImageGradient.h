/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradient.h
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
// .NAME vtkImageGradient - Computes the gradient vector.
// .SECTION Description
// vtkImageGradient computes the gradient vector of an image.  The vector
// results are stored as scalar components. The Dimensionality determines
// whether to perform a 2d or 3d gradient. The default is two dimensional 
// XY gradient.  OutputScalarType is always float. Gradient is computed using
// central differences.



#ifndef __vtkImageGradient_h
#define __vtkImageGradient_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageGradient : public vtkImageToImageFilter
{
public:
  static vtkImageGradient *New();
  vtkTypeRevisionMacro(vtkImageGradient,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Determines how the input is interpreted (set of 2d slices ...)
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);
  
  // Description:
  // If "HandleBoundariesOn" then boundary pixels are duplicated
  // So central differences can get values.
  vtkSetMacro(HandleBoundaries, int);
  vtkGetMacro(HandleBoundaries, int);
  vtkBooleanMacro(HandleBoundaries, int);

protected:
  vtkImageGradient();
  ~vtkImageGradient() {};

  int HandleBoundaries;
  int Dimensionality;
  
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData); 
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageGradient(const vtkImageGradient&);  // Not implemented.
  void operator=(const vtkImageGradient&);  // Not implemented.
};

#endif



