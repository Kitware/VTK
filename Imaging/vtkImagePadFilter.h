/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePadFilter.h
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
// .NAME vtkImagePadFilter - Super class for filters that fill in extra pixels.
// .SECTION Description
// vtkImagePadFilter Changes the image extent of an image.  If the image
// extent is larger than the input image extent, the extra pixels are
// filled by an algorithm determined by the subclass.
// The image extent of the output has to be specified.


#ifndef __vtkImagePadFilter_h
#define __vtkImagePadFilter_h

#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImagePadFilter : public vtkImageToImageFilter
{
public:
  static vtkImagePadFilter *New();
  vtkTypeMacro(vtkImagePadFilter,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The image extent of the output has to be set explicitly.
  void SetOutputWholeExtent(int extent[6]);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY, 
			    int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int *GetOutputWholeExtent() {return this->OutputWholeExtent;}

  // Description:
  // Set/Get the number of output scalar components.
  vtkSetMacro(OutputNumberOfScalarComponents, int);
  vtkGetMacro(OutputNumberOfScalarComponents, int);
  
protected:
  vtkImagePadFilter();
  ~vtkImagePadFilter() {};

  int OutputWholeExtent[6];
  int OutputNumberOfScalarComponents;

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
private:
  vtkImagePadFilter(const vtkImagePadFilter&);  // Not implemented.
  void operator=(const vtkImagePadFilter&);  // Not implemented.
};

#endif



