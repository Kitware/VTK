/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMask.h
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
// .NAME vtkImageMask - Combines a mask and an image.
// .SECTION Description
// vtkImageMask combines a mask with an image.  Non zero mask
// implies the output pixel will be the same as the image.
// If a mask pixel is zero,  the the output pixel
// is set to "MaskedValue".  The filter also has the option to pass
// the mask through a boolean not operation before processing the image.
// This reverses the passed and replaced pixels.
// The two inputs should have the same "WholeExtent".
// The mask input should be unsigned char, and the image scalar type
// is the same as the output scalar type.


#ifndef __vtkImageMask_h
#define __vtkImageMask_h


#include "vtkImageTwoInputFilter.h"

class VTK_EXPORT vtkImageMask : public vtkImageTwoInputFilter
{
public:
  static vtkImageMask *New();
  vtkTypeMacro(vtkImageMask,vtkImageTwoInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // SetGet the value of the output pixel replaced by mask.
  void SetMaskedOutputValue(int num, float *v);
  void SetMaskedOutputValue(float v) {this->SetMaskedOutputValue(1, &v);}
  void SetMaskedOutputValue(float v1, float v2) 
    {float v[2]; v[0]=v1; v[1]=v2; this->SetMaskedOutputValue(2, v);}
  void SetMaskedOutputValue(float v1, float v2, float v3) 
    {float v[3]; v[0]=v1; v[1]=v2; v[2]=v3; this->SetMaskedOutputValue(3, v);}
  float *GetMaskedOutputValue() {return this->MaskedOutputValue;}
  int GetMaskedOutputValueLength() {return this->MaskedOutputValueLength;}

  // Description:
  // Set the input to be masked.
  void SetImageInput(vtkImageData *in) {this->SetInput1(in);}

  // Description:
  // Set the mask to be used.
  void SetMaskInput(vtkImageData *in) {this->SetInput2(in);}
  
  // Description:
  // When Not Mask is on, the mask is passed through a boolean not
  // before it is used to mask the image.  The effect is to pass the
  // pixels where the input mask is zero, and replace the pixels
  // where the input value is non zero.
  vtkSetMacro(NotMask,int);
  vtkGetMacro(NotMask,int);
  vtkBooleanMacro(NotMask, int);
  
protected:
  vtkImageMask();
  ~vtkImageMask();
  vtkImageMask(const vtkImageMask&);
  void operator=(const vtkImageMask&);

  float *MaskedOutputValue;
  int MaskedOutputValueLength;
  int NotMask;
  
  void ExecuteInformation(vtkImageData **inDatas, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageTwoInputFilter::ExecuteInformation();};
 
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
		       int extent[6], int id);
};

#endif



