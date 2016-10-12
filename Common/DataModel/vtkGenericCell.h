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
/**
 * @class   vtkGenericCell
 * @brief   provides thread-safe access to cells
 *
 * vtkGenericCell is a class that provides access to concrete types of cells.
 * It's main purpose is to allow thread-safe access to cells, supporting
 * the vtkDataSet::GetCell(vtkGenericCell *) method. vtkGenericCell acts
 * like any type of cell, it just dereferences an internal representation.
 * The SetCellType() methods use \#define constants; these are defined in
 * the file vtkCellType.h.
 *
 * @sa
 * vtkCell vtkDataSet
*/

#ifndef vtkGenericCell_h
#define vtkGenericCell_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"

class VTKCOMMONDATAMODEL_EXPORT vtkGenericCell : public vtkCell
{
public:
  /**
   * Create handle to any type of cell; by default a vtkEmptyCell.
   */
  static vtkGenericCell *New();

  vtkTypeMacro(vtkGenericCell,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set the points object to use for this cell. This updates the internal cell
   * storage as well as the public member variable Points.
   */
  void SetPoints(vtkPoints *points);

  /**
   * Set the point ids to use for this cell. This updates the internal cell
   * storage as well as the public member variable PointIds.
   */
  void SetPointIds(vtkIdList *pointIds);

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  void ShallowCopy(vtkCell *c) VTK_OVERRIDE;
  void DeepCopy(vtkCell *c) VTK_OVERRIDE;
  int GetCellType() VTK_OVERRIDE;
  int GetCellDimension() VTK_OVERRIDE;
  int IsLinear() VTK_OVERRIDE;
  int RequiresInitialization() VTK_OVERRIDE;
  void Initialize() VTK_OVERRIDE;
  int RequiresExplicitFaceRepresentation() VTK_OVERRIDE;
  void SetFaces(vtkIdType *faces) VTK_OVERRIDE;
  vtkIdType *GetFaces() VTK_OVERRIDE;
  int GetNumberOfEdges() VTK_OVERRIDE;
  int GetNumberOfFaces() VTK_OVERRIDE;
  vtkCell *GetEdge(int edgeId) VTK_OVERRIDE;
  vtkCell *GetFace(int faceId) VTK_OVERRIDE;
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights) VTK_OVERRIDE;
  void EvaluateLocation(int& subId, double pcoords[3],
                        double x[3], double *weights) VTK_OVERRIDE;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId,
               vtkCellData *outCd) VTK_OVERRIDE;
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *connectivity,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) VTK_OVERRIDE;
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3],
                        int& subId) VTK_OVERRIDE;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) VTK_OVERRIDE;
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) VTK_OVERRIDE;
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;
  double *GetParametricCoords() VTK_OVERRIDE;
  int IsPrimaryCell() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double *weights) VTK_OVERRIDE;
  void InterpolateDerivs(double pcoords[3], double *derivs) VTK_OVERRIDE;
  //@}

  /**
   * This method is used to support the vtkDataSet::GetCell(vtkGenericCell *)
   * method. It allows vtkGenericCell to act like any cell type by
   * dereferencing an internal instance of a concrete cell type. When
   * you set the cell type, you are resetting a pointer to an internal
   * cell which is then used for computation.
   */
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
  void SetCellTypeToQuadraticPolygon() {this->SetCellType(VTK_QUADRATIC_POLYGON);}
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

  /**
   * Instantiate a new vtkCell based on it's cell type value
   */
  static vtkCell* InstantiateCell(int cellType);

  vtkCell* GetRepresentativeCell() { return this->Cell; }

protected:
  vtkGenericCell();
  ~vtkGenericCell() VTK_OVERRIDE;

  vtkCell *Cell;

private:
  vtkGenericCell(const vtkGenericCell&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGenericCell&) VTK_DELETE_FUNCTION;
};

#endif
