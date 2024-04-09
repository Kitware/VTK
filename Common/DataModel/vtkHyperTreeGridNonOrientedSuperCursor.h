// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridNonOrientedSuperCursor
 * @brief   Objects for traversal a HyperTreeGrid.
 *
 * Objects that can perform depth traversal of a hyper tree grid,
 * take into account more parameters (related to the grid structure) than
 * the compact hyper tree cursor implemented in vtkHyperTree can.
 * This is an abstract class.
 * Cursors are created by the HyperTreeGrid implementation.
 *
 * Supercursor allows to retrieve various kind of cursor for any childs.
 * This class is also a building block for Moore and VonNeumann SuperCursor,
 * which have neighborhood traversal abilities.
 *
 * @sa
 * vtkHyperTree vtkHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien, 2014.
 * This class was re-written by Philippe Pebay, 2016.
 * This class was re-written and optimized by Jacques-Bernard Lekien,
 * Guenole Harel and Jerome Dubois, 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridNonOrientedSuperCursor_h
#define vtkHyperTreeGridNonOrientedSuperCursor_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkSmartPointer.h" // Used internally

#include "vtkHyperTreeGridGeometryLevelEntry.h" // Used Internally

#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTree;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedGeometryCursor;
class vtkHyperTreeGridOrientedGeometryCursor;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridNonOrientedSuperCursor : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTreeGridNonOrientedSuperCursor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create a copy of `this'.
   * \post results_exists:result!=0
   */
  virtual vtkHyperTreeGridNonOrientedSuperCursor* Clone();

  /**
   * Initialize cursor at root of given tree index in grid.
   * The create option only applies to the central HT.
   */
  virtual void Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false) = 0;

  ///@{
  /**
   * Set the hyper tree grid to which the cursor is pointing.
   */
  vtkHyperTreeGrid* GetGrid();
  ///@}

  ///@{
  /**
   * Return if a Tree pointing exist.
   */
  bool HasTree();
  ///@}

  /**
   * Return if a HyperTree pointing exist.
   */
  bool HasTree(unsigned int icursor);

  ///@{
  /**
   * Set the hyper tree to which the cursor is pointing.
   */
  vtkHyperTree* GetTree();
  vtkHyperTree* GetTree(unsigned int icursor);
  ///@}

  /**
   * Return the index of the current vertex in the tree.
   */
  vtkIdType GetVertexId();
  vtkIdType GetVertexId(unsigned int icursor);

  /**
   * Return the global index (relative to the hypertree grid and
   * defined by server) of the current vertex in the tree.
   */
  vtkIdType GetGlobalNodeIndex();

  /**
   * Return the global index (relative to the hypertree grid and
   * defined by server) of the neighbor icursor current vertex in
   * the tree.
   */
  vtkIdType GetGlobalNodeIndex(unsigned int icursor);

  /**
   * Combine three get information into one
   */
  vtkHyperTree* GetInformation(
    unsigned int icursor, unsigned int& level, bool& leaf, vtkIdType& id);

  /**
   * Return the dimension of the tree.
   * \post positive_result: result>0
   */
  unsigned char GetDimension();

  /**
   * Return the number of children for each node (non-vertex leaf) of the tree.
   * \post positive_number: result>0
   */
  unsigned char GetNumberOfChildren();

  /**
   * Calls this method once per HyperTree to set the global index of the first cell.
   * This initializes implicit indexing.
   * /!\ This appeal is inconsistent with SetGlobalIndexFromLocal's appeal.
   */
  void SetGlobalIndexStart(vtkIdType index);

  /**
   * Calls this method for each cell in the HT to set the global index
   * associated with them. This initializes explicit indexing.
   * /!\ This appeal is inconsistent with SetGlobalIndexStart's appeal.
   */
  void SetGlobalIndexFromLocal(vtkIdType index);

  /**
   * Get the origin cell
   */
  double* GetOrigin();
  double* GetOrigin(unsigned int icursor);

  /**
   * Get the size cell
   */
  double* GetSize();

  /**
   * Set the blanking mask is empty or not
   * \pre not_tree: tree
   */
  void SetMask(bool state);
  void SetMask(unsigned int icursor, bool state);

  /**
   * Determine whether blanking mask is empty or not
   */
  bool IsMasked();
  bool IsMasked(unsigned int icursor);

  ///@{
  /**
   * Returns the coordinates of the bounding box :
   *  (xmin, xmax, ymin, ymax, zmin, zmax).
   */
  void GetBounds(double bounds[6]);
  void GetBounds(unsigned int icursor, double bounds[6]);
  ///@}

  ///@{
  /**
   * Returns the coordinates cell center
   */
  void GetPoint(double point[3]);
  void GetPoint(unsigned int icursor, double point[3]);
  ///@}

  /**
   * Is the cursor pointing to a leaf?
   */
  bool IsLeaf();
  bool IsLeaf(unsigned int icursor);

  /**
   * Subdivide Leaf.
   */
  void SubdivideLeaf();

  /**
   * Answer if a cursor is root.
   */
  bool IsRoot();

  /**
   * Get the level of the tree vertex pointed by the cursor.
   */
  unsigned int GetLevel();
  unsigned int GetLevel(unsigned int icursor);

  /**
   * Move the cursor to child `child' of the current vertex.
   * \pre not_tree: HasTree()
   * \pre not_leaf: !IsLeaf()
   * \pre valid_child: ichild>=0 && ichild<GetNumberOfChildren()
   * \pre depth_limiter: GetLevel() <= GetDepthLimiter()
   */
  void ToChild(unsigned char ichild);

  /**
   * Move the cursor to the root vertex.
   * \pre can be root
   * \post is_root: IsRoot()
   */
  void ToRoot();

  /**
   * Move the cursor to the parent of the current vertex.
   * Authorized if HasHistory return true.
   * \pre Non_root: !IsRoot()
   */
  void ToParent();

  /**
   * Get the number of cursors to describe neighboring cells and the current cell
   */
  unsigned int GetNumberOfCursors() const { return this->NumberOfCursors; }

  /**
   * Get the indice of central cursor, the current cell
   */
  unsigned int GetIndiceCentralCursor() const { return this->IndiceCentralCursor; }

  /**
   * Return the cursor pointing into i-th neighbor.
   * The neighborhood definition depends on the type of cursor.
   * NB: Only super cursors keep track of neighborhoods.
   */
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> GetOrientedGeometryCursor(
    unsigned int icursor);

  /**
   * Return the cursor pointing into i-th neighbor.
   * The neighborhood definition depends on the type of cursor.
   * NB: Only super cursors keep track of neighborhoods.
   */
  vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor> GetNonOrientedGeometryCursor(
    unsigned int icursor);

