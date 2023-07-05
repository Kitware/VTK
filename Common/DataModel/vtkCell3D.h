// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCell3D
 * @brief   abstract class to specify 3D cell interface
 *
 * vtkCell3D is an abstract class that extends the interfaces for 3D data
 * cells, and implements methods needed to satisfy the vtkCell API. The
 * 3D cells include hexehedra, tetrahedra, wedge, pyramid, and voxel.
 *
 * @sa
 * vtkTetra vtkHexahedron vtkVoxel vtkWedge vtkPyramid
 */

#ifndef vtkCell3D_h
#define vtkCell3D_h

#include "vtkCell.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOrderedTriangulator;
class vtkTetra;
class vtkCellArray;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkCell3D : public vtkCell
{
public:
  vtkTypeMacro(vtkCell3D, vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the pair of vertices that define an edge. The method returns the
   * number of vertices, along with an array of vertices. Note that the
   * vertices are 0-offset; that is, they refer to the ids of the cell, not
   * the point ids of the mesh that the cell belongs to. The edgeId must
   * range between 0<=edgeId<this->GetNumberOfEdges().
   */
  virtual void GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts) VTK_SIZEHINT(pts, 2)
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
   * @warning If the vtkCell3D is "inside out", i.e. normals point inside the cell, the order is
   * inverted.
   * @return The number of adjacent faces to faceId.
   */
  virtual vtkIdType GetFaceToAdjacentFaces(vtkIdType faceId, const vtkIdType*& faceIds)
    VTK_SIZEHINT(faceIds, _) VTK_EXPECTS(0 <= faceId && faceId < GetNumberOfFaces()) = 0;

  /**
   * Get the ids of the incident edges to point of id pointId. Edges are
   * sorted in counter clockwise order w.r.t. bisectrix pointing outside the cell
   * at point of id pointId.
   * The first edge corresponds to the edge containing point of id pts[0], where
   * pts is obtained from this->GetPointToOnRingVertices(pointId, pts).
   * Note that the edges are 0-offset; that is, they refer to the ids of the cell,
   * not the edge ids of the mesh that the cell belongs to.
   * The edgeId must be between 0<=edgeId<this->GetNumberOfEdges();
   *
   * @warning If the vtkCell3D is "inside out", i.e. normals point inside the cell, the order is
   * inverted.
   * @return The valence of point pointId.
   */
  virtual vtkIdType GetPointToIncidentEdges(vtkIdType pointId, const vtkIdType*& edgeIds)
    VTK_SIZEHINT(edgeIds, _) VTK_EXPECTS(0 <= pointId && pointId < GetNumberOfPoints()) = 0;

  /**
   * Get the ids of the incident faces point of id pointId. Faces are
   * sorted in counter clockwise order w.r.t. bisectrix pointing outside the cell
   * at point of id pointId.
   * The first face corresponds to the face containing edge of id edges[0],
   * where edges is obtained from this->GetPointToIncidentEdges(pointId, edges),
   * such that face faces[0] is the "most counterclockwise" face incident to
   * point pointId containing edges[0].
   * Note that the faces are 0-offset; that is, they refer to the ids of the cell,
   * not the face ids of the mesh that the cell belongs to.
   * The pointId must be between 0<=pointId<this->GetNumberOfPoints().
   *
   * @warning If the vtkCell3D is "inside out", i.e. normals point inside the cell, the order is
   * inverted.
   * @return The valence of point pointId.
   */
  virtual vtkIdType GetPointToIncidentFaces(vtkIdType pointId, const vtkIdType*& faceIds)
    VTK_SIZEHINT(faceIds, _) VTK_EXPECTS(0 <= pointId && pointId < GetNumberOfPoints()) = 0;

  /**
   * Get the ids of a one-ring surrounding point of id pointId. Points are
   * sorted in counter clockwise order w.r.t. bisectrix pointing outside the cell
   * at point of id pointId.
   * The first point corresponds to the point contained in edges[0], where
   * edges is obtained from this->GetPointToIncidentEdges(pointId, edges).
   * Note that the points are 0-offset; that is, they refer to the ids of the cell,
   * not the point ids of the mesh that the cell belongs to.
   * The pointId must be between 0<pointId<this->GetNumberOfPoints().
   * @return The valence of point pointId.
   */
  virtual vtkIdType GetPointToOneRingPoints(vtkIdType pointId, const vtkIdType*& pts)
    VTK_SIZEHINT(pts, _) VTK_EXPECTS(0 <= pointId && pointId < GetNumberOfPoints()) = 0;

  /**
   * Returns true if the normals of the vtkCell3D point inside the cell.
   *
   * @warning This flag is not precomputed. It is advised for the return result of
   * this method to be stored in a local boolean by the user if needed multiple times.
   */
  virtual bool IsInsideOut();

  /**
   * Computes the centroid of the cell.
   */
  virtual bool GetCentroid(double centroid[3]) const = 0;

  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;

  /**
   * Cut (or clip) the cell based on the input cellScalars and the specified
   * value. The output of the clip operation will be one or more cells of the
   * same topological dimension as the original cell.  The flag insideOut
   * controls what part of the cell is considered inside - normally cell
   * points whose scalar value is greater than "value" are considered
   * inside. If insideOut is on, this is reversed. Also, if the output cell
   * data is non-nullptr, the cell data from the clipped cell is passed to the
   * generated contouring primitives. (Note: the CopyAllocate() method must
   * be invoked on both the output cell and point data. The cellId refers to
   * the cell from which the cell data is copied.)  (Satisfies vtkCell API.)
   */
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* connectivity, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;

  /**
   * The topological dimension of the cell. (Satisfies vtkCell API.)
   */
  int GetCellDimension() override { return 3; }

  /**
   * Inflates the cell. Each face is displaced following its normal by a
   * distance of value `dist`. If dist is negative, then the cell shrinks.
   * The resulting cell edges / faces are colinear / coplanar to their previous
   * self.
   *
   * Degenerate parts of the 3D cell are unchanged. This happens a points to
   * which incident faces are homogeneous to a plane, to a line, or to a point.
   *
   * \return 1 if inflation was successful, 0 if no inflation was performed
   */
  int Inflate(double dist) override;

  ///@{
  /**
   * Set the tolerance for merging clip intersection points that are near
   * the vertices of cells. This tolerance is used to prevent the generation
   * of degenerate tetrahedra during clipping.
   */
  vtkSetClampMacro(MergeTolerance, double, 0.0001, 0.25);
  vtkGetMacro(MergeTolerance, double);
  ///@}

protected:
  vtkCell3D();
  ~vtkCell3D() override;

  vtkOrderedTriangulator* Triangulator;
  double MergeTolerance;

  // used to support clipping
  vtkTetra* ClipTetra;
  vtkDoubleArray* ClipScalars;

private:
  vtkCell3D(const vtkCell3D&) = delete;
  void operator=(const vtkCell3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
