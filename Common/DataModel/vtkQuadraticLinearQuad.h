// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQuadraticLinearQuad
 * @brief   cell represents a quadratic-linear, 6-node isoparametric quad
 *
 * vtkQuadraticQuad is a concrete implementation of vtkNonLinearCell to
 * represent a two-dimensional, 6-node isoparametric quadratic-linear quadrilateral
 * element. The interpolation is the standard finite element, quadratic-linear
 * isoparametric shape function. The cell includes a mid-edge node for two
 * of the four edges. The ordering of the six points defining
 * the cell are point ids (0-3,4-5) where ids 0-3 define the four corner
 * vertices of the quad; ids 4-7 define the midedge nodes (0,1) and (2,3) .
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra vtkQuadraticQuad
 * vtkQuadraticHexahedron vtkQuadraticWedge vtkQuadraticPyramid
 *
 * @par Thanks:
 * Thanks to Soeren Gebbert  who developed this class and
 * integrated it into VTK 5.0.
 */

#ifndef vtkQuadraticLinearQuad_h
#define vtkQuadraticLinearQuad_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkQuadraticEdge;
class vtkLine;
class vtkQuad;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticLinearQuad : public vtkNonLinearCell
{
public:
  static vtkQuadraticLinearQuad* New();
  vtkTypeMacro(vtkQuadraticLinearQuad, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() override { return VTK_QUADRATIC_LINEAR_QUAD; }
  int GetCellDimension() override { return 2; }
  int GetNumberOfEdges() override { return 4; }
  int GetNumberOfFaces() override { return 0; }
  vtkCell* GetEdge(int) override;
  vtkCell* GetFace(int) override { return nullptr; }
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
   * Clip this quadratic linear quad using scalar value provided. Like
   * contouring, except that it cuts the quad to produce linear triangles.
   */
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;

  /**
   * Line-edge intersection. Intersection has to occur within [0,1] parametric
   * coordinates and with specified tolerance.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;

  /**
   * Return the center of the pyramid in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  static void InterpolationFunctions(const double pcoords[3], double weights[6]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[12]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[6]) override
  {
    vtkQuadraticLinearQuad::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[12]) override
  {
    vtkQuadraticLinearQuad::InterpolationDerivs(pcoords, derivs);
  }
  ///@}
  /**
   * Return the ids of the vertices defining edge (`edgeId`).
   * Ids are related to the cell, not to the dataset.
   *
   * @note The return type changed. It used to be int*, it is now const vtkIdType*.
   * This is so ids are unified between vtkCell and vtkPoints.
   *
   * @note The return type changed. It used to be int*, it is now const vtkIdType*.
   * This is so ids are unified between vtkCell and vtkPoints.
   */
  static int* GetEdgeArray(vtkIdType edgeId);

protected:
  vtkQuadraticLinearQuad();
  ~vtkQuadraticLinearQuad() override;

  vtkQuadraticEdge* Edge;
  vtkLine* LinEdge;
  vtkQuad* Quad;
  vtkDoubleArray* Scalars;

private:
  vtkQuadraticLinearQuad(const vtkQuadraticLinearQuad&) = delete;
  void operator=(const vtkQuadraticLinearQuad&) = delete;
};
//----------------------------------------------------------------------------
inline int vtkQuadraticLinearQuad::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.;
  return 0;
}

VTK_ABI_NAMESPACE_END
#endif
