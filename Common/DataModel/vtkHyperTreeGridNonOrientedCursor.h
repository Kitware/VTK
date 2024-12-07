// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridNonOrientedCursor
 * @brief   Objects for traversal a HyperTreeGrid.
 *
 * Objects that can perform depth traversal of a hyper tree grid,
 * take into account more parameters (related to the grid structure) than
 * the compact hyper tree cursor implemented in vtkHyperTree can.
 * This is an abstract class.
 * Cursors are created by the HyperTreeGrid implementation.
 *
 * Non-oriented cursors have the ability to come back to their parents
 * and to go to the root.
 *
 * @sa
 * vtkHyperTree vtkHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien, 2014.
 * This class was re-written by Philippe Pebay, 2016.
 * This class was re-written for more optimisation by Jacques-Bernard Lekien,
 * Guenole Harel and Jerome Dubois, 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridNonOrientedCursor_h
#define vtkHyperTreeGridNonOrientedCursor_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTree;
class vtkHyperTreeGrid;
class vtkHyperTreeGridEntry;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridNonOrientedCursor : public vtkObject
{
public:
  vtkTypeMacro(vtkHyperTreeGridNonOrientedCursor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkHyperTreeGridNonOrientedCursor* New();

  /**
   * Create a copy of `this'.
   * This function allocates a new cursor that needs to be freed.
   * \post results_exists:result!=0
   */
  vtkHyperTreeGridNonOrientedCursor* Clone();

  /**
   * Create a copy of `this`, but discard history.
   * The cloned cursor cannot go any higher in the tree than the position it has been cloned at.
   * This function allocates a new cursor that needs to be freed.
   */
  vtkHyperTreeGridNonOrientedCursor* CloneFromCurrentEntry();

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  void Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false);

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  void Initialize(
    vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level, vtkHyperTreeGridEntry& entry);

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  void Initialize(vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level, vtkIdType index);

  ///@{
  /**
   * Set the hyper tree grid to which the cursor is pointing.
   */
  vtkHyperTreeGrid* GetGrid();
  ///@}

  ///@{
  /**
   * Return if a Tree pointing exist
   */
  bool HasTree() const;
  ///@}

  ///@{
  /**
   * Set the hyper tree to which the cursor is pointing.
   */
  vtkHyperTree* GetTree() const;
  ///@}

  /**
   * Return the index of the current vertex in the tree.
   */
  vtkIdType GetVertexId();

  /**
   * Return the global index (relative to the grid) of the
   * current vertex in the tree.
   */
  vtkIdType GetGlobalNodeIndex();

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

  void SetGlobalIndexStart(vtkIdType index);

  void SetGlobalIndexFromLocal(vtkIdType index);

  /**
   * Set the blanking mask is empty or not
   * \pre not_tree: tree
   */
  void SetMask(bool state);

  /**
   * Determine whether blanking mask is empty or not
   */
  bool IsMasked();

  /**
   * Is the cursor pointing to a leaf?
   */
  bool IsLeaf();

  void SubdivideLeaf();

  /**
   * Is the cursor at tree root?
   */
  bool IsRoot();

  /**
   * Get the level of the tree vertex pointed by the cursor.
   */
  unsigned int GetLevel();

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

protected:
  /**
   * Constructor
   */
  vtkHyperTreeGridNonOrientedCursor();

  /**
   * Destructor
   */
  ~vtkHyperTreeGridNonOrientedCursor() override;

  /**
   * Reference to the HTG currently processed
   */
  vtkHyperTreeGrid* Grid;

  vtkHyperTree* Tree;
  unsigned int Level;
  int LastValidEntry;

  /**
   * Hyper tree grid to which the cursor is attached
   */
  std::vector<vtkHyperTreeGridEntry> Entries;

private:
  vtkHyperTreeGridNonOrientedCursor(const vtkHyperTreeGridNonOrientedCursor&) = delete;
  void operator=(const vtkHyperTreeGridNonOrientedCursor&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
