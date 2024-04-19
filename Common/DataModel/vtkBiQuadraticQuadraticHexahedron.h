// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBiQuadraticQuadraticHexahedron
 * @brief   cell represents a biquadratic,
 * 24-node isoparametric hexahedron
 *
 * vtkBiQuadraticQuadraticHexahedron is a concrete implementation of vtkNonLinearCell to
 * represent a three-dimensional, 24-node isoparametric biquadratic
 * hexahedron. The interpolation is the standard finite element,
 * biquadratic-quadratic
 * isoparametric shape function. The cell includes mid-edge and center-face nodes. The
 * ordering of the 24 points defining the cell is point ids (0-7,8-19, 20-23)
 * where point ids 0-7 are the eight corner vertices of the cube; followed by
 * twelve midedge nodes (8-19), nodes 20-23 are the center-face nodes. Note that
 * these midedge nodes correspond lie
 * on the edges defined by (0,1), (1,2), (2,3), (3,0), (4,5), (5,6), (6,7),
 * (7,4), (0,4), (1,5), (2,6), (3,7). The center face nodes laying in quad
 * 22-(0,1,5,4), 21-(1,2,6,5), 23-(2,3,7,6) and 22-(3,0,4,7)
 *
 * \verbatim
 *
 * top
 *  7--14--6
 *  |      |
 * 15      13
 *  |      |
 *  4--12--5
 *
 *  middle
 * 19--23--18
 *  |      |
 * 20      21
 *  |      |
 * 16--22--17
 *
 * bottom
 *  3--10--2
 *  |      |
 * 11      9
 *  |      |
 *  0-- 8--1
 *
 * \endverbatim
 *
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
 * vtkQuadraticQuad vtkQuadraticPyramid vtkQuadraticWedge
 *
 * @par Thanks:
 * Thanks to Soeren Gebbert  who developed this class and
 * integrated it into VTK 5.0.
 */

#ifndef vtkBiQuadraticQuadraticHexahedron_h
#define vtkBiQuadraticQuadraticHexahedron_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkQuadraticEdge;
class vtkQuadraticQuad;
class vtkBiQuadraticQuad;
class vtkHexahedron;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkBiQuadraticQuadraticHexahedron : public vtkNonLinearCell
{
public:
  static vtkBiQuadraticQuadraticHexahedron* New();
  vtkTypeMacro(vtkBiQuadraticQuadraticHexahedron, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() override { return VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON; }
  int GetCellDimension() override { return 3; }
  int GetNumberOfEdges() override { return 12; }
  int GetNumberOfFaces() override { return 6; }
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
   * Clip this biquadratic hexahedron using scalar value provided. Like
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

  static void InterpolationFunctions(const double pcoords[3], double weights[24]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[72]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[24]) override
  {
    vtkBiQuadraticQuadraticHexahedron::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[72]) override
  {
    vtkBiQuadraticQuadraticHexahedron::InterpolationDerivs(pcoords, derivs);
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
  void JacobianInverse(const double pcoords[3], double** inverse, double derivs[72]);

protected:
  vtkBiQuadraticQuadraticHexahedron();
  ~vtkBiQuadraticQuadraticHexahedron() override;

  vtkQuadraticEdge* Edge;
  vtkQuadraticQuad* Face;
  vtkBiQuadraticQuad* BiQuadFace;
  vtkHexahedron* Hex;
  vtkPointData* PointData;
  vtkCellData* CellData;
  vtkDoubleArray* CellScalars;
  vtkDoubleArray* Scalars;

  void Subdivide(
    vtkPointData* inPd, vtkCellData* inCd, vtkIdType cellId, vtkDataArray* cellScalars);

private:
  vtkBiQuadraticQuadraticHexahedron(const vtkBiQuadraticQuadraticHexahedron&) = delete;
  void operator=(const vtkBiQuadraticQuadraticHexahedron&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
