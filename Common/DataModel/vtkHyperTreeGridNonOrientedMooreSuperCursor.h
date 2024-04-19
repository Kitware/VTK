// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridNonOrientedMooreSuperCursor
 * @brief   Objects for traversal a HyperTreeGrid.
 *
 * Objects that can perform depth traversal of a hyper tree grid,
 * take into account more parameters (related to the grid structure) than
 * the compact hyper tree cursor implemented in vtkHyperTree can.
 * This is an abstract class.
 * Cursors are created by the HyperTreeGrid implementation.
 *
 * @sa
 * vtkHyperTree vtkHyperTreeGrid
 *
 * This supercursor allows to visit all neighbors including diagonal ones.
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien, 2014.
 * This class was re-written by Philippe Pebay, 2016.
 * This class was re-written and optimized by Jacques-Bernard Lekien,
 * Guenole Harel and Jerome Dubois, 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridNonOrientedMooreSuperCursor_h
#define vtkHyperTreeGridNonOrientedMooreSuperCursor_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHyperTreeGridNonOrientedSuperCursor.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkHyperTree;
class vtkHyperTreeGrid;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridNonOrientedMooreSuperCursor
  : public vtkHyperTreeGridNonOrientedSuperCursor
{
public:
  vtkTypeMacro(vtkHyperTreeGridNonOrientedMooreSuperCursor, vtkHyperTreeGridNonOrientedSuperCursor);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkHyperTreeGridNonOrientedMooreSuperCursor* New();

  /**
   * Initialize cursor at root of given tree index in grid.
   * "create" only applies to the central HT
   */
  void Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create = false) override;

  /**
   * Return the list of cursors pointing to the leaves touching a
   * given corner of the cell.
   * Return whether the considered cell is the owner of said corner.
   * Used by filters vtkHyperTreeGridContour and vtkHyperTreeGridPlaneCutter.
   */
  bool GetCornerCursors(unsigned int, unsigned int, vtkIdList*);

protected:
  /**
   * Constructor
   */
  vtkHyperTreeGridNonOrientedMooreSuperCursor() = default;

  /**
   * Destructor
   */
  ~vtkHyperTreeGridNonOrientedMooreSuperCursor() override;

private:
  vtkHyperTreeGridNonOrientedMooreSuperCursor(
    const vtkHyperTreeGridNonOrientedMooreSuperCursor&) = delete;
  void operator=(const vtkHyperTreeGridNonOrientedMooreSuperCursor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
