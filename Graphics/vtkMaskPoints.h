/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskPoints.h
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
// .NAME vtkMaskPoints - selectively filter points
// .SECTION Description
// vtkMaskPoints is a filter that passes through points and point attributes 
// from input dataset. (Other geometry is not passed through.) It is 
// possible to mask every nth point, and to specify an initial offset
// to begin masking from. A special random mode feature enables random 
// selection of points. The filter can also generate vertices (topological
// primitives) as well as points. This is useful because vertices are
// rendered while points are not.

#ifndef __vtkMaskPoints_h
#define __vtkMaskPoints_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkMaskPoints : public vtkDataSetToPolyDataFilter
{
public:
  static vtkMaskPoints *New();
  vtkTypeMacro(vtkMaskPoints,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on every nth point.
  vtkSetClampMacro(OnRatio,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(OnRatio,int);

  // Description:
  // Limit the number of points that can be passed through.
  vtkSetClampMacro(MaximumNumberOfPoints,vtkIdType,0,VTK_LARGE_ID);
  vtkGetMacro(MaximumNumberOfPoints,vtkIdType);

  // Description:
  // Start with this point.
  vtkSetClampMacro(Offset,vtkIdType,0,VTK_LARGE_ID);
  vtkGetMacro(Offset,vtkIdType);

  // Description:
  // Special flag causes randomization of point selection. If this mode is on,
  // statistically every nth point (i.e., OnRatio) will be displayed.
  vtkSetMacro(RandomMode,int);
  vtkGetMacro(RandomMode,int);
  vtkBooleanMacro(RandomMode,int);

  // Description:
  // Generate output polydata vertices as well as points. A useful
  // convenience method because vertices are drawn (they are topology) while
  // points are not (they are geometry). By default this method is off.
  vtkSetMacro(GenerateVertices,int);
  vtkGetMacro(GenerateVertices,int);
  vtkBooleanMacro(GenerateVertices,int);

protected:
  vtkMaskPoints();
  ~vtkMaskPoints() {};
  vtkMaskPoints(const vtkMaskPoints&);
  void operator=(const vtkMaskPoints&);

  void Execute();

  int OnRatio;     // every OnRatio point is on; all others are off.
  vtkIdType Offset;      // offset (or starting point id)
  int RandomMode;  // turn on/off randomization
  vtkIdType MaximumNumberOfPoints;
  int GenerateVertices; //generate polydata verts
};

#endif


