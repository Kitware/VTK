// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkCell.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkGenericCell : public vtkCell
{
public:
  /**
   * Create handle to any type of cell; by default a vtkEmptyCell.
   */
  static vtkGenericCell* New();

  vtkTypeMacro(vtkGenericCell, vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the points object to use for this cell. This updates the internal cell
   * storage as well as the public member variable Points.
   */
  void SetPoints(vtkPoints* points);

  /**
   * Set the point ids to use for this cell. This updates the internal cell
   * storage as well as the public member variable PointIds.
   */
  void SetPointIds(vtkIdList* pointIds);

  ///@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  void ShallowCopy(vtkCell* c) override;
  void DeepCopy(vtkCell* c) override;
  int GetCellType() override;
  int GetCellDimension() override;
  int IsLinear() VTK_FUTURE_CONST override;
  int RequiresInitialization() override;
  void Initialize() override;
  int RequiresExplicitFaceRepresentation() VTK_FUTURE_CONST override;
  VTK_DEPRECATED_IN_9_4_0("Use SetCellFaces.")
  void SetFaces(vtkIdType* faces) override;
  VTK_DEPRECATED_IN_9_4_0("Use GetCellFaces.")
  vtkIdType* GetFaces() override;
  int SetCellFaces(vtkCellArray* faces);
  vtkCellArray* GetCellFaces();
  void GetCellFaces(vtkCellArray* faces);
  int GetNumberOfEdges() override;
  int GetNumberOfFaces() override;
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;
  int EvaluatePosition(const double x[3], double closestPoint[3], int& subId, double pcoords[3],
    double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* connectivity, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;
  int Triangulate(int index, vtkIdList* ptIds, vtkPoints* pts) override;
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;
  int TriangulateIds(int index, vtkIdList* ptIds) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  int GetParametricCenter(double pcoords[3]) override;
  double* GetParametricCoords() override;
  int IsPrimaryCell() VTK_FUTURE_CONST override;
  ///@}

  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double* weights) override;
  void InterpolateDerivs(const double pcoords[3], double* derivs) override;
  ///@}

  /**
   * This method is used to support the vtkDataSet::GetCell(vtkGenericCell *)
   * method. It allows vtkGenericCell to act like any cell type by
   * dereferencing an internal instance of a concrete cell type. When
   * you set the cell type, you are resetting a pointer to an internal
   * cell which is then used for computation.
   */
  void SetCellType(int cellType);
  void SetCellTypeToEmptyCell() { this->SetCellType(VTK_EMPTY_CELL); }
  void SetCellTypeToVertex() { this->SetCellType(VTK_VERTEX); }
  void SetCellTypeToPolyVertex() { this->SetCellType(VTK_POLY_VERTEX); }
  void SetCellTypeToLine() { this->SetCellType(VTK_LINE); }
  void SetCellTypeToPolyLine() { this->SetCellType(VTK_POLY_LINE); }
  void SetCellTypeToTriangle() { this->SetCellType(VTK_TRIANGLE); }
  void SetCellTypeToTriangleStrip() { this->SetCellType(VTK_TRIANGLE_STRIP); }
  void SetCellTypeToPolygon() { this->SetCellType(VTK_POLYGON); }
  void SetCellTypeToPixel() { this->SetCellType(VTK_PIXEL); }
  void SetCellTypeToQuad() { this->SetCellType(VTK_QUAD); }
  void SetCellTypeToTetra() { this->SetCellType(VTK_TETRA); }
  void SetCellTypeToVoxel() { this->SetCellType(VTK_VOXEL); }
  void SetCellTypeToHexahedron() { this->SetCellType(VTK_HEXAHEDRON); }
  void SetCellTypeToWedge() { this->SetCellType(VTK_WEDGE); }
  void SetCellTypeToPyramid() { this->SetCellType(VTK_PYRAMID); }
  void SetCellTypeToPentagonalPrism() { this->SetCellType(VTK_PENTAGONAL_PRISM); }
  void SetCellTypeToHexagonalPrism() { this->SetCellType(VTK_HEXAGONAL_PRISM); }
  void SetCellTypeToPolyhedron() { this->SetCellType(VTK_POLYHEDRON); }
  void SetCellTypeToConvexPointSet() { this->SetCellType(VTK_CONVEX_POINT_SET); }
  void SetCellTypeToQuadraticEdge() { this->SetCellType(VTK_QUADRATIC_EDGE); }
  void SetCellTypeToCubicLine() { this->SetCellType(VTK_CUBIC_LINE); }
  void SetCellTypeToQuadraticTriangle() { this->SetCellType(VTK_QUADRATIC_TRIANGLE); }
  void SetCellTypeToBiQuadraticTriangle() { this->SetCellType(VTK_BIQUADRATIC_TRIANGLE); }
  void SetCellTypeToQuadraticQuad() { this->SetCellType(VTK_QUADRATIC_QUAD); }
  void SetCellTypeToQuadraticPolygon() { this->SetCellType(VTK_QUADRATIC_POLYGON); }
  void SetCellTypeToQuadraticTetra() { this->SetCellType(VTK_QUADRATIC_TETRA); }
  void SetCellTypeToQuadraticHexahedron() { this->SetCellType(VTK_QUADRATIC_HEXAHEDRON); }
  void SetCellTypeToQuadraticWedge() { this->SetCellType(VTK_QUADRATIC_WEDGE); }
  void SetCellTypeToQuadraticPyramid() { this->SetCellType(VTK_QUADRATIC_PYRAMID); }
  void SetCellTypeToQuadraticLinearQuad() { this->SetCellType(VTK_QUADRATIC_LINEAR_QUAD); }
  void SetCellTypeToBiQuadraticQuad() { this->SetCellType(VTK_BIQUADRATIC_QUAD); }
  void SetCellTypeToQuadraticLinearWedge() { this->SetCellType(VTK_QUADRATIC_LINEAR_WEDGE); }
  void SetCellTypeToBiQuadraticQuadraticWedge()
  {
    this->SetCellType(VTK_BIQUADRATIC_QUADRATIC_WEDGE);
  }
  void SetCellTypeToTriQuadraticHexahedron() { this->SetCellType(VTK_TRIQUADRATIC_HEXAHEDRON); }
  void SetCellTypeToTriQuadraticPyramid() { this->SetCellType(VTK_TRIQUADRATIC_PYRAMID); }
  void SetCellTypeToBiQuadraticQuadraticHexahedron()
  {
    this->SetCellType(VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON);
  }
  void SetCellTypeToLagrangeTriangle() { this->SetCellType(VTK_LAGRANGE_TRIANGLE); }
  void SetCellTypeToLagrangeTetra() { this->SetCellType(VTK_LAGRANGE_TETRAHEDRON); }
  void SetCellTypeToLagrangeCurve() { this->SetCellType(VTK_LAGRANGE_CURVE); }
  void SetCellTypeToLagrangeQuadrilateral() { this->SetCellType(VTK_LAGRANGE_QUADRILATERAL); }
  void SetCellTypeToLagrangeHexahedron() { this->SetCellType(VTK_LAGRANGE_HEXAHEDRON); }
  void SetCellTypeToLagrangeWedge() { this->SetCellType(VTK_LAGRANGE_WEDGE); }

  void SetCellTypeToBezierTriangle() { this->SetCellType(VTK_BEZIER_TRIANGLE); }
  void SetCellTypeToBezierTetra() { this->SetCellType(VTK_BEZIER_TETRAHEDRON); }
  void SetCellTypeToBezierCurve() { this->SetCellType(VTK_BEZIER_CURVE); }
  void SetCellTypeToBezierQuadrilateral() { this->SetCellType(VTK_BEZIER_QUADRILATERAL); }
  void SetCellTypeToBezierHexahedron() { this->SetCellType(VTK_BEZIER_HEXAHEDRON); }
  void SetCellTypeToBezierWedge() { this->SetCellType(VTK_BEZIER_WEDGE); }
  /**
   * Instantiate a new vtkCell based on it's cell type value
   */
  static vtkCell* InstantiateCell(int cellType);

  vtkCell* GetRepresentativeCell() { return this->Cell; }

protected:
  vtkGenericCell();
  ~vtkGenericCell() override;

  vtkCell* Cell;
  vtkCell* CellStore[VTK_NUMBER_OF_CELL_TYPES];

private:
  vtkGenericCell(const vtkGenericCell&) = delete;
  void operator=(const vtkGenericCell&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
