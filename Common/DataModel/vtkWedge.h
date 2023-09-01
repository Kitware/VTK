// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWedge
 * @brief   a 3D cell that represents a linear wedge
 *
 * vtkWedge is a concrete implementation of vtkCell to represent a linear 3D
 * wedge. A wedge consists of two triangular and three quadrilateral faces
 * and is defined by the six points (0-5). vtkWedge uses the standard
 * isoparametric shape functions for a linear wedge. The wedge is defined
 * by the six points (0-5) where (0,1,2) is the base of the wedge which,
 * using the right hand rule, forms a triangle whose normal points outward
 * (away from the triangular face (3,4,5)).
 *
 * @sa
 * vtkConvexPointSet vtkHexahedron vtkPyramid vtkTetra vtkVoxel
 */

#ifndef vtkWedge_h
#define vtkWedge_h

#include "vtkCell3D.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkLine;
class vtkTriangle;
class vtkQuad;
class vtkUnstructuredGrid;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkWedge : public vtkCell3D
{
public:
  static vtkWedge* New();
  vtkTypeMacro(vtkWedge, vtkCell3D);
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
  static constexpr vtkIdType NumberOfPoints = 6;

  /**
   * static contexpr handle on the number of faces.
   */
  static constexpr vtkIdType NumberOfEdges = 9;

  /**
   * static contexpr handle on the number of edges.
   */
  static constexpr vtkIdType NumberOfFaces = 5;

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
  int GetCellType() override { return VTK_WEDGE; }
  int GetCellDimension() override { return 3; }
  int GetNumberOfEdges() override { return static_cast<int>(NumberOfEdges); }
  int GetNumberOfFaces() override { return static_cast<int>(NumberOfFaces); }
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
   * Return the center of the wedge in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  static void InterpolationFunctions(const double pcoords[3], double weights[6]);
  static void InterpolationDerivs(const double pcoords[3], double derivs[18]);
  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[6]) override
  {
    vtkWedge::InterpolationFunctions(pcoords, weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[18]) override
  {
    vtkWedge::InterpolationDerivs(pcoords, derivs);
  }
  int JacobianInverse(const double pcoords[3], double** inverse, double derivs[18]);
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
  static const vtkIdType* GetFaceArray(vtkIdType faceId) VTK_SIZEHINT(MaximumFaceSize + 1);
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
  vtkWedge();
  ~vtkWedge() override;

  vtkLine* Line;
  vtkTriangle* Triangle;
  vtkQuad* Quad;

private:
  vtkWedge(const vtkWedge&) = delete;
  void operator=(const vtkWedge&) = delete;
};

inline int vtkWedge::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.333333;
  pcoords[2] = 0.5;
  return 0;
}

VTK_ABI_NAMESPACE_END
#endif
