/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShrinkPolyData.h
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
// .NAME vtkShrinkPolyData - shrink cells composing PolyData
// .SECTION Description
// vtkShrinkPolyData shrinks cells composing a polygonal dataset (e.g., 
// vertices, lines, polygons, and triangle strips) towards their centroid. 
// The centroid of a cell is computed as the average position of the
// cell points. Shrinking results in disconnecting the cells from
// one another. The output dataset type of this filter is polygonal data.
//
// During execution the filter passes its input cell data to its
// output. Point data attributes are copied to the points created during the
// shrinking process.

// .SECTION Caveats
// It is possible to turn cells inside out or cause self intersection
// in special cases.

// .SECTION See Also
// vtkShrinkFilter

#ifndef __vtkShrinkPolyData_h
#define __vtkShrinkPolyData_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkShrinkPolyData : public vtkPolyDataToPolyDataFilter 
{
public:
  static vtkShrinkPolyData *New();
  vtkTypeMacro(vtkShrinkPolyData,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the fraction of shrink for each cell.
  vtkSetClampMacro(ShrinkFactor,float,0.0,1.0);

  // Description:
  // Get the fraction of shrink for each cell.
  vtkGetMacro(ShrinkFactor,float);

protected:
  vtkShrinkPolyData(float sf=0.5);
  ~vtkShrinkPolyData() {};
  vtkShrinkPolyData(const vtkShrinkPolyData&);
  void operator=(const vtkShrinkPolyData&);

  void Execute();
  float ShrinkFactor;
};

#endif