protected:
  /**
   * Constructor
   */
  vtkHyperTreeGridNonOrientedSuperCursor();

  /**
   * Destructor
   */
  ~vtkHyperTreeGridNonOrientedSuperCursor() override;

  /**
   * The pointer to the HyperTreeGrid instance during the crossing.
   */
  vtkHyperTreeGrid* Grid;

  /**
   * Describes the central cursor necessary an instance of
   * vtkHyperTreeGridNonOrientedGeometryCursor.
   */
  vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor> CentralCursor;

  /**
   * Hyper tree grid to which the cursor is attached
   */
  unsigned int CurrentFirstNonValidEntryByLevel;
  std::vector<unsigned int> FirstNonValidEntryByLevel;
  std::vector<vtkHyperTreeGridGeometryLevelEntry> Entries;

  /**
   * The last valid reference describing neighbors.
   * It is also the offset of the first neighbor at the last level.
   */
  unsigned int FirstCurrentNeighboorReferenceEntry;
  std::vector<unsigned int> ReferenceEntries;

  /**
   * Get index entry of icursor.
   */
  unsigned int GetIndiceEntry(unsigned int icursor);

  /**
   * The previous value. In the neighborhood, it does not have to be a parent.
   */
  unsigned int GetIndicePreviousEntry(unsigned int icursor);

  /**
   * Index central cursor
   */
  unsigned int IndiceCentralCursor;

  /*
   * Number of cursors in supercursor.
   */
  unsigned int NumberOfCursors;

  /*
   * Super cursor traversal table to go retrieve the parent index for each cursor
   * of the child node. There are f^d * NumberOfCursors entries in the table.
   */
  const unsigned int* ChildCursorToParentCursorTable;

  /*
   * Super cursor traversal table to go retrieve the child index for each cursor
   * of the child node. There are f^d * NumberOfCursors entries in the table.
   */
  const unsigned int* ChildCursorToChildTable;

private:
  vtkHyperTreeGridNonOrientedSuperCursor(const vtkHyperTreeGridNonOrientedSuperCursor&) = delete;
  void operator=(const vtkHyperTreeGridNonOrientedSuperCursor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
