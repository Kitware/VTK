// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTriQuadraticHexahedron
 * @brief   cell represents a parabolic, 27-node isoparametric hexahedron
 *
 * vtkTriQuadraticHexahedron is a concrete implementation of vtkNonLinearCell to
 * represent a three-dimensional, 27-node isoparametric triquadratic
 * hexahedron. The interpolation is the standard finite element, triquadratic
 * isoparametric shape function. The cell includes 8 edge nodes, 12 mid-edge nodes,
 * 6 mid-face nodes and one mid-volume node. The ordering of the 27 points defining the
 * cell is point ids (0-7,8-19, 20-25, 26)
 * where point ids 0-7 are the eight corner vertices of the cube; followed by
 * twelve midedge nodes (8-19); followed by 6 mid-face nodes (20-25) and the last node (26)
 * is the mid-volume node. Note that these midedge nodes correspond lie
 * on the edges defined by (0,1), (1,2), (2,3), (3,0), (4,5), (5,6), (6,7),
 * (7,4), (0,4), (1,5), (2,6), (3,7). The mid-surface nodes lies on the faces
 * defined by (first edge nodes id's, than mid-edge nodes id's):
 * (0,1,5,4;8,17,12,16), (1,2,6,5;9,18,13,17), (2,3,7,6,10,19,14,18),
 * (3,0,4,7;11,16,15,19), (0,1,2,3;8,9,10,11), (4,5,6,7;12,13,14,15).
 * The last point lies in the center of the cell (0,1,2,3,4,5,6,7).
 *
 * \verbatim
 *
 * top
 *  7--14--6
 *  |      |
 * 15  25  13
 *  |      |
 *  4--12--5
 *
 *  middle
 * 19--23--18
 *  |      |
 * 20  26  21
 *  |      |
 * 16--22--17
 *
 * bottom
 *  3--10--2
 *  |      |
 * 11  24  9
 *  |      |
 *  0-- 8--1
 *
 * \endverbatim
 *
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
 * vtkQuadraticQuad vtkQuadraticPyramid vtkQuadraticWedge
 * vtkBiQuadraticQuad
 *
 * @par Thanks:
 * Thanks to Soeren Gebbert who developed this class and
 * integrated it into VTK 5.0.
 */

#ifndef vtkTriQuadraticHexahedron_h
#define vtkTriQuadraticHexahedron_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkQuadraticEdge;
class vtkBiQuadraticQuad;
class vtkHexahedron;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkTriQuadraticHexahedron : public vtkNonLinearCell
{
public:
  static vtkTriQuadraticHexahedron* New();
  vtkTypeMacro(vtkTriQuadraticHexahedron, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() override { return VTK_TRIQUADRATIC_HEXAHEDRON; }
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
  int EvaluatePosition(const double x[3], double* closestPoint, int& subId, double pcoords[3],
    double& dist2, double* weights) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  double* GetParametricCoords() override;

  /**
   * Clip this triquadratic hexahedron using scalar value provided. Like
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

  static void InterpolationFunctions(const double pcoords[3], double weights[27]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[81]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[27]) override
  {
    vtkTriQuadraticHexahedron::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[81]) override
  {
    vtkTriQuadraticHexahedron::InterpolationDerivs(pcoords, derivs);
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
  void JacobianInverse(const double pcoords[3], double** inverse, double derivs[81]);

protected:
  vtkTriQuadraticHexahedron();
  ~vtkTriQuadraticHexahedron() override;

  vtkQuadraticEdge* Edge;
  vtkBiQuadraticQuad* Face;
  vtkHexahedron* Hex;
  vtkDoubleArray* Scalars;

private:
  vtkTriQuadraticHexahedron(const vtkTriQuadraticHexahedron&) = delete;
  void operator=(const vtkTriQuadraticHexahedron&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
