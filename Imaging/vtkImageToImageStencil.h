/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageStencil.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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
// .NAME vtkImageToImageStencil - clip an image with a mask image
// .SECTION Description
// vtkImageToImageStencil will convert a vtkImageData into an stencil
// that can be used with vtkImageStecil or other vtk classes that apply
// a stencil to an image.
// .SECTION see also
// vtkImageStencil vtkImplicitFunctionToImageStencil vtkPolyDataToImageStencil

#ifndef __vtkImageToImageStencil_h
#define __vtkImageToImageStencil_h


#include "vtkImageStencilSource.h"
#include "vtkImageData.h"

class VTK_EXPORT vtkImageToImageStencil : public vtkImageStencilSource
{
public:
  static vtkImageToImageStencil *New();
  vtkTypeMacro(vtkImageToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the image data to convert into a stencil.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();

  // Description:
  // The values greater than or equal to the value match.
  void ThresholdByUpper(float thresh);
  
  // Description:
  // The values less than or equal to the value match.
  void ThresholdByLower(float thresh);
  
  // Description:
  // The values in a range (inclusive) match
  void ThresholdBetween(float lower, float upper);
  
  // Description:
  // Get the Upper and Lower thresholds.
  vtkSetMacro(UpperThreshold, float);
  vtkGetMacro(UpperThreshold, float);
  vtkSetMacro(LowerThreshold, float);
  vtkGetMacro(LowerThreshold, float);

protected:
  vtkImageToImageStencil();
  ~vtkImageToImageStencil();
  vtkImageToImageStencil(const vtkImageToImageStencil&)
    {};
  void operator=(const vtkImageToImageStencil&) {};

  void ThreadedExecute(vtkImageStencilData *output,
		       int extent[6], int threadId);

  float UpperThreshold;
  float LowerThreshold;
  float Threshold;
};

#endif
