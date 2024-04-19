// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridEntry
 * @brief   Entries are cache data for cursors
 *
 * Entries are relevant for cursor/supercursor developers. Filters
 * developers should have a look at cursors/supercursors documentation.
 * (cf. vtkHyperTreeGridNonOrientedCursor). When writing a new cursor or
 * supercursor the choice of the entry is very important: it will drive
 * the performance and memory cost. This is even more important for
 * supercursors which have several neighbors: 6x for VonNeuman and 26x for
 * Moore.
 *
 * Several types of Entries exist:
 * 1. vtkHyperTreeGridEntry
 * This cache only memorizes the current cell index in one HyperTree.
 * Using the index, this entry provides several services such as:
 * is the cell coarse or leaf, get or set global index (to access
 * field value, cf. vtkHyperTree), descend into selected child,
 * subdivise the cell. Equivalent services are available for all entries.
 *
 * 2. vtkHyperTreeGridGeometryEntry
 * This cache adds the origin coordinates of the cell atop
 * vtkHyperTreeGridEntry. Getter is provided, as well as services related
 * to the bounding box and cell center.
 *
 * 3. vtkHyperTreeGridLevelEntry
 * This cache adds the following information with their getters atop
 * vtkHyperTreeGridEntry: pointer to the HyperTree, level of the current
 * cell.
 *
 * 4. vtkHyperTreeGridGeometryLevelEntry
 * This cache is a combination of vtkHyperTreeGridLevelEntry and
 * vtkHyperTreeGridLevelEntry: it provides all combined services.
 *
 * @sa
 * vtkHyperTreeGridEntry
 * vtkHyperTreeGridLevelEntry
 * vtkHyperTreeGridGeometryEntry
 * vtkHyperTreeGridGeometryLevelEntry
 * vtkHyperTreeGridOrientedCursor
 * vtkHyperTreeGridNonOrientedCursor
 *
 * @par Thanks:
 * This class was written by Jacques-Bernard Lekien, Jerome Dubois and
 * Guenole Harel, CEA 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridEntry_h
#define vtkHyperTreeGridEntry_h

#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTree;
class vtkHyperTreeGrid;

class vtkHyperTreeGridEntry
{
public:
  /**
   * Display info about the entry
   */
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Dump information
   */
  void Dump(ostream& os);

  /**
   * Constructor
   */
  vtkHyperTreeGridEntry() { this->Index = 0; }

  vtkHyperTreeGridEntry(vtkHyperTreeGridEntry const&) = default;
  vtkHyperTreeGridEntry& operator=(vtkHyperTreeGridEntry const&) = default;

  /**
   * Constructor
   */
  vtkHyperTreeGridEntry(vtkIdType index) { this->Index = index; }

  /**
   * Destructor
   */
  ~vtkHyperTreeGridEntry() = default;

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  vtkHyperTree* Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false);

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  void Initialize(vtkIdType index) { this->Index = index; }

  /**
   * Copy function
   */
  void Copy(const vtkHyperTreeGridEntry* entry) { this->Index = entry->Index; }

  /**
   * Return the index of the current vertex in the tree.
   */
  vtkIdType GetVertexId() const { return this->Index; }

  /**
   * Return the global index for the current cell (cf. vtkHyperTree).
   * \pre not_tree: tree
   */
  vtkIdType GetGlobalNodeIndex(const vtkHyperTree* tree) const;

  /**
   * Set the global index for the root cell of the HyperTree.
   * \pre not_tree: tree
   */
  void SetGlobalIndexStart(vtkHyperTree* tree, vtkIdType index);

  /**
   * Set the global index for the current cell of the HyperTree.
   * \pre not_tree: tree
   */
  void SetGlobalIndexFromLocal(vtkHyperTree* tree, vtkIdType index);

  /**
   * Set the blanking mask is empty or not
   * \pre not_tree: tree
   */
  void SetMask(const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, bool state);

  /**
   * Determine whether blanking mask is empty or not
   * \pre not_tree: tree
   */
  bool IsMasked(const vtkHyperTreeGrid* grid, const vtkHyperTree* tree) const;

  /**
   * Is the cursor pointing to a leaf?
   * \pre not_tree: tree
   * Return true if level == grid->GetDepthLimiter()
   */
  bool IsLeaf(const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level) const;

  /**
   * Change the current cell's status: if leaf then becomes coarse and
   * all its children are created, cf. HyperTree.
   * \pre not_tree: tree
   * \pre depth_limiter: level == grid->GetDepthLimiter()
   * \pre is_masked: IsMasked
   */
  void SubdivideLeaf(const vtkHyperTreeGrid* grid, vtkHyperTree* tree, unsigned int level);

  /**
   * Is the cursor pointing to a coarse with all children being leaves?
   * \pre not_tree: tree
   */
  bool IsTerminalNode(
    const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level) const;

  /**
   * Is the cursor at HyperTree root?
   */
  bool IsRoot() const { return (this->Index == 0); }

  /**
   * Move the cursor to i-th child of the current cell.
   * \pre not_tree: tree
   * \pre not_leaf: !IsLeaf()
   * \pre valid_child: ichild>=0 && ichild<this->GetNumberOfChildren()
   * \pre depth_limiter: level == grid->GetDepthLimiter()
   * \pre is_masked: !IsMasked()
   */
  void ToChild(const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level,
    unsigned char ichild);

protected:
  /**
   * index of the current cell in the HyperTree.
   */
  vtkIdType Index;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridEntry_h
// VTK-HeaderTest-Exclude: vtkHyperTreeGridEntry.h
