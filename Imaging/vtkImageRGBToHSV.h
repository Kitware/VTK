/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRGBToHSV.h
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
// .NAME vtkImageRGBToHSV - Converts RGB components to HSV.
// .SECTION Description
// For each pixel with red, blue, and green components this
// filter output the color coded as hue, saturation and value.
// Output type must be the same as input type.


#ifndef __vtkImageRGBToHSV_h
#define __vtkImageRGBToHSV_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageRGBToHSV : public vtkImageToImageFilter
{
public:
  static vtkImageRGBToHSV *New();
  vtkTypeMacro(vtkImageRGBToHSV,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:

  // Hue is an angle. Maximum specifies when it maps back to 0.  HueMaximum
  // defaults to 255 instead of 2PI, because unsigned char is expected as
  // input.  Maximum also specifies the maximum of the Saturation.
  vtkSetMacro(Maximum,float);
  vtkGetMacro(Maximum,float);
  
protected:
  vtkImageRGBToHSV();
  ~vtkImageRGBToHSV() {};

  float Maximum;
  
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
		       int ext[6], int id);
private:
  vtkImageRGBToHSV(const vtkImageRGBToHSV&);  // Not implemented.
  void operator=(const vtkImageRGBToHSV&);  // Not implemented.
};

#endif



