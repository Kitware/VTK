// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQuadraticTetra
 * @brief   cell represents a parabolic, 10-node isoparametric tetrahedron
 *
 * vtkQuadraticTetra is a concrete implementation of vtkNonLinearCell to
 * represent a three-dimensional, 10-node, isoparametric parabolic
 * tetrahedron. The interpolation is the standard finite element, quadratic
 * isoparametric shape function. The cell includes a mid-edge node on each of
 * the size edges of the tetrahedron. The ordering of the ten points defining
 * the cell is point ids (0-3,4-9) where ids 0-3 are the four tetra
 * vertices; and point ids 4-9 are the midedge nodes between (0,1), (1,2),
 * (2,0), (0,3), (1,3), and (2,3).
 *
 * Note that this class uses an internal linear tessellation for some internal operations
 * (e.g., clipping and contouring). This means that some artifacts may appear trying to
 * represent a non-linear interpolation function with linear tets.
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticWedge
 * vtkQuadraticQuad vtkQuadraticHexahedron vtkQuadraticPyramid
 */

#ifndef vtkQuadraticTetra_h
#define vtkQuadraticTetra_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkQuadraticEdge;
class vtkQuadraticTriangle;
class vtkTetra;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticTetra : public vtkNonLinearCell
{
public:
  static vtkQuadraticTetra* New();
  vtkTypeMacro(vtkQuadraticTetra, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() override { return VTK_QUADRATIC_TETRA; }
  int GetCellDimension() override { return 3; }
  int GetNumberOfEdges() override { return 6; }
  int GetNumberOfFaces() override { return 4; }
  vtkCell* GetEdge(int) override;
  vtkCell* GetFace(int) override;
  ///@}

  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;
  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;
  int EvaluatePosition(const double x[3], double closestPoint[3], int& subId, double pcoords[3],
    double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  double* GetParametricCoords() override;

  /**
   * Clip this edge using scalar value provided. Like contouring, except
   * that it cuts the tetra to produce new tetras.
   */
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* tetras, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;

  /**
   * Clip this edge using scalar value provided. Like contouring, except
   * that it cuts the tetra to produce new tetras.
   *
   * Returns true if newly inserted cell is a quadratic tetra, false otherwise.
   *
   * @see vtkNonLinearCell::StableClip
   */
  bool StableClip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* tetras, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;

  /**
   * Line-edge intersection. Intersection has to occur within [0,1] parametric
   * coordinates and with specified tolerance.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;

  /**
   * Return the center of the quadratic tetra in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * Return the distance of the parametric coordinate provided to the
   * cell. If inside the cell, a distance of zero is returned.
   */
  double GetParametricDistance(const double pcoords[3]) override;

  static void InterpolationFunctions(const double pcoords[3], double weights[10]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[30]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[10]) override
  {
    vtkQuadraticTetra::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[30]) override
  {
    vtkQuadraticTetra::InterpolationDerivs(pcoords, derivs);
  }
  ///@}
  ///@{
  /**
   * Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
   * Ids are related to the cell, not to the dataset.
   *
   * @note The return type changed. It used to be int*, it is now const vtkIdType*.
   * This is so ids are unified between vtkCell and vtkPoints.
   */
  static const vtkIdType* GetEdgeArray(vtkIdType edgeId);
  static const vtkIdType* GetFaceArray(vtkIdType faceId);
  ///@}

  /**
   * Given parametric coordinates compute inverse Jacobian transformation
   * matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
   * function derivatives.
   */
  void JacobianInverse(const double pcoords[3], double** inverse, double derivs[30]);

protected:
  vtkQuadraticTetra();
  ~vtkQuadraticTetra() override;

  vtkQuadraticEdge* Edge;
  vtkQuadraticTriangle* Face;
  vtkTetra* Tetra;
  vtkDoubleArray* Scalars; // used to avoid New/Delete in contouring/clipping

private:
  vtkQuadraticTetra(const vtkQuadraticTetra&) = delete;
  void operator=(const vtkQuadraticTetra&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
