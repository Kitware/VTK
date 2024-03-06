// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridGeometryEntry
 * @brief   GeometryEntry is a cache data for cursors requiring coordinates
 *
 * cf. vtkHyperTreeGridEntry
 *
 * @sa
 * vtkHyperTreeGridEntry
 * vtkHyperTreeGridLevelEntry
 * vtkHyperTreeGridGeometryEntry
 * vtkHyperTreeGridGeometryLevelEntry
 * vtkHyperTreeGridNonOrientedGeometryCursor
 * vtkHyperTreeGridNonOrientedSuperCursor
 * vtkHyperTreeGridNonOrientedSuperCursorLight
 *
 * @par Thanks:
 * This class was written by Jacques-Bernard Lekien, Jerome Dubois and
 * Guenole Harel, CEA 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridGeometryEntry_h
#define vtkHyperTreeGridGeometryEntry_h

#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTree;
class vtkHyperTreeGrid;

class vtkHyperTreeGridGeometryEntry
{
public:
  /**
   * Display info about the entry
   */
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Constructor
   */
  vtkHyperTreeGridGeometryEntry();

  vtkHyperTreeGridGeometryEntry(vtkHyperTreeGridGeometryEntry const&) = default;
  vtkHyperTreeGridGeometryEntry& operator=(vtkHyperTreeGridGeometryEntry const&) = default;

  /**
   * Constructor
   */
  vtkHyperTreeGridGeometryEntry(vtkIdType index, const double* origin)
  {
    this->Index = index;
    for (unsigned int d = 0; d < 3; ++d)
    {
      this->Origin[d] = origin[d];
    }
  }

  /**
   * Destructor
   */
  ~vtkHyperTreeGridGeometryEntry() = default;

  /**
   * Dump information
   */
  void Dump(ostream& os);

  /**
   * Initialize cursor at root of given tree index in grid.
   */
  vtkHyperTree* Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false);

  /**
   * Initialize cursor from explicit required data
   */
  void Initialize(vtkIdType index, const double* origin)
  {
    this->Index = index;
    for (unsigned int d = 0; d < 3; ++d)
    {
      this->Origin[d] = origin[d];
    }
  }

  /**
   * Copy function
   */
  void Copy(const vtkHyperTreeGridGeometryEntry* entry)
  {
    this->Index = entry->Index;
    for (unsigned int d = 0; d < 3; ++d)
    {
      this->Origin[d] = entry->Origin[d];
    }
  }

  /**
   * Return the index of the current vertex in the tree.
   */
  vtkIdType GetVertexId() const { return this->Index; }

  /**
   * Return the global index (relative to the grid) of the
   * current vertex in the tree.
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
   * Is the cursor pointing to a coarse with all children leaves ?
   * \pre not_tree: tree
   */
  bool IsTerminalNode(
    const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level) const;

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
  void ToChild(const vtkHyperTreeGrid* grid, const vtkHyperTree* tree, unsigned int level,
    const double* sizeChild, unsigned char ichild);

  /**
   * Getter for origin coordinates of the current cell.
   */
  double* GetOrigin() { return this->Origin; }
  const double* GetOrigin() const { return this->Origin; }

  /**
   * Getter for bounding box of the current cell.
   */
  void GetBounds(const double* sizeChild, double bounds[6]) const
  {
    // Compute bounds
    bounds[0] = this->Origin[0];
    bounds[1] = this->Origin[0] + sizeChild[0];
    bounds[2] = this->Origin[1];
    bounds[3] = this->Origin[1] + sizeChild[1];
    bounds[4] = this->Origin[2];
    bounds[5] = this->Origin[2] + sizeChild[2];
  }

  /**
   * Getter for center of the current cell.
   */
  void GetPoint(const double* sizeChild, double point[3]) const
  {
    // Compute center point coordinates
    point[0] = this->Origin[0] + sizeChild[0] / 2.;
    point[1] = this->Origin[1] + sizeChild[1] / 2.;
    point[2] = this->Origin[2] + sizeChild[2] / 2.;
  }

private:
  /**
   * index of the current cell in the HyperTree.
   */
  vtkIdType Index;

  /**
   * origin coordinates of the current cell
   */
  double Origin[3];
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridGeometryEntry_h
// VTK-HeaderTest-Exclude: vtkHyperTreeGridGeometryEntry.h
