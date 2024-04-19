// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVoxel
 * @brief   a cell that represents a 3D orthogonal parallelepiped
 *
 * vtkVoxel is a concrete implementation of vtkCell to represent a 3D
 * orthogonal parallelepiped. Unlike vtkHexahedron, vtkVoxel has interior
 * angles of 90 degrees, and sides are parallel to coordinate axes. This
 * results in large increases in computational performance.
 *
 * @sa
 * vtkConvexPointSet vtkHexahedron vtkPyramid vtkTetra vtkWedge
 */

#ifndef vtkVoxel_h
#define vtkVoxel_h

#include "vtkCell3D.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkLine;
class vtkPixel;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkVoxel : public vtkCell3D
{
public:
  static vtkVoxel* New();
  vtkTypeMacro(vtkVoxel, vtkCell3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * See vtkCell3D API for description of these methods.
   * @warning Face points of vtkVoxel are not sorted properly.
   * {pts[0], pts[1], pts[3], pts[2]} forms consecutive points of one face.
   */
  void GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts) override;
  vtkIdType GetFacePoints(vtkIdType faceId, const vtkIdType*& pts) override;
  void GetEdgeToAdjacentFaces(vtkIdType edgeId, const vtkIdType*& pts) override;
  vtkIdType GetFaceToAdjacentFaces(vtkIdType faceId, const vtkIdType*& faces) override;
  vtkIdType GetPointToIncidentEdges(vtkIdType pointId, const vtkIdType*& edges) override;
  vtkIdType GetPointToIncidentFaces(vtkIdType pointId, const vtkIdType*& faces) override;
  vtkIdType GetPointToOneRingPoints(vtkIdType pointId, const vtkIdType*& pts) override;
  double* GetParametricCoords() override;
  bool GetCentroid(double centroid[3]) const override;
  bool IsInsideOut() override;
  ///@}

  /**
   * Computes exact bounding sphere of this voxel.
   */
  double ComputeBoundingSphere(double center[3]) const override;

  /**
   * static constexpr handle on the number of points.
   */
  static constexpr vtkIdType NumberOfPoints = 8;

  /**
   * static contexpr handle on the number of faces.
   */
  static constexpr vtkIdType NumberOfEdges = 12;

  /**
   * static contexpr handle on the number of edges.
   */
  static constexpr vtkIdType NumberOfFaces = 6;

  /**
   * static contexpr handle on the maximum face size. It can also be used
   * to know the number of faces adjacent to one face.
   */
  static constexpr vtkIdType MaximumFaceSize = 4;

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
  int GetCellType() override { return VTK_VOXEL; }
  int GetCellDimension() override { return 3; }
  int GetNumberOfEdges() override { return 12; }
  int GetNumberOfFaces() override { return 6; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int faceId) override;
  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;
  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;
  int EvaluatePosition(const double x[3], double closestPoint[3], int& subId, double pcoords[3],
    double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  ///@}

  /**
   * Inflates voxel by moving every faces by dist. Since normals are not
   * ambiguous for degenerate voxels, degenerate voxels are inflated correctly.
   * For example, inflating a voxel collapsed to a single point will produce a
   * voxel of width 2 * dist.
   *
   * \return 1
   */
  int Inflate(double dist) override;

  static void InterpolationDerivs(const double pcoords[3], double derivs[24]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[8]) override
  {
    vtkVoxel::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[24]) override
  {
    vtkVoxel::InterpolationDerivs(pcoords, derivs);
  }
  ///@}

  /**
   * Compute the interpolation functions.
   * This static method is for convenience. Use the member function
   * if you already have an instance of a voxel.
   */
  static void InterpolationFunctions(const double pcoords[3], double weights[8]);

  /**
   * Return the case table for table-based isocontouring (aka marching cubes
   * style implementations). A linear 3D cell with N vertices will have 2**N
   * cases. The returned case array lists three edges in order to produce one
   * output triangle which may be repeated to generate multiple triangles. The
   * list of cases terminates with a -1 entry.
   */
  static int* GetTriangleCases(int caseId);

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
  static const vtkIdType* GetFaceArray(vtkIdType faceId) VTK_SIZEHINT(4);
  ///@}

  /**
   * Static method version of GetEdgeToAdjacentFaces.
   */
  static const vtkIdType* GetEdgeToAdjacentFacesArray(vtkIdType edgeId) VTK_SIZEHINT(2);

  /**
   * Static method version of GetFaceToAdjacentFaces.
   */
  static const vtkIdType* GetFaceToAdjacentFacesArray(vtkIdType faceId) VTK_SIZEHINT(4);

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
  vtkVoxel();
  ~vtkVoxel() override;

private:
  vtkVoxel(const vtkVoxel&) = delete;
  void operator=(const vtkVoxel&) = delete;

  vtkLine* Line;
  vtkPixel* Pixel;
};

VTK_ABI_NAMESPACE_END
#endif
