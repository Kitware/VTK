/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConvolve.h
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
// .NAME vtkImageConvolve - Convolution of an image with a kernel.
// .SECTION Description
// vtkImageConvolve convolves the image with a 3D NxNxN kernel or a
// 2D NxN kernal.  The output image is cropped to the same size as
// the input.

#ifndef __vtkImageConvolve_h
#define __vtkImageConvolve_h

#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageConvolve : public vtkImageToImageFilter
{
public:
  // Description:
  // Construct an instance of vtkImageConvolve filter.
  static vtkImageConvolve *New();
  vtkTypeRevisionMacro(vtkImageConvolve,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the kernel size
  vtkGetVector3Macro(KernelSize, int);

  // Description:
  // Set the kernel to be a given 3x3 or 5x5 or 7x7 kernel.
  void SetKernel3x3(const float kernel[9]);
  void SetKernel5x5(const float kernel[25]);
//BTX
  void SetKernel7x7(float kernel[49]);
//ETX

  // Description:
  // Return an array that contains the kernel.
  float* GetKernel3x3(); 
  void GetKernel3x3(float kernel[9]);
  float* GetKernel5x5(); 
  void GetKernel5x5(float kernel[25]);
//BTX
  float* GetKernel7x7(); 
  void GetKernel7x7(float kernel[49]);
//ETX

  // Description:
  // Set the kernel to be a 3x3x3 or 5x5x5 or 7x7x7 kernel.
  void SetKernel3x3x3(const float kernel[27]);
//BTX
  void SetKernel5x5x5(float kernel[125]);
  void SetKernel7x7x7(float kernel[343]);
//ETX

  // Description:
  // Return an array that contains the kernel
  float* GetKernel3x3x3(); 
  void GetKernel3x3x3(float kernel[27]);
//BTX
  float* GetKernel5x5x5(); 
  void GetKernel5x5x5(float kernel[125]);
  float* GetKernel7x7x7(); 
  void GetKernel7x7x7(float kernel[343]);
//ETX

protected:
  vtkImageConvolve();
  ~vtkImageConvolve();

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
                       int outExt[6], int id);

  void GetKernel(float *kernel);
  float* GetKernel();
  void SetKernel(const float* kernel,
                 int sizeX, int sizeY, int sizeZ);
  

  int KernelSize[3];
  float Kernel[343];
private:
  vtkImageConvolve(const vtkImageConvolve&);  // Not implemented.
  void operator=(const vtkImageConvolve&);  // Not implemented.
};

#endif



