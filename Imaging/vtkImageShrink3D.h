/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShrink3D.h
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
// .NAME vtkImageShrink3D - Subsamples an image.
// .SECTION Description
// vtkImageShrink3D shrinks an image by sub sampling on a 
// uniform grid (integer multiples).  

#ifndef __vtkImageShrink3D_h
#define __vtkImageShrink3D_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageShrink3D : public vtkImageToImageFilter
{
public:
  static vtkImageShrink3D *New();
  vtkTypeMacro(vtkImageShrink3D,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the shrink factors
  vtkSetVector3Macro(ShrinkFactors,int);
  vtkGetVector3Macro(ShrinkFactors,int);

  // Description:
  // Set/Get the pixel to use as origin.
  vtkSetVector3Macro(Shift,int);
  vtkGetVector3Macro(Shift,int);

  // Description:
  // Choose Mean, Minimum, Maximum, Median or sub sampling.
  // The neighborhood operations are not centered on the sampled pixel.
  // This may cause a half pixel shift in your output image.
  // You can changed "Shift" to get around this.
  // vtkImageGaussianSmooth or vtkImageMean with strides.
  void SetAveraging(int);
  int GetAveraging() {return this->GetMean();};
  vtkBooleanMacro(Averaging,int);
  
  void SetMean(int);
  vtkGetMacro(Mean,int);
  vtkBooleanMacro(Mean,int);
  
  void SetMinimum(int);
  vtkGetMacro(Minimum,int);
  vtkBooleanMacro(Minimum,int);
  
  void SetMaximum(int);
  vtkGetMacro(Maximum,int);
  vtkBooleanMacro(Maximum,int);
  
  void SetMedian(int);
  vtkGetMacro(Median,int);
  vtkBooleanMacro(Median,int);
  
  
protected:
  vtkImageShrink3D();
  ~vtkImageShrink3D() {};

  int ShrinkFactors[3];
  int Shift[3];
  int Mean;
  int Minimum;
  int Maximum;
  int Median;

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
		       int ext[6], int id);  
private:
  vtkImageShrink3D(const vtkImageShrink3D&);  // Not implemented.
  void operator=(const vtkImageShrink3D&);  // Not implemented.
};

#endif



