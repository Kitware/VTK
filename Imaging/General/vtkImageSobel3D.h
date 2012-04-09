/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSobel3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// the input is a volume.  Output is always doubles.  A little creative
// liberty was used to extend the 2D sobel kernels into 3D.




#ifndef __vtkImageSobel3D_h
#define __vtkImageSobel3D_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkImageSpatialAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageSobel3D : public vtkImageSpatialAlgorithm
{
public:
  static vtkImageSobel3D *New();
  vtkTypeMacro(vtkImageSobel3D,vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkImageSobel3D();
  ~vtkImageSobel3D() {};

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int outExt[6], int id);
  virtual int RequestInformation (vtkInformation *request,
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector);

private:
  vtkImageSobel3D(const vtkImageSobel3D&);  // Not implemented.
  void operator=(const vtkImageSobel3D&);  // Not implemented.
};

#endif
