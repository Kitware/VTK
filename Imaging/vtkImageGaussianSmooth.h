/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSmooth.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
// .NAME vtkImageGaussianSmooth - Performs a gaussian convolution.
// .SECTION Description
// vtkImageGaussianSmooth implements a convolution of the input image
// with a gaussian. Supports from one to three dimensional convolutions.

#ifndef __vtkImageGaussianSmooth_h
#define __vtkImageGaussianSmooth_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageGaussianSmooth : public vtkImageToImageFilter
{
public:
  vtkTypeMacro(vtkImageGaussianSmooth,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates an instance of vtkImageGaussianSmmoth with the following
  // defaults: Dimensioonality 3, StandardDeviations( 2, 2, 2), 
  // Radius Factors ( 1.5, 1.5, 1.5)
  static vtkImageGaussianSmooth *New();
  
  
  // Description:
  // Sets/Gets the Standard deviation of the gaussian in pixel units.
  vtkSetVector3Macro(StandardDeviations, float);
  void SetStandardDeviation(float std)
        {this->SetStandardDeviations(std,std,std);}
  void SetStandardDeviations(float a,float b)
        {this->SetStandardDeviations(a,b,0.0);}
  vtkGetVector3Macro(StandardDeviations, float);

  // Description:
  // Sets/Gets the Standard deviation of the gaussian in pixel units.
  // These methods are provided for compatibility with old scripts
  void SetStandardDeviation(float a,float b)
        {this->SetStandardDeviations(a,b,0.0);}
  void SetStandardDeviation(float a,float b,float c) 
        {this->SetStandardDeviations(a,b,c);}

  // Description:
  // Sets/Gets the Radius Factors of the gaussian in pixel units.
  // The radius factors determine how far out the gaussian kernel will 
  // go before being clamped to zero.
  vtkSetVector3Macro(RadiusFactors, float);
  void SetRadiusFactors(float f, float f2) {this->SetRadiusFactors(f,f2,1.5);}
  void SetRadiusFactor(float f) {this->SetRadiusFactors(f, f, f);}
  vtkGetVector3Macro(RadiusFactors, float);

  // Description:
  // Set/Get the dimensionality of this filter. This determines whether
  // a one, two, or three dimensional gaussian is performed.
  vtkSetMacro(Dimensionality, int);
  vtkGetMacro(Dimensionality, int);

protected:
  vtkImageGaussianSmooth();
  ~vtkImageGaussianSmooth();
  vtkImageGaussianSmooth(const vtkImageGaussianSmooth&);
  void operator=(const vtkImageGaussianSmooth&);

  int Dimensionality;
  float StandardDeviations[3];
  float RadiusFactors[3];
  
  void ComputeKernel(double *kernel, int min, int max, double std);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteAxis(int axis, vtkImageData *inData, int inExt[6],
		   vtkImageData *outData, int outExt[6],
		   int *pcycle, int target, int *pcount, int total);
  void ThreadedExecute(vtkImageData *inData, 
		       vtkImageData *outData, int outExt[6], int id);
  
};

#endif










