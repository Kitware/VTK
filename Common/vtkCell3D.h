/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCell3D.h
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
// .NAME vtkCell3D - abstract class to specify 3D cell interface
// .SECTION Description
// vtkCell3D is an abstract class that extends the interfaces for 3D data 
// cells, and implements methods needed to satisfy the vtkCell API. The 
// 3D cells include hexehedra, tetrahedra, wedge, pyramid, and voxel.

// .SECTION See Also
// vtkTetra vtkHexahedron vtkVoxel vtkWedge vtkPyramid

#ifndef __vtkCell3D_h
#define __vtkCell3D_h

#include "vtkCell.h"

class vtkOrderedTriangulator;

class VTK_COMMON_EXPORT vtkCell3D : public vtkCell
{
public:
  vtkTypeMacro(vtkCell3D,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the pair of vertices that define an edge. The method returns the
  // number of vertices, along with an array of vertices. Note that the
  // vertices are 0-offset; that is, they refer to the ids of the cell, not
  // the point ids of the mesh that the cell belongs to. The edgeId must
  // range between 0<=edgeId<this->GetNumberOfEdges().
  virtual void GetEdgePoints(int edgeId, int* &pts) = 0;
  
  // Description:
  // Get the list of vertices that define a face.  The list is terminated
  // with a negative number. Note that the vertices are 0-offset; that is,
  // they refer to the ids of the cell, not the point ids of the mesh that
  // the cell belongs to. The faceId must range between
  // 0<=faceId<this->GetNumberOfFaces().
  virtual void GetFacePoints(int faceId, int* &pts) = 0;

  // Description:
  // Cut (or clip) the cell based on the input cellScalars and the specified
  // value. The output of the clip operation will be one or more cells of the
  // same topological dimension as the original cell.  The flag insideOut
  // controls what part of the cell is considered inside - normally cell
  // points whose scalar value is greater than "value" are considered
  // inside. If insideOut is on, this is reversed. Also, if the output cell
  // data is non-NULL, the cell data from the clipped cell is passed to the
  // generated contouring primitives. (Note: the CopyAllocate() method must
  // be invoked on both the output cell and point data. The cellId refers to
  // the cell from which the cell data is copied.)  (Satisfies vtkCell API.)
  virtual void Clip(float value, vtkDataArray *cellScalars, 
                    vtkPointLocator *locator, vtkCellArray *connectivity,
                    vtkPointData *inPd, vtkPointData *outPd,
                    vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd, 
                    int insideOut);
  virtual void Clip(float value, vtkScalars *cellScalars, 
                    vtkPointLocator *locator, vtkCellArray *connectivity,
                    vtkPointData *inPd, vtkPointData *outPd,
                    vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd, 
                    int insideOut)
    {
      vtkWarningMacro("The use of this method has been deprecated.You should use vtkGenericCell::Clip(float, vtkDataArray*, vtkPointLocator*, vtkCellArray*, vtkPointData*, vtkPointData*, vtkCellData*, vtkIdType, vtkCellData*, int) instead.");
      this->Clip(value, cellScalars->GetData(), locator, connectivity, 
		 inPd, outPd, inCd, cellId, outCd, insideOut);
    }

  // Description:
  // The topological dimension of the cell. (Satisfies vtkCell API.)
  virtual int GetCellDimension() {return 3;}

protected:
  vtkCell3D():Triangulator(NULL) {}
  ~vtkCell3D();
  vtkCell3D(const vtkCell3D&);
  void operator=(const vtkCell3D&);
  
  vtkOrderedTriangulator *Triangulator;
  
};

#endif


