/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCell.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// The SetCellType() methods use \#define constants; these are defined in
// the file vtkCellType.h.

// .SECTION See Also
// vtkCell vtkDataSet

#ifndef __vtkGenericCell_h
#define __vtkGenericCell_h

#include "vtkCell.h"

class VTK_FILTERING_EXPORT vtkGenericCell : public vtkCell
{
public:
  // Description:
  // Create handle to any type of cell; by default a vtkEmptyCell.
  static vtkGenericCell *New();

  vtkTypeMacro(vtkGenericCell,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  void ShallowCopy(vtkCell *c);
  void DeepCopy(vtkCell *c);
  int GetCellType();
  int GetCellDimension();
  int IsLinear();
  int RequiresInitialization();
  void Initialize();
  int RequiresExplicitFaceRepresentation();
  void SetFaces(vtkIdType *faces);
  vtkIdType *GetFaces();
  int GetNumberOfEdges();
  int GetNumberOfFaces();
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3],
                        double x[3], double *weights);
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *connectivity,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs);
  int GetParametricCenter(double pcoords[3]);
  double *GetParametricCoords();
  int IsPrimaryCell();

  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double *weights);
  virtual void InterpolateDerivs(double pcoords[3], double *derivs);

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
  void SetCellTypeToPentagonalPrism() {this->SetCellType(VTK_PENTAGONAL_PRISM);}
  void SetCellTypeToHexagonalPrism() {this->SetCellType(VTK_HEXAGONAL_PRISM);}
  void SetCellTypeToPolyhedron() {this->SetCellType(VTK_POLYHEDRON);}
  void SetCellTypeToConvexPointSet() {this->SetCellType(VTK_CONVEX_POINT_SET);}
  void SetCellTypeToQuadraticEdge() {this->SetCellType(VTK_QUADRATIC_EDGE);}
  void SetCellTypeToCubicLine() {this->SetCellType(VTK_CUBIC_LINE);}
  void SetCellTypeToQuadraticTriangle() {this->SetCellType(VTK_QUADRATIC_TRIANGLE);}
  void SetCellTypeToBiQuadraticTriangle() {this->SetCellType(VTK_BIQUADRATIC_TRIANGLE);}
  void SetCellTypeToQuadraticQuad() {this->SetCellType(VTK_QUADRATIC_QUAD);}
  void SetCellTypeToQuadraticTetra() {this->SetCellType(VTK_QUADRATIC_TETRA);}
  void SetCellTypeToQuadraticHexahedron() {this->SetCellType(VTK_QUADRATIC_HEXAHEDRON);}
  void SetCellTypeToQuadraticWedge() {this->SetCellType(VTK_QUADRATIC_WEDGE);}
  void SetCellTypeToQuadraticPyramid() {this->SetCellType(VTK_QUADRATIC_PYRAMID);}
  void SetCellTypeToQuadraticLinearQuad() {this->SetCellType(VTK_QUADRATIC_LINEAR_QUAD);}
  void SetCellTypeToBiQuadraticQuad() {this->SetCellType(VTK_BIQUADRATIC_QUAD);}
  void SetCellTypeToQuadraticLinearWedge() {this->SetCellType(VTK_QUADRATIC_LINEAR_WEDGE);}
  void SetCellTypeToBiQuadraticQuadraticWedge() {
    this->SetCellType(VTK_BIQUADRATIC_QUADRATIC_WEDGE);}
  void SetCellTypeToTriQuadraticHexahedron() {
    this->SetCellType(VTK_TRIQUADRATIC_HEXAHEDRON);}
  void SetCellTypeToBiQuadraticQuadraticHexahedron() {
    this->SetCellType(VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON);}

  // Description:
  // Instantiate a new vtkCell based on it's cell type value
  static vtkCell* InstantiateCell(int cellType);

protected:
  vtkGenericCell();
  ~vtkGenericCell();

  vtkCell *Cell;

private:
  vtkGenericCell(const vtkGenericCell&);  // Not implemented.
  void operator=(const vtkGenericCell&);  // Not implemented.
};

#endif


