/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetTriangleFilter.h
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
// .NAME vtkDataSetTriangleFilter - triangulate any type of dataset
// .SECTION Description
// vtkTriangulateDataSet generates n-dimensional simplices from any input
// dataset. That is, 3D cells are converted to tetrahedral meshes, 2D cells
// to triangles, and so on. The triangulation is guaranteed compatible as
// long as the dataset is either zero-, one- or two-dimensional; or if
// a three-dimensional dataset, all cells in the 3D dataset are convex 
// with planar facets.
//
// .SECTION See Also
// vtkOrderedTriangulator vtkTriangleFilter

#ifndef __vtkDataSetTriangleFilter_h
#define __vtkDataSetTriangleFilter_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class vtkOrderedTriangulator;

class VTK_GRAPHICS_EXPORT vtkDataSetTriangleFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  static vtkDataSetTriangleFilter *New();
  vtkTypeMacro(vtkDataSetTriangleFilter,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkDataSetTriangleFilter():Triangulator(NULL) {}
  ~vtkDataSetTriangleFilter();

  // Usual data generation method
  void Execute();

  // different execute methods depending on whether input is structured
  void StructuredExecute();
  void UnstructuredExecute();
  
  vtkOrderedTriangulator *Triangulator;
private:
  vtkDataSetTriangleFilter(const vtkDataSetTriangleFilter&);  // Not implemented.
  void operator=(const vtkDataSetTriangleFilter&);  // Not implemented.
};

#endif


