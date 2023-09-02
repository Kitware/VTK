// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVertex
 * @brief   a cell that represents a 3D point
 *
 * vtkVertex is a concrete implementation of vtkCell to represent a 3D point.
 */

#ifndef vtkVertex_h
#define vtkVertex_h

#include "vtkCell.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkVertex : public vtkCell
{
public:
  static vtkVertex* New();
  vtkTypeMacro(vtkVertex, vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make a new vtkVertex object with the same information as this object.
   */

  ///@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() override { return VTK_VERTEX; }
  int GetCellDimension() override { return 0; }
  int GetNumberOfEdges() override { return 0; }
  int GetNumberOfFaces() override { return 0; }
  vtkCell* GetEdge(int) override { return nullptr; }
  vtkCell* GetFace(int) override { return nullptr; }
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* pts, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
    vtkCellData* outCd, int insideOut) override;
  int EvaluatePosition(const double x[3], double closestPoint[3], int& subId, double pcoords[3],
    double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  double* GetParametricCoords() override;
  ///@}

  /**
   * This method does nothing.
   *
   * \return 1 if inflation was successful, 0 if no inflation was performed
   */
  int Inflate(double) override { return 0; }

  /**
   * Given parametric coordinates of a point, return the closest cell
   * boundary, and whether the point is inside or outside of the cell. The
   * cell boundary is defined by a list of points (pts) that specify a vertex
   * (1D cell).  If the return value of the method is != 0, then the point is
   * inside the cell.
   */
  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;

  /**
   * Generate contouring primitives. The scalar list cellScalars are
   * scalar values at each cell point. The point locator is essentially a
   * points list that merges points as they are inserted (i.e., prevents
   * duplicates).
   */
  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts1, vtkCellArray* lines, vtkCellArray* verts2, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;

  /**
   * Return the center of the triangle in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * Intersect with a ray. Return parametric coordinates (both line and cell)
   * and global intersection coordinates, given ray definition and tolerance.
   * The method returns non-zero value if intersection occurs.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;

  /**
   * Triangulate the vertex. This method fills pts and ptIds with information
   * from the only point in the vertex.
   */
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;

  /**
   * Get the derivative of the vertex. Returns (0.0, 0.0, 0.0) for all
   * dimensions.
   */
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;

  static void InterpolationFunctions(const double pcoords[3], double weights[1]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[3]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[1]) override
  {
    vtkVertex::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[3]) override
  {
    vtkVertex::InterpolationDerivs(pcoords, derivs);
  }
  ///@}

protected:
  vtkVertex();
  ~vtkVertex() override = default;

private:
  vtkVertex(const vtkVertex&) = delete;
  void operator=(const vtkVertex&) = delete;
};

//----------------------------------------------------------------------------
inline int vtkVertex::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  return 0;
}

VTK_ABI_NAMESPACE_END
#endif
