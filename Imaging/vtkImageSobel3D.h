/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSobel3D.h
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
// .NAME vtkImageSobel3D - Computes a vector field using sobel functions.
// .SECTION Description
// vtkImageSobel3D computes a vector field from a scalar field by using
// Sobel functions.  The number of vector components is 3 because
// the input is a volume.  Output is always floats.  A little creative 
// liberty was used to extend the 2D sobel kernels into 3D.




#ifndef __vtkImageSobel3D_h
#define __vtkImageSobel3D_h


#include "vtkImageSpatialFilter.h"

class VTK_IMAGING_EXPORT vtkImageSobel3D : public vtkImageSpatialFilter
{
public:
  static vtkImageSobel3D *New();
  vtkTypeRevisionMacro(vtkImageSobel3D,vtkImageSpatialFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkImageSobel3D();
  ~vtkImageSobel3D() {};

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int outExt[6], int id);
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
private:
  vtkImageSobel3D(const vtkImageSobel3D&);  // Not implemented.
  void operator=(const vtkImageSobel3D&);  // Not implemented.
};

#endif



