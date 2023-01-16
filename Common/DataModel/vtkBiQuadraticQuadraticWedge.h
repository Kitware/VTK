/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBiQuadraticQuadraticWedge.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBiQuadraticQuadraticWedge
 * @brief   cell represents a parabolic, 18-node isoparametric wedge
 *
 * vtkBiQuadraticQuadraticWedge is a concrete implementation of vtkNonLinearCell to
 * represent a three-dimensional, 18-node isoparametric biquadratic
 * wedge. The interpolation is the standard finite element,
 * biquadratic-quadratic isoparametric shape function plus the linear functions.
 * The cell includes a mid-edge node. The
 * ordering of the 18 points defining the cell is point ids (0-5,6-15, 16-18)
 * where point ids 0-5 are the six corner vertices of the wedge; followed by
 * nine midedge nodes (6-15) and 3 center-face nodes. Note that these midedge
 * nodes correspond lie
 * on the edges defined by (0,1), (1,2), (2,0), (3,4), (4,5), (5,3), (0,3),
 * (1,4), (2,5), and the center-face nodes are laying in quads 16-(0,1,4,3),
 * 17-(1,2,5,4) and (2,0,3,5).
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
 * vtkQuadraticHexahedron vtkQuadraticQuad vtkQuadraticPyramid
 *
 * @par Thanks:
 * Thanks to Soeren Gebbert who developed this class and
 * integrated it into VTK 5.0.
 */

#ifndef vtkBiQuadraticQuadraticWedge_h
#define vtkBiQuadraticQuadraticWedge_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkQuadraticEdge;
class vtkBiQuadraticQuad;
class vtkQuadraticTriangle;
class vtkWedge;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkBiQuadraticQuadraticWedge : public vtkNonLinearCell
{
public:
  static vtkBiQuadraticQuadraticWedge* New();
  vtkTypeMacro(vtkBiQuadraticQuadraticWedge, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() override { return VTK_BIQUADRATIC_QUADRATIC_WEDGE; }
  int GetCellDimension() override { return 3; }
  int GetNumberOfEdges() override { return 9; }
  int GetNumberOfFaces() override { return 5; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  ///@}

  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;
  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;
  int EvaluatePosition(const double x[3], double* closestPoint, int& subId, double pcoords[3],
    double& dist2, double* weights) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  int Triangulate(int index, vtkIdList* ptIds, vtkPoints* pts) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  double* GetParametricCoords() override;

  /**
   * Clip this quadratic Wedge using scalar value provided. Like
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
   * Return the center of the quadratic wedge in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  static void InterpolationFunctions(const double pcoords[3], double weights[18]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[54]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[18]) override
  {
    vtkBiQuadraticQuadraticWedge::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[54]) override
  {
    vtkBiQuadraticQuadraticWedge::InterpolationDerivs(pcoords, derivs);
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
  void JacobianInverse(const double pcoords[3], double** inverse, double derivs[54]);

protected:
  vtkBiQuadraticQuadraticWedge();
  ~vtkBiQuadraticQuadraticWedge() override;

  vtkQuadraticEdge* Edge;
  vtkQuadraticTriangle* TriangleFace;
  vtkBiQuadraticQuad* Face;
  vtkWedge* Wedge;
  vtkDoubleArray* Scalars; // used to avoid New/Delete in contouring/clipping

private:
  vtkBiQuadraticQuadraticWedge(const vtkBiQuadraticQuadraticWedge&) = delete;
  void operator=(const vtkBiQuadraticQuadraticWedge&) = delete;
};
//----------------------------------------------------------------------------
// Return the center of the quadratic wedge in parametric coordinates.
inline int vtkBiQuadraticQuadraticWedge::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 1. / 3;
  pcoords[2] = 0.5;
  return 0;
}

VTK_ABI_NAMESPACE_END
#endif
