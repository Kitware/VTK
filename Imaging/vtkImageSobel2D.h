/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSobel2D.h
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
// .NAME vtkImageSobel2D - Computes a vector field using sobel functions.
// .SECTION Description
// vtkImageSobel2D computes a vector field from a scalar field by using
// Sobel functions.  The number of vector components is 2 because
// the input is an image.  Output is always floats.


#ifndef __vtkImageSobel2D_h
#define __vtkImageSobel2D_h


#include "vtkImageSpatialFilter.h"

class VTK_IMAGING_EXPORT vtkImageSobel2D : public vtkImageSpatialFilter
{
public:
  static vtkImageSobel2D *New();
  vtkTypeRevisionMacro(vtkImageSobel2D,vtkImageSpatialFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkImageSobel2D();
  ~vtkImageSobel2D() {};

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int outExt[6], int id);
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
private:
  vtkImageSobel2D(const vtkImageSobel2D&);  // Not implemented.
  void operator=(const vtkImageSobel2D&);  // Not implemented.
};

#endif



