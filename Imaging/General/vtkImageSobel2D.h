/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSobel2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// the input is an image.  Output is always doubles.


#ifndef __vtkImageSobel2D_h
#define __vtkImageSobel2D_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkImageSpatialAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageSobel2D : public vtkImageSpatialAlgorithm
{
public:
  static vtkImageSobel2D *New();
  vtkTypeMacro(vtkImageSobel2D,vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkImageSobel2D();
  ~vtkImageSobel2D() {};

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int outExt[6], int id);
  virtual int RequestInformation (vtkInformation *request,
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector);

private:
  vtkImageSobel2D(const vtkImageSobel2D&);  // Not implemented.
  void operator=(const vtkImageSobel2D&);  // Not implemented.
};

#endif



