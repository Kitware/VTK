/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLaplacian.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageLaplacian - Computes divergence of gradient.
// .SECTION Description
// vtkImageLaplacian computes the Laplacian (like a second derivative)
// of a scalar image.  The operation is the same as taking the
// divergence after a gradient.  Boundaries are handled, so the input
// is the same as the output.
// Dimensionality determines how the input regions are interpreted.
// (images, or volumes). The Dimensionality defaults to two.



#ifndef vtkImageLaplacian_h
#define vtkImageLaplacian_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageLaplacian : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageLaplacian *New();
  vtkTypeMacro(vtkImageLaplacian,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Determines how the input is interpreted (set of 2d slices ...)
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);

protected:
  vtkImageLaplacian();
  ~vtkImageLaplacian() {}

  int Dimensionality;

  virtual int RequestUpdateExtent (vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int outExt[6], int id);

private:
  vtkImageLaplacian(const vtkImageLaplacian&);  // Not implemented.
  void operator=(const vtkImageLaplacian&);  // Not implemented.
};

#endif



