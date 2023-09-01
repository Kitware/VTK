// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTetra
 * @brief   a 3D cell that represents a tetrahedron
 *
 * vtkTetra is a concrete implementation of vtkCell to represent a 3D
 * tetrahedron. vtkTetra uses the standard isoparametric shape functions
 * for a linear tetrahedron. The tetrahedron is defined by the four points
 * (0-3); where (0,1,2) is the base of the tetrahedron which, using the
 * right hand rule, forms a triangle whose normal points in the direction
 * of the fourth point.
 *
 * @sa
 * vtkConvexPointSet vtkHexahedron vtkPyramid vtkVoxel vtkWedge
 */

#ifndef vtkTetra_h
#define vtkTetra_h

#include "vtkCell3D.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkLine;
class vtkTriangle;
class vtkUnstructuredGrid;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkTetra : public vtkCell3D
{
public:
  static vtkTetra* New();
  vtkTypeMacro(vtkTetra, vtkCell3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * See vtkCell3D API for description of these methods.
   */
  void GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts) override;
  vtkIdType GetFacePoints(vtkIdType faceId, const vtkIdType*& pts) override;
  void GetEdgeToAdjacentFaces(vtkIdType edgeId, const vtkIdType*& pts) override;
  vtkIdType GetFaceToAdjacentFaces(vtkIdType faceId, const vtkIdType*& faceIds) override;
  vtkIdType GetPointToIncidentEdges(vtkIdType pointId, const vtkIdType*& edgeIds) override;
  vtkIdType GetPointToIncidentFaces(vtkIdType pointId, const vtkIdType*& faceIds) override;
  vtkIdType GetPointToOneRingPoints(vtkIdType pointId, const vtkIdType*& pts) override;
  bool GetCentroid(double centroid[3]) const override;
  bool IsInsideOut() override;
  ///@}

  /**
   * static constexpr handle on the number of points.
   */
  static constexpr vtkIdType NumberOfPoints = 4;

  /**
   * static contexpr handle on the number of faces.
   */
  static constexpr vtkIdType NumberOfEdges = 6;

  /**
   * static contexpr handle on the number of edges.
   */
  static constexpr vtkIdType NumberOfFaces = 4;

  /**
   * static contexpr handle on the maximum face size. It can also be used
   * to know the number of faces adjacent to one face.
   */
  static constexpr vtkIdType MaximumFaceSize = 3;

  /**
   * static constexpr handle on the maximum valence of this cell.
   * The valence of a vertex is the number of incident edges (or equivalently faces)
   * to this vertex. It is also equal to the size of a one ring neighborhood of a vertex.
   */
  static constexpr vtkIdType MaximumValence = 3;

