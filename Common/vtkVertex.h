/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertex.h
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
// .NAME vtkVertex - a cell that represents a 3D point
// .SECTION Description
// vtkVertex is a concrete implementation of vtkCell to represent a 3D point.

#ifndef __vtkVertex_h
#define __vtkVertex_h

#include "vtkCell.h"

class VTK_COMMON_EXPORT vtkVertex : public vtkCell
{
public:
  static vtkVertex *New();
  vtkTypeMacro(vtkVertex,vtkCell);
  
  // Description:
  // Make a new vtkVertex object with the same information as this object.
  vtkCell *MakeObject();

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_VERTEX;};
  int GetCellDimension() {return 0;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int) {return 0;};
  vtkCell *GetFace(int) {return 0;};
  void Clip(float value, vtkDataArray *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *pts,
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
  int EvaluatePosition(float x[3], float* closestPoint, 
                       int& subId, float pcoords[3], 
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);

  // Description:
  // Given parametric coordinates of a point, return the closest cell
  // boundary, and whether the point is inside or outside of the cell. The
  // cell boundary is defined by a list of points (pts) that specify a vertex
  // (1D cell).  If the return value of the method is != 0, then the point is
  // inside the cell.
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);

  // Description:
  // Generate contouring primitives. The scalar list cellScalars are
  // scalar values at each cell point. The point locator is essentially a 
  // points list that merges points as they are inserted (i.e., prevents 
  // duplicates). 
  void Contour(float value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts1, 
               vtkCellArray *lines, vtkCellArray *verts2, 
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  virtual void Contour(float value, vtkScalars *cellScalars, 
                       vtkPointLocator *locator, vtkCellArray *verts, 
                       vtkCellArray *lines, vtkCellArray *polys, 
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, vtkIdType cellId,
                       vtkCellData *outCd)
    {
      VTK_LEGACY_METHOD("Contour", "4.0");
      this->Contour(value, cellScalars->GetData(), locator, verts, 
		    lines, polys, inPd, outPd, inCd, cellId, outCd);
    }

  // Description:
  // Intersect with a ray. Return parametric coordinates (both line and cell)
  // and global intersection coordinates, given ray definition and tolerance. 
  // The method returns non-zero value if intersection occurs.
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);
  
  // Description:
  // Triangulate the vertex. This method fills pts and ptIds with information
  // from the only point in the vertex.
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);

  // Description:
  // Get the derivative of the vertex. Returns (0.0, 0.0, 0.0) for all 
  // dimensions.
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Description:
  // Vertex specific methods.
  static void InterpolationFunctions(float pcoords[3], float weights[1]);

protected:
  vtkVertex();
  ~vtkVertex() {};
  vtkVertex(const vtkVertex&);
  void operator=(const vtkVertex&);
  
};

#endif


