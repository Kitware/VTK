/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformPolyDataFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkTransformPolyDataFilter - transform points and associated normals and vectors for polygonal dataset
// .SECTION Description
// vtkTransformPolyDataFilter is a filter to transform point
// coordinates and associated point and cell normals and
// vectors. Other point and cell data is passed through the filter
// unchanged. This filter is specialized for polygonal data. See
// vtkTransformFilter for more general data.
//
// An alternative method of transformation is to use vtkActor's methods
// to scale, rotate, and translate objects. The difference between the
// two methods is that vtkActor's transformation simply effects where
// objects are rendered (via the graphics pipeline), whereas
// vtkTransformPolyDataFilter actually modifies point coordinates in the 
// visualization pipeline. This is necessary for some objects 
// (e.g., vtkProbeFilter) that require point coordinates as input.

// .SECTION See Also
// vtkTransform vtkTransformFilter vtkActor

#ifndef __vtkTransformPolyDataFilter_h
#define __vtkTransformPolyDataFilter_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkTransform.h"

class VTK_GRAPHICS_EXPORT vtkTransformPolyDataFilter : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkTransformPolyDataFilter *New();
  vtkTypeMacro(vtkTransformPolyDataFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the MTime also considering the transform.
  unsigned long GetMTime();

  // Description:
  // Specify the transform object used to transform points.
  vtkSetObjectMacro(Transform,vtkAbstractTransform);
  vtkGetObjectMacro(Transform,vtkAbstractTransform);

protected:
  vtkTransformPolyDataFilter();
  ~vtkTransformPolyDataFilter();

  void Execute();
  vtkAbstractTransform *Transform;
private:
  vtkTransformPolyDataFilter(const vtkTransformPolyDataFilter&);  // Not implemented.
  void operator=(const vtkTransformPolyDataFilter&);  // Not implemented.
};

#endif


