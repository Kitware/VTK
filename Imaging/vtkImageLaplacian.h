/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLaplacian.h
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
// .NAME vtkImageLaplacian - Computes divergence of gradient.
// .SECTION Description
// vtkimageLaplacian computes the Laplacian (like a second derivative)
// of a scalar image.  The operation is the same as taking the
// divergence after a gradient.  Boundaries are handled, so the input
// is the same as the output.
// Dimensionality determines how the input regions are interpreted.
// (images, or volumes). The Dimensionality defaults to two.



#ifndef __vtkimageLaplacian_h
#define __vtkimageLaplacian_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageLaplacian : public vtkImageToImageFilter
{
public:
  static vtkImageLaplacian *New();
  vtkTypeRevisionMacro(vtkImageLaplacian,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Determines how the input is interpreted (set of 2d slices ...)
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);
  
protected:
  vtkImageLaplacian();
  ~vtkImageLaplacian() {};

  int Dimensionality;

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id);
private:
  vtkImageLaplacian(const vtkImageLaplacian&);  // Not implemented.
  void operator=(const vtkImageLaplacian&);  // Not implemented.
};

#endif



