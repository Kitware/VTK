/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageEuclideanToPolar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageEuclideanToPolar - Converts 2D Euclidean coordinates to polar.
// .SECTION Description
// For each pixel with vector components x,y, this filter outputs
// theta in component0, and radius in component1.

#ifndef __vtkImageEuclideanToPolar_h
#define __vtkImageEuclideanToPolar_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageEuclideanToPolar : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageEuclideanToPolar *New();
  vtkTypeMacro(vtkImageEuclideanToPolar,
                       vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Theta is an angle. Maximum specifies when it maps back to 0.
  // ThetaMaximum defaults to 255 instead of 2PI, because unsigned char
  // is expected as input. The output type must be the same as input type.
  vtkSetMacro(ThetaMaximum,double);
  vtkGetMacro(ThetaMaximum,double);

protected:
  vtkImageEuclideanToPolar();
  ~vtkImageEuclideanToPolar() {};

  double ThetaMaximum;

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id);
private:
  vtkImageEuclideanToPolar(const vtkImageEuclideanToPolar&);  // Not implemented.
  void operator=(const vtkImageEuclideanToPolar&);  // Not implemented.
};

#endif



