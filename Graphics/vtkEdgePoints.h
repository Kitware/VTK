/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgePoints.h
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
// .NAME vtkEdgePoints - generate points on isosurface
// .SECTION Description
// vtkEdgePoints is a filter that takes as input any dataset and 
// generates for output a set of points that lie on an isosurface. The 
// points are created by interpolation along cells edges whose end-points are 
// below and above the contour value.
// .SECTION Caveats
// vtkEdgePoints can be considered a "poor man's" dividing cubes algorithm
// (see vtkDividingCubes). Points are generated only on the edges of cells, 
// not in the interior, and at lower density than dividing cubes. However, it 
// is more general than dividing cubes since it treats any type of dataset.

#ifndef __vtkEdgePoints_h
#define __vtkEdgePoints_h

#include "vtkDataSetToPolyDataFilter.h"
#include "vtkMergePoints.h"

class VTK_GRAPHICS_EXPORT vtkEdgePoints : public vtkDataSetToPolyDataFilter
{
public:
  vtkTypeMacro(vtkEdgePoints,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with contour value of 0.0.
  static vtkEdgePoints *New();

  // Description:
  // Set/get the contour value.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);

protected:
  vtkEdgePoints();
  ~vtkEdgePoints();

  void Execute();

  float Value;
  vtkMergePoints *Locator;
private:
  vtkEdgePoints(const vtkEdgePoints&);  // Not implemented.
  void operator=(const vtkEdgePoints&);  // Not implemented.
};

#endif


