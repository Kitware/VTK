/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCell.h
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
// .NAME vtkGenericCell - provides thread-safe access to cells
// .SECTION Description
// vtkGenericCell is a class that provides access to concrete types of cells.
// It's main purpose is to allow thread-safe access to cells, supporting
// the vtkDataSet::GetCell(vtkGenericCell *) method. vtkGenericCell acts
// like any type of cell, it just dereferences an internal representation.
// .SECTION See Also
// vtkCell vtkDataSet

#ifndef __vtkGenericCell_h
#define __vtkGenericCell_h

#include "vtkCell.h"

class VTK_EXPORT vtkGenericCell : public vtkCell
{
public:
  // Description:
  // Create handle to any type of cell; by default a vtkEmptyCell.
  static vtkGenericCell *New();

  vtkTypeMacro(vtkGenericCell,vtkCell);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  void ShallowCopy(vtkCell *c);
  void DeepCopy(vtkCell *c);
  int GetCellType();
  int GetCellDimension();
  int GetInterpolationOrder();
  int GetNumberOfEdges();
  int GetNumberOfFaces();
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  int EvaluatePosition(float x[3], float* closestPoint, 
		       int& subId, float pcoords[3], 
		       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], 
                        float x[3], float *weights);
  void Contour(float value, vtkDataArray *cellScalars, 
	       vtkPointLocator *locator, vtkCellArray *verts, 
	       vtkCellArray *lines, vtkCellArray *polys, 
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
  void Clip(float value, vtkDataArray *cellScalars, 
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
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
			float x[3], float pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
		   int dim, float *derivs);
  int GetParametricCenter(float pcoords[3]);

  // Description:
  // This method is used to support the vtkDataSet::GetCell(vtkGenericCell *)
  // method. It allows vtkGenericCell to act like any cell type by
  // dereferencing an internal instance of a concrete cell type. When
  // you set the cell type, you are resetting a pointer to an internal
  // cell which is then used for computation.
  void SetCellType(int cellType);
  void SetCellTypeToEmptyCell() {this->SetCellType(VTK_EMPTY_CELL);}
  void SetCellTypeToVertex() {this->SetCellType(VTK_VERTEX);}
  void SetCellTypeToPolyVertex() {this->SetCellType(VTK_POLY_VERTEX);}
  void SetCellTypeToLine() {this->SetCellType(VTK_LINE);}
  void SetCellTypeToPolyLine() {this->SetCellType(VTK_POLY_LINE);}
  void SetCellTypeToTriangle() {this->SetCellType(VTK_TRIANGLE);}
  void SetCellTypeToTriangleStrip() {this->SetCellType(VTK_TRIANGLE_STRIP);}
  void SetCellTypeToPolygon() {this->SetCellType(VTK_POLYGON);}
  void SetCellTypeToPixel() {this->SetCellType(VTK_PIXEL);}
  void SetCellTypeToQuad() {this->SetCellType(VTK_QUAD);}
  void SetCellTypeToTetra() {this->SetCellType(VTK_TETRA);}
  void SetCellTypeToVoxel() {this->SetCellType(VTK_VOXEL);}
  void SetCellTypeToHexahedron() {this->SetCellType(VTK_HEXAHEDRON);}
  void SetCellTypeToWedge() {this->SetCellType(VTK_WEDGE);}
  void SetCellTypeToPyramid() {this->SetCellType(VTK_PYRAMID);}

protected:
  vtkGenericCell();
  ~vtkGenericCell();
  vtkGenericCell(const vtkGenericCell&);
  void operator=(const vtkGenericCell&);

  vtkCell *Cell;
  
};

#endif


