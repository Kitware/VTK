/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConvolve.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Z. F. Knops who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageConvolve - Convolution of an image with a kernel.
// .SECTION Description
// vtkImageConvolve convolves the image with a 3D NxNxN kernel or a
// 2D NxN kernal.  The output image is cropped to the same size as
// the input.

#ifndef __vtkImageConvolve_h
#define __vtkImageConvolve_h

#include "vtkImageToImageFilter.h"

class VTK_EXPORT vtkImageConvolve : public vtkImageToImageFilter
{
public:
  // Description:
  // Construct an instance of vtkImageConvolve filter.
  static vtkImageConvolve *New();
  vtkTypeMacro(vtkImageConvolve,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the kernel size
  vtkGetVector3Macro(KernelSize, int);

  // Description:
  // Set the kernel to be a given 3x3 or 5x5 or 7x7 kernel.
  void SetKernel3x3(const float kernel[9]);
  void SetKernel5x5(const float kernel[25]);
  void SetKernel7x7(const float kernel[49]);

  // Description:
  // Return an array that contains the kernel.
  float* GetKernel3x3(); 
  float* GetKernel5x5(); 
  float* GetKernel7x7(); 
  void GetKernel3x3(float kernel[9]);
  void GetKernel5x5(float kernel[25]);
  void GetKernel7x7(float kernel[49]);

  // Description:
  // Set the kernel to be a 3x3x3 or 5x5x5 or 7x7x7 kernel.
  void SetKernel3x3x3(const float kernel[27]);
  void SetKernel5x5x5(const float kernel[75]);
  void SetKernel7x7x7(const float kernel[343]);

  // Description:
  // Return an array that contains the kernel
  float* GetKernel3x3x3(); 
  float* GetKernel5x5x5(); 
  float* GetKernel7x7x7(); 
  void GetKernel3x3x3(float kernel[27]);
  void GetKernel5x5x5(float kernel[75]);
  void GetKernel7x7x7(float kernel[343]);

protected:
  vtkImageConvolve();
  ~vtkImageConvolve();
  vtkImageConvolve(const vtkImageConvolve&) {};
  void operator=(const vtkImageConvolve&) {};

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
                       int outExt[6], int id);

  void GetKernel(float *kernel);
  float* GetKernel();
  void SetKernel(const float* kernel,
                 int sizeX, int sizeY, int sizeZ);
  

  int KernelSize[3];
  float Kernel[343];
};

#endif



