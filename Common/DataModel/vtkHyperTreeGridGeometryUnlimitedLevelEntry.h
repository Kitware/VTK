// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridGeometryUnlimitedLevelEntry
 * @brief   Cursor cache data with coordinates and level info
 *
 * cf. vtkHyperTreeGridEntry
 *
 * @sa
 * vtkHyperTreeGridEntry
 * vtkHyperTreeGridLevelEntry
 * vtkHyperTreeGridGeometryEntry
 * vtkHyperTreeGridGeometryLevelEntry
 * vtkHyperTreeGridGeometryUnlimitedLevelEntry
 * vtkHyperTreeGridNonOrientedSuperCursor
 * vtkHyperTreeGridNonOrientedSuperCursorLight
 *
 * @par Thanks:
 * This class was written by Jacques-Bernard Lekien, Jerome Dubois and
 * Guenole Harel, CEA 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridGeometryUnlimitedLevelEntry_h
#define vtkHyperTreeGridGeometryUnlimitedLevelEntry_h

#include "assert.h"

#include "vtkObject.h"
#include "vtkSmartPointer.h"

#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkHyperTreeGridOrientedGeometryCursor.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTree;
class vtkHyperTreeGrid;

class vtkHyperTreeGridGeometryUnlimitedLevelEntry
{
public:
  /**
   * Display info about the entry
   */
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Constructor
   */
  vtkHyperTreeGridGeometryUnlimitedLevelEntry(vtkHyperTree* tree = nullptr)
  {
    this->Tree = tree;
    this->Level = 0;
    this->Index = 0;
    for (unsigned int d = 0; d < 3; ++d)
    {
      this->Origin[d] = 0.;
    }
  }

  vtkHyperTreeGridGeometryUnlimitedLevelEntry(
    vtkHyperTreeGridGeometryUnlimitedLevelEntry const&) = default;
  vtkHyperTreeGridGeometryUnlimitedLevelEntry& operator=(
    vtkHyperTreeGridGeometryUnlimitedLevelEntry const&) = default;

  /**
   * Destructor
   */
  ~vtkHyperTreeGridGeometryUnlimitedLevelEntry() = default;

  /**
   * Dump information
   */
  void Dump(ostream& os);

  /**
   * Initialize cache entry from explicit required data
   */
  void Initialize(vtkHyperTree* tree, unsigned int level, vtkIdType index, const double* origin);

  /**
   * Initialize cache entry at root of given tree index in grid.
   */
  vtkHyperTree* Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false);

  /**
   * Reset function
   */
  void Reset()
  {
    this->Tree = nullptr;
    this->Index = 0;
  }

  /**
   * Copy function
   */
  void Copy(const vtkHyperTreeGridGeometryUnlimitedLevelEntry* entry)
  {
    this->Initialize(entry->Tree, entry->Level, entry->Index, entry->Origin);
    this->LastRealIndex = entry->LastRealIndex;
    this->LastRealLevel = entry->LastRealLevel;
  }

  /**
   * Create a vtkHyperTreeGridOrientedCursor from input grid and
   * current entry data.
   */
  vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> GetHyperTreeGridOrientedGeometryCursor(
    vtkHyperTreeGrid* grid)
  {
    vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor> cursor =
      vtkSmartPointer<vtkHyperTreeGridOrientedGeometryCursor>::New();
    cursor->Initialize(grid, this->Tree, this->Level, this->Index, this->Origin);
    return cursor;
  }

  /**
   * Create a vtkHyperTreeGridNonOrientedCursor from input grid and
   * current entry data.
   */
  vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor>
  GetHyperTreeGridNonOrientedGeometryCursor(vtkHyperTreeGrid* grid)
  {
    assert("pre: level==0" && this->Level == 0);
    vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor> cursor =
      vtkSmartPointer<vtkHyperTreeGridNonOrientedGeometryCursor>::New();
    cursor->Initialize(grid, this->Tree, this->Level, this->Index, this->Origin);
    return cursor;
  }

  /**
   * Return the index of the current vertex in the tree.
   * \pre not_tree: tree
   */
  vtkIdType GetVertexId() const { return this->LastRealIndex; }

  /**
   * Return the global index (relative to the grid) of the
   * current vertex in the tree.
   * \pre not_tree: tree
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
   * Is the cursor pointing to a leaf?
   * \pre not_tree: tree
   */
  bool IsRealLeaf(const vtkHyperTreeGrid* grid) const;

  /**
   * Is the cursor pointing to a virtually subdivided leaf?
   * \pre not_tree: tree
   */
  bool IsVirtualLeaf(const vtkHyperTreeGrid* grid) const;

  /**
   * Is the cursor pointing to a coarse with all children being leaves ?
   * \pre not_tree: tree
   */
  bool IsTerminalNode(const vtkHyperTreeGrid* grid) const;

  /**
   * Is the cursor at tree root?
   */
  bool IsRoot() { return this->Index == 0 && this->Level == 0; }

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

  ///@{
  /**
   * Get level info from current cache entry.
   */
  unsigned int GetLevel() const { return this->Level; }
  unsigned int GetLastRealLevel() const { return this->LastRealLevel; }
  ///@}

  ///@{
  /**
   * Getter for origin coordinates of the current cell.
   */
  double* GetOrigin() { return this->Origin; }
  const double* GetOrigin() const { return this->Origin; }
  ///@}

  /**
   * Getter for bounding box of the current cell.
   */
  void GetBounds(double bounds[6]) const;

  /**
   * Getter for center of the current cell.
   */
  void GetPoint(double point[3]) const;

private:
  /**
   * pointer to the HyperTree containing the current cell.
   */
  vtkHyperTree* Tree;

  /**
   * level of the current cell in the HyperTree.
   */
  unsigned int Level = 0;

  /**
   * index of the current cell in the HyperTree.
   */
  vtkIdType Index = 0;

  /**
   * index of the last real cell visited in the HyperTree
   */
  vtkIdType LastRealIndex = 0;

  /**
   * level of the last real cell visited in the HyperTree
   */
  vtkIdType LastRealLevel = 0;

  /**
   * origin coiordinates of the current cell
   */
  double Origin[3];
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridGeometryUnlimitedLevelEntry
// VTK-HeaderTest-Exclude: vtkHyperTreeGridGeometryUnlimitedLevelEntry.h
