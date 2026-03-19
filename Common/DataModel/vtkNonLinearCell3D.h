// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkNonLinearCell3D
 * @brief   abstract superclass for non-linear 3d cells
 *
 * vtkNonLinearCell3D is an abstract superclass for cells of fixed order (i.e., not varying
 * cell by cell) that have nominal order along any axis > 1. Thus, "multi-linear" cells such as
 * vtkTetra and vtkHexahedron are not included, but cells that may have interior critical points
 * (minima, maxima, or inflection points) along a single parameter-space axis are included.
 */

#ifndef vtkNonLinearCell3D_h
#define vtkNonLinearCell3D_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkNonLinearCell3D : public vtkNonLinearCell
{
public:
  vtkTypeMacro(vtkNonLinearCell3D, vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellDimension() override { return 3; }

  /**
   * Describes the topological role of a point within a non-linear cell.
   * - CornerPoint:   a vertex of the cell's underlying linear topology. It is
   *                  shared by 3 or more faces and belongs to 3 or more edges.
   * - EdgeMidPoint:  a point sitting on an edge between two corners. It belongs
   *                  to exactly 2 faces and exactly 1 edge.
   * - FaceMidPoint:  a point sitting on the interior of a face. It belongs to
   *                  exactly 1 face and no edges.
   * - CenterPoint:   a point sitting in the interior of the cell volume. It
   *                  belongs to no face and no edge.
   */
  enum PointType
  {
    CornerPoint,
    EdgeMidPoint,
    FaceMidPoint,
    CenterPoint,
  };

  /**
   * Get the topological type of point of id pointId within this cell.
   * This determines which topology API calls are meaningful for this point:
   * - CornerPoint and EdgeMidPoint have incident edges and a non-empty one-ring.
   * - CornerPoint, EdgeMidPoint, and FaceMidPoint have incident faces.
   * - CenterPoint has no incident edges, faces, or one-ring neighbors.
   *
   * @return The PointType of point pointId.
   */
  virtual PointType GetPointType(vtkIdType pointId)
    VTK_EXPECTS(0 <= pointId && pointId < GetNumberOfPoints()) = 0;

  /**
   * Get the vertices that define an edge. The method returns the
   * number of vertices, along with an array of vertices. Note that the
   * vertices are 0-offset; that is, they refer to the ids of the cell, not
   * the point ids of the mesh that the cell belongs to. The edgeId must
   * range between 0<=edgeId<this->GetNumberOfEdges().
   *
   * @return The number of points in edge edgeId
   */
  virtual vtkIdType GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts) VTK_SIZEHINT(pts, _)
    VTK_EXPECTS(0 <= edgeId && edgeId < GetNumberOfEdges()) = 0;

  /**
   * Get the list of vertices that define a face. The list is terminated
   * with a negative number. Note that the vertices are 0-offset; that is,
   * they refer to the ids of the cell, not the point ids of the mesh that
   * the cell belongs to. The faceId must range between
   * 0<=faceId<this->GetNumberOfFaces().
   *
   * @return The number of points in face faceId
   */
  virtual vtkIdType GetFacePoints(vtkIdType faceId, const vtkIdType*& pts) VTK_SIZEHINT(pts, _)
    VTK_EXPECTS(0 <= faceId && faceId < GetNumberOfFaces()) = 0;

  /**
   * Get the ids of the two adjacent faces to edge of id edgeId.
   * The output face ids are sorted from id of lowest rank to highest.
   * Note that the faces are 0-offset; that is, they refer to the ids of the cells,
   * not the face ids of the mesh that the cell belongs to. The edgeId must range
   * between 0<=edgeId<this->GetNumberOfEdges().
   */
  virtual void GetEdgeToAdjacentFaces(vtkIdType edgeId, const vtkIdType*& faceIds)
    VTK_SIZEHINT(faceIds, 2) VTK_EXPECTS(0 <= edgeId && edgeId < GetNumberOfEdges()) = 0;

  /**
   * Get the ids of the adjacent faces to face of id faceId. The order of
   * faces is consistent. They are always ordered in counter clockwise w.r.t.
   * normal orientation.
   * The first id faces[0] corresponds to the face sharing point of id pts[0]
   * where pts is obtained from this->GetFacePoints(faceId, pts), being
   * the "most counter clockwise" oriented w.r.t. face faceId.
   * Note that the faces are 0-offset; that is, they
   * refer to the ids of the cell, not the face ids of the mesh that the cell belongs to.
   * The faceId must be between 0<=faceId<this->GetNumberOfFaces();
   *
   * @warning If the vtkNonLinearCell3D is "inside out", i.e. normals point inside the cell, the
   * order is inverted.
   * @return The number of adjacent faces to faceId.
   */
  virtual vtkIdType GetFaceToAdjacentFaces(vtkIdType faceId, const vtkIdType*& faceIds)
    VTK_SIZEHINT(faceIds, _) VTK_EXPECTS(0 <= faceId && faceId < GetNumberOfFaces()) = 0;

  /**
   * Get the ids of the edges incident to point of id pointId. Edges are
   * sorted in counter clockwise order w.r.t. the bisectrix pointing outside the
   * cell at point of id pointId.
   * The first edge corresponds to the edge containing point of id pts[0], where
   * pts is obtained from this->GetPointToOneRingPoints(pointId, pts).
   * Note that the edges are 0-offset; that is, they refer to the ids of the cell,
   * not the edge ids of the mesh that the cell belongs to.
   *
   * Only corner and mid-edge points have incident edges. Mid-face and center
   * points have no incident edges, so the returned list is empty and 0 is returned.
   *
   * @warning If the vtkNonLinearCell3D is "inside out", i.e. normals point inside
   * the cell, the order is inverted.
   * @return The number of incident edges (valence) of point pointId.
   */
  virtual vtkIdType GetPointToIncidentEdges(vtkIdType pointId, const vtkIdType*& edgeIds)
    VTK_SIZEHINT(edgeIds, _) VTK_EXPECTS(0 <= pointId && pointId < GetNumberOfPoints()) = 0;

  /**
   * Get the ids of the faces incident to point of id pointId. Faces are
   * sorted in counter clockwise order w.r.t. the bisectrix pointing outside the
   * cell at point of id pointId.
   * The first face corresponds to the face containing edge of id edges[0],
   * where edges is obtained from this->GetPointToIncidentEdges(pointId, edges),
   * such that faces[0] is the "most counterclockwise" face incident to pointId
   * containing edges[0].
   * Note that the faces are 0-offset; that is, they refer to the ids of the cell,
   * not the face ids of the mesh that the cell belongs to.
   *
   * All point types can have incident faces:
   * - Corner points belong to 3 or more faces.
   * - Mid-edge points belong to exactly 2 faces (the two faces sharing that edge).
   * - Mid-face points belong to exactly 1 face (the face they sit on).
   * - Center points belong to no face; the returned list is empty and 0 is returned.
   *
   * @warning If the vtkNonLinearCell3D is "inside out", i.e. normals point inside
   * the cell, the order is inverted.
   * @return The number of incident faces of point pointId.
   */
  virtual vtkIdType GetPointToIncidentFaces(vtkIdType pointId, const vtkIdType*& faceIds)
    VTK_SIZEHINT(faceIds, _) VTK_EXPECTS(0 <= pointId && pointId < GetNumberOfPoints()) = 0;

  /**
   * Get the ids of the one-ring neighbors of point of id pointId. Points are
   * sorted in counter clockwise order w.r.t. the bisectrix pointing outside the
   * cell at point of id pointId.
   * The first point corresponds to the point contained in edges[0], where
   * edges is obtained from this->GetPointToIncidentEdges(pointId, edges).
   * Note that the points are 0-offset; that is, they refer to the ids of the cell,
   * not the point ids of the mesh that the cell belongs to.
   *
   * The one-ring contains only points reachable via edges from pointId:
   * - Corner points: alternating corner neighbor and mid-edge point around the ring.
   * - Mid-edge points: the two corner endpoints of their edge.
   * - Mid-face and center points: no incident edges, so the one-ring is empty
   *   and 0 is returned.
   *
   * @return The number of one-ring neighbors of point pointId.
   */
  virtual vtkIdType GetPointToOneRingPoints(vtkIdType pointId, const vtkIdType*& pts)
    VTK_SIZEHINT(pts, _) VTK_EXPECTS(0 <= pointId && pointId < GetNumberOfPoints()) = 0;

protected:
  vtkNonLinearCell3D() = default;
  ~vtkNonLinearCell3D() override = default;

private:
  vtkNonLinearCell3D(const vtkNonLinearCell3D&) = delete;
  void operator=(const vtkNonLinearCell3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkNonLinearCell3D_h
