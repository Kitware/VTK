/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridLevelEntry.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridLevelEntry
 *
 * @brief   LevelEntry is a cache data for cursors requiring level info
 *
 * cf. vtkHyperTreeGridEntry
 *
 * @sa
 * vtkHyperTreeGridEntry
 * vtkHyperTreeGridLevelEntry
 * vtkHyperTreeGridGeometryEntry
 * vtkHyperTreeGridGeometryLevelEntry
 * vtkHyperTreeGridNonOrientedSuperCursor
 * vtkHyperTreeGridNonOrientedSuperCursorLight
 *
 * @par Thanks:
 * This class was written by Jacques-Bernard Lekien, Jerome Dubois and
 * Guenole Harel, CEA 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridLevelEntry_h
#define vtkHyperTreeGridLevelEntry_h

#ifndef __VTK_WRAP__

#include "vtkObject.h"
#include "vtkSmartPointer.h" // Used internally

class vtkHyperTree;
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;

class vtkHyperTreeGridLevelEntry
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
  vtkHyperTreeGridLevelEntry()
    : Tree(nullptr)
    , Level(0)
    , Index(0)
  {
  }

  /**
   * Constructor
   */
  vtkHyperTreeGridLevelEntry(vtkHyperTreeGridLevelEntry* entry)
    : Tree(entry->Tree)
    , Level(entry->Level)
    , Index(entry->Index)
  {
  }

  /**
   * Constructor
   */
  vtkHyperTreeGridLevelEntry(vtkHyperTree* tree, unsigned int level, vtkIdType index)
    : Tree(tree)
    , Level(level)
    , Index(index)
  {
  }

  /**
   * Constructor
   */
  vtkHyperTreeGridLevelEntry(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false);

  /**
   * Destructor
   */
  ~vtkHyperTreeGridLevelEntry() = default;

  /**
   * Reset function
   */
  void Reset()
  {
    this->Tree = nullptr;
    this->Level = 0;
    this->Index = 0;
  }

  /**
   * Initialize cursor from explicit required data
   */
  void Initialize(vtkHyperTree* tree, unsigned int level, vtkIdType index)
  {
    this->Tree = tree;
    this->Level = level;
    this->Index = index;
  }

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  vtkHyperTree* Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false);

  /**
   * Copy function
   */
  void Copy(const vtkHyperTreeGridLevelEntry* entry)
  {
    this->Tree = entry->Tree;
    this->Level = entry->Level;
    this->Index = entry->Index;
  }

  /**
   * Create a vtkHyperTreeGridNonOrientedCursor from input grid and
   * current entry data
   */
  vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> GetHyperTreeGridNonOrientedCursor(
    vtkHyperTreeGrid* grid);

  /**
   * Return the index of the current vertex in the tree.
   */
  vtkIdType GetVertexId() const { return this->Index; }

  /**
   * Return the global index (relative to the grid) of the
   * current vertex in the tree.
   */
  vtkIdType GetGlobalNodeIndex() const;

  /**
   * Set the global index for the root cell of the HyperTree.
   * \pre not_tree: tree
   */
  void SetGlobalIndexStart(vtkIdType index);

  /**
   * Set the global index for the current cell of the HyperTree.
   * \pre not_tree: tree
   */
  void SetGlobalIndexFromLocal(vtkIdType index);

  /**
   * Set the blanking mask is empty or not
   * \pre not_tree: tree
   */
  void SetMask(const vtkHyperTreeGrid* grid, bool state);

  /**
   * Determine whether blanking mask is empty or not
   * \pre not_tree: tree
   */
  bool IsMasked(const vtkHyperTreeGrid* grid) const;

  /**
   * Is the cursor pointing to a leaf?
   * \pre not_tree: tree
   * Return true if level == grid->GetDepthLimiter()
   */
  bool IsLeaf(const vtkHyperTreeGrid* grid) const;

  /**
   * Change the current cell's status: if leaf then becomes coarse and
   * all its children are created, cf. HyperTree.
   * \pre not_tree: tree
   * \pre depth_limiter: level == grid->GetDepthLimiter()
   * \pre is_masked: IsMasked
   */
  void SubdivideLeaf(const vtkHyperTreeGrid* grid);

  /**
   * Is the cursor pointing to a coarse with all childrens being leaves ?
   * \pre not_tree: tree
   */
  bool IsTerminalNode(const vtkHyperTreeGrid* grid) const;

  /**
   * Is the cursor at tree root?
   */
  bool IsRoot() const { return (this->Index == 0); }

  /**
   * Move the cursor to child `child' of the current vertex.
   * \pre not_tree: tree
   * \pre not_leaf: !IsLeaf()
   * \pre valid_child: ichild>=0 && ichild<this->GetNumberOfChildren()
   * \pre depth_limiter: level == grid->GetDepthLimiter()
   * \pre is_masked: !IsMasked()
   */
  void ToChild(const vtkHyperTreeGrid* grid, unsigned char ichild);

  /**
   * Get HyperTree from current cache entry.
   */
  vtkHyperTree* GetTree() const { return this->Tree; }

  /**
   * Get level info from current cache entry.
   */
  unsigned int GetLevel() const { return this->Level; }

protected:
  /**
   * pointer to the HyperTree containing the current cell.
   */
  vtkHyperTree* Tree;

  /**
   * level of the current cell in the HyperTree.
   */
  unsigned int Level;

  /**
   * index of the current cell in the HyperTree.
   */
  vtkIdType Index;
};

#endif // __VTK_WRAP__

#endif // vtkHyperTreeGridLevelEntry_h
// VTK-HeaderTest-Exclude: vtkHyperTreeGridLevelEntry.h
