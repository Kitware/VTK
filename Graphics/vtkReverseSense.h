/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReverseSense.h
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
// .NAME vtkReverseSense - reverse the ordering of polygonal cells and/or vertex normals
// .SECTION Description
// 
// vtkReverseSense is a filter that reverses the order of polygonal cells
// and/or reverses the direction of point and cell normals. Two flags are
// used to control these operations. Cell reversal means reversing the order
// of indices in the cell connectivity list. Normal reversal means
// multiplying the normal vector by -1 (both point and cell normals, 
// if present).

// .SECTION Caveats
// Normals can be operated on only if they are present in the data.

#ifndef __vtkReverseSense_h
#define __vtkReverseSense_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkReverseSense : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeMacro(vtkReverseSense,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object so that behavior is to reverse cell ordering and
  // leave normal orientation as is.
  static vtkReverseSense *New();

  // Description:
  // Flag controls whether to reverse cell ordering.
  vtkSetMacro(ReverseCells,int);
  vtkGetMacro(ReverseCells,int);
  vtkBooleanMacro(ReverseCells,int);

  // Description:
  // Flag controls whether to reverse normal orientation.
  vtkSetMacro(ReverseNormals,int);
  vtkGetMacro(ReverseNormals,int);
  vtkBooleanMacro(ReverseNormals,int);


protected:
  vtkReverseSense();
  ~vtkReverseSense() {};
  vtkReverseSense(const vtkReverseSense&);
  void operator=(const vtkReverseSense&);

  // Usual data generation method
  void Execute();

  int ReverseCells;
  int ReverseNormals;
};

#endif


