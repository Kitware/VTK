// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPixel
 * @brief   a cell that represents an orthogonal quadrilateral
 *
 * vtkPixel is a concrete implementation of vtkCell to represent a 2D
 * orthogonal quadrilateral. Unlike vtkQuad, the corners are at right angles,
 * and aligned along x-y-z coordinate axes leading to large increases in
 * computational efficiency.
 */

#ifndef vtkPixel_h
#define vtkPixel_h

#include "vtkCell.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkLine;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkPixel : public vtkCell
{
public:
  static vtkPixel* New();
  vtkTypeMacro(vtkPixel, vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() override { return VTK_PIXEL; }
  int GetCellDimension() override { return 2; }
  int GetNumberOfEdges() override { return 4; }
  int GetNumberOfFaces() override { return 0; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int) override { return nullptr; }
  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;
  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;
  int EvaluatePosition(const double x[3], double closestPoint[3], int& subId, double pcoords[3],
    double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  ///@}

  /**
   * Inflates this pixel by a distance of dist by moving the edges of the pixel
   * by that distance. Since a pixel lies in 3D, the degenerate case where the
   * pixel is homogeneous to a line are discarted because of normal direction
   * ambiguity. Hence, if you shrink a 2D pixel so it loses thickness in one
   * dimension. inflating it back to its previous form is impossible.
   *
   * A degenerate pixel of dimension 1 is inflated the same way a segment would be
   * inflated. A degenerate pixel of dimension 0 is untouched.
   *
   * \return 1
   */
  int Inflate(double dist) override;

  /**
   * Computes exact bounding sphere of this pixel.
   */
  double ComputeBoundingSphere(double center[3]) const override;

  /**
   * Return the center of the triangle in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  double* GetParametricCoords() override;

  static void InterpolationFunctions(const double pcoords[3], double weights[4]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[8]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[4]) override
  {
    vtkPixel::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[8]) override
  {
    vtkPixel::InterpolationDerivs(pcoords, derivs);
  }
  ///@}

  /**
   * vtkPixel's normal cannot be computed using vtkPolygon::ComputeNormal because
   * its points are not sorted such that circulating on them forms the pixel.
   * This is a convenient method so one can compute normals on a pixel.
   */
  int ComputeNormal(double n[3]);

protected:
  vtkPixel();
  ~vtkPixel() override;

  vtkLine* Line;

private:
  vtkPixel(const vtkPixel&) = delete;
  void operator=(const vtkPixel&) = delete;
};

//----------------------------------------------------------------------------
inline int vtkPixel::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.0;
  return 0;
}

VTK_ABI_NAMESPACE_END
#endif
