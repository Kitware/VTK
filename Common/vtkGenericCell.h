/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCell.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericCell - provides thread-safe access to cells
// .SECTION Description
// vtkGenericCell is a class that provides access to concrete types of cells.
// It's main purpose is to allow thread-safe access to cells, supporting
// the vtkDataSet::GetCell(vtkGenericCell *) method. vtkGenericCell acts
// like any type of cell, it just dereferences an internal representation.
// The SetCellType() methods use #define constants; these are defined in
// the file vtkCellType.h.

// .SECTION See Also
// vtkCell vtkDataSet

#ifndef __vtkGenericCell_h
#define __vtkGenericCell_h

#include "vtkCell.h"

class VTK_COMMON_EXPORT vtkGenericCell : public vtkCell
{
public:
  // Description:
  // Create handle to any type of cell; by default a vtkEmptyCell.
  static vtkGenericCell *New();

  vtkTypeRevisionMacro(vtkGenericCell,vtkCell);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  void ShallowCopy(vtkCell *c);
  void DeepCopy(vtkCell *c);
  int GetCellType();
  int GetCellDimension();
  int IsLinear();
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
  void Clip(float value, vtkDataArray *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *connectivity,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd, 
            int insideOut);
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
  void SetCellTypeToConvexPointSet() {this->SetCellType(VTK_CONVEX_POINT_SET);}
  void SetCellTypeToQuadraticEdge() {this->SetCellType(VTK_QUADRATIC_EDGE);}
  void SetCellTypeToQuadraticTriangle() {this->SetCellType(VTK_QUADRATIC_TRIANGLE);}
  void SetCellTypeToQuadraticQuad() {this->SetCellType(VTK_QUADRATIC_QUAD);}
  void SetCellTypeToQuadraticTetra() {this->SetCellType(VTK_QUADRATIC_TETRA);}
  void SetCellTypeToQuadraticHexahedron() {this->SetCellType(VTK_QUADRATIC_HEXAHEDRON);}

protected:
  vtkGenericCell();
  ~vtkGenericCell();

  vtkCell *Cell;
  
private:
  vtkGenericCell(const vtkGenericCell&);  // Not implemented.
  void operator=(const vtkGenericCell&);  // Not implemented.
};

#endif


