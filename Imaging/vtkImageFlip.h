/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFlip.h
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
// .NAME vtkImageFlip - This flips an axis of an image. Right becomes left ...
// .SECTION Description
// vtkImageFlip will reflect the data along the filtered axis.
// If PreserveImageExtent is "On", then the 
// image is shifted so that it has the same image extent, and the origin is
// shifted appropriately. When PreserveImageExtent is "off",
// The Origin  is not changed, min and max of extent (of filtered axis) are
// negated, and are swapped. The default preserves the extent of the input.

#ifndef __vtkImageFlip_h
#define __vtkImageFlip_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageFlip : public vtkImageToImageFilter
{
public:
  static vtkImageFlip *New();

  vtkTypeMacro(vtkImageFlip,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify which axes will be flipped.
  vtkSetMacro(FilteredAxis, int);
  vtkGetMacro(FilteredAxis, int);

  // Description:
  // Specify which axes will be flipped.
  // For compatibility with old scripts
  void SetFilteredAxes(int axis) {this->SetFilteredAxis(axis);}
  
  // Description:
  // If PreseveImageExtent is off, then extent of axis0 is simply
  // multiplied by -1.  If it is on, then the new image min (-imageMax0)
  // is shifted to old image min (imageMin0).
  vtkSetMacro(PreserveImageExtent, int);
  vtkGetMacro(PreserveImageExtent, int);
  vtkBooleanMacro(PreserveImageExtent, int);
  
protected:
  vtkImageFlip();
  ~vtkImageFlip() {};
  vtkImageFlip(const vtkImageFlip&);
  void operator=(const vtkImageFlip&);

  int FilteredAxis;
  int PreserveImageExtent;
  
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
		       int outExt[6], int id);
};

#endif



