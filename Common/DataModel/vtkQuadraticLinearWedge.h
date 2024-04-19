// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQuadraticLinearWedge
 * @brief   cell represents a, 12-node isoparametric wedge
 *
 * vtkQuadraticLinearWedge is a concrete implementation of vtkNonLinearCell to
 * represent a three-dimensional, 12-node isoparametric linear quadratic
 * wedge. The interpolation is the standard finite element, quadratic
 * isoparametric shape function in xy - layer and the linear functions in z - direction.
 * The cell includes mid-edge node in the triangle edges. The
 * ordering of the 12 points defining the cell is point ids (0-5,6-12)
 * where point ids 0-5 are the six corner vertices of the wedge; followed by
 * six midedge nodes (6-12). Note that these midedge nodes correspond lie
 * on the edges defined by (0,1), (1,2), (2,0), (3,4), (4,5), (5,3).
 * The Edges (0,3), (1,4), (2,5) don't have midedge nodes.
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
 * vtkQuadraticHexahedron vtkQuadraticQuad vtkQuadraticPyramid
 *
 * @par Thanks:
 * Thanks to Soeren Gebbert who developed this class and
 * integrated it into VTK 5.0.
 */

#ifndef vtkQuadraticLinearWedge_h
#define vtkQuadraticLinearWedge_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkQuadraticEdge;
class vtkLine;
class vtkQuadraticLinearQuad;
class vtkQuadraticTriangle;
class vtkWedge;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticLinearWedge : public vtkNonLinearCell
{
public:
  static vtkQuadraticLinearWedge* New();
  vtkTypeMacro(vtkQuadraticLinearWedge, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() override { return VTK_QUADRATIC_LINEAR_WEDGE; }
  int GetCellDimension() override { return 3; }
  int GetNumberOfEdges() override { return 9; }
  int GetNumberOfFaces() override { return 5; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  ///@}

  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;

  ///@{
  /**
   * The quadratic linear wedge is split into 4 linear wedges,
   * each of them is contoured by a provided scalar value
   */
  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;
  int EvaluatePosition(const double x[3], double* closestPoint, int& subId, double pcoords[3],
    double& dist2, double* weights) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  double* GetParametricCoords() override;
  ///@}

  /**
   * Clip this quadratic linear wedge using scalar value provided. Like
   * contouring, except that it cuts the hex to produce linear
   * tetrahedron.
   */
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* tetras, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;

  /**
   * Line-edge intersection. Intersection has to occur within [0,1] parametric
   * coordinates and with specified tolerance.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;

  /**
   * Return the center of the quadratic linear wedge in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  static void InterpolationFunctions(const double pcoords[3], double weights[12]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[36]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[12]) override
  {
    vtkQuadraticLinearWedge::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[36]) override
  {
    vtkQuadraticLinearWedge::InterpolationDerivs(pcoords, derivs);
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
  void JacobianInverse(const double pcoords[3], double** inverse, double derivs[36]);

protected:
  vtkQuadraticLinearWedge();
  ~vtkQuadraticLinearWedge() override;

  vtkQuadraticEdge* QuadEdge;
  vtkLine* Edge;
  vtkQuadraticTriangle* TriangleFace;
  vtkQuadraticLinearQuad* Face;
  vtkWedge* Wedge;
  vtkDoubleArray* Scalars; // used to avoid New/Delete in contouring/clipping

private:
  vtkQuadraticLinearWedge(const vtkQuadraticLinearWedge&) = delete;
  void operator=(const vtkQuadraticLinearWedge&) = delete;
};
//----------------------------------------------------------------------------
// Return the center of the quadratic wedge in parametric coordinates.
inline int vtkQuadraticLinearWedge::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 1. / 3;
  pcoords[2] = 0.5;
  return 0;
}

VTK_ABI_NAMESPACE_END
#endif
