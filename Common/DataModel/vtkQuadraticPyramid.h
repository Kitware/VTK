// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQuadraticPyramid
 * @brief   cell represents a parabolic, 13-node isoparametric pyramid
 *
 * vtkQuadraticPyramid is a concrete implementation of vtkNonLinearCell to
 * represent a three-dimensional, 13-node isoparametric parabolic
 * pyramid. The interpolation is the standard finite element, quadratic
 * isoparametric shape function. The cell includes a mid-edge node. The
 * ordering of the thirteen points defining the cell is point ids (0-4,5-12)
 * where point ids 0-4 are the five corner vertices of the pyramid; followed
 * by eight midedge nodes (5-12). Note that these midedge nodes lie
 * on the edges defined by (0,1), (1,2), (2,3), (3,0), (0,4), (1,4), (2,4),
 * (3,4), respectively. The parametric location of vertex #4 is [0, 0, 1].
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
 * vtkQuadraticHexahedron vtkQuadraticQuad vtkQuadraticWedge
 *
 * @par Thanks:
 * The shape functions and derivatives could be implemented thanks to
 * the report Pyramid Solid Elements Linear and Quadratic Iso-P Models
 * From Center For Aerospace Structures
 */

#ifndef vtkQuadraticPyramid_h
#define vtkQuadraticPyramid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkQuadraticEdge;
class vtkQuadraticQuad;
class vtkQuadraticTriangle;
class vtkTetra;
class vtkPyramid;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticPyramid : public vtkNonLinearCell
{
public:
  static vtkQuadraticPyramid* New();
  vtkTypeMacro(vtkQuadraticPyramid, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() override { return VTK_QUADRATIC_PYRAMID; }
  int GetCellDimension() override { return 3; }
  int GetNumberOfEdges() override { return 8; }
  int GetNumberOfFaces() override { return 5; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
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
   * Clip this quadratic triangle using scalar value provided. Like
   * contouring, except that it cuts the triangle to produce linear
   * triangles.
   */
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* tets, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;

  /**
   * Line-edge intersection. Intersection has to occur within [0,1] parametric
   * coordinates and with specified tolerance.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;

  /**
   * Return the center of the quadratic pyramid in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  static void InterpolationFunctions(const double pcoords[3], double weights[13]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[39]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[13]) override
  {
    vtkQuadraticPyramid::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[39]) override
  {
    vtkQuadraticPyramid::InterpolationDerivs(pcoords, derivs);
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
  void JacobianInverse(const double pcoords[3], double** inverse, double derivs[39]);

protected:
  vtkQuadraticPyramid();
  ~vtkQuadraticPyramid() override;

  vtkQuadraticEdge* Edge;
  vtkQuadraticTriangle* TriangleFace;
  vtkQuadraticQuad* Face;
  vtkTetra* Tetra;
  vtkPyramid* Pyramid;
  vtkPointData* PointData;
  vtkCellData* CellData;
  vtkDoubleArray* CellScalars;
  vtkDoubleArray* Scalars; // used to avoid New/Delete in contouring/clipping

  ///@{
  /**
   * This method adds in a point at the center of the quadrilateral face
   * and then interpolates values to that point. In order to do this it
   * also resizes certain member variable arrays. For safety should call
   * ResizeArrays() after the results of Subdivide() are not needed anymore.
   **/
  void Subdivide(
    vtkPointData* inPd, vtkCellData* inCd, vtkIdType cellId, vtkDataArray* cellScalars);
  ///@}
  ///@{
  /**
   * Resize the superclasses' member arrays to newSize where newSize should either be
   * 13 or 14. Call with 13 to reset the reallocation done in the Subdivide()
   * method or call with 14 to add one extra tuple for the generated point in
   * Subdivice. For efficiency it only resizes the superclasses' arrays.
   **/
  void ResizeArrays(vtkIdType newSize);
  ///@}

private:
  vtkQuadraticPyramid(const vtkQuadraticPyramid&) = delete;
  void operator=(const vtkQuadraticPyramid&) = delete;
};
//----------------------------------------------------------------------------
// Return the center of the quadratic pyramid in parametric coordinates.
//
inline int vtkQuadraticPyramid::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 6.0 / 13.0;
  pcoords[2] = 3.0 / 13.0;
  return 0;
}

VTK_ABI_NAMESPACE_END
#endif
