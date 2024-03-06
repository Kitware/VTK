// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor
 * @brief   Specific Moore super cursor that can subdivied neighborhood
 *
 * This supercursor behave like the Moore supercursor but relies on the
 * vtkHyperTreeGridNonOrientedUnlimitedSuperCursor so the neighborhood
 * can be refined to reach the level of the current cell in any case.
 *
 * @sa
 * vtkHyperTree vtkHyperTreeGrid vtkHyperTreeGridNonOrientedMooreSuperCursor
 */

#ifndef vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor_h
#define vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor_h

#include "vtkCommonDataModelModule.h" // For export macro

#include "vtkHyperTreeGridNonOrientedUnlimitedSuperCursor.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkHyperTree;
class vtkHyperTreeGrid;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor
  : public vtkHyperTreeGridNonOrientedUnlimitedSuperCursor
{
public:
  vtkTypeMacro(vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor,
    vtkHyperTreeGridNonOrientedUnlimitedSuperCursor);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* New();

  /**
   * Initialize cursor at root of given tree index in grid.
   * "create" only applies to the central HT
   */
  void Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false) override;

  /**
   * Return the list of cursors pointing to the leaves touching a
   * given corner of the cell.
   * Return whether the considered cell is the owner of said corner.
   */
  bool GetCornerCursors(unsigned int, unsigned int, vtkIdList*);

protected:
  /**
   * Constructor
   */
  vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor() = default;

  /**
   * Destructor
   */
  ~vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor() override;

private:
  vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor(
    const vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor&) = delete;
  void operator=(const vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
