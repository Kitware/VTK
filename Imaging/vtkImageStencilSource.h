/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilSource.h
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
// .NAME vtkImageStencilSource - helper class for clipping images
// .SECTION Description
// vtkImageStencilSource is a helper class for vtkImageToImageFilter
// classes.  Given a clipping object such as a vtkImplicitFunction, it
// will set up a list of clipping extents for each x-row through the
// image data.  The extents for each x-row can be retrieved via the 
// GetNextExtent() method after the extent lists have been built
// with the BuildExtents() method.  For large images, using clipping
// extents is much more memory efficient (and slightly more time-efficient)
// than building a mask.  This class can be subclassed to allow clipping
// with objects other than vtkImplicitFunction.
// .SECTION see also
// vtkImplicitFunction vtkImageStencil vtkImagePolyDataStencilSource

#ifndef __vtkImageStencilSource_h
#define __vtkImageStencilSource_h


#include "vtkSource.h"
#include "vtkImageStencilData.h"

class VTK_IMAGING_EXPORT vtkImageStencilSource : public vtkSource
{
public:
  static vtkImageStencilSource *New();
  vtkTypeMacro(vtkImageStencilSource, vtkSource);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get or set the output for this source.
  void SetOutput(vtkImageStencilData *output);
  vtkImageStencilData *GetOutput();

protected:
  vtkImageStencilSource();
  ~vtkImageStencilSource();

  void ExecuteData(vtkDataObject *out);
  vtkImageStencilData *AllocateOutputData(vtkDataObject *out);

  // Description:
  // Override this method to support clipping with different kinds
  // of objects.  Eventually the extent could be split up and handled
  // by multiple threads, but it isn't for now.  But please ensure
  // that all code inside this method is thread-safe.
  virtual void ThreadedExecute(vtkImageStencilData *output,
			       int extent[6], int threadId);
private:
  vtkImageStencilSource(const vtkImageStencilSource&);  // Not implemented.
  void operator=(const vtkImageStencilSource&);  // Not implemented.
};

#endif
