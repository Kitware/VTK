/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkCompositeFilter - Composites multiple images.
// .SECTION Description
// vtkCompositeFilter Takes a number of inputs of structured points with
// pixel data and z-buffer data, and composites them into one.  The pixel
// data should be stored in point scalars, and the z buffer data should be 
// stored in a point field called ZBuffer.  This is the format produced by 
// vtkRendererSource.  

// .SECTION Notes
// Although this filter processes structured points, future plans are to
// have it produce vtkImageData and have it render select pieces of the 
// image. Also, this filter ignores alpha (for now).

// .SECTION See Also
// vtkRendererSource

#ifndef __vtkCompositeFilter_h
#define __vtkCompositeFilter_h

#include "vtkStructuredPointsToStructuredPointsFilter.h"

class VTK_EXPORT vtkCompositeFilter : public vtkStructuredPointsToStructuredPointsFilter
{
public:
  static vtkCompositeFilter *New();

  vtkTypeMacro(vtkCompositeFilter,vtkStructuredPointsToStructuredPointsFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a dataset to the list of data to append.
  void AddInput(vtkStructuredPoints *);

  // Description:
  // Remove a dataset from the list of data to append.
  void RemoveInput(vtkStructuredPoints *);

  // Description:
  // Get any input of this filter.
  vtkStructuredPoints *GetInput(int idx);

protected:
  vtkCompositeFilter();
  ~vtkCompositeFilter();
  vtkCompositeFilter(const vtkCompositeFilter&) {};
  void operator=(const vtkCompositeFilter&) {};

  // Usual data generation method
  void Execute();
  int ComputeInputUpdateExtents(vtkDataObject *data);
};

#endif


