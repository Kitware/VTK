/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergePoints2D.h
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
// .NAME vtkMergePoints2D - merge exactly coincident points
// .SECTION Description
// vtkMergePoints2D is a locator object to quickly locate points in 3D.
// The primary difference between vtkMergePoints2D and its superclass
// vtkPointLocator is that vtkMergePoints2D merges precisely coincident points
// and is therefore much faster.
// .SECTION See Also
// vtkCleanPolyData

#ifndef __vtkMergePoints2D_h
#define __vtkMergePoints2D_h

#include "vtkPointLocator2D.h"

class VTK_FILTERING_EXPORT vtkMergePoints2D : public vtkPointLocator2D
{
public:
  static vtkMergePoints2D *New();
  vtkTypeMacro(vtkMergePoints2D,vtkPointLocator2D);

  // Description:
  // Determine whether point given by x[] has been inserted into points list.
  // Return id of previously inserted point if this is true, otherwise return
  // -1.
  int IsInsertedPoint(float x[2]);

protected:
  vtkMergePoints2D() {};
  ~vtkMergePoints2D() {};

private:
  vtkMergePoints2D(const vtkMergePoints2D&);  // Not implemented.
  void operator=(const vtkMergePoints2D&);  // Not implemented.
};

#endif


