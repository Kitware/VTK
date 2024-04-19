// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridGeometry3DImpl
 * @brief vtkHyperTreeGridGeometry internal classes for 3D vtkHyperTreeGrid
 *
 * This class is an internal class used in by the vtkHyperTreeGridGeometry filter
 * to generate the HTG surface in the 3D case.
 */

#ifndef vtkHyperTreeGridGeometry3DImpl_h
#define vtkHyperTreeGridGeometry3DImpl_h

#include "vtkHyperTreeGridGeometryImpl.h"

#include <map> // For std::map

VTK_ABI_NAMESPACE_BEGIN

class vtkBitArray;
class vtkHyperTreeGridNonOrientedVonNeumannSuperCursor;

class vtkHyperTreeGridGeometry3DImpl : public vtkHyperTreeGridGeometryImpl
{
public:
  vtkHyperTreeGridGeometry3DImpl(bool mergePoints, vtkHyperTreeGrid* input, vtkPoints* outPoints,
    vtkCellArray* outCells, vtkDataSetAttributes* inCellDataAttributes,
    vtkDataSetAttributes* outCellDataAttributes, bool passThroughCellIds,
    const std::string& originalCellIdArrayName);

  ~vtkHyperTreeGridGeometry3DImpl() override;

  /**
   * Generate the external surface of the input vtkHyperTreeGrid.
   */
  void GenerateGeometry() override;

protected:
  /**
   * Recursively browse the input HTG in order to generate the output surface.
   * This method is called by GenerateGeometry.
   *
   * XXX: We need to determine a common interface for all cursors in order to
   * define RecursivelyProcessTree as virtual in upper classes.
   */
  void RecursivelyProcessTree(vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor,
    unsigned char coarseCellFacesToBeTreated = 0);

private:
  struct HTG3DPoint;

  /**
   * Generate the surface for a leaf cell if needed, taking account of the
   * presence of interface(s) in the cell.
   */
  void GenerateCellSurface(vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor,
    unsigned char coarseCellFacesToBeTreated, vtkIdType cellId);

  /**
   * Iteratively generate the face at faceId for the leaf cell at cellId.
   */
  void GenerateOneCellFace(std::vector<HTG3DPoint>& cellPoints,
    std::vector<std::pair<HTG3DPoint, HTG3DPoint>>& edgePoints, unsigned int faceId,
    vtkIdType cellId, const double* cellOrigin, const double* cellSize, unsigned int zOffset,
    unsigned int orientation,
    std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFaceA,
    std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFaceB);

  /**
   * This method compute the intermediate point(s) on the given edge.
   * These points describe the interface points on the edges of the cell faces.
   * They are contained in the edgePoints variable.
   * edgeId corresponds to the id of the edge we consider.
   * internalFaceA and internalFaceB are structures filled during successive calls
   * of ComputeEdge and represent a linked list of points describing the internal
   * faces (i.e. the interface faces).
   */
  void ComputeEdge(const HTG3DPoint& firstPoint, const HTG3DPoint& secondPoint,
    std::vector<std::pair<HTG3DPoint, HTG3DPoint>>& edgePoints, unsigned int edgeAxis,
    unsigned int edgeId,
    std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFaceA,
    std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFaceB,
    unsigned int& currentEdgePointA, unsigned int& currentEdgePointB);

  /**
   * Compute the coordinates of the intermediate point representing the intersection
   * between the interface and the edges of the current cell. The result is stored in
   * pointInter. This method return true if the interface corresponds exactly to the
   * edge (which is an edge case).
   */
  bool ComputeEdgeInterface(const HTG3DPoint& firstPoint, const HTG3DPoint& secondPoint,
    std::vector<std::pair<HTG3DPoint, HTG3DPoint>>& edgePoints, unsigned int edgeAxis,
    unsigned int edgeId, std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFace,
    HTG3DPoint& pointInter, unsigned int& edgePointId, bool isInterfaceA);

  /**
   * Construct the internal faces of the cells (cut inside the cell by the interface).
   * This face is described by the internalFace structure, which is a linked list of
   * intersection points. Each point index is mapped to the corresponding HTG3DPoint
   * and the next point index of the linkage.
   */
  void CompleteLinkage(std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFace,
    unsigned int edgePointId1, unsigned int edgePointId2);

  /**
   * Initialize the internalFace variable, containing representing an internal (interface) face
   * with a linked list. Associate an edge with it's associate intersection point, then map it to
   * the next one.
   */
  void SetInterfaceFace(unsigned int edgeId,
    std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFace, HTG3DPoint* point);

  /**
   * Return if the given point is inside the cell, taking account the presence of an interface.
   */
  bool IsInside(const HTG3DPoint& point);

  /**
   * Set the point coordinates.
   */
  void SetXYZ(HTG3DPoint& point, const double* coords);

  /**
   * Set the intersection point coordinates.
   */
  void SetIntersectXYZ(HTG3DPoint& point, const double* coords, bool isInterfaceA);

  /**
   * Helper methods used to insert new points into the output polydata
   * (constructed surface). The point will be inserted only if it it has not
   * already been. If a locator is set, this method will internally use it
   * during the point insertion.
   */
  vtkIdType InsertUniquePoint(HTG3DPoint& point);

  /**
   * Return true if the cell has a "valid" (coherent) interface.
   * - HasIterface is true,
   * - Intercepts[2] != 2,
   * - Normals is defined and not null.
   */
  bool GetHasInterface(vtkIdType cellId) const;

  /**
   * Branch factor of the input HTG, stored for quick access
   */
  int BranchFactor;

  /**
   * Retrieved from the input HTG.
   * Bit arrays indicating which HTG cells are marked as "pure".
   * Note that cells with "invalid" interfaces will also be considered as such.
   */
  vtkBitArray* InPureMaskArray;

  /**
   * Locator used to merge duplicated points during insertion.
   */
  vtkSmartPointer<vtkMergePoints> Locator;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGeometry3DImpl_h */
