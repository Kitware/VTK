// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkHyperTreeGridFeatureEdges
 * @brief Generates feature edges from an Hyper Tree Grid
 *
 * vtkHyperTreeGridFeatureEdges generates feature edges from an input vtkHyperTreeGrid.
 * The nature of feature edges in the case of HTGs depends on the dimension of the HTG:
 * - in 1D, it corresponds to the HTG geometry (HTG cells are already edges),
 * - in 2D, it corresponds to border edges (HTG cells are quads),
 * - in 3D, it corresponds to edges describing an angle in the HTG geometry
 *   (necessarily 90 degrees).
 *
 * Due to the nature of HTGs (T-Junctions), we cannot rely directly on the HTG geometry
 * to construct feature edges. This filter iterates on the HTG to generate them on the fly.
 *
 * @sa
 * vtkFeatureEdges vtkHyperTreeGrid vtkHyperTreeGridAlgorithm vtkHyperTreeGridGeometryFilter
 *
 * @par Thanks:
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridFeatureEdges_h
#define vtkHyperTreeGridFeatureEdges_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"
#include "vtkMergePoints.h"  // For vtkMergePoints
#include "vtkSmartPointer.h" // For vtkNew

VTK_ABI_NAMESPACE_BEGIN

class vtkCellArray;
class vtkDataObject;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedGeometryCursor;
class vtkHyperTreeGridNonOrientedMooreSuperCursor;
class vtkHyperTreeGridNonOrientedVonNeumannSuperCursor;
class vtkInformation;
class vtkPoints;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridFeatureEdges : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridFeatureEdges* New();
  vtkTypeMacro(vtkHyperTreeGridFeatureEdges, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Turn on/off merging of coincident points using a locator.
   * Note that when merging is on, points with different point attributes
   * (e.g., normals) are merged, which may cause rendering artifacts.
   */
  vtkSetMacro(MergePoints, bool);
  vtkGetMacro(MergePoints, bool);
  ///@}

protected:
  vtkHyperTreeGridFeatureEdges() = default;
  ~vtkHyperTreeGridFeatureEdges() override = default;

  /**
   * For this algorithm, the output is a vtkPolyData instance.
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to generate feature edges.
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * If true, uses a vtkMergePoints locator instance when inserting
   * new points to the output.
   */
  bool MergePoints = false;

private:
  vtkHyperTreeGridFeatureEdges(const vtkHyperTreeGridFeatureEdges&) = delete;
  void operator=(const vtkHyperTreeGridFeatureEdges&) = delete;

  ///@{
  /**
   * These methods are input points to generate feature edges from input the HTG.
   * A different cursor type will be used depending on the dimension of the HTG:
   * - In 1D, we just need to generate cells geometry
   * - In 2D, we need Von Neumann neighbors for each cell
   * - In 3D, we need Moore neighbors for each cell
   */
  void Process1DHTG(vtkHyperTreeGrid* input, vtkPoints* outPoints, vtkCellArray* outCells);
  void Process2DHTG(vtkHyperTreeGrid* input, vtkPoints* outPoints, vtkCellArray* outCells);
  void Process3DHTG(vtkHyperTreeGrid* input, vtkPoints* outPoints, vtkCellArray* outCells);
  ///@}

  ///@{
  /**
   * These recursive methods contain the logic used to generate the feature edges for a given cell.
   * Note that the time complexity increases significantely with the dimension of the HTG because
   * of the cursor type needed.
   */
  void RecursivelyProcess1DHTGTree(vtkHyperTreeGrid* input, vtkPoints* outPoints,
    vtkCellArray* outCells, vtkHyperTreeGridNonOrientedGeometryCursor* cursor);
  void RecursivelyProcess2DHTGTree(vtkHyperTreeGrid* input, vtkPoints* outPoints,
    vtkCellArray* outCells, vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor);
  void RecursivelyProcess3DHTGTree(vtkHyperTreeGrid* input, vtkPoints* outPoints,
    vtkCellArray* outCells, vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor);
  ///@}

  ///@{
  /**
   * These methods build cell points (geometry) of the input HTG cells.
   * These points are then used to construct edges.
   */
  vtkSmartPointer<vtkPoints> Build1DCellPoints(vtkHyperTreeGridNonOrientedGeometryCursor* cursor);
  vtkSmartPointer<vtkPoints> Build2DCellPoints(
    vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor);
  vtkSmartPointer<vtkPoints> Build3DCellPoints(vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor);
  ///@}

  /**
   * Return true if the edge at edgeId should be added for the given cell (cursor) in the 2D case.
   * For a cell in a 2D HTG, edges are shared with Von Neumann neighbors.
   * An edge will should be added if it forms a boundary. For a given level, there are 2
   * specific cases:
   * 1) A visible cell can create an edge by looking at masked/absent neighboring leaf cells of same
   * level
   * 2) A masked cell can create an edge by looking at visible cells of lower level (bigger),
   * because coarse cell are not treated in 1)
   */
  bool ShouldAddEdge2D(
    vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor, unsigned int edgeId);

  /**
   * Return true if the edge at edgeId should be added for the given cell (cursor) in the 3D case.
   * For a cell in a 3D HTG, edges are shared with Moore neighbors.
   * An edge will should be added if it forms an 90 degrees angle between cells. For a given level,
   * there are 2 specific cases:
   * 1) A visible cell can create an edge by looking at masked/absent neighboring leaf cells of same
   * level
   * 2) A masked cell can create an edge by looking at visible cells of lower level (bigger),
   * because coarse cell are not treated in 1)
   */
  bool ShouldAddEdge3D(vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, unsigned int edgeId);

  /**
   * Insert a new edge in the output geometry.
   * Data of the cell from which the edge is generated is copied to the output.
   * Note that this includes data from masked cells, because they can create edges
   * (see ShouldAddEdge2D and ShouldAddEdge3D methods), that can be irrelevant.
   */
  void InsertNewEdge(double* edgePt1, double* edgePt2, vtkPoints* outPoints, vtkCellArray* outCells,
    vtkIdType cellId);

  unsigned int OrientationAxe1D = 0;
  const unsigned int* OrientationAxes2D = nullptr;

  /**
   * Locator used to merge duplicated points during insertion.
   */
  vtkSmartPointer<vtkMergePoints> Locator;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridFeatureEdges_h */