  ///@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() override { return VTK_TETRA; }
  int GetNumberOfEdges() override { return 6; }
  int GetNumberOfFaces() override { return 4; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* connectivity, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;
  int EvaluatePosition(const double x[3], double closestPoint[3], int& subId, double pcoords[3],
    double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  double* GetParametricCoords() override;
  ///@}

  /**
   * Return the case table for table-based isocontouring (aka marching cubes
   * style implementations). A linear 3D cell with N vertices will have 2**N
   * cases. The returned case array lists three edges in order to produce one
   * output triangle which may be repeated to generate multiple triangles. The
   * list of cases terminates with a -1 entry.
   */
  static int* GetTriangleCases(int caseId);

  /**
   * Returns the set of points that are on the boundary of the tetrahedron that
   * are closest parametrically to the point specified. This may include faces,
   * edges, or vertices.
   */
  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;

  /**
   * Return the center of the tetrahedron in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * Return the distance of the parametric coordinate provided to the
   * cell. If inside the cell, a distance of zero is returned.
   */
  double GetParametricDistance(const double pcoords[3]) override;

  /**
   * Compute the center of the tetrahedron,
   */
  static void TetraCenter(double p1[3], double p2[3], double p3[3], double p4[3], double center[3]);

  /**
   * Compute the circumcenter (center[3]) and radius squared (method
   * return value) of a tetrahedron defined by the four points x1, x2,
   * x3, and x4.
   */
  static double Circumsphere(
    double x1[3], double x2[3], double x3[3], double x4[3], double center[3]);

  /**
   * Compute the center (center[3]) and radius (method return value) of
   * a sphere that just fits inside the faces of a tetrahedron defined
   * by the four points x1, x2, x3, and x4.
   */
  static double Insphere(double p1[3], double p2[3], double p3[3], double p4[3], double center[3]);

  /**
   * Given a 3D point x[3], determine the barycentric coordinates of the point.
   * Barycentric coordinates are a natural coordinate system for simplices that
   * express a position as a linear combination of the vertices. For a
   * tetrahedron, there are four barycentric coordinates (because there are
   * four vertices), and the sum of the coordinates must equal 1. If a
   * point x is inside a simplex, then all four coordinates will be strictly
   * positive.  If three coordinates are zero (so the fourth =1), then the
   * point x is on a vertex. If two coordinates are zero, the point x is on an
   * edge (and so on). In this method, you must specify the vertex coordinates
   * x1->x4. Returns 0 if tetrahedron is degenerate.
   */
  static int BarycentricCoords(
    double x[3], double x1[3], double x2[3], double x3[3], double x4[3], double bcoords[4]);

  /**
   * Compute the volume of a tetrahedron defined by the four points
   * p1, p2, p3, and p4.
   */
  static double ComputeVolume(double p1[3], double p2[3], double p3[3], double p4[3]);

  /**
   * Given parametric coordinates compute inverse Jacobian transformation
   * matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
   * function derivatives. Returns 0 if no inverse exists.
   */
  int JacobianInverse(double** inverse, double derivs[12]);

  static void InterpolationFunctions(const double pcoords[3], double weights[4]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[12]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[4]) override
  {
    vtkTetra::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[12]) override
  {
    vtkTetra::InterpolationDerivs(pcoords, derivs);
  }
  ///@}

  ///@{
  /**
   * Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
   * Ids are related to the cell, not to the dataset.
   *
   * @note The return type changed. It used to be int*, it is now const vtkIdType*.
   * This is so ids are unified between vtkCell and vtkPoints, and so vtkCell ids
   * can be used as inputs in algorithms such as vtkPolygon::ComputeNormal.
   */
  static const vtkIdType* GetEdgeArray(vtkIdType edgeId) VTK_SIZEHINT(2);
  static const vtkIdType* GetFaceArray(vtkIdType faceId) VTK_SIZEHINT(3);
  ///@}

  /**
   * Static method version of GetEdgeToAdjacentFaces.
   */
  static const vtkIdType* GetEdgeToAdjacentFacesArray(vtkIdType edgeId) VTK_SIZEHINT(2);

  /**
   * Static method version of GetFaceToAdjacentFaces.
   */
  static const vtkIdType* GetFaceToAdjacentFacesArray(vtkIdType faceId) VTK_SIZEHINT(3);

  /**
   * Static method version of GetPointToIncidentEdgesArray.
   */
  static const vtkIdType* GetPointToIncidentEdgesArray(vtkIdType pointId) VTK_SIZEHINT(3);

  /**
   * Static method version of GetPointToIncidentFacesArray.
   */
  static const vtkIdType* GetPointToIncidentFacesArray(vtkIdType pointId) VTK_SIZEHINT(3);

  /**
   * Static method version of GetPointToOneRingPoints.
   */
  static const vtkIdType* GetPointToOneRingPointsArray(vtkIdType pointId) VTK_SIZEHINT(3);

  /**
   * Static method version of GetCentroid.
   */
  static bool ComputeCentroid(vtkPoints* points, const vtkIdType* pointIds, double centroid[3]);

protected:
  vtkTetra();
  ~vtkTetra() override;

  vtkLine* Line;
  vtkTriangle* Triangle;

private:
  vtkTetra(const vtkTetra&) = delete;
  void operator=(const vtkTetra&) = delete;
};

inline int vtkTetra::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.25;
  return 0;
}

VTK_ABI_NAMESPACE_END
#endif
