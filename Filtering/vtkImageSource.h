/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSource.h
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
// .NAME vtkImageSource - Source of data for the imaging pipeline
// .SECTION Description
// vtkImageSource is the superclass for all imaging sources and filters.
// The method Update(), called by the cache, is the major interface
// to the source.

// .SECTION See Also
// vtkImageToImageFilter


#ifndef __vtkImageSource_h
#define __vtkImageSource_h

#include "vtkImageData.h"
#include "vtkSource.h"

class VTK_EXPORT vtkImageSource : public vtkSource
{
public:
  vtkTypeMacro(vtkImageSource,vtkSource);
  static vtkImageSource *New();

  // Description:
  // Get the output of this source.
  void SetOutput(vtkImageData *output);
  vtkImageData *GetOutput();
  vtkImageData *GetOutput(int idx)
    {return (vtkImageData *) this->vtkSource::GetOutput(idx);}
  
protected:
  vtkImageSource();
  ~vtkImageSource() {};
  vtkImageSource(const vtkImageSource&);
  void operator=(const vtkImageSource&);

  void Execute();
  virtual void Execute(vtkImageData *data);

  // a helper method that sets the extent and allocates the output 
  // passed into it and returns it as an image data
  vtkImageData *AllocateOutputData(vtkDataObject *out);

  void ComputeRequiredInputUpdateExtent( int *vtkNotUsed(in), 
					 int *vtkNotUsed(out) ) 
    {VTK_LEGACY_METHOD(ComputeRequiredInputUpdateExtent,"3.2");}
  
};


#endif


