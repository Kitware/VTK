/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencil.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G Gobbi who developed this class.

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
// .NAME vtkImageStencil - combine images via a cookie-cutter operation
// .SECTION Description
// vtkImageStencil will combine two images together using a stencil.
// The stencil should be provided in the form of a vtkImageStencilData,


#ifndef __vtkImageStencil_h
#define __vtkImageStencil_h

#include "vtkImageToImageFilter.h"
#include "vtkImageStencilData.h"

class VTK_EXPORT vtkImageStencil : public vtkImageToImageFilter
{
public:
  static vtkImageStencil *New();
  vtkTypeMacro(vtkImageStencil, vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the stencil to use.  The stencil can be created
  // from a vtkImplicitFunction or a vtkPolyData.
  virtual void SetStencil(vtkImageStencilData *stencil);
  vtkImageStencilData *GetStencil();

  // Description:
  // Reverse the stencil.
  vtkSetMacro(ReverseStencil, int);
  vtkBooleanMacro(ReverseStencil, int);
  vtkGetMacro(ReverseStencil, int);

  // Description:
  // NOTE: Not yet implemented, use SetBackgroundValue instead.
  // Set the second input.  This image will be used for the 'outside' of the
  // stencil.  If not set, the output voxels will be filled with
  // BackgroundValue instead.
  virtual void SetBackgroundInput(vtkImageData *input);
  vtkImageData *GetBackgroundInput();

  // Description:
  // Set the default output value to use when the second input is not set.
  void SetBackgroundValue(float val) {
    this->SetBackgroundColor(val,val,val,val); };
  float GetBackgroundValue() {
    return this->BackgroundColor[0]; };

  // Description:
  // Set the default color to use when the second input is not set.
  // This is like SetBackgroundValue, but for multi-component images.
  vtkSetVector4Macro(BackgroundColor, float);
  vtkGetVector4Macro(BackgroundColor, float);

protected:
  vtkImageStencil();
  ~vtkImageStencil();
  vtkImageStencil(const vtkImageStencil&) {};
  void operator=(const vtkImageStencil&) {};

  void ExecuteInformation() {
    this->vtkImageToImageFilter::ExecuteInformation(); };
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
  
  int ReverseStencil;
  float BackgroundColor[4];
};

#endif













