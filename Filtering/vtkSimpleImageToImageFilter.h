/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleImageToImageFilter.h
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
// .NAME vtkSimpleImageToImageFilter - Generic image filter with one input.
// .SECTION Description
// vtkSimpleImageToImageFilter is a filter which aims to avoid much
// of the complexity associated with vtkImageToImageFilter (i.e.
// support for pieces, multi-threaded operation). If you need to write
// a simple image-image filter which operates on the whole input, use
// this as the superclass. The subclass has to provide only an execute
// method which takes input and output as arguments. Memory allocation
// is handled in vtkSimpleImageToImageFilter. Also, you are guaranteed to
// have a valid input in the Execute(input, output) method. By default, 
// this filter
// requests it's input's whole extent and copies the input's information
// (spacing, whole extent etc...) to the output. If the output's setup
// is different (for example, if it performs some sort of sub-sampling),
// ExecuteInformation has to be overwritten. As an example of how this
// can be done, you can look at vtkImageShrink3D::ExecuteInformation.
// For a complete example which uses templates to support generic data
// types, see  vtkSimpleFilterExample.

// .SECTION See also
// vtkImageToImageFilter vtkSimpleImageFilterExample

#ifndef __vtkSimpleImageToImageFilter_h
#define __vtkSimpleImageToImageFilter_h

#include "vtkImageSource.h"

class VTK_FILTERING_EXPORT vtkSimpleImageToImageFilter : public vtkImageSource
{
public:

  vtkTypeMacro(vtkSimpleImageToImageFilter,vtkImageSource);

  // Description:
  // Set the Input of a filter. 
  virtual void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
  
protected:
  vtkSimpleImageToImageFilter();
  ~vtkSimpleImageToImageFilter();
  vtkSimpleImageToImageFilter(const vtkSimpleImageToImageFilter&);
  void operator=(const vtkSimpleImageToImageFilter&);

  // These are called by the superclass.
  // You might have to override ExecuteInformation
  virtual void ExecuteInformation();
  virtual void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);

  // You don't have to touch this unless you have a good reason.
  virtual void ExecuteData(vtkDataObject *output);
  // In the simplest case, this is the only method you need to define.
  virtual void SimpleExecute(vtkImageData* input, vtkImageData* output) = 0;
};

#endif







