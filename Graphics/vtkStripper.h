/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStripper.h
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
// .NAME vtkStripper - create triangle strips and/or poly-lines
// .SECTION Description
// vtkStripper is a filter that generates triangle strips and/or poly-lines
// from input polygons, triangle strips, and lines. Input polygons are 
// assumed to be triangles. (Use vtkTriangleFilter to triangulate 
// non-triangular polygons.) The filter will pass through (to the output)
// vertices if they are present in the input poly-data.
//
// The ivar MaximumLength can be used to control the maximum
// allowable triangle strip and poly-line length.

// .SECTION Caveats
// If triangle strips or poly-lines exist in the input data they will
// be passed through to the output data. This filter will only construct
// triangle strips if triangle polygons are available; and will only 
// construct poly-lines if lines are available.

// .SECTION See Also
// vtkTriangleFilter

#ifndef __vtkStripper_h
#define __vtkStripper_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_EXPORT vtkStripper : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeMacro(vtkStripper,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct object with MaximumLength set to 1000.
  static vtkStripper *New();

  // Description:
  // Specify the maximum number of triangles in a triangle strip,
  // and/or the maximum number of lines in a poly-line.
  vtkSetClampMacro(MaximumLength,int,4,100000);
  vtkGetMacro(MaximumLength,int);

protected:
  vtkStripper();
  ~vtkStripper() {};
  vtkStripper(const vtkStripper&);
  void operator=(const vtkStripper&);

  // Usual data generation method
  void Execute();

  int MaximumLength;
};

#endif


