/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConvolve.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageConvolve - Convolution of an image with a kernel.
// .SECTION Description
// vtkImageConvolve convolves the image with a 3D NxNxN kernel or a
// 2D NxN kernal.  The output image is cropped to the same size as
// the input.

#ifndef vtkImageConvolve_h
#define vtkImageConvolve_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageConvolve : public vtkThreadedImageAlgorithm
{
public:
  // Description:
  // Construct an instance of vtkImageConvolve filter.
  static vtkImageConvolve *New();
  vtkTypeMacro(vtkImageConvolve,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the kernel size
  vtkGetVector3Macro(KernelSize, int);

  // Description:
  // Set the kernel to be a given 3x3 or 5x5 or 7x7 kernel.
  void SetKernel3x3(const double kernel[9]);
  void SetKernel5x5(const double kernel[25]);
//BTX
  void SetKernel7x7(const double kernel[49]);
//ETX

  // Description:
  // Return an array that contains the kernel.
  double* GetKernel3x3();
  void GetKernel3x3(double kernel[9]);
  double* GetKernel5x5();
  void GetKernel5x5(double kernel[25]);
//BTX
  double* GetKernel7x7();
  void GetKernel7x7(double kernel[49]);
//ETX

  // Description:
  // Set the kernel to be a 3x3x3 or 5x5x5 or 7x7x7 kernel.
  void SetKernel3x3x3(const double kernel[27]);
//BTX
  void SetKernel5x5x5(const double kernel[125]);
  void SetKernel7x7x7(const double kernel[343]);
//ETX

  // Description:
  // Return an array that contains the kernel
  double* GetKernel3x3x3();
  void GetKernel3x3x3(double kernel[27]);
//BTX
  double* GetKernel5x5x5();
  void GetKernel5x5x5(double kernel[125]);
  double* GetKernel7x7x7();
  void GetKernel7x7x7(double kernel[343]);
//ETX

protected:
  vtkImageConvolve();
  ~vtkImageConvolve();

  void ThreadedRequestData(vtkInformation *request,
                           vtkInformationVector **inputVector,
                           vtkInformationVector *outputVector,
                           vtkImageData ***inData, vtkImageData **outData,
                           int outExt[6], int id);

  void GetKernel(double *kernel);
  double* GetKernel();
  void SetKernel(const double* kernel,
                 int sizeX, int sizeY, int sizeZ);


  int KernelSize[3];
  double Kernel[343];
private:
  vtkImageConvolve(const vtkImageConvolve&);  // Not implemented.
  void operator=(const vtkImageConvolve&);  // Not implemented.
};

#endif



