/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSeparableConvolution.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    

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
// .NAME vtkImageSeparableConvolution -  3 1D convolutions on an image
// .SECTION Description
// vtkImageSeparableConvolution performs a convolution along the X, Y,
// and Z axes of an image, based on the three different 1D convolution
// kernels.  The kernels must be of odd size, and are considered to be
// centered at (int)((kernelsize - 1) / 2.0 ).  If a kernel is NULL,
// that dimension is skipped.  This filter is designed to efficiently
// convolve separable filters that can be decomposed into 1 or more 1D
// convolutions.  It also handles arbitrarly large kernel sizes.

#ifndef __vtkImageSeparableConvolution_h
#define __vtkImageSeparableConvolution_h


#include "vtkImageDecomposeFilter.h"
#include "vtkFloatArray.h"

class VTK_IMAGING_EXPORT vtkImageSeparableConvolution : public vtkImageDecomposeFilter
{
public:
  static vtkImageSeparableConvolution *New();
  vtkTypeMacro(vtkImageSeparableConvolution,vtkImageDecomposeFilter);


  // Set the X convolution kernel, a null value indicates no convolution to be done
  vtkSetObjectMacro ( XKernel, vtkFloatArray );
  vtkGetObjectMacro ( XKernel, vtkFloatArray );

  // Set the Y convolution kernel, a null value indicates no convolution to be done
  vtkSetObjectMacro ( YKernel, vtkFloatArray );
  vtkGetObjectMacro ( YKernel, vtkFloatArray );

  // Set the Z convolution kernel, a null value indicates no convolution to be done
  vtkSetObjectMacro ( ZKernel, vtkFloatArray );
  vtkGetObjectMacro ( ZKernel, vtkFloatArray );

  void PrintSelf(ostream& os, vtkIndent indent);
  
protected:
  vtkImageSeparableConvolution();
  ~vtkImageSeparableConvolution();
  
  vtkFloatArray* XKernel;
  vtkFloatArray* YKernel;
  vtkFloatArray* ZKernel;

  // void AllocateOutputScalars(vtkImageData *outData);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void IterativeExecuteData(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(vtkImageData *input, vtkImageData *output);
  void ExecuteInformation()
    {this->vtkImageIterateFilter::ExecuteInformation();}


  //void AllocateOutputScalars(vtkImageData *outData);

  
private:
  vtkImageSeparableConvolution(const vtkImageSeparableConvolution&);  // Not implemented.
  void operator=(const vtkImageSeparableConvolution&);  // Not implemented.
};

#endif










